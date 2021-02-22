#define FUEL_PUMP_GPIO	2
#define FUEL_PUMP_LED_GPIO 15

#define millis() (uint32_t) (micros() / 1000ULL) 

#define FUEL_PUMP "fuelpump_state"

// Отсечка счетчика, Время работы общее, Время работы сегодня, Расход общий, Расход сегодня,Время работы вчера,Расход вчера

#define COUNTER_THRESHOLD sensors_param.cfgdes[0]
#define SAVE_FUEL_WORKTIME_TOTAL sensors_param.cfgdes[1]
#define SAVE_FUEL_WORKTIME_DAY sensors_param.cfgdes[2]
#define SAVE_FUEL_CONSUMPTION_TOTAL sensors_param.cfgdes[3]
#define SAVE_FUEL_CONSUMPTION_DAY sensors_param.cfgdes[4]
#define SAVE_FUEL_WORKTIME_PREV sensors_param.cfgdes[5]
#define SAVE_FUEL_CONSUMPTION_PREV sensors_param.cfgdes[6]

//#define ADDLISTSENS {201,LSENSFL0,"имя","val1",&cnttest,NULL}, uint16_t cnttest;
// можно так уже к существущей переменной
// #define ADDLISTSENS {201,LSENSFL0,"имя","val1",&valdes[0],NULL},
// 201- номер, 2 параметр - режим. В данном случае 0 знаков после запятой
/*
1,98	л/ч
1980	мл/ч
33		мл/мин
0,55	мл/сек
*/

#define CONSUMP_ML_SEC 0.55f
#define CONSUMP_L_SEC 0.00055f

uint16_t fuel_pump_state = 0;
uint32_t fuel_pump_working_time = 0;
uint32_t fuel_pump_today_time = 0;
uint32_t fuel_pump_prev_time = 0;

uint32_t get_consump();
uint32_t i_fuel_consump;
uint32_t i_fuel_consump_now;

uint32_t get_consump_today();
uint32_t i_fuel_consump_today;

uint32_t get_consump_prev();
uint32_t i_fuel_consump_prev;

uint16_t on_cnt = 0;
uint32_t on_duration = 0;
uint32_t prev_duration = 0;

#define ADDLISTSENS {200,LSENSFL0,"FuelPump",FUEL_PUMP,&fuel_pump_state,NULL}, \
					{201,LSENSFL3|LSENS32BIT|LSENSFUNS,"FuelConsumpTotal","fuelcons_total",get_consump,NULL}, \
					{202,LSENSFL3|LSENS32BIT|LSENSFUNS,"FuelConsumpToday","fuelcons_day",get_consump_today,NULL}, \
					{203,LSENSFL3|LSENS32BIT|LSENSFUNS,"FuelConsumpPrev","fuelcons_prev",get_consump_prev,NULL}, \
					{204,LSENSFL0|LSENS32BIT,"WorkTimeTotal","fuelpump_total",&fuel_pump_working_time,NULL}, \
					{205,LSENSFL0|LSENS32BIT,"WorkTimeDay","fuelpump_day",&fuel_pump_today_time,NULL}, \
					{206,LSENSFL0|LSENS32BIT,"WorkTimePrev","fuelpump_prev",&fuel_pump_prev_time,NULL}, \
					{207,LSENSFL0,"OnCount","oncnt",&on_cnt,NULL}, \
					{208,LSENSFL0|LSENS32BIT,"OnDuration","ondur",&prev_duration,NULL}, 


	typedef struct {
		uint32_t fuel_consump_total;
		uint32_t fuel_consump_day;
		uint32_t fuel_consump_prev;
		uint32_t fuel_worktime_total;
		uint32_t fuel_worktime_day;
		uint32_t fuel_worktime_prev;
		uint16_t on_cnt;
		uint32_t prev_duration;
		uint32_t crc32;
	} rtc_data_t;

	rtc_data_t rtc_data;
	
	uint8_t opt_saving = 0;
	
