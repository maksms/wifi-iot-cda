os_timer_t gpio_timer;

#define TIMESTAMP_DEFAULT 1614081600

uint32_t ts1 = 0;
uint32_t ts2 = 0;

static uint32_t pt1 = 0;
static uint8_t press_flag = 0;

static uint32_t pt2 = 0;
static uint8_t press_flag2 = 0;


static uint32_t btn2_pressed = 0;
	
#define millis() (unsigned long) (micros() / 1000ULL)
#define GPIO_STATE_CLOSE 0
//#define GPIO_COUNTER1 3
//#define GPIO_COUNTER2 1

#define COUNTER_IMPL 10
//uint32_t counter = 0;

#define AUTO_WASH_START_DELTA 	30			// порог расхода счетчика2 при выключенной промывки для автоопределения начала промывки
// 
// Количество настроек: Счетчик1 GPIO,Расход1 общий,Расход1 сегодня,Расход1 вчера,Счетчик2 GPIO,Расход2 общий,Расход2 сегодня,Расход2 вчера,Последняя промывка,Счетчик1-промывка,Счетчик2-промывка-до,Счетчик2-промывка-после,Объем до промывки

#define WRITE_BIT   0
#define READ_BIT    1


// светодиоды:
// индикация в режиме промывки:
// 1. зеленый горит секунду, гаснет на 500 мсек
// 2. вспышка красного на 100 мсек, пауза 200 мсек и так 2 раза
// 3. и по кругу

// индикация в нормальном режиме:
// 1. не превысили 70%
// 		вспышка зеленого на 100 мсек, пауза 400 мсек
// 2. превысили 70%, но не дошли до 100%
//		вспышка красного на 100 мсек, пауза 400 мсек
// 3. превыссили 100%
// 		вспышка красного на 100 мсек, пауза 100 мсек

#define LED_GREEN 0
#define LED_RED 1

os_timer_t blink_timer;

#define BUTTON1	4
#define BUTTON2	5


#define WATERCNT1_GPIO 		sensors_param.cfgdes[0]		// счетчик 1 gpio
#define WATERCNT1 			sensors_param.cfgdes[1]		// счетчик 1 общий расход
#define WATERCNT1_T 		sensors_param.cfgdes[2]		// счетчик 1 расход сегодня
#define WATERCNT1_Y 		sensors_param.cfgdes[3]		// счетчик 1 расход вчера

#define WATERCNT2_GPIO 		sensors_param.cfgdes[4]		// счетчик 2 gpio
#define WATERCNT2 			sensors_param.cfgdes[5]		// счетчик 2 общий расход
#define WATERCNT2_T 		sensors_param.cfgdes[6]		// счетчик 2 расход сегодня
#define WATERCNT2_Y 		sensors_param.cfgdes[7]		// счетчик 2 расход вчера


#define LAST_WASH_DT 		sensors_param.cfgdes[8]		// дата и время завершения последней промывки
#define LAST_WASH_CNT1 		sensors_param.cfgdes[9]		// показания счетчика 1 на начало промывки
#define LAST_WASH_CNT2_1 	sensors_param.cfgdes[10]		// показания счетчика 2 на начало промывки
//#define LAST_WASH_CNT2_F 	
uint32_t LAST_WASH_CNT2_F;								// показания счетчика после обезжелезывания
#define LAST_WASH_CNT2_2 	sensors_param.cfgdes[11]		// показания счетчика 2 на окончание промывки

#define CLEAN_WATER_VOLUME_DEFAULT 	10000 	// 10 кубов, объем чистой воды до следующей промывки		// можно выставлять через интерпретер
#define CLEAN_WATER_VOLUME 	sensors_param.cfgdes[12]	//10000 	// 10 кубов, объем чистой воды до следующей промывки		// можно выставлять через интерпретер
#define CLEAN_WATER_PERCENT 70		// порог в %%, после которого сигнализируется красным светодиодом о приближении времени промывки

#define TOPIC_WASH_STATE "washstate"
#define TOPIC_WASH_START "washstart"
#define TOPIC_WASH_END "washend"
#define TOPIC_WASH_LITRES "washlitres"

uint32_t clean_water;		// объем чистой воды после последней промывки (литры)
uint16_t percent;

typedef enum {
	STATE_NORMA,
	STATE_WASH,
	STATE_MAX = 0xFFFF
} wash_state_e;

