//https://www.radiokot.ru/forum/viewtopic.php?p=2969222


/// если этот макрос есть, оттенок (HUE для HSV-модели цвета) будет в градусах
/// иначе в условных единицах от 0 до 255
#define HUE_SMOOTH

#if defined(HUE_SMOOTH)
#define hue_t   uint16_t
#define HUE_MAX   360
#else
#define hue_t   uint8_t
#define HUE_MAX   256
#endif


#define PWM_LED_RED 0
#define PWM_LED_GREEN 1
#define PWM_LED_BLUE 2

#define HUE_QUADRANT   ((HUE_MAX + 5)/ 6)

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

void hsv_to_rgb(const color_hsv_t *hsv, color_rgb_t *rgb);
void rgb_to_hsv(const color_rgb_t *rgb, color_hsv_t *hsv);

void ICACHE_FLASH_ATTR hsv_to_rgb(const color_hsv_t *hsv, color_rgb_t *rgb){
   uint8_t region, fpart, p, q, t;

   if(hsv->s == 0) {
      /* color is grayscale */
      rgb->r = rgb->g = rgb->b = hsv->v;
      return;
   }

   /* make hue 0-5 */
   region = hsv->h / HUE_QUADRANT;     // region <--> hi
   /* find remainder part, make it from 0-255 */
   fpart = (hsv->h - (region * HUE_QUADRANT)) * 6;

   /* calculate temp vars, doing integer multiplication */
   p = (hsv->v * (255 - hsv->s)) >> 8;
   q = (hsv->v * (255 - ((hsv->s * fpart) >> 8))) >> 8;
   t = (hsv->v * (255 - ((hsv->s * (255 - fpart)) >> 8))) >> 8;

   /* assign temp vars based on color cone region */
   switch(region) {
      case 0:
         rgb->r = hsv->v; rgb->g = t; rgb->b = p; break;
      case 1:
         rgb->r = q; rgb->g = hsv->v; rgb->b = p; break;
      case 2:
         rgb->r = p; rgb->g = hsv->v; rgb->b = t; break;
      case 3:
         rgb->r = p; rgb->g = q; rgb->b = hsv->v; break;
      case 4:
         rgb->r = t; rgb->g = p; rgb->b = hsv->v; break;
      default:
         rgb->r = hsv->v; rgb->g = p; rgb->b = q; break;
   }
}

#define MIN(x,y)   ((x) < (y))?(x):(y)
#define MAX(x,y)   ((x) > (y))?(x):(y)

void ICACHE_FLASH_ATTR rgb_to_hsv(const color_rgb_t *rgb, color_hsv_t *hsv){
   uint8_t min, max, delta;
   int16_t hsvh;

   min = MIN(rgb->r, MIN(rgb->g, rgb->b));
   max = MAX(rgb->r, MAX(rgb->g, rgb->b));

   hsv->v = max;                // v, 0..255
   delta = max - min;                      // 0..255, < v

   if(max != 0)
      hsv->s = (int)(delta)*255 / max;        // s, 0..255
   else {
      // r = g = b = 0        // s = 0, v is undefined
      hsv->s = 0;
      hsv->h = 0;
      return;
   }

   if(rgb->r == max)
      hsvh = (rgb->g - rgb->b)*HUE_QUADRANT/delta;        // between yellow & magenta
   else if(rgb->g == max)
      hsvh = (HUE_QUADRANT*2) + (rgb->b - rgb->r)*HUE_QUADRANT/delta;    // between cyan & yellow
   else
      hsvh = (HUE_QUADRANT*4) + (rgb->r - rgb->g)*HUE_QUADRANT/delta;    // between magenta & cyan

   if(hsvh < 0) hsvh += HUE_MAX;

   hsv->h = hsvh;
}

void ICACHE_FLASH_ATTR
startfunc(){
// выполняется один раз при старте модуля.
}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
// выполнение кода каждую 1 секунду
    color_hsv_t hsv;
    hsv.h = valdes[0];
    hsv.s = valdes[1];
    hsv.v = valdes[2];

    color_rgb_t *rgb = (struct color_rgb_t *)os_zalloc(sizeof(struct color_rgb_t));
    hsv_to_rgb(&hsv, rgb);

    analogWrite(PWM_LED_RED,     rgb->r);
    analogWrite(PWM_LED_GREEN,   rgb->g);
    analogWrite(PWM_LED_BLUE,    rgb->b);
    
    os_free(rgb);

if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}
}

void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"HSV: %d : %d : %d", valdes[0], valdes[1], valdes[2]); 
os_sprintf(HTTPBUFF,"<br>RGB: %d : %d : %d", pwm_state(PWM_LED_RED), pwm_state(PWM_LED_GREEN), pwm_state(PWM_LED_BLUE) ); 
}