// http://we.easyelectronics.ru/Soft/cvetovye-palitry-dlya-prostranstva-hsv.html

#define PWM_LED_RED 0
#define PWM_LED_GREEN 1
#define PWM_LED_BLUE 2

typedef struct RGB_TypeDef
{
    uint8_t R;  /* Red */
    uint8_t G;  /* Green */
    uint8_t B;  /* Blue */                  
} RGB_TypeDef;

typedef struct HSV_TypeDef
{
    uint8_t H;  /* Hue */
    uint8_t S;  /* Saturation */
    uint8_t V;  /* Value */                
} HSV_TypeDef;   

typedef enum
{
    RAINBOW,      /* Standart rainbow from red to purple*/
    YODA_RAINBOW, /* Rainbow with colors in other order*/
    SUNNY,        /* Yellow, orange, red mix */
    COLDY,        /* Cyan, light blue and blue mix */
    GREENY,       /* Hues of green and yellow */
    RUSSIA        /* Russian Federation flag colors =) */
} CPallete_Name_TypeDef;

typedef struct 
{
    RGB_TypeDef *Colors;
    uint8_t ColorsTotal;                 
} CPallete_TypeDef;

/* Colors            R   G   B*/
#define WHITE      {255,255,255}
#define RED        {255,  0,  0}
#define ORANGE     {255,128,  0}
#define YELLOW     {255,255,  0}
#define GREENLIME  {191,255,  0}
#define LIGHTGREEN {128,255,  0}
#define GREEN      {  0,255,  0}
#define CYAN       {  0,255,255}
#define LIGHTBLUE  {  0,128,255}
#define BLUE       {  0,  0,255}
#define PURPLE     {128,  0,255}

/* Color palletes
 *
 * Strongly recomended to set _COLORS_TOTAL with one of this values:
 * 2, 4, 8, 16, 32, 64, 128
 */ 
 
 /*RAINBOW*/
    #define RAINBOW_COLORS_TOTAL    8
    RGB_TypeDef rainbow_colors[RAINBOW_COLORS_TOTAL] = {RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, RED};                  
    #define _RAINBOW  {rainbow_colors, RAINBOW_COLORS_TOTAL}  

/*YODA_RAINBOW*/  
    #define YODA_RAINBOW_COLORS_TOTAL    8
    RGB_TypeDef yoda_rainbow_colors[YODA_RAINBOW_COLORS_TOTAL] = {CYAN, PURPLE, BLUE, YELLOW, GREEN, ORANGE, RED, CYAN};                  
    #define _YODA_RAINBOW  {yoda_rainbow_colors, YODA_RAINBOW_COLORS_TOTAL}  

/*SUNNY*/
    #define SUNNY_COLORS_TOTAL      4
    RGB_TypeDef sunny_colors[SUNNY_COLORS_TOTAL] = {YELLOW, ORANGE, RED, YELLOW};                  
    #define _SUNNY  {sunny_colors, SUNNY_COLORS_TOTAL} 

/*COLDY*/
    #define COLDY_COLORS_TOTAL      4
    RGB_TypeDef coldy_colors[COLDY_COLORS_TOTAL] = {CYAN, LIGHTBLUE, BLUE, CYAN};                  
    #define _COLDY  {coldy_colors, COLDY_COLORS_TOTAL} 

/*GREENY*/
    #define GREENY_COLORS_TOTAL     4
    RGB_TypeDef greeny_colors[GREENY_COLORS_TOTAL] = {LIGHTGREEN, GREENLIME, YELLOW, LIGHTGREEN};                  
    #define _GREENY  {greeny_colors, GREENY_COLORS_TOTAL} 

/*RUSSIA*/
    #define RUSSIA_COLORS_TOTAL     4
    RGB_TypeDef russia_colors[RUSSIA_COLORS_TOTAL] = {WHITE, BLUE, RED, WHITE};                  
    #define _RUSSIA  {russia_colors, RUSSIA_COLORS_TOTAL} 

/*List of color palletes must be the same order as in CPallete_Name_TypeDef*/
const CPallete_TypeDef cpallete[] = 
{
    _RAINBOW,
    _YODA_RAINBOW, 
    _SUNNY, 
    _COLDY, 
    _GREENY, 
    _RUSSIA
}; 

