static os_timer_t esp_timer; static uint8_t data[255];
 char s,m, on,re1,gr1,bl1,cola1,cola2,tim, tim1;
 int col1, col2;

void ICACHE_FLASH_ATTR migal()
{
if (on==1) {
for (s=0;s<m;s++){

if (col1 >0 & col1 <255) 		{ re1=255; 				gr1=col1;					bl1=0;    }
if (col1 >255 & col1 <512)  	{ re1=255-(col1-255); 	gr1=255;  				bl1=0;  }
if (col1 >512 & col1 <768)  	{ re1=0;  					gr1=255;   				bl1=col1-512;}
if (col1 >768 & col1 <1024)  	{ re1=0;  					gr1=255-(col1-768); 	bl1=255; }
if (col1 >1024 & col1 <1280)	{ re1=col1-1024;   		gr1=0;   					bl1=255;}
if (col1 >1280 & col1 <1536) 	{ re1=255;   				gr1=0;  					bl1=(255-(col1-1280)); }

if (s%3==0) {data[s] = gr1;}  
if (s%3==1) {data[s] = re1;}  
if (s%3==2) {data[s] = bl1; col1+=cola1; if (col1>1536) {col1-=1536;}}}
col2+=cola2;  if (col2>1536) {col2-=1536;}
col1=col2;

ws2812_push(data, m);}
 }
 
void ICACHE_FLASH_ATTR startfunc(){
	os_timer_disarm(&esp_timer);
os_timer_setfn(&esp_timer, (os_timer_func_t *) migal, NULL);
os_timer_arm(&esp_timer, 1000, 1);
}

void ICACHE_FLASH_ATTR  timerfunc(uint32_t  timersrc) {
on= (sensors_param.cfgdes[0]);		// выключатель
m= (sensors_param.cfgdes[1])*3;	// кол-во диодов на ленте
cola1= (sensors_param.cfgdes[2]);	// шаг цвета между диодами
cola2= (sensors_param.cfgdes[3]);		//шаг анимации
tim= (sensors_param.cfgdes[4]);		//скорость анимации

if (tim != tim1){ tim1=tim;
os_timer_disarm(&esp_timer);
os_timer_setfn(&esp_timer, (os_timer_func_t *) migal, NULL);
os_timer_arm(&esp_timer, tim*10, 1);
}

}

//col1=cola1*5; 
void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br>tik = %d",re1); // вывод данных на главной модуля
}
