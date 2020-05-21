// выключаем подсветку только после того, как спустя 10 сек после отсутствия движения больше не будет движения
/* History
    3.1 поддержка IR
    -----------------------------------------------------------------------
    4.1
    управление rgb через hsv
    rgb эффекты 
    управление разными цветами с пульта
    цветовые эффекты
        плавное изменение WHEEL
    ----------------------------------------------------------------------
*/

/* Options
    valdes[0] - sunset -  флаг авто включения белой ленты
    valdes[1] - auto_mode -  авто режим по датчику движения
    valdes[2] - int32 color code (hex) for RGB
    valdes[3] - debug mode for rgb

    cfgdes[0] - fade_delay - время свечения на каждом уровне pwm (логичнее переименовать в .....)
    cfgdes[1] - fade_out_delay - через сколько погасить ленту (задержка выключения), если с датчика движения пришел сигнал на выключение
    cfgdes[2] - auto_mode -  авто режим по датчику движения
*/

#define FW_VER "4.29"

uint8_t mm = 0;
    // row 1
#define IR_CODE_ONOFF                    0x10EFD02F       // ALL OFF
#define IR_CODE_MODE                     0x10EF30CF       // Auto Mode
#define IR_CODE_PLAY_PAUSE               0x10EF2AD5       // FAN Attic   ?????  
#define IR_CODE_MUTE                     0x10EF12ED       // FAN Kitchen
    //row 2
#define IR_CODE_BND_SYS                  0x10EF827D         // JUMP 7 RAINBOW
#define IR_CODE_UP                       0x10EF807F         // JUMP 3 RGB
#define IR_CODE_TITLE                    0x10EF40BF         // COLOR WHEEL
#define IR_CODE_SUB_T                    0x10EFC03F         // mode sunset
    //row 3
#define IR_CODE_LEFT                     0x10EF52AD         // FADE 7 RAINBOW
#define IR_CODE_ENTER                    0x10EF20DF         // FADE 3 RGB
#define IR_CODE_RIGHT                    0x10EFA05F         // FADE 1 CURRENT
#define IR_CODE_SETUP                    0x10EF609F         // SPEED UP
    // row 4
#define IR_CODE_STOP_BACK                0x10EF28D7         // COLOR OLIVE
#define IR_CODE_DOWN                     0x10EFE01F         // COLOR TEAL
#define IR_CODE_ANGLE                    0x10EF10EF         // COLOR_DARKPURPLE
#define IR_CODE_SLOW                     0x10EF906F         // SPEED DOWN
    // row 5
#define IR_CODE_AMS_RPT                  0x10EFA857         // COLOR_YELLOW
#define IR_CODE_ST_PROG                  0x10EF8A75         // COLOR_CYAN
#define IR_CODE_VOL_PLUS                 0x10EF4AB5         // COLOR_PURPLE 
#define IR_CODE_ZOOM                     0x10EF40FF         // BRIGHTNESS_UP
    // row 6
#define IR_CODE_LOC_RDM                  0x10EFF807         // COLOR_PINK
#define IR_CODE_SEEK_MINUS               0x10EF38C7         // COLOR_LIGHTGREEN
#define IR_CODE_SEL                      0x10EFB847         // COLOR_VIOLET
#define IR_CODE_SEEK_PLUS                0x10EFCA35         // BRIGHTNESS DOWN
    // row 7
#define IR_CODE_PBC                      0x10EF58A7       // COLOR_ORANGE
#define IR_CODE_OSD                      0x10EF9A65       // COLOR_LIMEGREEN
#define IR_CODE_VOL_MINUS                0x10EF02FD       // COLOR_LIGHTBLUE
#define IR_CODE_AUDIO                    0x10EF6897       // RESET COLORS
    // row 8
#define IR_CODE_BTN_1                    0x10EF629D       // RED Led - Increase
#define IR_CODE_BTN_2                    0x10EF9867       // GREEN Led - Increase
#define IR_CODE_BTN_3                    0x10EFB04F       // BLUE Led - Increase
#define IR_CODE_BTN_4                    0x10EFD22D       // WHITE Led - Increase
    // row 9