/**
  * brief   Convert HSV values to RGB in the current color pallete
  * params  
    *       HSV : struct to get input values
    *       RGB : struct to put calculated values
    *       name: color pallete name
  * retval  none
  */
void hsv2rgb(HSV_TypeDef* HSV, RGB_TypeDef* RGB, CPallete_Name_TypeDef name)
{
    uint8_t tempR;
    uint8_t tempG;
    uint8_t tempB;
    int16_t diff;

    uint8_t sector_basecolor;
    uint8_t next_sector_basecolor;
    uint8_t hues_per_sector;  
    uint8_t hue_in_sector;  

    hues_per_sector = 256 / cpallete[name].ColorsTotal;
    hue_in_sector = HSV->H % hues_per_sector; 

    sector_basecolor = HSV->H / hues_per_sector;
    if(sector_basecolor == (cpallete[name].ColorsTotal-1))
        next_sector_basecolor = 0;
    else next_sector_basecolor = sector_basecolor + 1;
    
    /* Get Red from Hue */
    diff = ((cpallete[name].Colors[next_sector_basecolor].R-cpallete[name].Colors[sector_basecolor].R)/hues_per_sector)*hue_in_sector; 
    if((cpallete[name].Colors[sector_basecolor].R + diff) < 0)
        tempR = 0;
    else if((cpallete[name].Colors[sector_basecolor].R + diff) > 255)
        tempR = 255;
    else tempR = (uint8_t)(cpallete[name].Colors[sector_basecolor].R + diff); 

    /* Get Green from Hue */
    diff = ((cpallete[name].Colors[next_sector_basecolor].G-cpallete[name].Colors[sector_basecolor].G)/hues_per_sector)*hue_in_sector; 
    if((cpallete[name].Colors[sector_basecolor].G + diff) < 0)
        tempG = 0;
    if((cpallete[name].Colors[sector_basecolor].G + diff) > 255)
        tempG = 255;
    else tempG = (uint8_t)(cpallete[name].Colors[sector_basecolor].G + diff); 

    /* Get Blue from Hue */
    diff = ((cpallete[name].Colors[next_sector_basecolor].B-cpallete[name].Colors[sector_basecolor].B)/hues_per_sector)*hue_in_sector; 
    if((cpallete[name].Colors[sector_basecolor].B + diff) < 0)
        tempB = 0;
    if((cpallete[name].Colors[sector_basecolor].B + diff) > 255)
        tempB = 255;
    else tempB = (uint8_t)(cpallete[name].Colors[sector_basecolor].B + diff); 

    /* Saturation regulation */
    tempR = (255-((255-tempR)*(HSV->S))/255);
    tempG = (255-((255-tempG)*(HSV->S))/255);
    tempB = (255-((255-tempB)*(HSV->S))/255);

    /* Value (brightness) regulation to get final result */
    RGB->R = (tempR*(HSV->V))/255;
    RGB->G = (tempG*(HSV->V))/255;
    RGB->B = (tempB*(HSV->V))/255;
}


RGB_TypeDef *rgb;
HSV_TypeDef *hsv;

void ICACHE_FLASH_ATTR
startfunc(){
// выполняется один раз при старте модуля.
hsv = (struct HSV_TypeDef *)os_zalloc(sizeof(struct HSV_TypeDef));
rgb = (struct RGB_TypeDef *)os_zalloc(sizeof(struct RGB_TypeDef));
}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
// выполнение кода каждую 1 секунду
    
	// red  255, 0, 0
    //hsv.H = 0;
    //hsv.S = 100;
    //hsv.V = 100;

	// green  0, 255, 0
    hsv->H = valdes[0];
    hsv->S = valdes[1];
    hsv->V = valdes[2];
	
    
    hsv2rgb(hsv, rgb, RAINBOW);


    analogWrite(PWM_LED_RED,     rgb->R);
    analogWrite(PWM_LED_GREEN,   rgb->G);
    analogWrite(PWM_LED_BLUE,    rgb->B);

    //os_free(rgb);
    //os_free(hsv);

if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}
}

void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br>ver. 3.1"); // вывод данных на главной модуля
os_sprintf(HTTPBUFF,"<br>HSV: %d : %d : %d", hsv->H, hsv->S, hsv->V); 
os_sprintf(HTTPBUFF,"<br>RGB: %d : %d : %d", rgb->R, rgb->G, rgb->B); 
}