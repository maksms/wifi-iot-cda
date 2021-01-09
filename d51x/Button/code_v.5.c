/*
* v.5 настройка кнопки и видов нажатий через sensors_param.cfgdes[]
*           в поле указывается число:   0 - 254 - локальные gpio и vgpio
*                                       255 -  нажатие кнопки не обрабатывается
*                                       1000 - 1010 - номер строки конструктора строк, в которой располагается mqtt топик с ! в начале или url (без http:// и в качестве хоста содержит ip)
*   Options: Button GPIO, 1st short press GPIO, 2nd short press GPIO, 3rd short press GPIO, 1st long press GPIO, 1st long press timeout (ms),  2nd long press GPIO, 2nd long press timeout (ms), 3rd long press GPIO, 3rd long press timeout (ms)
*
* v.4 отправка по mqtt, если в строка начинается с !, иначе по http - в строке надо указать url с http://
* v.3 отправка по mqtt в коллбеке обработчика кнопки (топик для mqtt берем из конструктора строк - костыль такой! ))))
* v.2 динамическое назначение колбеков для обработчика кнопок вместо хардкода
*/


#define BTN_GLITCH_TIME 50
#define BTN_SHORT_PRESS_TIMEOUT 300

#define REMOTE_CALL_SIGN 1000
#define ESP_NONOS

#ifdef ESP_NONOS
#define memcpy os_memcpy 
#define malloc os_malloc
#define memset os_memset 
#define free os_free
#define sprintf os_sprintf
#define strlen os_strlen
#define strcmp os_strcmp
#define millis() (unsigned long) (micros() / 1000ULL)
#else
#define micros() (unsigned long) (esp_timer_get_time())
#define millis() (unsigned long) (esp_timer_get_time() / 1000ULL)
static const char *TAG = "MAIN";
#endif

typedef void (* button_cb)(void *); 

typedef struct {
    button_cb cb;
    void *args;
    uint16_t timeout;   // задержка для long_press
    uint8_t idx;  //short press index
} btn_cb_t;

typedef struct {
  uint8_t pin;
  uint8_t level;
  
    btn_cb_t *short_press_cb;
    uint8_t short_press_cb_cnt;

    btn_cb_t *long_press_cb;
    uint8_t long_press_cb_cnt;

#ifdef ESP_NONOS
  os_timer_t read_timer;
  os_timer_t short_press_timer; 
#else
  TimerHandle_t  read_timer;
  TimerHandle_t  short_press_timer;
#endif

  uint8_t short_press_count;
  uint32_t long_press_time;
  uint8_t state;
  
  uint32_t pressed_time;
  uint32_t prev_pressed_time;
  uint32_t ms;
  uint32_t released_time;
  uint32_t hold_time;
  uint32_t wait_time;
} button_t;