#define IR_CODE_BTN_5                    0x10EFFA05       // RED Led - Decrease
#define IR_CODE_BTN_6                    0x10EFDA25       // GREEN Led - Decrease
#define IR_CODE_BTN_7                    0x10EFF20D       // BLUE Led - Decrease
#define IR_CODE_BTN_8                    0x10EFEA15       // WHITE Led - Decrease
    // row 10
#define IR_CODE_BTN_9                    0x10EF7A85       // RED Led
#define IR_CODE_BTN_0                    0x10EF5AA5       // GREEN Led  
#define IR_CODE_BTN_10_PLUS              0x10EF728D       // BLUE Led
#define IR_CODE_GOTO                     0x10EF6A95       // WHITE Led


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

//-----------------------------------------------------------------------------
/* hsv --> rgb */
#define hue_t   uint16_t
#define HUE_MAX   360

/* Colors                                R   G   B*/
const uint8_t COLOR_BLACK[]         = {  0,  0,  0};
// main 7 colors   
const uint8_t COLOR_RED[]           = {255,   0,   0};
const uint8_t COLOR_ORANGE[]        = {255, 128,   0};      
const uint8_t COLOR_YELLOW[]        = {255, 255,   0};
const uint8_t COLOR_GREEN[]         = {  0, 255,   0};
const uint8_t COLOR_CYAN[]          = {  0, 255, 255};
const uint8_t COLOR_BLUE[]          = {  0,   0, 255};
const uint8_t COLOR_PURPLE[]        = {255,   0, 255};
const uint8_t COLOR_WHITE[]         = {255, 255, 255};
// light colors
const uint8_t COLOR_LIMEGREEN[]     = {128, 255,   0}; 
const uint8_t COLOR_LIGHTBLUE[]     = {  0, 128, 255}; 
const uint8_t COLOR_VIOLET[]        = {128,   0, 255}; 
const uint8_t COLOR_LIGHTGREEN[]    = {  0, 255, 128}; 
const uint8_t COLOR_PINK[]          = {255,   0, 128}; 

const uint8_t COLOR_DARKPURPLE[]    = {128,   0, 128};
const uint8_t COLOR_TEAL[]          = {  0, 128, 128};
const uint8_t COLOR_OLIVE[]         = {128, 128,   0};

typedef struct color_rgb_t {
   uint8_t      r;
   uint8_t      g;
   uint8_t      b;
} color_rgb_t;

typedef struct color_hsv_t {
   hue_t      h;
   uint8_t      s;
   uint8_t      v;
} color_hsv_t;

color_hsv_t hsv;
color_rgb_t *rgb;



typedef enum {
    OFF,
    FADE1,
    FADE3,
    FADE7,
    JUMP3,
    JUMP7,
    WHEEL
} color_effect_t;

color_effect_t color_effect = OFF;  // режим анимации

void hsv_to_rgb(volatile color_rgb_t *rgb, const color_hsv_t hsv);
void hex_to_rgb(uint32_t color32, volatile color_rgb_t *rgb);
void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t reset);
void set_color_effect(color_effect_t effect);

void stop_color_effect();
void set_color_effect__wheel(uint16_t start);
void set_color_effect__jump3(uint8_t start);
void set_color_effect__jump7(uint8_t start);
void set_color_effect__fade3();
void set_color_effect__fade7();
void set_color_effect__fade1();

void update_rgb_brightness(uint8_t br);
uint16_t color_effect_speed_inc(uint16_t speed, uint8_t step);
uint16_t color_effect_speed_dec(uint16_t speed, uint8_t step);

uint8_t set_brightness_up(uint8_t br);
uint8_t set_brightness_down(uint8_t br);

#define COLOR_EFFECT_SPEED_MIN 20
#define COLOR_EFFECT_SPEED_MAX 5000

#define COLOR_BRIGHTNESS_MIN 1
#define COLOR_BRIGHTNESS_MAX 255

uint8_t color_brightness = COLOR_BRIGHTNESS_MAX;
#define COLOR_EFFECT_SPEED 300
uint16_t color_effect_speed = COLOR_EFFECT_SPEED; // in msec
uint8_t color_effect_speed_step = 20; // in msec

static volatile os_timer_t timer_color_effect;