uint32_t ICACHE_FLASH_ATTR get_consump()
{
	return 	i_fuel_consump / 100;
}

uint32_t ICACHE_FLASH_ATTR get_consump_today()
{
	return 	i_fuel_consump_today / 100;
}

uint32_t ICACHE_FLASH_ATTR get_consump_prev()
{
	return 	i_fuel_consump_prev / 100;
}

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

void ICACHE_FLASH_ATTR startfunc()
{
	
	system_rtc_mem_read(70, &rtc_data, sizeof(rtc_data_t));
	uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	if ( crc32 != rtc_data.crc32 ) {
		// кривые данные, обнулим
		os_memset(&rtc_data, 0, sizeof(rtc_data_t));
		crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
		rtc_data.crc32 = crc32;
		// пишем в rtc обнуленные данные
		system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));		
	}
	
	i_fuel_consump = rtc_data.fuel_consump_total;
	i_fuel_consump_today = rtc_data.fuel_consump_day;
	i_fuel_consump_prev = rtc_data.fuel_consump_prev;
	
	fuel_pump_working_time = rtc_data.fuel_worktime_total;
	fuel_pump_today_time = rtc_data.fuel_worktime_day;
	fuel_pump_prev_time = rtc_data.fuel_worktime_prev;
	
	on_cnt = rtc_data.on_cnt;
	prev_duration = rtc_data.prev_duration;
	
	// если значения данных из rtс меньше чем сохраненные данные
	if ( fuel_pump_working_time < SAVE_FUEL_WORKTIME_TOTAL ) fuel_pump_working_time = SAVE_FUEL_WORKTIME_TOTAL;
	if ( fuel_pump_today_time < SAVE_FUEL_WORKTIME_DAY )	 fuel_pump_today_time = SAVE_FUEL_WORKTIME_DAY;
	if ( fuel_pump_prev_time < SAVE_FUEL_WORKTIME_PREV )	 fuel_pump_prev_time = SAVE_FUEL_WORKTIME_PREV;
	
	if ( i_fuel_consump < SAVE_FUEL_CONSUMPTION_TOTAL )		 i_fuel_consump = SAVE_FUEL_CONSUMPTION_TOTAL;
	if ( i_fuel_consump_today < SAVE_FUEL_CONSUMPTION_DAY )  i_fuel_consump_today = SAVE_FUEL_CONSUMPTION_DAY;
	if ( i_fuel_consump_prev < SAVE_FUEL_CONSUMPTION_PREV )  i_fuel_consump_prev = SAVE_FUEL_CONSUMPTION_PREV;

}


static void ICACHE_FLASH_ATTR mqtt_send_int(const char *topic, int val)
{
	#if mqtte
	if ( !sensors_param.mqtten ) return;		
	char payload[10];
	os_sprintf(payload,"%d", val);
	MQTT_Publish(&mqttClient, topic, payload, os_strlen(payload), 0, 0, 0);
	#endif
}


void ICACHE_FLASH_ATTR save_data()
{
	SAVE_FUEL_WORKTIME_TOTAL = fuel_pump_working_time;
	SAVE_FUEL_WORKTIME_DAY = fuel_pump_today_time;
	SAVE_FUEL_WORKTIME_PREV = fuel_pump_prev_time;
	
	SAVE_FUEL_CONSUMPTION_TOTAL = i_fuel_consump;
	SAVE_FUEL_CONSUMPTION_DAY = i_fuel_consump_today;
	SAVE_FUEL_CONSUMPTION_PREV = i_fuel_consump_prev;

}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
	if ( safemode ) return;