typedef enum {
	WASH_FERRUM_FREE,
	WASH_SOFTENER,
	WASH_MAX = 0xFFFF
} wash_type_e;

wash_state_e wash_state = STATE_NORMA;
wash_type_e wash_type = WASH_FERRUM_FREE;

uint16_t wash_cnt = 0;			// кол-во промывок
uint32_t wash_start_dt = 0;		// дата и время начала промывки

uint32_t wash_time = 0;
/*
	настройка кол-ва кубов воды до следующей промывки
	настройка кол-ва дней до следующей промывки ( что раньше наступит - кубы или дни)
	
	показания счетчика 1 - вода после обезжелезывателя и умягчителя
	счетчик 2 стоит на промывке и считает кол-во пройденой воды при промывке и обезжелезывателя, и умягчителя
	
	как только показания счетчика 2 изменились более чем на 30 литров и прошло времени более чем 1 день, значит началась промывка.
	запоминаем дату и время начала промывки
	запоминаем показания счетчика 1
	
	
	красный светодиод:
		горит постоянно - превышено кол-во дней с последней промывки или объем воды с послежней промывки ( текущее значения > зафиксированное значение перед началом промывки + кол-во кубов)
		редко мигает, когда превышение по кубам более 70%
	
	кнопка 1 (нажать и держать 5 сек) фиксирует дату и время + показания счетчика 1 перед началом промывки любой?
	
	
	
	
*/

#define		RTC_MAGIC		0x55aaaa55

typedef struct {
	uint32_t magic;
	uint32_t watercnt1a;
	uint32_t watercnt1t;
	uint32_t watercnt1y;
	uint32_t watercnt2a;
	uint32_t watercnt2t;
	uint32_t watercnt2y;
	uint16_t  wash_state;
	uint16_t  wash_type;
	uint32_t wash_start_dt;
	uint32_t wash_start_cnt_1;		// счетчик 2 на начало промывки (обезжелезывание)
	uint32_t wash_start_cnt_f;		// счетчик 2 на начало умягчения

	//uint32_t crc32;
} rtc_data_t;

rtc_data_t rtc_data;
	

#define ADDLISTSENS {200,LSENSFL3|LSENS32BIT,"WaterCnt1","watercnt1",&WATERCNT1,NULL}, \
					{201,LSENSFL3|LSENS32BIT,"WaterCnt1Y","watercnt1y",&WATERCNT1_Y,NULL}, \
					{202,LSENSFL3|LSENS32BIT,"WaterCnt1T","watercnt1t",&WATERCNT1_T,NULL}, \
					{203,LSENSFL3|LSENS32BIT,"WaterCnt2","watercnt2",&WATERCNT2,NULL}, \
					{204,LSENSFL3|LSENS32BIT,"WaterCnt2Y","watercnt2y",&WATERCNT2_Y,NULL}, \
					{205,LSENSFL3|LSENS32BIT,"WaterCnt2T","watercnt2t",&WATERCNT2_T,NULL}, \
					{206,LSENSFL0,"WashState","washstate",&wash_state,NULL}, \
					{207,LSENSFL0|LSENS32BIT,"WashTime","washtime",&wash_time,NULL}, \
					{208,LSENSFL0,"WashCnt","washcnt",&wash_cnt,NULL}, \
					{209,LSENSFL0|LSENS32BIT,"WashStart","washstart",&wash_start_dt,NULL}, \
					{210,LSENSFL0|LSENS32BIT,"WashEnd","washend",&LAST_WASH_DT,NULL}, \
					{211,LSENSFL0,"WashResrc","washrsrc",&percent,NULL}, 

uint8_t pcf_data;
	
#define B(bit_no)         (1 << (bit_no))
#define BIT_CLEAR(reg, bit_no)   (reg) &= ~B(bit_no)
#define BIT_SET(reg, bit_no)   (reg) |= B(bit_no)
#define BIT_CHECK(reg, bit_no)   ( ((reg) & B(bit_no)) > 0 )
#define BIT_TRIGGER(reg, bit_no)   (reg) ^= B(bit_no)

#define PCF_LED_ON(num) (BIT_CLEAR(pcf_data, num))
#define PCF_LED_OFF(num) (BIT_SET(pcf_data, num))