uint8_t system_started = 0;
//-----------------------------------------------------------------------------

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

    color_effect = OFF;
    os_timer_disarm(&timer_color_effect);
    hsv.h = hsv.s = hsv.v = 0;
    rgb->r = rgb->g = rgb->b = 0;
    GPIO_ALL(GPIO_FAN, 0);
    // TODO: плавно выключать с текущего уровня яркости !!!!
    analogWrite(PWM_LED_White, 0);
    analogWrite(PWM_LED_RED, 0);
    analogWrite(PWM_LED_GREEN, 0);
    analogWrite(PWM_LED_BLUE, 0);
    analogWrite(PWM_LED_WWHITE, 0);
}

void ICACHE_FLASH_ATTR read_ir_cb() {

    //if (IR_KEYSND != 2 && IR_KEYSND != 3 && IR_KEYSND != 4 && IR_KEYSND != 8 && IR_KEYSND != 12 && IR_KEYSND != 16 && IR_KEYSND != 20 && IR_KEYSND != 24 && IR_KEYSND != 32 && IR_KEYSND != 36 && IR_KEYSND != 40) 
	switch ( IR_KEYSND ) {
        // row 1 -------------------------------------------------------------------
		case IR_CODE_ONOFF:  
			power_off_all();
            valdes[2] = 0; 
			break;
		case IR_CODE_MODE:  
			auto_mode = ! auto_mode;
            sensors_param.cfgdes[2] = auto_mode;
            SAVEOPT
			break;
		case IR_CODE_PLAY_PAUSE:  // fan attic

			break;
		case IR_CODE_MUTE:       // fan kitchen
			//GPIO_ALL_SET(GPIO_FAN, !GPIO_ALL(GPIO_FAN));
			break;

        // row 2 -------------------------------------------------------------------
		case IR_CODE_BND_SYS:       // JUMP 7 RAINBOW
            color_effect = JUMP7;
            set_color_effect( color_effect );            
			break;
		case IR_CODE_UP:            // JUMP 3 RGB
            color_effect = JUMP3;
            set_color_effect( color_effect );            
			break;
		case IR_CODE_TITLE:         // COLOR WHEEL
            color_effect = WHEEL;
            set_color_effect( color_effect );
			break;
		case IR_CODE_SUB_T:  
			sunset = 1;
            valdes[0]=1;
			break;

        // row 3 -------------------------------------------------------------------
        case IR_CODE_LEFT:          // FADE 7 RAINBOW
            color_effect = FADE7;
            set_color_effect( color_effect );            
            break;
        case IR_CODE_ENTER:         // FADE 3 RGB
            color_effect = FADE3;
            set_color_effect( color_effect );            
            break;
        case IR_CODE_RIGHT:         // FADE 1 CURRENT
            color_effect = FADE1;
            set_color_effect( color_effect );            
            break;
        case IR_CODE_SETUP:         // SPEED UP
            color_effect_speed = color_effect_speed_inc(color_effect_speed, color_effect_speed_step);
            break;

        // row 4 -------------------------------------------------------------------
        case IR_CODE_STOP_BACK:     // COLOR_OLIVE
            set_color(COLOR_OLIVE[0], COLOR_OLIVE[1], COLOR_OLIVE[2], 1);
            hsv.h = 60; hsv.s = 255; hsv.v = 128; //50.2*255 / 100;
            break;
        case IR_CODE_DOWN:          // COLOR_TEAL
            set_color(COLOR_TEAL[0], COLOR_TEAL[1], COLOR_TEAL[2], 1);
            hsv.h = 180; hsv.s = 255; hsv.v = 128; //50.2*255 / 100;
            break;
        case IR_CODE_ANGLE:         // COLOR_DARKPURPLE
            set_color(COLOR_DARKPURPLE[0], COLOR_DARKPURPLE[1], COLOR_DARKPURPLE[2], 1);
            hsv.h = 300; hsv.s = 255; hsv.v = 128; //50.2*255 / 100;
            break;
        case IR_CODE_SLOW:          // SPEED DOWN
            color_effect_speed = color_effect_speed_dec(color_effect_speed, color_effect_speed_step);
            break;

        // row 5 -------------------------------------------------------------------
        case IR_CODE_AMS_RPT:       // COLOR_YELLOW
            set_color(COLOR_YELLOW[0], COLOR_YELLOW[1], COLOR_YELLOW[2], 1);
             hsv.h = 60; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_ST_PROG:       // COLOR_CYAN
            set_color(COLOR_CYAN[0], COLOR_CYAN[1], COLOR_CYAN[2], 1);
            hsv.h = 180; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_VOL_PLUS:      // COLOR_PURPLE 
            set_color(COLOR_PURPLE[0], COLOR_PURPLE[1], COLOR_PURPLE[2], 1);
            hsv.h = 300; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_ZOOM:          // BRIGHTNESS_UP
            color_brightness = set_brightness_up(color_brightness);
            update_rgb_brightness(color_brightness);
            break;

        // row 6 -------------------------------------------------------------------
        case IR_CODE_LOC_RDM:       // COLOR_PINK
            set_color(COLOR_PINK[0], COLOR_PINK[1], COLOR_PINK[2], 1);
            hsv.h = 330; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_SEEK_MINUS:    // COLOR_LIGHTGREEN
            set_color(COLOR_LIGHTGREEN[0], COLOR_LIGHTGREEN[1], COLOR_LIGHTGREEN[2], 1);
            hsv.h = 150; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_SEL:           // COLOR_VIOLET
            set_color(COLOR_VIOLET[0], COLOR_VIOLET[1], COLOR_VIOLET[2], 1);
            hsv.h = 300; hsv.s = 255; hsv.v = 128; //50.2*255 / 100;
            break;
        case IR_CODE_SEEK_PLUS:     // BRIGHTNESS DOWN
            color_brightness = set_brightness_down(color_brightness);
            update_rgb_brightness(color_brightness);
            break;
        // row 7 -------------------------------------------------------------------
        case IR_CODE_PBC:           // COLOR_ORANGE
            set_color(COLOR_ORANGE[0], COLOR_ORANGE[1], COLOR_ORANGE[2], 1);
            hsv.h = 30; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_OSD:           // COLOR_LIMEGREEN
            set_color(COLOR_LIMEGREEN[0], COLOR_LIMEGREEN[1], COLOR_LIMEGREEN[2], 1);
            hsv.h = 90; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_VOL_MINUS:     // COLOR_LIGHTBLUE
            set_color(COLOR_LIGHTBLUE[0], COLOR_LIGHTBLUE[1], COLOR_LIGHTBLUE[2], 1);
            hsv.h = 210; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_AUDIO:         // RESET COLORS                     
            set_color(0, 0, 0, 1);
            hsv.h = 0; hsv.s = 0; hsv.v = 0; 
            break;
        // row 8 -------------------------------------------------------------------
        case IR_CODE_BTN_1:         // RED Led - Increase by using brightness array
            hsv.h = 0; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_BTN_2:         // GREEN Led - Increase by using brightness array
            hsv.h = 120; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_BTN_3:         // BLUE Led - Increase by using brightness array
            hsv.h = 240; hsv.s = 255; hsv.v = 255; 
            break;
        case IR_CODE_BTN_4:         // WHITE Led - Increase by using brightness array

            break;
        // row 9 -------------------------------------------------------------------
        case IR_CODE_BTN_5:         // RED Led - Decrease by using brightness array

            break;
        case IR_CODE_BTN_6:         // GREEN Led - Decrease by using brightness array

            break;
        case IR_CODE_BTN_7:         // BLUE Led - Decrease by using brightness array

            break;
        case IR_CODE_BTN_8:         // WHITE Led - Decrease by using brightness array

            break;
        // row 10 -------------------------------------------------------------------
        case IR_CODE_BTN_9:         // RED Led

            break;
        case IR_CODE_BTN_0:         // GREEN Led  

            break;
        case IR_CODE_BTN_10_PLUS:   // BLUE Led

            break;
        case IR_CODE_GOTO:          // WHITE Led
            manual_led_white = !sunset & auto_mode & (pwm_state(PWM_LED_White) == 0);  // сбросить через 1 сек
            break;
	}
}

