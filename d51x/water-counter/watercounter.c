os_timer_t gpio_timer;

uint32_t ts1 = 0;
uint32_t ts2 = 0;

#define millis() (unsigned long) (micros() / 1000ULL)
#define GPIO_STATE_CLOSE 0
//#define GPIO_COUNTER1 3
//#define GPIO_COUNTER2 1

#define COUNTER_IMPL 10
//uint32_t counter = 0;

// Количество настроек: Счетчик1 GPIO,Расход1 общий,Расход1 сегодня,Расход1 вчера,Счетчик2 GPIO,Расход2 общий,Расход2 сегодня,Расход2 вчера

#define WATERCNT1_GPIO sensors_param.cfgdes[0]		// счетчик 1 gpio
#define WATERCNT1_CFG sensors_param.cfgdes[1]		// счетчик 1 общий расход
#define WATERCNT1_T_CFG sensors_param.cfgdes[2]		// счетчик 1 расход сегодня
#define WATERCNT1_Y_CFG sensors_param.cfgdes[3]		// счетчик 1 расход вчера

#define WATERCNT2_GPIO sensors_param.cfgdes[4]		// счетчик 2 gpio
#define WATERCNT2_CFG sensors_param.cfgdes[5]		// счетчик 2 общий расход
#define WATERCNT2_T_CFG sensors_param.cfgdes[6]		// счетчик 2 расход сегодня
#define WATERCNT2_Y_CFG sensors_param.cfgdes[7]		// счетчик 2 расход вчера

typedef struct {
	uint32_t watercnt1a;
	uint32_t watercnt1t;
	uint32_t watercnt1y;
	uint32_t watercnt2a;
	uint32_t watercnt2t;
	uint32_t watercnt2y;
	uint32_t crc32;
} rtc_data_t;

rtc_data_t rtc_data;
	

#define ADDLISTSENS {200,LSENSFL3|LSENS32BIT,"WaterCnt1","watercnt1",&WATERCNT1_CFG,NULL}, \
					{201,LSENSFL3|LSENS32BIT,"WaterCnt1Y","watercnt1y",&WATERCNT1_Y_CFG,NULL}, \
					{202,LSENSFL3|LSENS32BIT,"WaterCnt1T","watercnt1t",&WATERCNT1_T_CFG,NULL}, \
					{203,LSENSFL3|LSENS32BIT,"WaterCnt2","watercnt2",&WATERCNT2_CFG,NULL}, \
					{204,LSENSFL3|LSENS32BIT,"WaterCnt2Y","watercnt2y",&WATERCNT2_Y_CFG,NULL}, \
					{205,LSENSFL3|LSENS32BIT,"WaterCnt2T","watercnt2t",&WATERCNT2_T_CFG,NULL}, 


	
uint32_t ICACHE_FLASH_ATTR calcCRC32(const uint8_t *data, uint16_t sz) {
  // Обрабатываем все данные, кроме последних четырёх байтов,
  // где и будет храниться проверочная сумма.
  size_t length = sz-4;
 
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
	uint32_t i;
    for (i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}


void ICACHE_FLASH_ATTR read_gpio_cb() 
{
	// счетик 1
	if ( WATERCNT1_GPIO < 16 && digitalRead(WATERCNT1_GPIO) == GPIO_STATE_CLOSE )
	{
		// геркон замкнут (замыкание начинается на значении бликом к 1, а размыкание на значении после 3)
		// Сами счетчик замыкаются при переходе с 0 на 9 и размыкание происходит при переходе с 2 на 3.
		if ( ts1 == 0 )
		{
			//counter++;
			WATERCNT1_CFG += COUNTER_IMPL;
			WATERCNT1_T_CFG += COUNTER_IMPL;
		}
		
		ts1 = millis();
		
	} else {
		ts1 = 0;
	}
	
	// счетик 2
	if ( WATERCNT2_GPIO < 16 &&  digitalRead(WATERCNT2_GPIO) == GPIO_STATE_CLOSE )
	{
		// геркон замкнут (замыкание начинается на значении бликом к 1, а размыкание на значении после 3)
		if ( ts2 == 0 )
		{
			//counter++;
			WATERCNT2_CFG += COUNTER_IMPL;
			WATERCNT2_T_CFG += COUNTER_IMPL;
		}
		
		ts2 = millis();
		
	} else {
		ts2 = 0;
	}	
}


void ICACHE_FLASH_ATTR startfunc()
{
	system_rtc_mem_read(70, &rtc_data, sizeof(rtc_data_t));
	uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	
	if ( crc32 != rtc_data.crc32 ) 
	{
		// кривые данные, обнулим
		os_memset(&rtc_data, 0, sizeof(rtc_data_t));
		crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
		rtc_data.crc32 = crc32;
		// пишем в rtc обнуленные данные
		system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));		
	}
	
	// проверим сохраненные данные во flash с данными из rtc, если данные во флешь меньше, чем данные в rtc, значит был сброс по питанию, а данные не успели сохраниться во flash
	uint8_t need_save = 0;
	if ( WATERCNT1_CFG < rtc_data.watercnt1a )  
	{
		WATERCNT1_CFG = rtc_data.watercnt1a;
		need_save = 1;
	}
	
	if ( WATERCNT1_Y_CFG < rtc_data.watercnt1y )  
	{
		WATERCNT1_Y_CFG = rtc_data.watercnt1y;
		need_save = 1;
	}
	
	if ( WATERCNT1_T_CFG < rtc_data.watercnt1t )  
	{
		WATERCNT1_T_CFG = rtc_data.watercnt1t;
		need_save = 1;
	}

	if ( WATERCNT2_CFG < rtc_data.watercnt2a )  
	{
		WATERCNT2_CFG = rtc_data.watercnt2a;
		need_save = 1;
	}
	
	if ( WATERCNT2_Y_CFG < rtc_data.watercnt2y )  
	{
		WATERCNT2_Y_CFG = rtc_data.watercnt2y;
		need_save = 1;
	}
	
	if ( WATERCNT2_T_CFG < rtc_data.watercnt2t )  
	{
		WATERCNT2_T_CFG = rtc_data.watercnt2t;
		need_save = 1;
	}
	
	if ( need_save ) SAVEOPT; 
	
	os_timer_disarm(&gpio_timer);
	os_timer_setfn(&gpio_timer, (os_timer_func_t *)read_gpio_cb, NULL);
	os_timer_arm(&gpio_timer, 100, 1);
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {


	rtc_data.watercnt1a = WATERCNT1_CFG;
	rtc_data.watercnt1y = WATERCNT1_Y_CFG;
	rtc_data.watercnt1t = WATERCNT1_T_CFG;
	rtc_data.watercnt2a = WATERCNT2_CFG;
	rtc_data.watercnt2y = WATERCNT2_Y_CFG;
	rtc_data.watercnt2t = WATERCNT2_T_CFG;
	
	uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	rtc_data.crc32 = crc32;
	system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));
	
	if (timersrc%1800==0)  //30*60 сек  каждые 30 мин сохраняем данные во флеш
	{
		SAVEOPT;
	}


	if ( time_loc.hour == 0 && time_loc.min == 0 && time_loc.sec == 0 )
	{
		// обнулить суточные данные ночью
		WATERCNT1_Y_CFG = WATERCNT1_T_CFG;
		WATERCNT1_T_CFG = 0;
		
		WATERCNT2_Y_CFG = WATERCNT2_T_CFG;
		WATERCNT2_T_CFG = 0;		
	} 
	

}

void webfunc(char *pbuf) {


}