#define GET_TS()(sntp_get_current_timestamp() - sntp_get_timezone()*3600 + sensors_param.utc * 3600)
#define PASSED_DAY_AFTER_WASH() ( (GET_TS()  - LAST_WASH_DT ) / (3600*24) )

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


void ICACHE_FLASH_ATTR blink_one_led(uint8_t led, uint16_t before, uint16_t on_delay, uint16_t off_delay)
{
	// частота запуска 100 мсек
	static uint32_t blink = 0;
	uint16_t cnt = (before + on_delay + off_delay) / 100;
	
	if ( (blink % cnt >= (before / 100)) && (blink % cnt < ( (before + on_delay) / 100)))
		PCF_LED_ON(led);
	else
		PCF_LED_OFF(led);

	blink++;
	if ( blink > (cnt-1) ) blink = 0;
}

void ICACHE_FLASH_ATTR blink_one_led_twice(uint8_t led, uint16_t before, uint16_t on1_delay, uint16_t off1_delay, uint16_t on2_delay, uint16_t off2_delay )
{
	// частота запуска 100 мсек
	static uint32_t blink = 0;
	uint16_t cnt = (before + on1_delay + off1_delay + on2_delay + off2_delay) / 100;
	
	if ( ( blink % cnt >= (before / 100) && blink % cnt < ( ( before + on1_delay ) / 100) )// первая вспышка
		 || 								
	     ( blink % cnt >= ( (before + on1_delay + off1_delay ) / 100) &&  blink % cnt < ( (before + on1_delay + off1_delay + on2_delay) / 100) )     // вторая вспышка
		)
		PCF_LED_ON(led);
	else
		PCF_LED_OFF(led);

	blink++;
	if ( blink > (cnt-1) ) blink = 0;
}

void ICACHE_FLASH_ATTR mqtt_send_wash_start()
{
	#if mqtte
	if ( !sensors_param.mqtten ) return;		
	char payload[10];
	os_sprintf(payload,"%d", wash_state);
	MQTT_Publish(&mqttClient, TOPIC_WASH_STATE, payload, os_strlen(payload), 0, 0, 0);
	
	os_sprintf(payload,"%d", wash_start_dt);
	MQTT_Publish(&mqttClient, TOPIC_WASH_START, payload, os_strlen(payload), 0, 0, 0);
		
	#endif	
}

void ICACHE_FLASH_ATTR mqtt_send_wash_end()
{
	#if mqtte
	if ( !sensors_param.mqtten ) return;		
	char payload[10];
	
	os_sprintf(payload,"%d", (LAST_WASH_CNT2_2 > LAST_WASH_CNT2_1 && LAST_WASH_CNT2_F >= LAST_WASH_CNT2_1) ? (LAST_WASH_CNT2_2 - LAST_WASH_CNT2_1) : 0);
	MQTT_Publish(&mqttClient, TOPIC_WASH_LITRES, payload, os_strlen(payload), 0, 0, 0);
			
	os_sprintf(payload,"%d", LAST_WASH_DT);
	MQTT_Publish(&mqttClient, TOPIC_WASH_END, payload, os_strlen(payload), 0, 0, 0);
	
	os_sprintf(payload,"%d", wash_state);
	MQTT_Publish(&mqttClient, TOPIC_WASH_STATE, payload, os_strlen(payload), 0, 0, 0);	
	
	#endif		
}


void ICACHE_FLASH_ATTR do_wash_start(uint16_t counter_offset)
{
	if ( wash_start_dt > TIMESTAMP_DEFAULT ) return;
	if ( wash_state == STATE_WASH ) return;
	
	wash_state = STATE_WASH;
	wash_type = WASH_FERRUM_FREE;
	wash_start_dt = GET_TS();
	LAST_WASH_CNT2_F = LAST_WASH_CNT2_1;
	LAST_WASH_CNT1 = WATERCNT1;		// фиксируем показания счетчика 1 на начало промывки
	LAST_WASH_CNT2_1 = WATERCNT2 -  counter_offset;		// фиксируем показания счетчика 2 на начало промывки
	wash_time = 0;
	wash_cnt++;
	SAVEOPT;	
	
	mqtt_send_wash_start();

}