void ICACHE_FLASH_ATTR get_options() {
    fade_delay = sensors_param.cfgdes[0] > 255 ? FADE_DELAY : sensors_param.cfgdes[0];
    fade_out_delay = sensors_param.cfgdes[1];
    auto_mode = sensors_param.cfgdes[2] > 0 ? 1 : 0;   // valdes[1]
    sunset = valdes[0];
}


// colors --------------------------------------------------
void ICACHE_FLASH_ATTR update_rgb_brightness(uint8_t br){
    if ( color_effect == OFF ) {
            hsv.v = br;
            hsv_to_rgb(rgb, hsv);    
            set_color(rgb->r, rgb->g, rgb->b, 0);
    }
}

uint16_t ICACHE_FLASH_ATTR color_effect_speed_inc(uint16_t speed, uint8_t step) {
    if ( speed > COLOR_EFFECT_SPEED_MIN + step ) return speed - step;
    return speed;
}

uint16_t ICACHE_FLASH_ATTR color_effect_speed_dec(uint16_t speed, uint8_t step) {
    if ( speed < COLOR_EFFECT_SPEED_MAX - step ) return speed + step;
    return speed;
}

uint8_t ICACHE_FLASH_ATTR set_brightness_up(uint8_t br){
    if ( br < COLOR_BRIGHTNESS_MAX) return br = br + 1;
    return br;
}

