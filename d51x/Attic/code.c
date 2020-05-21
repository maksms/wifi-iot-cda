#define FW_VER "1.12"
#define GPIO_FAN 0
#define GPIO_REDALERT 2
#define PWM_REDALERT 0

#define GPIO_AUTO_MODE 6
#define GPIO_BLYNK_ENABLED 4

#define REDALERT_BLYNK_TIME 3  // сек
#define REDALERT_BLYNK_DELAY 100  // мсек

#define MQTTD
#ifdef MQTTD
MQTT_Client* mqtt_client;
static char payload[20];
#endif

uint8_t auto_mode = 1;      // valdes[0] управляется через GPIO6, 1 - RedAlert управляется автоматически, 0 - RedAlert управляется вручную
uint8_t blynk_enabled = 1; // valdes[1] - 1 - разрешено / запрещено  а так же через GPIO4
uint8_t present = 0; // valdes[2] - 1 - кто-то есть дома / 0 - никого нет дома (сейчас определяется в OpenHAB)
uint8_t alarm = 0; // valdes[3] -  сигнал с датчика движения камера (отправляет OpenHAB)
uint8_t alarmed = 0; // идет процесс мигания RedAlert
uint8_t alarm_blynk_time = REDALERT_BLYNK_TIME;
uint32_t alarm_blynk_delay = REDALERT_BLYNK_DELAY;
uint8_t pwm_fan = 0; //valdes[4] - уровень можщности вентилятора от 0 до 100 %

#define BRIGHTNESS_STEPS 18
static uint8_t brightness[BRIGHTNESS_STEPS] = {2,3,4,6,8,12,16,22,29,39,51,67,86,109,136,170,209,255};   // 0-100%, средняя, S=(S1+S2)/2
static uint8_t brightness_steps_value = 0;
static uint8_t brightness_direction = 1;  // 1 - увеличить, 0 - уменьшить

static os_timer_t redalert_blynk_timer;
static os_timer_t redalert_control_timer;

#define millis() (uint32_t) (micros() / 1000ULL) 

void ICACHE_FLASH_ATTR get_options();
void ICACHE_FLASH_ATTR redalert_blynk_start();
void ICACHE_FLASH_ATTR redalert_blynk_cb(uint32_t t);
void ICACHE_FLASH_ATTR redalert_control_cb();


void ICACHE_FLASH_ATTR redalert_blynk_start() {
    if ( alarmed ) return;
    if ( alarm_blynk_time == 0) return;
    alarmed = 1;
    brightness_steps_value = BRIGHTNESS_STEPS-1;
    brightness_direction = 0;
    os_timer_disarm(&redalert_blynk_timer);
    os_timer_setfn(&redalert_blynk_timer, (os_timer_func_t *) redalert_blynk_cb, millis());
    os_timer_arm(&redalert_blynk_timer, alarm_blynk_delay, 1);  
}

void ICACHE_FLASH_ATTR redalert_blynk_cb(uint32_t t) {
    static uint32_t td = 0;
    if ( ( millis() - t) > alarm_blynk_time*1000 ) {
        os_timer_disarm(&redalert_blynk_timer);
        GPIO_ALL(GPIO_REDALERT, !present && auto_mode);
        alarmed = 0;
        valdes[3] = 0;
        brightness_steps_value = (present && auto_mode) ? 0 : BRIGHTNESS_STEPS-1;
        brightness_direction = (present && auto_mode) ? 1 : 0;
    } else {
        // плавно зажигаем и гасим   
        analogWrite(PWM_REDALERT, brightness[brightness_steps_value]);
        if ( brightness_direction ) {
            // плавно зажигаем
            brightness_steps_value++;
            if ( brightness_steps_value >= BRIGHTNESS_STEPS-1 ) { 
                brightness_steps_value = BRIGHTNESS_STEPS-1;
                // delay 200
                if ( millis() - td > 200 ) {
                    brightness_direction = 0; 
                }
            } else {
                td = millis();
            }
        } else {
            // плавно гасим
            brightness_steps_value--;
            if ( brightness_steps_value == 0 ) brightness_direction = 1;   
        }
    }
}