button_t *button;

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR short_press_timer_cb(void *arg) 
#else
void short_press_timer_cb(xTimerHandle tmr)   // rtos
#endif
{
    #ifdef ESP_NONOS
      button_t *btn = (button_t *)arg; // nonos
    #else
       button_t *btn = (button_t *)pvTimerGetTimerID(tmr); // rtos
    #endif
    
    uint8_t i = 0;
    for ( i = 0; i < btn->short_press_cb_cnt; i++ )
    {
        btn_cb_t cb = btn->short_press_cb[i];
        if ( cb.idx == btn->short_press_count )
        {
            if ( cb.cb  != NULL )
            {
                cb.cb(cb.args);
            }
            break;
        }
    }
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR reschedule_short_press_timer(button_t *btn)
#else
void reschedule_short_press_timer(button_t *btn)
#endif
{
  #ifdef ESP_NONOS
    os_timer_disarm(&btn->short_press_timer);
    os_timer_setfn(&btn->short_press_timer, (os_timer_func_t *)short_press_timer_cb, btn);
    os_timer_arm(&btn->short_press_timer, BTN_SHORT_PRESS_TIMEOUT, 0); 
  #else  
    
    if ( btn->short_press_timer == NULL )
    {
        btn->short_press_timer = xTimerCreate("shortpress", BTN_SHORT_PRESS_TIMEOUT / portTICK_PERIOD_MS, pdFALSE, btn, short_press_timer_cb);
    } 
    
    if ( xTimerIsTimerActive(btn->short_press_timer) == pdTRUE )
    {
        xTimerStop(btn->short_press_timer, 0);
    }

    xTimerStart(btn->short_press_timer, 0);
  #endif    
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR btn_read_cb(void *arg) // nonos
#else
void btn_read_cb(xTimerHandle tmr) // rtos
#endif
{

  #ifdef ESP_NONOS
     button_t *btn = (button_t *)arg;  // nonos
  #else    
    button_t *btn = (button_t *)pvTimerGetTimerID(tmr); //rtos
  #endif

#ifdef ESP_NONOS
  int level = digitalRead( btn->pin );
#else
  int level = gpio_get_level( btn->pin );
#endif

  btn->ms = millis();

    if ( level == 0 && !btn->state && ( (btn->ms - btn->pressed_time) > BTN_GLITCH_TIME))
    {
        btn->state = 1;
        btn->pressed_time = btn->ms;
    }
          else if ( level == 1 && btn->state && ((btn->ms - btn->pressed_time) > BTN_GLITCH_TIME))
    {
        btn->hold_time = btn->ms - btn->pressed_time;
        btn->released_time = millis();

        btn->state = 0;

        if ( btn->short_press_count == 0 ) btn->short_press_count = 1;
        if ( btn->hold_time < BTN_SHORT_PRESS_TIMEOUT )
        {
            btn->wait_time = btn->pressed_time - btn->prev_pressed_time;
            if ( btn->wait_time < BTN_SHORT_PRESS_TIMEOUT )
            {
                btn->short_press_count++;
            } 
            else 
            {
                btn->short_press_count = 1;
            }        
            reschedule_short_press_timer(btn);
        } 
        else 
        {
            btn->short_press_count = 1;
            btn->long_press_time = btn->hold_time;

            if ( btn->long_press_time < 1000 ) {
                if ( btn->short_press_cb[0].cb != NULL  )
                {
                    btn->short_press_cb[0].cb(btn->short_press_cb[0].args );
                }
            } else {
                uint8_t i = 0;
                for ( i = btn->long_press_cb_cnt; i > 0; i--)
                {
                    if ( btn->long_press_time >= btn->long_press_cb[i-1].timeout)
                    {
                        if ( btn->long_press_cb[i-1].cb != NULL )
                        {
                            btn->long_press_cb[i-1].cb( btn->long_press_cb[i-1].args );
                        }
                        break;
                    }
                }
            }
        }
        btn->prev_pressed_time = btn->pressed_time;
        btn->pressed_time = btn->ms;
        btn->ms = millis();
    }
}

#ifdef ESP_NONOS
button_t* ICACHE_FLASH_ATTR create_button(uint8_t pin, uint8_t level)
#else
button_t* create_button(uint8_t pin, uint8_t level)
#endif
{
	button_t *btn = (button_t *) malloc(sizeof(button_t));
	btn->level = level;
	btn->pin = pin;
	btn->state = 0;

	btn->short_press_count = 0;
	btn->long_press_time = 0;

	btn->pressed_time = 0;
	btn->prev_pressed_time = 0;
	btn->ms = 0;
	btn->released_time = 0;
	btn->hold_time = 0;
	btn->wait_time = 0;
	
	#ifndef ESP_NONOS
	btn->short_press_timer = NULL;
	#endif
	
	btn->short_press_cb = NULL;
	btn->short_press_cb_cnt = 0;

	btn->long_press_cb = NULL;
	btn->long_press_cb_cnt = 0;


    #ifdef ESP_NONOS
		os_timer_disarm(&btn->read_timer);
		os_timer_setfn(&btn->read_timer, (os_timer_func_t *)btn_read_cb, btn);
		os_timer_arm(&btn->read_timer, 10, 1);    
	#else
		btn->read_timer = xTimerCreate("btn", 10 / portTICK_PERIOD_MS, pdTRUE, btn, btn_read_cb);
		xTimerStart(btn->read_timer, 0);
	#endif

	return btn;
}


#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR button_add_short_press(button_t *btn, uint8_t press_cnt, button_cb cb, void *args)
#else
void button_add_short_press(button_t *btn, uint8_t press_cnt, button_cb cb, void *args)
#endif
{
    uint8_t i = 0;
    for ( i = 0; i < btn->short_press_cb_cnt; i++ )
    {
        if ( btn->short_press_cb[i].cb == cb && i == press_cnt - 1)
            return;
    }

    btn_cb_t *tmp = NULL;
    if ( btn->short_press_cb_cnt > 0 )
    {
        tmp = (btn_cb_t *) malloc( btn->short_press_cb_cnt * sizeof(btn_cb_t));
        memset( tmp, 0, btn->short_press_cb_cnt * sizeof(btn_cb_t));
        memcpy( tmp, btn->short_press_cb, (btn->short_press_cb_cnt) * sizeof(btn_cb_t));
    }
    
    btn->short_press_cb_cnt++;
    btn->short_press_cb = (btn_cb_t *) malloc(btn->short_press_cb_cnt * sizeof(btn_cb_t) );
    memset( btn->short_press_cb, 0, btn->short_press_cb_cnt * sizeof(btn_cb_t));
    
    if ( btn->short_press_cb_cnt > 1 )
    {
        memcpy( btn->short_press_cb, tmp, (btn->short_press_cb_cnt - 1) * sizeof(btn_cb_t));
    }
    free( tmp);
    btn->short_press_cb[ btn->short_press_cb_cnt - 1].cb = cb;
    btn->short_press_cb[ btn->short_press_cb_cnt - 1].args = args;
    btn->short_press_cb[ btn->short_press_cb_cnt - 1].idx = press_cnt;
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR button_add_long_press(button_t *btn, uint16_t timeout, button_cb cb, void *args)
#else
void button_add_long_press(button_t *btn, uint16_t timeout, button_cb cb, void *args)
#endif
{
    uint8_t i = 0;
    for ( i = 0; i < btn->long_press_cb_cnt; i++ )
    {
        if ( btn->long_press_cb[i].cb == cb && btn->long_press_cb[i].timeout == timeout )
            return;
    }

    btn_cb_t *tmp = NULL;
    if ( btn->long_press_cb_cnt > 0 )
    {
        tmp = (btn_cb_t *) malloc( btn->long_press_cb_cnt * sizeof(btn_cb_t));
        memset( tmp, 0, btn->long_press_cb_cnt * sizeof(btn_cb_t));
        memcpy( tmp, btn->long_press_cb, (btn->long_press_cb_cnt) * sizeof(btn_cb_t));
    }
    
    btn->long_press_cb_cnt++;
    btn->long_press_cb = (btn_cb_t *) malloc(btn->long_press_cb_cnt * sizeof(btn_cb_t) );
    memset( btn->long_press_cb, 0, btn->long_press_cb_cnt * sizeof(btn_cb_t));
    
    if ( btn->long_press_cb_cnt > 1 )
    {
        memcpy( btn->long_press_cb, tmp, (btn->long_press_cb_cnt - 1) * sizeof(btn_cb_t));
    }
    free( tmp);
    btn->long_press_cb[ btn->long_press_cb_cnt - 1].cb = cb;
    btn->long_press_cb[ btn->long_press_cb_cnt - 1].args = args;
    btn->long_press_cb[ btn->long_press_cb_cnt - 1].timeout = timeout;    

    // sort by timeout

    if ( btn->long_press_cb_cnt > 1) 
    {
        for ( i = 0; i < btn->long_press_cb_cnt; i++)
        {
            uint8_t j = 0;
            for ( j = i + 1; j < btn->long_press_cb_cnt; j++ )
            {
                if ( btn->long_press_cb[i].timeout > btn->long_press_cb[j].timeout )
                {
                    btn_cb_t *t = (btn_cb_t *) malloc( sizeof(btn_cb_t));
                    memcpy(t, &btn->long_press_cb[i], sizeof(btn_cb_t));
                    memcpy(&btn->long_press_cb[i], &btn->long_press_cb[j], sizeof(btn_cb_t));
                    memcpy(&btn->long_press_cb[j], t, sizeof(btn_cb_t));
                    free(t);
                }
            } 
        }
    }    
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR send_mqtt(const char *topic)
#else
void send_mqtt(const char *topic)
#endif
{
    #if mqtte || mqttjsone
	if ( sensors_param.mqtten != 1 ) return;
	char payload[2] = "2";
	
    #ifdef ESP_NONOS
    MQTT_Publish(&mqttClient, topic, payload, strlen(payload), 2, 0, 1);
    #else
    MQTT_Publish(topic, payload, strlen(payload), 0, 0, 0);
    #endif
    #endif
}

#ifdef ESP_NONOS
char* ICACHE_FLASH_ATTR cut_str_from_str(char *str, const char *str2)
#else
char* cut_str_from_str(char *str, const char *str2)
#endif
{
    if ( strlen(str) == 0 ) return NULL;
    char *p = strstr(str, str2);
    int pos = 0;
    if ( p != NULL ) {
        pos = p - str;
    } else {
        pos = strlen(str);
    }

    char *r = (char *) malloc(pos + 1);
    memset(r, 0, pos + 1);
    strncpy(r, str, pos);

    if ( p != NULL ) {
        strcpy(str, p+1);
    } else {
         memset(str, 0, strlen(str)+1);
    }
    
    return r;
}

#ifdef ESP_NONOS
static void ICACHE_FLASH_ATTR tcpclient_connect_cb(void *arg) 
#else
static void tcpclient_connect_cb(void *arg) 
#endif
{
    struct espconn *pespconn = (struct espconn *)arg;
    char payload[200];
    espconn_regist_sentcb(pespconn, tcpclient_sent_cb);
    espconn_regist_disconcb(pespconn, tcpclient_discon_cb);
    os_sprintf(payload, "GET /%s", (char *)pespconn->reverse);
    os_sprintf(payload + os_strlen(payload), " HTTP/1.1\r\nHost: testdomen\r\nConnection: keep-alive\r\nAccept: */*\r\n\r\n");
    espconn_sent(pespconn, payload, strlen(payload));
    free(pespconn->reverse);
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR send_http(const char *url)
#else
void send_http(const char *url)
#endif
{
    
    struct espconn *pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
    if (pCon == NULL) 
    {
        // ошибка
        return;
    }
    pCon->type = ESPCONN_TCP;
    pCon->state = ESPCONN_NONE;
    pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    pCon->proto.tcp->local_port = espconn_port();
    pCon->proto.tcp->remote_port = 80; 

    char *data = (char *)malloc(100);
    memset(data, 0, 100);
    strcpy(data, url);
    char *addr = cut_str_from_str(data, "/");
    
    pCon->reverse = data;  // сюда положить uri

    uint32_t ip = ipaddr_addr(addr);        // сервер
    os_memcpy(pCon->proto.tcp->remote_ip, &ip, 4);

    espconn_regist_connectcb(pCon, tcpclient_connect_cb); // функция отправки GET запроса
    espconn_regist_reconcb(pCon, tcpclient_recon_cb);
    espconn_connect(pCon);  
    free(addr);
    //free(data);  // free data here tcpclient_connect_cb

}

void ICACHE_FLASH_ATTR remote_call(uint8_t idx)
{
    char data[100];
    replacesens(idx,data); 
    if ( strlen(data) > 0 )
    {
        //if ( os_strcmp(data, "!", 1) > 0 )
        if ( data[0] == 33 )
            send_mqtt(data);
        else
            send_http(data);    
    }  
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR button_press_cb(void *args)
#else
void button_press_cb(void *args)
#endif
{
    uint32_t *val = (uint32_t *)args; 
    uint32_t gpio = *val % REMOTE_CALL_SIGN;

    if ( *val / REMOTE_CALL_SIGN == 1 ) 
    {
        // mqtt or http
        remote_call(gpio);  
    }
    else    
    {
        if ( gpio < 255 )
            GPIO_ALL( gpio, !GPIO_ALL_GET( gpio ) );
    }
}


void ICACHE_FLASH_ATTR startfunc()
{
	/*
*       sensors_param.cfgdes[0] - GPIO кнопки                              
*       sensors_param.cfgdes[1] - короткое нажатие 1 - GPIO или номер конструктора строки ( x 1000 )
*       sensors_param.cfgdes[2] - короткое нажатие 2 - GPIO или номер конструктора строки ( x 1000 )
*       sensors_param.cfgdes[3] - короткое нажатие 3 - GPIO или номер конструктора строки ( x 1000 )
*       sensors_param.cfgdes[4] - долгое нажатие 1 - GPIO или номер конструктора строки ( x 1000 )
*       sensors_param.cfgdes[5] - мсек, длительность долгого нажатия 1
*       sensors_param.cfgdes[6] - долгое нажатие 2 - GPIO или номер конструктора строки ( x 1000 )
*       sensors_param.cfgdes[7] - мсек, длительность долгого нажатия 2
*       sensors_param.cfgdes[8] - долгое нажатие 3 - GPIO или номер конструктора строки ( x 1000 )
*       sensors_param.cfgdes[9] - мсек, длительность долгого нажатия 3
    */

    // выполняется один раз при старте модуля.
    if ( sensors_param.cfgdes[0] < 16 )
    {
        button = create_button( sensors_param.cfgdes[0] , 0);

        button_add_short_press(button, 1, button_press_cb, &sensors_param.cfgdes[1]);
        button_add_short_press(button, 2, button_press_cb, &sensors_param.cfgdes[2]);
        button_add_short_press(button, 3, button_press_cb, &sensors_param.cfgdes[3]);

        button_add_long_press(button, sensors_param.cfgdes[5], button_press_cb, &sensors_param.cfgdes[4]);
        button_add_long_press(button, sensors_param.cfgdes[7], button_press_cb, &sensors_param.cfgdes[6]);
        button_add_long_press(button, sensors_param.cfgdes[9], button_press_cb, &sensors_param.cfgdes[8]);
    }
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
    // выполнение кода каждую 1 секунду
    if(timersrc%30==0)
    {
        // выполнение кода каждые 30 секунд
    }
}

void webfunc(char *pbuf) 
{

}