uint8_t ICACHE_FLASH_ATTR set_brightness_down(uint8_t br){
    if ( br > COLOR_BRIGHTNESS_MIN) return br = br - 1;
    return br;
}

void ICACHE_FLASH_ATTR set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t reset){
    if (reset) {
        stop_color_effect();
    }
    
    pwm_set_duty_iot(r, PWM_LED_RED);
    //analogWrite(PWM_LED_RED,    r);
    //os_delay_us(100);
    //analogWrite(PWM_LED_GREEN,  g);
    pwm_set_duty_iot(g, PWM_LED_GREEN);
    //os_delay_us(100);
    //analogWrite(PWM_LED_BLUE,   b);
    //os_delay_us(100);
    pwm_set_duty_iot(b, PWM_LED_BLUE);
    pwm_start_iot();
}

void ICACHE_FLASH_ATTR stop_color_effect() {
    os_timer_disarm(&timer_color_effect);
    color_effect = OFF;
    hsv.v = 255;
    hsv.s = 255;
}

void ICACHE_FLASH_ATTR set_color_effect__jump3_cb() {
    uint8_t r,g,b;
    r=g=b=0;
    switch (mm) {
        //case 0: rgb->r = COLOR_RED[0]; rgb->g = COLOR_RED[1]; rgb->b = COLOR_RED[2]; break;
        case 0:  r=255; break;
        //case 1: rgb->r = COLOR_GREEN[0]; rgb->g = COLOR_GREEN[1]; rgb->b = COLOR_GREEN[2]; break;
        case 1: g=255; break;
        //case 2: rgb->r = COLOR_BLUE[0]; rgb->g = COLOR_BLUE[1]; rgb->b = COLOR_BLUE[2]; break;
        case 2: b=255; break;
    }

    //analogWrite(PWM_LED_RED,    r);
    pwm_set_duty_iot(r, PWM_LED_RED);
    //analogWrite(PWM_LED_GREEN,  g);
    pwm_set_duty_iot(g, PWM_LED_GREEN);
    //os_delay_us(100);
    //analogWrite(PWM_LED_BLUE,   b);
    pwm_set_duty_iot(b, PWM_LED_BLUE);
    //os_delay_us(100);
    pwm_start_iot();

    //set_color(rgb->r, rgb->g, rgb->b, 0);
    mm++;
    if (mm == 3) mm = 0;

}

void ICACHE_FLASH_ATTR set_color_effect__jump3(uint8_t start) {
    mm = 0;
    os_timer_disarm(&timer_color_effect);
    os_timer_setfn(&timer_color_effect, (os_timer_func_t *) set_color_effect__jump3_cb, NULL);
    os_timer_arm(&timer_color_effect, color_effect_speed*10, 1); 
    
}

