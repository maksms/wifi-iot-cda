
#define GPIO_BTN 4
#define GPIO_LED1 12
#define GPIO_LED2 13
#define GPIO_LED3 15
#define GPIO_LED4 2

#define BTN_GLITCH_TIME 50
#define BTN_SHORT_PRESS_TIMEOUT 300

#define ESP_NONOS

#ifdef ESP_NONOS
#define memcpy os_memcpy 
#define malloc os_malloc
#define memset os_memset 
#define free os_free
#define millis() (unsigned long) (micros() / 1000ULL)
#else
#define micros() (unsigned long) (esp_timer_get_time())
#define millis() (unsigned long) (esp_timer_get_time() / 1000ULL)
static const char *TAG = "MAIN";
#endif

char str[64] = "";

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
void ICACHE_FLASH_ATTR short_press1_cb(void *args)
#else
void short_press1_cb(void *args)
#endif
{
	digitalWrite( GPIO_LED1, !digitalRead(GPIO_LED1) );
	strcpy(str, (char *)args);
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR short_press2_cb(void *args)
#else
void short_press2_cb(void *args)
#endif
{
	digitalWrite( GPIO_LED2, !digitalRead(GPIO_LED2) );
	strcpy(str, (char *)args);
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR short_press3_cb(void *args)
#else
void short_press3_cb(void *args)
#endif
{
	digitalWrite( GPIO_LED3, !digitalRead(GPIO_LED3) );
	strcpy(str, (char *)args);
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR long_press_1s_cb(void *args)
#else
void long_press_1s_cb(void *args)
#endif
{
	digitalWrite( GPIO_LED4, !digitalRead(GPIO_LED4) );
	strcpy(str, (char *)args);
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR long_press_2s_cb(void *args)
#else
void long_press_2s_cb(void *args)
#endif
{
	digitalWrite( GPIO_LED3, !digitalRead(GPIO_LED3) );
	strcpy(str, (char *)args);
}

#ifdef ESP_NONOS
void ICACHE_FLASH_ATTR long_press_3s_cb(void *args)
#else
void long_press_3s_cb(void *args)
#endif
{
	digitalWrite( GPIO_LED2, !digitalRead(GPIO_LED2) );
	strcpy(str, (char *)args);
}

void ICACHE_FLASH_ATTR startfunc()
{
	
    // выполняется один раз при старте модуля.
	button = create_button(GPIO_BTN, 0);

    button_add_short_press(button, 1, short_press1_cb, "short press 1 callback");
    button_add_short_press(button, 2, short_press2_cb, "short press 2 callback");
    button_add_short_press(button, 3, short_press3_cb, "short press 3 callback");

    button_add_long_press(button, 1000, long_press_1s_cb, "long press 1s callback");
    button_add_long_press(button, 2000, long_press_2s_cb, "long press 2s callback");
    button_add_long_press(button, 3000, long_press_3s_cb, "long press 3s callback");
	
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
    os_sprintf(HTTPBUFF,"<br>press count: %d", button->short_press_count);
    os_sprintf(HTTPBUFF,"<br>%s", str);
}