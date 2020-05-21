// выключаем подсветку только после того, как спустя 10 сек после отсутствия движения больше не будет движения

#define FW_VER "2.11"

#define GPIO_PIR 1
//#define GPIO_LED_RED 4
//#define GPIO_LED_GREEN 5

#define GPIO_LED_White 12
#define PWM_LED_White 3

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

static volatile os_timer_t timer_gpio_read;
static volatile os_timer_t led_strip_fade_timer;
static volatile os_timer_t led_strip_delayed_fadeout_timer;

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
        led_strip_fade_start(0);
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
        if ( auto_mode && sunset ) {  // автоматически от сигнала датчика движения управляем в авторежиме и при наступлении заката до рассвета (из внешней системы)
            if ( !pir ) {
                // пришел сигнал отсутсвия движения
                t_fade_out = fade_out_delay;
                t_fade_in = 0;
                led_strip_delayed_fadeout_start();
            } else {
                // запустить цикл включения
                if ( pwm_state(PWM_LED_White) == 0)  { // если яркость больше 0, то ничего не делаем, чтобы не было мерцания
                    t_fade_in = 0;
                    led_strip_fade_start(1);  // поджигаем только если до этого не был запущен отложенный таймер на выключение
                }
            }
        }
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