void ICACHE_FLASH_ATTR set_color_effect__jump7(uint8_t start) {
    static uint8_t i = 0;
    i = start;
    mm = start;
    switch (i) {
        case 0: rgb->r = COLOR_RED[0]; rgb->g = COLOR_RED[1]; rgb->b = COLOR_RED[2]; break;
        case 1: rgb->r = COLOR_ORANGE[0]; rgb->g = COLOR_ORANGE[1]; rgb->b = COLOR_ORANGE[2]; break;
        case 2: rgb->r = COLOR_YELLOW[0]; rgb->g = COLOR_YELLOW[1]; rgb->b = COLOR_YELLOW[2]; break;
        case 3: rgb->r = COLOR_GREEN[0]; rgb->g = COLOR_GREEN[1]; rgb->b = COLOR_GREEN[2]; break;
        case 4: rgb->r = COLOR_CYAN[0]; rgb->g = COLOR_CYAN[1]; rgb->b = COLOR_CYAN[2]; break;
        case 5: rgb->r = COLOR_BLUE[0]; rgb->g = COLOR_BLUE[1]; rgb->b = COLOR_BLUE[2]; break;
        case 6: rgb->r = COLOR_PURPLE[0]; rgb->g = COLOR_PURPLE[1]; rgb->b = COLOR_PURPLE[2]; break;
    }
    set_color(rgb->r, rgb->g, rgb->b, 0);
    i++;
    if (i == 7) i = 0;
    os_timer_disarm(&timer_color_effect);
    os_timer_setfn(&timer_color_effect, (os_timer_func_t *) set_color_effect__jump7, i);
    os_timer_arm(&timer_color_effect, color_effect_speed*10, 0); 
    
}

void ICACHE_FLASH_ATTR set_color_effect__fade3() {

}

void ICACHE_FLASH_ATTR set_color_effect__fade7() {

}

void ICACHE_FLASH_ATTR set_color_effect__fade1() {

}

void ICACHE_FLASH_ATTR set_color_effect__wheel(uint16_t start){
    static uint16_t i = 0; // 360
    i = start;
    static uint8_t direction = 1;
	hsv.h = i;
    hsv.s = 255; // get from global var
    hsv.v = color_brightness; // get from global var
	hsv_to_rgb(rgb, hsv);    
    set_color(rgb->r, rgb->g, rgb->b, 0);
	if ( direction == 0 ) {
		i--;
	} else {
		i++;
	}	
	if ( i == 0) direction = 1;
	if (i == 360 ) direction = 0;    

    os_timer_disarm(&timer_color_effect);
    os_timer_setfn(&timer_color_effect, (os_timer_func_t *) set_color_effect__wheel, i);
    os_timer_arm(&timer_color_effect, color_effect_speed, 0);     
}

void ICACHE_FLASH_ATTR set_color_effect(color_effect_t effect){

    // reset speed to default
    color_effect_speed = COLOR_EFFECT_SPEED;

    os_timer_disarm(&timer_color_effect);

    switch (effect) {
        case FADE1:
            set_color_effect__fade1();
            break;
        case FADE3:
            set_color_effect__fade3();    
            break;
        case FADE7:
            set_color_effect__fade7();    
            break;
        case JUMP3:
            set_color_effect__jump3(0);
            break;
        case JUMP7:
            set_color_effect__jump7(0);    
            break;
        case WHEEL:
            set_color_effect__wheel(0);
            break;
        default:
            //stop color effect
            stop_color_effect();
            break;
    }
}

/****************************************************************
hue - 0..359	цвет
sat - 0..255	насыщенность
val - 0..255	яркость
*****************************************************************/

void ICACHE_FLASH_ATTR hsv_to_rgb(volatile color_rgb_t *rgb, const color_hsv_t hsv) {
	uint8_t hi,fr, p, q, t;
	volatile uint8_t h_pr;

	if( hsv.s == 0) {
		/* color is grayscale */
		rgb->r = rgb->g = rgb->b = hsv.v;
		return;
	}

	hi = hsv.h / 60;
	h_pr = hsv.h - hi*60; 

	fr = ( h_pr * 255 ) / 60;
	p  = hsv.v * ( 255 - hsv.s ) / 255;
	q  = hsv.v * ( 255 - ( ( hsv.s * fr ) / 255 ) ) / 255;
	t  = hsv.v * ( 255 - ( hsv.s * ( 255 - fr ) / 255 ) ) / 255;

	switch ( hi ) {
		case 0: rgb->r = hsv.v; 	rgb->g = t; 		rgb->b = p; break;
		case 1: rgb->r = q; 		rgb->g = hsv.v; 	rgb->b = p; break;
		case 2: rgb->r = p; 		rgb->g = hsv.v; 	rgb->b = t; break;
		case 3: rgb->r = p; 		rgb->g = q; 		rgb->b = hsv.v; break;
		case 4: rgb->r = t; 		rgb->g = p; 		rgb->b = hsv.v; break;
		case 5: rgb->r = hsv.v; 	rgb->g = p; 		rgb->b = q; break;
	}
}