/* 	if ( timersrc % 5 == 0 )
	{
		if ( SAVE_FUEL_WORKTIME_PREV != fuel_pump_prev_time ) fuel_pump_prev_time = SAVE_FUEL_WORKTIME_PREV;
		if ( SAVE_FUEL_CONSUMPTION_DAY != i_fuel_consump_today ) i_fuel_consump_today = SAVE_FUEL_CONSUMPTION_DAY;
		if ( SAVE_FUEL_CONSUMPTION_PREV != i_fuel_consump_prev ) i_fuel_consump_prev = SAVE_FUEL_CONSUMPTION_PREV;
		
	} */
	
	// корректировка нулевых значений счетчиков
	if ( rtc_data.fuel_consump_total == 0) rtc_data.fuel_consump_total = i_fuel_consump;
	if ( rtc_data.fuel_consump_day == 0) rtc_data.fuel_consump_day = i_fuel_consump_today;
	if ( rtc_data.fuel_consump_prev == 0) rtc_data.fuel_consump_prev = i_fuel_consump_prev;
	
	if ( rtc_data.fuel_worktime_total == 0) rtc_data.fuel_worktime_total = fuel_pump_working_time;
	if ( rtc_data.fuel_worktime_day == 0) rtc_data.fuel_worktime_day = fuel_pump_today_time;
	if ( rtc_data.fuel_worktime_prev == 0) rtc_data.fuel_worktime_prev = fuel_pump_prev_time;
	
	if ( rtc_data.on_cnt == 0) rtc_data.fuel_worktime_day = on_cnt;
	if ( rtc_data.prev_duration == 0) rtc_data.prev_duration = prev_duration;
	

	
	if ( time_loc.hour == 0 && time_loc.min == 0 && time_loc.sec == 0 )
	{
		// обнулить суточные данные ночью
		fuel_pump_prev_time = fuel_pump_today_time;
		fuel_pump_today_time = 0;
		//fuel_consump_today = 0;
		i_fuel_consump_prev = i_fuel_consump_today;
		i_fuel_consump_today = 0;
		on_cnt = 0;
	} 
	
	static uint8_t prev_state = 0;
	static uint32_t t_start = 0;
	
	//fuel_pump_state = ( cnt1i > 0 );
	//fuel_pump_state = ( count60end[0] > 0 );
	fuel_pump_state = ( count60end[0] > COUNTER_THRESHOLD );  // если просто > 0? то проскакивают левые импульсы
	
	
	if ( prev_state != fuel_pump_state )
	{
		GPIO_ALL( FUEL_PUMP_LED_GPIO, fuel_pump_state );
		mqtt_send_int( FUEL_PUMP, fuel_pump_state );
		prev_state = fuel_pump_state;
		
		if ( fuel_pump_state ) 
		{
			// переключился из 0 в 1  (!!! может проскакивать импульс и поэтому cnt увеличивается на 1 и предыдущее время обнуляется, регулируется отсечкой подсчета импульсов)
			on_cnt++;
			t_start = millis(); // при включении начали отсчет
			i_fuel_consump_now = 0; // обнуление текущего расхода
			
		} else {
			// переключился из 1 в 0
			prev_duration = on_duration;
			i_fuel_consump_now = 0;
			on_duration = 0;
		}
	}
	
	if ( fuel_pump_state ) {
		fuel_pump_working_time++;
		
		//fuel_consump += CONSUMP_L_SEC;
		i_fuel_consump += CONSUMP_L_SEC*100000;
		
		fuel_pump_today_time++;
		
		//fuel_consump_today += CONSUMP_L_SEC;
		i_fuel_consump_today += CONSUMP_L_SEC*100000;
		
		i_fuel_consump_now += CONSUMP_L_SEC*100000;
		on_duration = millis() - t_start;	// считаем время
	}
	
	
	if ( GPIO_ALL_GET( 6 ) == 1 )
	{
		i_fuel_consump = 0;
		i_fuel_consump_today = 0;
		i_fuel_consump_prev = 0;
		
		fuel_pump_working_time = 0;
		fuel_pump_today_time = 0;
		fuel_pump_prev_time = 0;
		on_cnt = 0;
		
		GPIO_ALL(6, 0);
		
		//save_data();
	}
	
	// пишем данные в rtc
	rtc_data.fuel_consump_total = i_fuel_consump;
	rtc_data.fuel_consump_day = i_fuel_consump_today;
	rtc_data.fuel_consump_prev = i_fuel_consump_prev;
	
	rtc_data.fuel_worktime_total = fuel_pump_working_time;
	rtc_data.fuel_worktime_day = fuel_pump_today_time;
	rtc_data.fuel_worktime_prev = fuel_pump_prev_time;
	
	rtc_data.on_cnt = on_cnt;
	rtc_data.prev_duration = prev_duration;
	
	uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	rtc_data.crc32 = crc32;
	system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));
	
	save_data(0);
	
	if ( timersrc % 3600 ) {
		save_data();
	}
}


