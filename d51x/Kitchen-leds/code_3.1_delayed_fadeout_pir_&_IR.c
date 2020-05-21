// выключаем подсветку только после того, как спустя 10 сек после отсутствия движения больше не будет движения

#define FW_VER "3.4"

#define IR_CODE_POWER           284151855
#define IR_CODE_AUDIO           284125335 // button AUDIO, изменяет AutoMode
#define IR_CODE_LED_WHITE       284125845 


#define GPIO_PIR 1
#define GPIO_LED_RED 4
#define GPIO_LED_GREEN 5
#define GPIO_LED_BLUE 14
#define GPIO_LED_White 12
#define GPIO_LED_WWHITE 13
#define GPIO_FAN 15

#define PWM_LED_White 3
#define PWM_LED_RED 0
#define PWM_LED_GREEN 1
#define PWM_LED_BLUE 2
#define PWM_LED_WWHITE 4

#define FADE_DELAY 35
#define FADE_OUT_DELAY 20 //сек
#define BRIGHTNESS_STEPS 20
static uint8_t brightness[BRIGHTNESS_STEPS] = {0,1,2,3,4,6,8,12,16,22,29,39,51,67,86,109,136,170,209,255};   // 0-100%, средняя, S=(S1+S2)/2
static uint8_t brightness_steps_value = 0;
static uint8_t brightness_direction = 1;  // 1 - увеличить, 0 - уменьшить

uint8_t fade_delay = FADE_DELAY;   // sensors_param.cfgdes[0]
uint32_t fade_out_delay = FADE_OUT_DELAY; // sensors_param.cfgdes[1]

uint32_t t_fade_out = 0;
uint32_t t_fade_in = 0;

uint8_t pir = 0;
uint8_t processed = 0;
uint8_t sunset = 1; // valdes[0]
uint8_t auto_mode = 1; // 1 - авторежим, 0 - ручной режим без автоквключения и выключения...  valdes[1]  // sensors_param.cfgdes[2]

uint8_t manual_led_white = 0;

static volatile os_timer_t timer_gpio_read;
static volatile os_timer_t led_strip_fade_timer;
static volatile os_timer_t led_strip_delayed_fadeout_timer;
static volatile os_timer_t timer_read_ir; 

void ICACHE_FLASH_ATTR led_strip_fade_cb() {
    uint8_t finished = 0;
    analogWrite(PWM_LED_White, brightness[brightness_steps_value]);
    if ( brightness_direction ) {
        // плавно зажигаем
        brightness_steps_value++;
        if ( brightness_steps_value >= BRIGHTNESS_STEPS ) finished = 1;
    } else {
        // плавно гасим
        if ( brightness_steps_value == 0 ) finished = 1;
        brightness_steps_value--;
        
    }    
    if ( finished ) {
        os_timer_disarm(&led_strip_fade_timer);
        processed = 0;
        t_fade_out = 0;
    }
}

void ICACHE_FLASH_ATTR led_strip_fade_start(uint8_t direction) {
    
    if ( processed ) return;
    if ( direction ) os_timer_disarm(&led_strip_delayed_fadeout_timer); // отменим таймер отложенного выключения

    processed = 1;
    brightness_direction = direction;
    if ( direction ) {  
        // 1 - вверх=зажечь
        brightness_steps_value = 0;
    } else {
        // 0 - вниз=погасить
        brightness_steps_value = BRIGHTNESS_STEPS-1;
    }

    os_timer_disarm(&led_strip_fade_timer);
    os_timer_setfn(&led_strip_fade_timer, (os_timer_func_t *) led_strip_fade_cb, NULL);
    os_timer_arm(&led_strip_fade_timer, fade_delay, 1);  

}

void ICACHE_FLASH_ATTR led_strip_delayed_fadeout_cb() {
    // гасим ленту через 20 сек после прихода сигнала отсутствия движения
    if ( !pir ) {
        if ( pwm_state(PWM_LED_White) > 0) {  // лента не выключена
            led_strip_fade_start(0);
            manual_led_white = 0;
        }
    }
}

void ICACHE_FLASH_ATTR led_strip_delayed_fadeout_start(){
    os_timer_disarm(&led_strip_delayed_fadeout_timer);
    os_timer_setfn(&led_strip_delayed_fadeout_timer, (os_timer_func_t *) led_strip_delayed_fadeout_cb, NULL);
    os_timer_arm(&led_strip_delayed_fadeout_timer, fade_out_delay * 1000, 0);    // разовый таймер на 20 сек
}