void ICACHE_FLASH_ATTR hex_to_rgb(uint32_t color32, volatile color_rgb_t *rgb) {
    rgb->r = (color32 >> 16) & 0xff;
    rgb->g = (color32 >> 8) & 0xff;
    rgb->b = color32 & 0xff;
}

void ICACHE_FLASH_ATTR set_hex_color(uint32_t color32) {
    hex_to_rgb(color32, rgb);
    set_color(rgb->r, rgb->g, rgb->b, 1);
}

//----------------------------------------------------------
void ICACHE_FLASH_ATTR startfunc() {

    get_options();

    os_timer_disarm(&timer_gpio_read);
    os_timer_setfn(&timer_gpio_read, (os_timer_func_t *) gpio_read_cb, NULL);
    os_timer_arm(&timer_gpio_read, 100, 1); 

	os_timer_disarm(&timer_read_ir);	
	os_timer_setfn(&timer_read_ir, (os_timer_func_t *) read_ir_cb, NULL);
	os_timer_arm(&timer_read_ir, 250, 1);     

    rgb = (struct color_rgb_t *)os_zalloc(sizeof(struct color_rgb_t));    

    

}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    get_options();
    if ( !pir ) {
        if ( t_fade_out > 0) t_fade_out--;
    } else {
        t_fade_in++;
    }

/*
    if ( timersrc == 30 ) {
        system_started = 1;
        analogWrite(PWM_LED_White, 0);
        delayMicroseconds(2);
        analogWrite(PWM_LED_RED, 0);
        delayMicroseconds(2);
        analogWrite(PWM_LED_GREEN, 0);
        delayMicroseconds(2);
        analogWrite(PWM_LED_BLUE, 0);
        delayMicroseconds(2);
        analogWrite(PWM_LED_WWHITE, 0);
        delayMicroseconds(2);
    }
    */
    //if (valdes[3]) set_hex_color(valdes[2]);
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
    
    os_sprintf(HTTPBUFF,"<br><b>index:</b> %d", mm);
    os_sprintf(HTTPBUFF,"<br><b>Color brightness:</b> %d", color_brightness);
    //os_sprintf(HTTPBUFF,"<br><b>Color effect speed:</b> %d", color_effect_speed);
    //os_sprintf(HTTPBUFF,"<br><b>Color effect speed step:</b> %d", color_effect_speed_step);

    switch (color_effect) {
        case JUMP3: os_sprintf(HTTPBUFF,"<br><b>Color effect:</b> JUMP3"); break;
        case JUMP7: os_sprintf(HTTPBUFF,"<br><b>Color effect:</b> JUMP7"); break;
        case FADE1: os_sprintf(HTTPBUFF,"<br><b>Color effect:</b> FADE1"); break;
        case FADE3: os_sprintf(HTTPBUFF,"<br><b>Color effect:</b> FADE3"); break;
        case FADE7: os_sprintf(HTTPBUFF,"<br><b>Color effect:</b> FADE7"); break;
        case WHEEL: os_sprintf(HTTPBUFF,"<br><b>Color effect:</b> WHEEL"); break;
        case OFF: os_sprintf(HTTPBUFF,"<br><b>Color effect:</b> OFF"); break;
    }

    //os_sprintf(HTTPBUFF,"<br><b>RGB debug:</b> %d", valdes[3]);
    os_sprintf(HTTPBUFF,"<br><b>Color32:</b> %d (#%X), HSV %d %d %d   RGB: %d %d %d", valdes[2], valdes[2], hsv.h, hsv.s, hsv.v, rgb->r, rgb->g, rgb->b);

    os_sprintf(HTTPBUFF,"<br><b>Версия прошивки:</b> %s", FW_VER);
}