void ICACHE_FLASH_ATTR redalert_control_cb(){
    uint8_t st = GPIO_ALL_GET(GPIO_BLYNK_ENABLED);
    if ( blynk_enabled != st ) {
        // state is changed, send update
        #ifdef MQTTD
        if ( sensors_param.mqtten && mqtt_client != NULL) {
	        os_memset(payload, 0, 20);
	        os_sprintf(payload,"%d", st);
	        MQTT_Publish(mqtt_client, "valuedes1", payload, os_strlen(payload), 2, 0, 1);
	        os_delay_us(20);
	        system_soft_wdt_feed();	
        }
        #endif
        blynk_enabled = valdes[1] = st;
    }
    

    st = GPIO_ALL_GET( GPIO_AUTO_MODE );
    if ( auto_mode != st ) {
        // state is changed, send update
        #ifdef MQTTD
        if ( sensors_param.mqtten && mqtt_client != NULL) {
	        os_memset(payload, 0, 20);
	        os_sprintf(payload,"%d", st);
	        MQTT_Publish(mqtt_client, "valuedes0", payload, os_strlen(payload), 2, 0, 1);
	        os_delay_us(20);
	        system_soft_wdt_feed();	
        }
        #endif
        auto_mode = valdes[0] = st; 
    }   
    
}

void ICACHE_FLASH_ATTR get_options() {
    alarm_blynk_time = sensors_param.cfgdes[0] > 100 ? REDALERT_BLYNK_TIME : sensors_param.cfgdes[0];
    alarm_blynk_delay = (sensors_param.cfgdes[1] > 1000 || sensors_param.cfgdes[1] < 10) ? REDALERT_BLYNK_DELAY : sensors_param.cfgdes[1];
}

void ICACHE_FLASH_ATTR startfunc() {
    // выполняется один раз при старте модуля.
    alarmed = 0;
    present = 0;
    blynk_enabled = 1;
    auto_mode = 1;

    get_options();

    #ifdef MQTTD
        mqtt_client = (MQTT_Client*) &mqttClient;
    #endif

    os_timer_disarm(&redalert_control_timer);
    os_timer_setfn(&redalert_control_timer, (os_timer_func_t *) redalert_control_cb, NULL);
    os_timer_arm(&redalert_control_timer, 100, 1);      
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду

    auto_mode = valdes[0];
    GPIO_ALL(GPIO_AUTO_MODE, auto_mode);

    blynk_enabled = valdes[1];
    GPIO_ALL(GPIO_BLYNK_ENABLED, blynk_enabled);

    present = (valdes[2] > 0 );
    alarm = (valdes[3] > 0 );
    pwm_fan = ( valdes[4] > 100 ) ? 100 : valdes[4];

    get_options();

    if ( auto_mode && !alarmed ) GPIO_ALL(GPIO_REDALERT, !present); // выставим состояние только если не мигаем
    if ( auto_mode && alarm && !present && blynk_enabled) {
        // плавно мигаем RedAlert 3 сек, если никого нет дома и пришел сигнал с камеры
        redalert_blynk_start();
    }
}

void webfunc(char *pbuf) {
    os_sprintf(HTTPBUFF,"<br><b>Режим:</b> %s", auto_mode ? "Авто" : "Ручной");
    os_sprintf(HTTPBUFF,"<br><b>Мигание разрешено:</b> %s", blynk_enabled ? "Да" : "Нет");
    os_sprintf(HTTPBUFF,"<br><b>Присутствие:</b> %s", present ? "Да" : "Нет");
    os_sprintf(HTTPBUFF,"<br><b>Тревога:</b> %s", alarm ? "Да" : "Нет");
    
    os_sprintf(HTTPBUFF,"<br><b>Fan pwm:</b> %d %%", pwm_fan);
    os_sprintf(HTTPBUFF,"<br><b>Версия прошивки:</b> %s", FW_VER);
}