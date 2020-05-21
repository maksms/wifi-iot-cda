#define hue_t   uint16_t
#define HUE_MAX   360

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


/****************************************************************
hue - 0..359	цвет
sat - 0..255	насыщенность
val - 0..255	яркость
*****************************************************************/
void hsv_to_rgb(volatile color_rgb_t *rgb, const color_hsv_t hsv) {
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
   
void hex_to_rgb(uint32_t color32, volatile color_rgb_t *rgb) {
    rgb->r = (color32 >> 16) & 0xff;
    rgb->g = (color32 >> 8) & 0xff;
    rgb->b = color32 & 0xff;
}

#define PWM_LED_RED 0
#define PWM_LED_GREEN 1
#define PWM_LED_BLUE 2
	color_hsv_t hsv;
	color_rgb_t *rgb;

static volatile os_timer_t timer_color;

void ICACHE_FLASH_ATTR color_cb() {
	static uint16_t i = 360;
    static uint8_t direction = 0;
	hsv.h = i;
	hsv_to_rgb(rgb, hsv);

    analogWrite(PWM_LED_RED,     rgb->r);
    analogWrite(PWM_LED_GREEN,   rgb->g);
    analogWrite(PWM_LED_BLUE,    rgb->b);

	if ( direction == 0 ) {
		i--;
	} else {
		i++;
	}	
	if ( i == 0) direction = 1;
	if (i == 360 ) direction = 0;
}

void ICACHE_FLASH_ATTR
startfunc(){
// выполняется один раз при старте модуля.
rgb = (struct color_rgb_t *)os_zalloc(sizeof(struct color_rgb_t));

    os_timer_disarm(&timer_color);
    os_timer_setfn(&timer_color, (os_timer_func_t *) color_cb, NULL);
    os_timer_arm(&timer_color, 300, 1); 

}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
// выполнение кода каждую 1 секунду

    //hsv.h = valdes[0];
    //hsv.s = valdes[1]*255/100;
    //hsv.v = valdes[2]*255/100;

    //hsv.h = 360;
    hsv.s = valdes[1]*255/100;
    hsv.v = valdes[2]*255/100;
    

    //os_free(rgb);

if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}
}

void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br>ver. 2.4"); // вывод данных на главной модуля
os_sprintf(HTTPBUFF,"<br>HSV: %d : %d : %d", hsv.h, hsv.s*100/255, hsv.v*100/255); 
os_sprintf(HTTPBUFF,"<br>RGB: %d : %d : %d", rgb->r, rgb->g, rgb->b); 
}