void ICACHE_FLASH_ATTR do_wash_end()
{
	if ( wash_state == STATE_NORMA ) return;
	
	wash_start_dt = 0;
	wash_state = STATE_NORMA;
	wash_type = WASH_FERRUM_FREE;
	LAST_WASH_DT = GET_TS();	// фиксируем дату и время завершения промывки
	LAST_WASH_CNT2_2 = WATERCNT2;		// фиксируем показания счетчика 2 на окончание промывки
	SAVEOPT;	
	
	mqtt_send_wash_end();
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
			WATERCNT1 += COUNTER_IMPL;
			WATERCNT1_T += COUNTER_IMPL;
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
			WATERCNT2 += COUNTER_IMPL;
			WATERCNT2_T += COUNTER_IMPL;
		}
		
		ts2 = millis();
		
	} else {
		ts2 = 0;
	}	
	
	
	
	pcf_data = pcfgpior8(0x20);
	
	// грязный хак: перед тем как прочитать состояние гпио как входа, надо этот пин пометить как вход (1) записью в pcf
	//if ( BIT_CHECK(data, 4) > 0)
	if ( GPIO_ALL_GET(224) == 0)
	{
		// нажали
		if ( pt1 == 0 ) 
			pt1 = millis(); // фиксируем время нажатия
	} else {
		// отпустили
		pt1 = 0;
		press_flag = 0;
	}
	
	// грязный хак: перед тем как прочитать состояние гпио как входа, надо этот пин пометить как вход (1) записью в pcf
	BIT_SET( pcf_data, 4); BIT_SET( pcf_data, 5);
	pcfgpiow8(0x20, pcf_data);
	
	if ( GPIO_ALL_GET(225) == 0)
	{
		// нажали
		if ( pt2 == 0 ) 
			pt2 = millis(); // фиксируем время нажатия
	} else {
		// отпустили
		pt2 = 0;
		press_flag2 = 0;
	}
	
	if ( press_flag == 0 && millis() - pt1 >= 5000 && pt1 > 0) {
		// кнопка нажата более 5 сек
		if ( wash_state == STATE_NORMA ) 
		{
			// включаем режим промывки
			do_wash_start(0);
		}
		else
		{
			// отключаем режим промывки
			do_wash_end();
		}	
		press_flag = 1;		
	} 


	if ( press_flag2 == 0 && millis() - pt2 >= 5000 && pt2 > 0) {
		// кнопка нажата более 5 сек
		if ( wash_state == STATE_NORMA ) 
		{
			// обычный режим, например, для сохранения во флеш при нажатии или для сброса показаний
			btn2_pressed = 1;
			SAVEOPT;
		} else {
			btn2_pressed = 1;
			// режим промывки
			if ( wash_type == WASH_FERRUM_FREE ) 
			{
				// включаем режим промывки умягчителя
				wash_type = WASH_SOFTENER;
				// зафиксировать показания счетчика 2 промежуточные (узнаем, сколько воды было потрачено на промывку обезжелезывателя
				LAST_WASH_CNT2_F = WATERCNT2;
				
			} else {
				// включаем режим промывки обезжелезывателя
				wash_type = WASH_FERRUM_FREE;
			}
		}
		press_flag2 = 1;	
	} 
	
	// индикации и другие действия по статусу промывки
	if ( wash_state == STATE_WASH )
	{
		PCF_LED_OFF(LED_RED);
		
		if ( btn2_pressed > 0 ) 
		{
			blink_one_led_twice(LED_RED, 0, 100, 200, 100, 200 );
			btn2_pressed += 100;
			if ( btn2_pressed > 700 ) btn2_pressed = 0;
			goto pcf;
		}
		
		// режим промывки включен, мигаем светодиодом,  сюда попадаем каждые 100 мсек
		blink_one_led(LED_GREEN, 0, 100, 100);
		
	} else {
		// режим промывки выключен
		PCF_LED_OFF(LED_RED);	//BIT_SET( pcf_data, LED_RED);  // погасить красный светодиод
		
		if ( btn2_pressed > 0 ) 
		{
			blink_one_led_twice(LED_RED, 0, 100, 200, 100, 200 );
			btn2_pressed += 100;
			if ( btn2_pressed > 700 ) btn2_pressed = 0;
			goto pcf;
		}
		// в зависимости от потребленного расхода чистой воды после промывки показываем индикацию
		// если расход после промывки Б 70%, то включаем зеленый или редко мигаем BIT_CLEAR(data, LED_GREEN);
		clean_water = WATERCNT1 - LAST_WASH_CNT1;
		if ( clean_water >= CLEAN_WATER_VOLUME )
		{
			// превышение на 100%
			PCF_LED_OFF(LED_GREEN);
			blink_one_led(LED_RED, 0, 100, 100);			
		}
		else
		{
			if ( (clean_water*100)/CLEAN_WATER_VOLUME > CLEAN_WATER_PERCENT )
			{
				// превысили 70%
				PCF_LED_OFF(LED_GREEN);
				PCF_LED_OFF(LED_RED);			
				
				blink_one_led(LED_GREEN, 0, 100, 2900);
				blink_one_led_twice(LED_RED, 900, 100, 200, 100, 1700 );
								
			} else {
				// расход чистой воды менее 70%
				// индикация: зеленая вспышка на 100 мсек, далее 1900 мсек погашено
				PCF_LED_OFF(LED_RED);			
				blink_one_led(LED_GREEN, 0, 100, 1900);
			}
		}
	}
	
	/* если кнопка 4 нажата, то включить led 0
	if ( BIT_CHECK(data, 4) > 0)
	{
		BIT_SET( data, 0);
	} else {
		BIT_CLEAR(data, 0);
	}

	if ( BIT_CHECK(data, 5) > 0)
	{
		BIT_SET( data, 1);
	} else {
		BIT_CLEAR(data, 1);
	}
	*/