void ICACHE_FLASH_ATTR gpio_read_cb() {
    uint8_t val = GPIO_ALL_GET(GPIO_PIR);
    if ( val != pir ) { // значение изменилось
        pir = val;
        if ( !pir ) {
            // пришел сигнал отсутсвия движения
            t_fade_out = fade_out_delay;
            t_fade_in = 0;
            // в автоматическом режиме можно выключать по сигналу с датчика движения не зависимо от опции Закат
            if ( auto_mode ) {
                    led_strip_delayed_fadeout_start();
            }
        } else {
            // запустить цикл включения
            // в автоматическом режиме можно включать по сигналу с датчика движения только при опции Закат
            if ( (auto_mode && sunset) || (auto_mode && manual_led_white )) {  
                if ( pwm_state(PWM_LED_White) == 0)  { // если яркость больше 0, то ничего не делаем, чтобы не было мерцания
                    t_fade_in = 0;
                    led_strip_fade_start(1);  // поджигаем только если до этого не был запущен отложенный таймер на выключение
                }
            }
        }
    }
}


void ICACHE_FLASH_ATTR power_off_all() {
    GPIO_ALL(GPIO_FAN, 0);
    // TODO: плавно выключать с текущего уровня яркости !!!!
    analogWrite(PWM_LED_White, 0);
    analogWrite(PWM_LED_RED, 0);
    analogWrite(PWM_LED_GREEN, 0);
    analogWrite(PWM_LED_BLUE, 0);
    analogWrite(PWM_LED_WWHITE, 0);
}

void ICACHE_FLASH_ATTR read_ir_cb() {

	switch ( IR_KEYSND ) {
		case IR_CODE_POWER:  
			power_off_all();
			break;
		case IR_CODE_AUDIO:  
			auto_mode = ! auto_mode;
            sensors_param.cfgdes[2] = auto_mode;
            SAVEOPT
			break;
        case IR_CODE_LED_WHITE:
            manual_led_white = !sunset & auto_mode & (pwm_state(PWM_LED_White) == 0);  // сбросить через 1 сек
            break;    
        // TODO: добавить обработку кнопок управления RGB - сделать шаг логарифмический по табличке, 20 значений увеличить до 40 и управлять индексом массива яркости    
	}
}

void ICACHE_FLASH_ATTR get_options() {
    fade_delay = sensors_param.cfgdes[0] > 255 ? FADE_DELAY : sensors_param.cfgdes[0];
    fade_out_delay = sensors_param.cfgdes[1];
    auto_mode = sensors_param.cfgdes[2] > 0 ? 1 : 0;   // valdes[1]
    sunset = valdes[0];
}

void ICACHE_FLASH_ATTR startfunc() {

    get_options();

    os_timer_disarm(&timer_gpio_read);
    os_timer_setfn(&timer_gpio_read, (os_timer_func_t *) gpio_read_cb, NULL);
    os_timer_arm(&timer_gpio_read, 100, 1); 

	os_timer_disarm(&timer_read_ir);	
	os_timer_setfn(&timer_read_ir, (os_timer_func_t *) read_ir_cb, NULL);
	os_timer_arm(&timer_read_ir, 200, 1);            
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    get_options();
    if ( !pir ) {
        if ( t_fade_out > 0) t_fade_out--;
    } else {
        t_fade_in++;
    }
}

void webfunc(char *pbuf) {
    os_sprintf(HTTPBUFF,"<br><b>Режим:</b> %s", auto_mode ? "Авто" : "Ручной");
    os_sprintf(HTTPBUFF,"<br><b>Закат:</b> %s", sunset ? "Да" : "Нет");
    os_sprintf(HTTPBUFF,"<br><b>Движение:</b> %s", pir ? "Да" : "Нет");
    if ( !pir ) os_sprintf(HTTPBUFF,"<br><b>Осталось сек до выключения:</b> %d", t_fade_out);
    os_sprintf(HTTPBUFF,"<br><b>Прошло сек после начала движения:</b> %d", t_fade_in);
 
    //os_sprintf(HTTPBUFF,"<br><b>brightness_steps_value:</b> %d", brightness_steps_value);
    //os_sprintf(HTTPBUFF,"<br><b>brightness_value:</b> %d", brightness[brightness_steps_value-1]);
 
    os_sprintf(HTTPBUFF,"<br><b>Fade delay (msec):</b> %d", fade_delay);
    os_sprintf(HTTPBUFF,"<br><b>Fade out delay (sec):</b> %d", fade_out_delay);
    os_sprintf(HTTPBUFF,"<br><b>Версия прошивки:</b> %s", FW_VER);
}