void webfunc(char *pbuf) 
{
	os_sprintf(HTTPBUFF, "<b>Fuel Pump:</b> %s &nbsp; <b>count:</b> %d <br>", fuel_pump_state ? "ON" : "OFF", on_cnt );
	os_sprintf(HTTPBUFF, "<b>PrevDuration:</b> %d:%02d <br>", (prev_duration / 1000) / 60,  (prev_duration / 1000) % 60);
	
	uint32_t sec = fuel_pump_working_time % 60;
	uint32_t min = fuel_pump_working_time / 60;
	uint32_t hour = (min / 60 % 24);
	min = min % 60;

	
	
		
	//os_sprintf(HTTPBUFF, "<b>Working time (total):</b> %d days %02d:%02d:%02d<br>", day, hour, min, sec );
	//os_sprintf(HTTPBUFF, "<b>Total consumption:</b> %d.%03d litres<br>", (int)(fuel_consump * 1000)/ 1000, (int)(fuel_consump * 1000 )% 1000);
	
	
	//os_sprintf(HTTPBUFF, "<b>Working time (today):</b> %02d:%02d:%02d<br>", today_hour, today_min, today_sec );
	//os_sprintf(HTTPBUFF, "<b>Today consumption:</b> %d.%03d litres<br>", (int)(fuel_consump_today * 1000)/ 1000, (int)(fuel_consump_today * 1000 )% 1000);


	os_sprintf(HTTPBUFF, "<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>"
							"<tr align='center'>"
								"<th></th><th>Work time:</th><th>Consumption, L:</th>"
							"</tr>"
				);
				

	os_sprintf(HTTPBUFF, 	"<tr align='center'>"
								"<td><b>Now:</b></td><td>%02d:%02d</td><td>%d.%03d</td>"
							"</tr>"
							, (on_duration / 1000) / 60,  (on_duration / 1000) % 60
							, i_fuel_consump_now / 100000, (i_fuel_consump_now % 100000) / 100
	);				

	uint32_t _sec = fuel_pump_today_time % 60;
	uint32_t _min = fuel_pump_today_time / 60;
	uint32_t _hour = _min / 60;
	_min = _min % 60;

	os_sprintf(HTTPBUFF, 	"<tr align='center'>"
								"<td><b>Today:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td>"
							"</tr>"
							, _hour, _min, _sec
							, i_fuel_consump_today / 100000, (i_fuel_consump_today % 100000) / 100
	);


	_sec = fuel_pump_prev_time % 60;
	_min = fuel_pump_prev_time / 60;
	_hour = _min / 60;
	_min = _min % 60;

	os_sprintf(HTTPBUFF, 	"<tr align='center'>"
								"<td><b>Yesterday:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td>"
							"</tr>"
							, _hour, _min, _sec
							, i_fuel_consump_prev / 100000, (i_fuel_consump_prev % 100000) / 100
	);
	
	os_sprintf(HTTPBUFF, 	"<tr align='center'>"
								"<td><b>Total:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td>"
							"</tr>"
							, hour, min, sec
							, i_fuel_consump / 100000, (i_fuel_consump % 100000) / 100
	);	
	
	os_sprintf(HTTPBUFF, 	"</table>");	
	
}