pcf:
	BIT_SET( pcf_data, 4); BIT_SET( pcf_data, 5);
	pcfgpiow8(0x20, pcf_data);
	

}


void ICACHE_FLASH_ATTR startfunc()
{

	
	system_rtc_mem_read(70, &rtc_data, sizeof(rtc_data_t));
	//uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	
	//if ( crc32 != rtc_data.crc32 ) 
	if ( rtc_data.magic != RTC_MAGIC ) 
	{
		// кривые данные, обнулим
		os_memset(&rtc_data, 0, sizeof(rtc_data_t));
		//crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
		//rtc_data.crc32 = crc32;
		rtc_data.magic = RTC_MAGIC;
		
		// пишем в rtc обнуленные данные
		system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));		
	}
	
	// проверим сохраненные данные во flash с данными из rtc, если данные во флешь меньше, чем данные в rtc, значит был сброс по питанию, а данные не успели сохраниться во flash
	uint8_t need_save = 0;
	
	if ( CLEAN_WATER_VOLUME < 1000 ) 
	{
		CLEAN_WATER_VOLUME = CLEAN_WATER_VOLUME_DEFAULT;
		need_save = 1;
	}	
	
	if ( LAST_WASH_DT < TIMESTAMP_DEFAULT ) 
	{
		LAST_WASH_DT = TIMESTAMP_DEFAULT;
		need_save = 1;
	}
	
	if ( WATERCNT1 < rtc_data.watercnt1a )  
	{
		WATERCNT1 = rtc_data.watercnt1a;
		need_save = 1;
	}
	
	if ( WATERCNT1_Y < rtc_data.watercnt1y )  
	{
		WATERCNT1_Y = rtc_data.watercnt1y;
		need_save = 1;
	}
	
	if ( WATERCNT1_T < rtc_data.watercnt1t )  
	{
		WATERCNT1_T = rtc_data.watercnt1t;
		need_save = 1;
	}

	if ( WATERCNT2 < rtc_data.watercnt2a )  
	{
		WATERCNT2 = rtc_data.watercnt2a;
		need_save = 1;
	}
	
	if ( WATERCNT2_Y < rtc_data.watercnt2y )  
	{
		WATERCNT2_Y = rtc_data.watercnt2y;
		need_save = 1;
	}
	
	if ( WATERCNT2_T < rtc_data.watercnt2t )  
	{
		WATERCNT2_T = rtc_data.watercnt2t;
		need_save = 1;
	}
	
	wash_state = rtc_data.wash_state;
	wash_type = rtc_data.wash_type;
	
	if ( LAST_WASH_CNT2_1 < rtc_data.wash_start_cnt_1 )
	{
		LAST_WASH_CNT2_1 = rtc_data.wash_start_cnt_1;
		need_save = 1;
	}
	
	LAST_WASH_CNT2_F = rtc_data.wash_start_cnt_f;
	
	if ( wash_start_dt  < rtc_data.wash_start_dt )
	{
		wash_start_dt = rtc_data.wash_start_dt;
		need_save = 1;
	}
	
	if ( need_save ) SAVEOPT; 

	pcf_data = 0b00110011;
	pcfgpiow8(0x20, pcf_data);
	
	os_timer_disarm(&gpio_timer);
	os_timer_setfn(&gpio_timer, (os_timer_func_t *)read_gpio_cb, NULL);
	os_timer_arm(&gpio_timer, 100, 1);

}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {

    //i2c_pcf8574_read(0x20, &data);

	if ( CLEAN_WATER_VOLUME < 1000 ) CLEAN_WATER_VOLUME = CLEAN_WATER_VOLUME_DEFAULT;
	
	rtc_data.magic = RTC_MAGIC;
	rtc_data.watercnt1a = WATERCNT1;
	rtc_data.watercnt1y = WATERCNT1_Y;
	rtc_data.watercnt1t = WATERCNT1_T;
	rtc_data.watercnt2a = WATERCNT2;
	rtc_data.watercnt2y = WATERCNT2_Y;
	rtc_data.watercnt2t = WATERCNT2_T;
	
	rtc_data.wash_state = wash_state;
	rtc_data.wash_type = wash_type;
	rtc_data.wash_start_cnt_1 = LAST_WASH_CNT2_1;
	rtc_data.wash_start_cnt_f = LAST_WASH_CNT2_F;
	rtc_data.wash_start_dt = wash_start_dt;
	
	//uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	//rtc_data.crc32 = crc32;
	system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));
	
	if (timersrc%1800==0)  //30*60 сек  каждые 30 мин сохраняем данные во флеш
	{
		SAVEOPT;
	}


	if ( time_loc.hour == 0 && time_loc.min == 0 && time_loc.sec == 0 )
	{
		// обнулить суточные данные ночью
		WATERCNT1_Y = WATERCNT1_T;
		WATERCNT1_T = 0;
		
		WATERCNT2_Y = WATERCNT2_T;
		WATERCNT2_T = 0;		
	} 
	

	if ( wash_state == STATE_WASH )
	{
		wash_time++;
	} else {
		 percent = (clean_water*100)/CLEAN_WATER_VOLUME;
	}
	
	
	// автоматическая фиксация начала промывки ( если по счетчику 2 начал увеличиваться расход, но факта начала промывки не зафиксировано)
	if ( wash_state == STATE_NORMA 	// режима Норма, промывка не включена
		 && WATERCNT2 - LAST_WASH_CNT2_1 > AUTO_WASH_START_DELTA // показания счетчика увеличились на 30 литров
		 && wash_start_dt < TIMESTAMP_DEFAULT
		)
	{
		// автоматическое определение начала промывки
		do_wash_start(AUTO_WASH_START_DELTA);	
	}
	
	if ( wash_start_dt > TIMESTAMP_DEFAULT && 
	    ((GET_TS() - wash_start_dt) / 3600 ) >= 5 
		) 
	{
		// промывка длится более 5 часов, значит забыли вручную зафиксировать завершение промывки, зафиксируем автоматически
		// отключаем режим промывки
		do_wash_end();		
	}		
}

