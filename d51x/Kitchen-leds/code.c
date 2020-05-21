#define FW_VER "1.3"

#define GPIO_PIR 1
//#define GPIO_LED_RED 4
//#define GPIO_LED_GREEN 5

#define GPIO_LED_White 12
#define PWM_LED_White 3

#define FADE_DELAY 35
#define BRIGHTNESS_STEPS 20
static uint8_t brightness[BRIGHTNESS_STEPS] = {0,1,2,3,4,6,8,12,16,22,29,39,51,67,86,109,136,170,209,255};   // 0-100%, средняя, S=(S1+S2)/2
static uint8_t brightness_steps_value = 0;
static uint8_t brightness_direction = 1;  // 1 - увеличить, 0 - уменьшить

uint8_t fade_delay = FADE_DELAY;   // sensors_param.cfgdes[0]

uint8_t pir = 0;
uint8_t processed = 0;
uint8_t sunset = 0;

static volatile os_timer_t timer_gpio_read;
static volatile os_timer_t led_strip_fade_timer;

void ICACHE_FLASH_ATTR led_strip_fade_cb() {
    uint8_t finished = 0;
    analogWrite(PWM_LED_White, brightness[brightness_steps_value]);
    if ( brightness_direction ) {
        // плавно зажигаем
        brightness_steps_value++;
        if ( brightness_steps_value >= BRIGHTNESS_STEPS ) finished = 1;
    } else {
        // плавно гасим
        brightness_steps_value--;
        if ( brightness_steps_value == 0 ) finished = 1;
    }    
    if ( finished ) {
        os_timer_disarm(&led_strip_fade_timer);
        processed = 0;
    }
}

void ICACHE_FLASH_ATTR led_strip_fade_start(uint8_t direction) {
    if ( processed ) return;
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

void ICACHE_FLASH_ATTR gpio_read_cb() {
    uint8_t val = GPIO_ALL_GET(GPIO_PIR);
    if ( val != pir ) { // значение изменилось
        pir = val;

        // запустить цикл включения/выключения ленты
        led_strip_fade_start(pir);
    }
}

void ICACHE_FLASH_ATTR get_options() {
    fade_delay = sensors_param.cfgdes[0] > 255 ? FADE_DELAY : sensors_param.cfgdes[0];
    //sunset = valdes[0];
}

void ICACHE_FLASH_ATTR startfunc() {

    get_options();

    os_timer_disarm(&timer_gpio_read);
    os_timer_setfn(&timer_gpio_read, (os_timer_func_t *) gpio_read_cb, NULL);
    os_timer_arm(&timer_gpio_read, 100, 1);         
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    get_options();
}

void webfunc(char *pbuf) {
    os_sprintf(HTTPBUFF,"<br><b>Движение:</b> %s", pir ? "Да" : "Нет");
    os_sprintf(HTTPBUFF,"<br><b>Версия прошивки:</b> %s", FW_VER);
}