void webfunc(char *pbuf) {

	os_sprintf(HTTPBUFF,"<br> <b>Режим:</b> %s", (wash_state == STATE_WASH ) ? "Промывка" : "Норма");
	
	uint32_t water[3];
	if ( wash_state == STATE_WASH )
	{
		if ( wash_type == WASH_FERRUM_FREE ) 
		{
			os_sprintf(HTTPBUFF," (обезжелезывание)");
			water[1] = WATERCNT2 - LAST_WASH_CNT2_1;
			water[2] = 0;
		}
		else
		{
			os_sprintf(HTTPBUFF," (умягчение)");
			water[1] = LAST_WASH_CNT2_F - LAST_WASH_CNT2_1;
			water[2] = WATERCNT2 - LAST_WASH_CNT2_F;
		}	
	
		water[0] = WATERCNT2 - LAST_WASH_CNT2_1;
		
		os_sprintf(HTTPBUFF,"<br>Текущая промывка");
	} 
	else 
	{
		os_sprintf(HTTPBUFF,"<br>Предыдущая промывка");
	
		water[0] = 0;		
		water[1] = 0;
		water[2] = 0;

		if ( LAST_WASH_CNT2_F > 0 ) 
			water[0] = LAST_WASH_CNT2_2 - LAST_WASH_CNT2_1;		

		if ( LAST_WASH_CNT2_F >= LAST_WASH_CNT2_1) 
		{
			water[1] = LAST_WASH_CNT2_F - LAST_WASH_CNT2_1;	
		}
		
		if ( LAST_WASH_CNT2_F > 0 && LAST_WASH_CNT2_2 > LAST_WASH_CNT2_1 && LAST_WASH_CNT2_F > LAST_WASH_CNT2_1)
			water[2] = LAST_WASH_CNT2_2 - LAST_WASH_CNT2_F;	
	
	}

	os_sprintf(HTTPBUFF,"<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>"
						"<tr  align='center'>"
						"<th>Общий</th>"
						"<th>Железо</th>"
						"<th>Умягчение</th>"
						"</tr>"
						"<tr  align='center'>"
						"<td>%d</td><td>%d</td><td>%d</td>"
						"</tr></table>"
						, water[0], water[1], water[2]
				);


	

	int hh = wash_time / 3600;
	int mm = (wash_time % 3600) / 60;
	int ss = (wash_time % 3600) % 60;
	os_sprintf(HTTPBUFF,"<br> <b>Время промывки:</b> %02d:%02d:%02d", hh, mm, ss);

	os_sprintf(HTTPBUFF,"<br> <b>Промывок:</b> %d", wash_cnt);

	os_sprintf(HTTPBUFF,"<br> <b>Чистая вода:</b> %d", clean_water);

	char color[10];

	if ( percent < CLEAN_WATER_PERCENT )
		strcpy(color, "green");
	else if ( percent < 100 )
		strcpy(color, "orange");
	else
		strcpy(color, "red");

	os_sprintf(HTTPBUFF,"<br> <b>Ресурс:</b> <b><span style='color:%s'>%d %%</span></b>", color, percent );
		
	os_sprintf(HTTPBUFF,"<br><br> Промывка <b>%d дн. назад</b> (%s)", ( GET_TS() > TIMESTAMP_DEFAULT ) ? PASSED_DAY_AFTER_WASH() : 0, sntp_get_real_time(LAST_WASH_DT)); 
	
//os_sprintf(HTTPBUFF,"<br> Счетчик1 на начало: %d", LAST_WASH_CNT1);
//os_sprintf(HTTPBUFF,"<br> Счетчик2 на начало: %d", LAST_WASH_CNT2_1);
//os_sprintf(HTTPBUFF,"<br> Счетчик2 на окончание: %d", LAST_WASH_CNT2_2);
//os_sprintf(HTTPBUFF,"<br> LAST_WASH_CNT2_F: %d", LAST_WASH_CNT2_F);

//os_sprintf(HTTPBUFF,"<br> pt1 = %d", pt1);

//os_sprintf(HTTPBUFF,"<br> pt1 = %d", pt1);
//os_sprintf(HTTPBUFF,"<br> press_flag = %d", press_flag);
//os_sprintf(HTTPBUFF,"<br> pt2 = %d", pt2);
//os_sprintf(HTTPBUFF,"<br> press_flag2 = %d", press_flag2);
//os_sprintf(HTTPBUFF,"<br> btn2_pressed = %d", btn2_pressed);


//data2 = pcfgpior8(0x20);
//os_sprintf(HTTPBUFF,"<br> data0 = %d", (data & ( 1 << 0)) == 0);
//os_sprintf(HTTPBUFF,"<br> data0 = %d", BIT_CHECK(pcf_data, 0) );
//os_sprintf(HTTPBUFF,"<br> data1 = %d", BIT_CHECK(pcf_data, 1) );
//os_sprintf(HTTPBUFF,"<br> Кнопка1 = %d", BIT_CHECK(pcf_data, 4) );
//os_sprintf(HTTPBUFF,"<br> Кнопка2 = %d", BIT_CHECK(pcf_data, 5) );
//os_sprintf(HTTPBUFF,"<br> data1 = %d", (data & ( 1 << 1)) == 0);
//os_sprintf(HTTPBUFF,"<br> data4 = %d", (data & ( 1 << 4)) == 0);
//os_sprintf(HTTPBUFF,"<br> data5 = %d", (data & ( 1 << 5)) == 0); 

os_sprintf(HTTPBUFF,"<br><br> <small>FW ver. %s</small>", "1.43");
}