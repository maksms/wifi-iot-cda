//#include "driver/uart.h"
	#include "../moduls/uart_register.h"
	#include "../moduls/uart.h"
	#include "../moduls/uart.c" // ??????

	#define MQTTD

	#define FW_VER "3.9"
	
	/*
	* SDM Task Delay
	* MQTT Send Interval
	* Значения превышения по току
	* время срабатывания (мсек)
	* время отпускания сек

	sensors_param.cfgdes -  12
		cfgdes[0] - задержка чтения данных с SDM
		cfgdes[1] - время отправки данных с SDM по mqtt
		cfgdes[2] - превышение по току	
		cfgdes[3] - время определения перегрузки, мсек, control_current_delay
		cfgdes[4] - время определения отсутствия перегрузки, сек, control_load_on_delay

	*/

	#define ELECTRO_C20_V1_P1__E10						// читаем 20 раз подряд значение тока, затем 1 раз напряжение, затем 1 раз мощность, и каждое 10е чтение получаем значение потраченной энергии

	#define OVERLOAD_DETECT_DELAY_MS 			500		// мсек, интервал определения перегрузки
	#define OVERLOAD_TIME 						20		// сек,  интервал длительности перегрузки
	#define OVERLOAD_TRESHOLD 					200		// A*10
	#define DELAYED_START						60   	// sec

	#define UART_READ_TIMEOUT					1000  // влияет на результаты чтения из юсарт

	#define CUT_OFF_INCORRECT_VALUE			// если ток превышает 100А, напряжение 400В (или 0В), мощность 25 кВт, то текущему значению присваивается предыдущее
	#define SDM_PAUSE_TASK_MS 					50
	#define MQTT_PAUSE_TASK_MS 					50

	#define CHECK_ERROR_COUNT			100

	#define RESET_LOAD_GPIO				2
	#define RESET_GPIO_OFF				0
	#define RESET_GPIO_ON				1

	#define SDM_ADDR					0x0001
	#define SDM_NO_COMMAND				0xFFFF
	#define SDM_VOLTAGE 				0x0000
	#define SDM_CURRENT 				0x0006
	#define SDM_POWER   				0x000C
	#define SDM_ENERGY  				(uint16_t)0x0156
	#define SDM_ENERGY_RESETTABLE  		(uint16_t)0x0180

	#define high_byte(val) (uint8_t) ( val >> 8 )
	#define low_byte(val) (uint8_t) ( val & 0xFF )

	typedef struct {
		float value;
		uint32_t dt;
	} dt_value_t;

	typedef struct {
		dt_value_t voltage_min;
		dt_value_t voltage_max;
		dt_value_t current_max;
		float energy_00;		// запомним значение счетчика в 00:00
		float energy_07;		// запомним значение счетчика в 07:00
		float energy_07_prev;		// запомним значение счетчика в 07:00
		float energy_23;		// запомним значения счетчика в 23:00
		float energy_23_prev;		// запомним значения счетчика в 23:00
		float energy_yesterday; // вычисленное значение
		float energy_23_07;
		float energy_07_23;
		uint32_t crc32;
	} rtc_data_t;

	rtc_data_t rtc_data;

	typedef struct SDMCommand_request {
		uint8_t addr;
		uint8_t func_code;		// input "04" or holding register "03"
		uint8_t start_addr[2];	//start address register for request - hugh byte, low byte
		uint8_t count_reg[2];	// count of registers for request -  high byte, low byte
		uint8_t crc[2];			// crc - low byte, high byte
	} SDMCommand_request_t;

	#define REQUEST_SIZE sizeof(SDMCommand_request_t)

	typedef struct SDMCommand_response {
		uint8_t addr;
		uint8_t func_code;		// input "04" or holding register "03"
		uint8_t byte_cnt;		// bytes count in response
		uint8_t resp_data[4];   // response data: reg1_hi, reg1_low, reg2_hi, reg2_low
		uint8_t crc[2];			// crc - low byte, high byte
	} SDMCommand_response_t;

	#define RESPONSE_SIZE sizeof(SDMCommand_response_t)
	#define RESPONSE_DATA_SIZE 4

#ifdef MQTTD
	#define MQTT_SEND_INTERVAL 10 // sec
	#define VOLTAGE_MQTT_TOPIC_PARAM	"pmv"
	#define CURRENT_MQTT_TOPIC_PARAM	"pmc"
	#define POWER_MQTT_TOPIC_PARAM		"pmw"
	#define ENERGY_MQTT_TOPIC_PARAM		"pmwh"
	#define OVERLOAD_MQTT_TOPIC_PARAM		"overload"
	#define MQTT_PAYLOAD_BUF 20
	char payload[MQTT_PAYLOAD_BUF];
	uint32_t mqtt_send_interval_sec = MQTT_SEND_INTERVAL;
	MQTT_Client* mqtt_client;
	static volatile os_timer_t mqtt_send_timer;	
	void mqtt_send_cb();
#endif


	uint8_t sdm_task_delay = SDM_PAUSE_TASK_MS;
	uint32_t command = SDM_NO_COMMAND;
	static volatile float voltage = 0;
	static volatile float voltage_prev = 0;
	static volatile float current = 0;
	static volatile float power = 0;
	static volatile float energy = 0;
	static volatile float energy_resettable = 0;

	float energy_yesterday = 0;
	float energy_today = 0;
	float energy_23_07 = 0;
	float energy_07_23 = 0;

	uint8_t delayed_counter = DELAYED_START;

	static volatile os_timer_t read_electro_timer;
	static volatile os_timer_t system_start_timer;
	static volatile os_timer_t overload_detect_timer;
	static volatile os_timer_t overload_reset_timer;


	uint8_t overload = 0;										// флаг наличия перегрузки
	uint16_t overload_treshold = OVERLOAD_TRESHOLD;					// valdes[0]
	uint32_t overload_time = OVERLOAD_TIME;
	uint32_t overload_detect_delay = OVERLOAD_DETECT_DELAY_MS;

	uint16_t error_count = 0;
	uint8_t opt_saving = 0;

	void system_start_cb( );
	void read_electro_cb();	
	void overload_detect_cb();	
	void overload_reset_cb();	

	// uart0
	void send_buffer(uint8_t *buffer, uint8_t len);
	void read_buffer();
	
	void sdm_send (uint8_t addr, uint8_t fcode, uint32_t reg);
	float sdm_read(uint8_t addr, uint8_t *buffer, uint8_t cnt);

	uint16_t sdm_crc(uint8_t *data, uint8_t sz);

	void request_voltage(uint8_t addr);
	void request_current(uint8_t addr);
	void request_power(uint8_t addr);
	void request_energy(uint8_t addr);
	void request_energy_resettable(uint8_t addr);
	
	#define millis() (uint32_t) (micros() / 1000ULL) 

void calc_min(const float value, dt_value_t *min) {
	if ( value == 0 ) return;
	if ( min->value == 0 ) min->value = value;
	else if ( min->value > value ) min->value = value;
}

void calc_max(const float value, dt_value_t *max) {
	if ( value == 0 ) return;
	// max
	if ( max->value == 0 ) max->value = value;
	else if ( max->value < value ) max->value = value;
}

void send_buffer(uint8_t *buffer, uint8_t len){
	uart0_tx_buffer(buffer, len);
}

void read_buffer(){	
	static char rx_buf[125];
	static uint8_t i = 0;

	uint32_t ts = micros();

	WRITE_PERI_REG(UART_INT_CLR(UART0),UART_RXFIFO_FULL_INT_CLR);
	while ( READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S) 
			&& ( micros() - ts < UART_READ_TIMEOUT*1000)) 
	{
		WRITE_PERI_REG(0X60000914, 0x73); //WTD
		uint8_t read = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		rx_buf[i] = read; // buffer[i++] = read;
		i++;
		ts = micros();
		if ( RESPONSE_SIZE == i) {
		
			// что то прочитали
			float v;
			v = sdm_read(SDM_ADDR, rx_buf, i);

			switch ( command ) {
				case SDM_VOLTAGE:
					#ifdef CUT_OFF_INCORRECT_VALUE
						voltage = ( v == 0 || v > 400) ? voltage : v;
					#else
						voltage = ( v == 0 ) ? voltage : v;
					#endif
					calc_min( voltage, &rtc_data.voltage_min);
					calc_max( voltage, &rtc_data.voltage_max);
					break;
				case SDM_CURRENT:
					#ifdef CUT_OFF_INCORRECT_VALUE
						current = ( v == 0 || v > 100) ? current : v;
					#else
						current = ( v == 0) ? current : v;		
					#endif
					calc_max( current, &rtc_data.current_max);
					break;
				case SDM_POWER:
					#ifdef CUT_OFF_INCORRECT_VALUE
						power = ( v == 0 || v > 25000) ? power : v;	
					#else
						power = ( v == 0) ? power : v;
					#endif					
					break;
				case SDM_ENERGY:
					energy = ( v == 0) ? energy : v;
					break;
				case SDM_ENERGY_RESETTABLE:
					energy_resettable = ( v == 0) ? energy_resettable : v;
					break;					
				default:
					break;
			}
			command = SDM_NO_COMMAND;
			i = 0;		
			break;
		}	
	}
}


void ICACHE_FLASH_ATTR mqtt_send_valdes(uint32_t val, uint8_t idx) {
	if ( sensors_param.mqtten != 1 ) return;
	if ( mqtt_client == NULL) return;
	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", val);
	system_soft_wdt_feed();	
	MQTT_Publish(mqtt_client, "valuedes0", payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);
}

uint8_t ICACHE_FLASH_ATTR get_config_values(uint8_t r) {   // return 0 - no need reinitialize, 1 - need reinitialize

	uint8_t reinit = 0;
	//reinit =  r && (pzem_enabled != sensors_param.cfgdes[0]);  // данные изменились
	sdm_task_delay = (sensors_param.cfgdes[0] > 0) ? sensors_param.cfgdes[0] : SDM_PAUSE_TASK_MS; 

	#ifdef MQTTD	
		//reinit = r && (mqtt_send_interval_sec != sensors_param.cfgdes[1]);
		mqtt_send_interval_sec = (sensors_param.cfgdes[1] == 0) ? sensors_param.mqttts : sensors_param.cfgdes[1];		
	#endif	

	uint16_t prev_treshhold = overload_treshold;
	overload_treshold = (sensors_param.cfgdes[2] == 0 || sensors_param.cfgdes[2] > 300) ? OVERLOAD_TRESHOLD :  sensors_param.cfgdes[2];

	if ( prev_treshhold != overload_treshold ) {
		// поменялось значение в настройках, надо обновить valdes[0]
		prev_treshhold = overload_treshold;
		valdes[0] = overload_treshold;
		#ifdef MQTTD
			mqtt_send_valdes(valdes[0], 0);
		#endif		
	} else {
		// значение не менялось, но могло поменяться в valdes[0]
		uint16_t tmp_current_treshold = valdes[0];  // получили по mqtt или через get, но здесь может быть и предыдущее значение
		if ( tmp_current_treshold > 0 && tmp_current_treshold != overload_treshold ) {  
			// значение в valdes[0] отличается от текущего и в опциях
			overload_treshold = tmp_current_treshold;
			sensors_param.cfgdes[2] = overload_treshold;
			opt_saving = 1;
			system_soft_wdt_feed();
			SAVEOPT
			os_delay_us(500);
			system_soft_wdt_feed();
			opt_saving = 0;
		}
	}

	overload_detect_delay = (sensors_param.cfgdes[3] == 0) ? OVERLOAD_DETECT_DELAY_MS : sensors_param.cfgdes[3];
	overload_time = (sensors_param.cfgdes[4] == 0) ? OVERLOAD_TIME : sensors_param.cfgdes[4];

	return reinit;
}


uint32_t calcCRC32(const uint8_t *data, uint16_t sz) {
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

void ICACHE_FLASH_ATTR startfunc(){

	overload = 0;

	// выполняется один раз при старте модуля.
	uart_init(BIT_RATE_9600);	  
	ETS_UART_INTR_ATTACH(read_buffer, NULL);

	get_config_values(0);

	// читаем rtc
	system_rtc_mem_read(70, &rtc_data, sizeof(rtc_data_t));
	// проверяем crc
	uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	if ( crc32 != rtc_data.crc32 ) {
		// кривые данные, обнулим
		os_memset(&rtc_data, 0, sizeof(rtc_data_t));
		crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
		rtc_data.crc32 = crc32;
		// пишем в rtc обнуленные данные
		system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));
	}

	// запуск таймера, чтобы мой основной код начал работать через Х секунд после старта, чтобы успеть запустить прошивку
	os_timer_disarm(&system_start_timer);
	os_timer_setfn(&system_start_timer, (os_timer_func_t *)system_start_cb, NULL);
	os_timer_arm(&system_start_timer, DELAYED_START * 1000, 0);
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
	
	get_config_values(0);

	if ( delayed_counter > 0 ) { 
		delayed_counter--;	
	}	

	// проверяем на зависание чтения (последнее время чтение данных зависает примерно через 4 дня и данные не изменяются)
	// будем проверять по напряжению, если напряжение было одинаково 100 сек подряд, значит могло зависнуть
	if ( voltage != voltage_prev ) {
		// все хорошо, запомним предыдущее показание
		voltage_prev = voltage;
		error_count = 0;
	} else {
		// показания одинаковы, увеличиваем ошибку
		error_count++;

		/*
			если кол-во превысило лимит и время не равно
			0:00:00-59 или
			0:07:00-59 или
			0:23:00-59 
			отправляем по mqtt
			ребутим
		*/
	}


	// корректировка нулевых значений счетчиков
	if ( rtc_data.energy_00 == 0) rtc_data.energy_00 = energy;
	if ( rtc_data.energy_07 == 0) rtc_data.energy_07 = energy;
	if ( rtc_data.energy_07_prev == 0) rtc_data.energy_07_prev = energy;
	if ( rtc_data.energy_23 == 0) rtc_data.energy_23 = energy;
	if ( rtc_data.energy_23_prev == 0) rtc_data.energy_23_prev = energy;

	// пишем данные в rtc
	if ( time_loc.hour == 0 && time_loc.min == 0 && time_loc.sec == 0 )
	{
		if ( energy > 0 ) {
			rtc_data.energy_yesterday = energy - rtc_data.energy_00;
			rtc_data.energy_00 = energy;
		}
	} 
	else if ( time_loc.hour == 7 && time_loc.min == 0 && time_loc.sec == 0 )
	{
		if ( energy > 0 ) {
			rtc_data.energy_23_07 = rtc_data.energy_07 - rtc_data.energy_23_prev;
			rtc_data.energy_07_prev = rtc_data.energy_07; 
			rtc_data.energy_07 = energy;
		}
	}
	else if ( time_loc.hour == 23 && time_loc.min == 0 && time_loc.sec == 0 )
	{
		rtc_data.energy_07_23 = rtc_data.energy_23 - rtc_data.energy_07;
		if ( energy > 0 ) {
			rtc_data.energy_23_prev = rtc_data.energy_23;
			rtc_data.energy_23 = energy;
		}
	}

	uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	rtc_data.crc32 = crc32;
	// пишем в rtc обнуленные данные
	system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));

	energy_today = (energy > 0) ? energy - rtc_data.energy_00 : 0;

	if ( time_loc.hour >=7 && time_loc.hour < 23 )
	{
		energy_07_23 = (energy > 0) ? energy - rtc_data.energy_07 : 0;
	} else {
		energy_07_23 = rtc_data.energy_07_23;
	}
	
	if ( time_loc.hour < 7 || time_loc.hour >= 23 )
	{
		energy_23_07 = ( energy > 0 ) ? energy - rtc_data.energy_23 : 0;
	} else {
		energy_23_07 = rtc_data.energy_23_07;
	}	

}

void system_start_cb( ){

	os_timer_disarm(&read_electro_timer);
	os_timer_setfn(&read_electro_timer, (os_timer_func_t *)read_electro_cb, NULL);
	os_timer_arm(&read_electro_timer, sdm_task_delay, 0); // будет рестартовать сам себя

	os_timer_disarm(&overload_detect_timer);
	os_timer_setfn(&overload_detect_timer, (os_timer_func_t *)overload_detect_cb, NULL);
	os_timer_arm(&overload_detect_timer, overload_detect_delay, 1);


	mqtt_client = (MQTT_Client*) &mqttClient;
	os_timer_disarm(&mqtt_send_timer);
	os_timer_setfn(&mqtt_send_timer, (os_timer_func_t *)mqtt_send_cb, NULL);
	os_timer_arm(&mqtt_send_timer, mqtt_send_interval_sec * 1000, 1);

	command = SDM_NO_COMMAND;
}

void sdm_send (uint8_t addr, uint8_t fcode, uint32_t reg) {
	if ( opt_saving ) return;
	SDMCommand_request_t sdm;
	sdm.addr = addr;
	sdm.func_code = fcode;
	sdm.start_addr[0] = high_byte(reg);
	sdm.start_addr[1] = low_byte(reg);
  	sdm.count_reg[0] = 0;  //high
	sdm.count_reg[1] = 2;  //low

	uint8_t *bytes = (uint8_t*)&sdm;
	uint16_t crc = sdm_crc(bytes, sizeof(sdm) - 2 );	
	sdm.crc[0] = low_byte(crc);
	sdm.crc[1] = high_byte(crc);

	send_buffer(bytes, sizeof(sdm));
	os_delay_us(20);
}

float sdm_read(uint8_t addr, uint8_t *buffer, uint8_t cnt) {
	uint8_t fcode = 4;
	uint8_t i;
	uint8_t data[RESPONSE_DATA_SIZE];
	float value = 0;
	if ( cnt ==  RESPONSE_SIZE && buffer[0] == addr && buffer[1] == fcode && buffer[2] ==  RESPONSE_DATA_SIZE ) {
		uint16_t crc = sdm_crc(buffer, RESPONSE_SIZE - 2 );
		if ( buffer[6] | (buffer[7] << 8) == crc) {
          ((uint8_t*)&value)[2]= buffer[4];
          ((uint8_t*)&value)[3]= buffer[3];
          ((uint8_t*)&value)[1]= buffer[5];
          ((uint8_t*)&value)[0]= buffer[6];
		}		
	}
	return value;
}

uint16_t sdm_crc(uint8_t *data, uint8_t sz) {
	uint16_t _crc, _flag;
	_crc = 0xFFFF;
	uint8_t i,j;
	for (i = 0; i < sz; i++) {
    	_crc = _crc ^ data[i];
    	for (j = 8; j; j--) {
    		_flag = _crc & 0x0001;
    		_crc >>= 1;
    		if (_flag)
        	_crc ^= 0xA001;
    	}
  	}
  	return _crc;	
}

void request_voltage(uint8_t addr) {
	sdm_send(addr, 4, SDM_VOLTAGE);
}

void request_current(uint8_t addr) {
	sdm_send(addr, 4, SDM_CURRENT);
}

void request_power(uint8_t addr) {
	sdm_send(addr, 4, SDM_POWER);
}

void request_energy(uint8_t addr) {
	sdm_send(addr, 4, SDM_ENERGY);
}

void request_energy_resettable(uint8_t addr) {
	sdm_send(addr, 4, SDM_ENERGY_RESETTABLE);
}

void read_electro_params_c3_v1_c3_p1__e120(uint8_t counter) {
		int m = counter % 4;
		if ( counter == 119 ) {
			command = SDM_ENERGY;
			request_energy(SDM_ADDR);
		} else if ( counter == 120) {
			command = SDM_ENERGY_RESETTABLE;
			request_energy_resettable(SDM_ADDR);
		} else if ( m != 0) {
			command = SDM_CURRENT;
			request_current(SDM_ADDR);
		} else {
			int d = counter / 4;
			int mm = d % 2;
			if ( mm == 1 ) {
				command = SDM_VOLTAGE;
				request_voltage(SDM_ADDR);
			} else {
				command = SDM_POWER;
				request_power(SDM_ADDR);
			}				
		}
}

void read_electro_params_c20vp__er_10sec(uint8_t counter) {
	// c20vp - ток 20 раз подряд, 1 раз, 1 раз
	// er_60sec - расход 1 раз в 10 сек
	// 20 * 50 msec = 1000 msec, => 20 раз/сек
	// 20 * 100 msec = 2000 msec, => 10 раз/сек
	static uint32_t ts = 0;
	static uint32_t ts2 = 0;

	if ( millis() - ts> 10 * 1000 ) {  // раз в 10 сек
		command = SDM_ENERGY;
		request_energy(SDM_ADDR);
		ts = millis();
	} else if ( millis() - ts2> 11 * 1000 ) { // раз в 11 сек
		command = SDM_ENERGY_RESETTABLE;
		request_energy_resettable(SDM_ADDR);
		ts2 = millis();
	} else if ( counter < 21 ) {
		command = SDM_CURRENT;
		request_current(SDM_ADDR);
	} else if ( counter == 21 ) {
		command = SDM_VOLTAGE;
		request_voltage(SDM_ADDR);		
	} else if ( counter == 22) {
		command = SDM_POWER;
		request_power(SDM_ADDR);		
	}
}

void read_electro_cb()
{
	static uint8_t el_cnt = 0;
	if ( command == SDM_NO_COMMAND) {	
		// можно писать в uart
		el_cnt++;  // увеличим счетчик

		system_soft_wdt_feed();
		#ifdef ELECTRO_C20_V1_P1__E10
			if ( el_cnt > 22 ) el_cnt = 1;		
			read_electro_params_c20vp__er_10sec(el_cnt);
		#else
			if ( el_cnt > 120 ) el_cnt = 1;
			read_electro_params_c3_v1_c3_p1__e120(el_cnt);
		#endif
		os_delay_us(500);	
	}
	os_timer_disarm(&read_electro_timer);
	os_timer_setfn(&read_electro_timer, (os_timer_func_t *)read_electro_cb, NULL);
	os_timer_arm(&read_electro_timer, sdm_task_delay, 0);		
}

#ifdef MQTTD
void mqtt_send_cb() {
	if ( sensors_param.mqtten != 1 ) return;

	system_soft_wdt_feed();

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d.%d", (int)voltage, 		(int)(voltage*10) % 10);
	MQTT_Publish(mqtt_client, VOLTAGE_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d.%d", (uint16_t)current, 		(uint16_t)(current*100) % 100);
	MQTT_Publish(mqtt_client, CURRENT_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", (int)power);
	MQTT_Publish(mqtt_client, POWER_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	//os_sprintf(payload,"%d", (int)energy);
	os_sprintf(payload,"%d.%d", (uint32_t)energy, 		(uint32_t)(energy*100) % 100);
	MQTT_Publish(mqtt_client, ENERGY_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", (int)overload);
	MQTT_Publish(mqtt_client, OVERLOAD_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);	
}	
#endif

void overload_detect_cb()
{
	//  отрабатывает каждые 50 мсек

	if ( current >= ((float)overload_treshold / 10.0f) )
	{
		// есть перегрузка по току
		overload = 1;
		GPIO_ALL( RESET_LOAD_GPIO, RESET_GPIO_ON);

		// запустим единичный overload_reset_timer, который обнулит флаг перегрузки и выставит gpio2 в 0
		os_timer_disarm(&overload_reset_timer);
		os_timer_setfn(&overload_reset_timer, (os_timer_func_t *)overload_reset_cb, NULL);
		os_timer_arm(&overload_reset_timer, overload_time * 1000, 0);

		// overload_reset_timer сбросит флаг перегрузки через Х сек после сработки перегрузки
		// если перегрузка не прекратилась, таймер перезапускается, т.е. в итоге флаг сбросится таймером только после Х сек после последнего детекта перегрузки		
	} 
}

void overload_reset_cb()
{
	overload = 0;
	GPIO_ALL( RESET_LOAD_GPIO, RESET_GPIO_OFF);
}

void webfunc(char *pbuf) {

	if ( delayed_counter > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных счетчика осталось %d секунд", delayed_counter);
	}
	
	if ( opt_saving ) os_sprintf(HTTPBUFF,"<p style='color: red;'><small><b>Идет сохранение настроек!</b></small></p>"); 
	//os_sprintf(HTTPBUFF,"<p><small><b>valdes[0]:</b> %d</small></p>", valdes[0]); 
	//os_sprintf(HTTPBUFF,"<p><small><b>sensros_param[3]:</b> %d</small></p>", sensors_param.cfgdes[3]); 
	//os_sprintf(HTTPBUFF,"<p><small><b>current tr:</b> %d</small></p>", overload_treshold); 
	//os_sprintf(HTTPBUFF,"<p><small><b>Задержка определения перегрузки:</b> %d</small></p>", control_current_delay); 
	//os_sprintf(HTTPBUFF,"<p><small><b>Задержка после пропадания перегрузки:</b> %d</small></p>", control_load_on_delay); 
	
	os_sprintf(HTTPBUFF, "<table width='100%%' cellpadding='2' cellspacing='2' cols='2'>"
							"<tr>"
							"<td align='left'><b>Отсечка по току:</b> %d.%d A</td>"
							"<td align='right'><b>Перегрузка: </b><strong><span style='color: %s;'>%s</span></strong></td>"
							"</tr>"
						  "</table>", 
						  (uint16_t)overload_treshold / 10, 		
						  (uint16_t)overload_treshold % 10,		
						  overload ? "red" : "green", 
						  overload ? "ДА" : "НЕТ"
				);

	os_sprintf(HTTPBUFF,"<b>GPIO2: </b><span style='color: %s;'><b>%s</b></span>"
							, GPIO_ALL_GET( RESET_LOAD_GPIO ) > 0 ? "red" : "blue"
							, GPIO_ALL_GET( RESET_LOAD_GPIO ) > 0 ? "ВКЛ" : "ВЫКЛ" 
				); 

	os_sprintf(HTTPBUFF, "<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>"
							"<tr>"
							"<td><b>Напряжение: </b>%d <small>В</small></td>"
							"<td><b>Ток: </b>%d.%d <small>А</small></td>"
							"<td><b>Мощность: </b>%d <small>Вт</small></td>"
							"</tr>"
						  "</table>", 
						  (int)voltage, 		
						  (uint16_t)current, 		(uint16_t)(current*100) % 100,
						  (uint16_t)power
				);
	
	
	os_sprintf(HTTPBUFF, "<table width='100%%' cellpadding='2' cellspacing='2' cols='2'>"
							"<tr>"
							"<td><b>Расход общий: </b>%d.%d <small>кВт*ч</small></td>"
							"<td><b>обнуляемый: </b>%d.%d <small>кВт*ч</small></td>"
							"</tr>"
						  "</table>", 
						  (uint32_t)energy, 		(uint32_t)(energy*100) % 100,
						  (uint32_t)energy_resettable, 		(uint32_t)(energy_resettable*100) % 100
				);

	
	os_sprintf(HTTPBUFF, "<div>");
	
	os_sprintf(HTTPBUFF, "<div><b>Напряжение (min):</b> ");
	if ( rtc_data.voltage_min.value > 0 )
		os_sprintf(HTTPBUFF, "%d В</div>", (int)rtc_data.voltage_min.value);
	else
		os_sprintf(HTTPBUFF, "---</div>");
	
	os_sprintf(HTTPBUFF, "<div><b>Напряжение (max):</b> ");
	if ( rtc_data.voltage_max.value > 0 )
		os_sprintf(HTTPBUFF, "%d В</div>", (int)rtc_data.voltage_max.value);
	else
		os_sprintf(HTTPBUFF, "---</div>");


	os_sprintf(HTTPBUFF, "<div><b>Ток (max):</b> ");
	if ( rtc_data.current_max.value > 0 )
		os_sprintf(HTTPBUFF, "%d.%d А</div>", (uint16_t)rtc_data.current_max.value, 		(uint16_t)(rtc_data.current_max.value*100) % 100);
	else
		os_sprintf(HTTPBUFF, "---</div>");

	os_sprintf(HTTPBUFF, "</div>");

	//=====================================
	os_sprintf(HTTPBUFF, "<style>"
						 ".tn {float: left; width: 70px; text-align: right; margin-right: 10px;}"
						 "</style>");

	os_sprintf(HTTPBUFF, "<div>");
	os_sprintf(HTTPBUFF, "<div><b>Расход</b></div>");
	os_sprintf(HTTPBUFF, "<div><b class='tn'>вчера:</b> %d.%d кВт*ч</div>", (uint32_t)rtc_data.energy_yesterday, 		(uint32_t)(rtc_data.energy_yesterday*100) % 100);
	os_sprintf(HTTPBUFF, "<div><b class='tn'>сегодня:</b> %d.%d кВт*ч</div>", (uint32_t)energy_today, 		(uint32_t)(energy_today*100) % 100);
	os_sprintf(HTTPBUFF, "<div><b class='tn'>день:</b> %d.%d кВт*ч</div>", (uint32_t)energy_07_23, 		(uint32_t)(energy_07_23*100) % 100);
	os_sprintf(HTTPBUFF, "<div><b class='tn'>ночь:</b> %d.%d кВт*ч</div>", (uint32_t)energy_23_07, 		(uint32_t)(energy_23_07*100) % 100);
	os_sprintf(HTTPBUFF, "</div>");

	//=========================================
	os_sprintf(HTTPBUFF, "<p><small><b>Ошибки чтения:</b> %d</small></p>", error_count);

	os_sprintf(HTTPBUFF,"<p><small><b>Версия прошивки:</b> %s</small></p>", FW_VER); 
	os_sprintf(HTTPBUFF,"<p style='color: red;'><small><b>Прежде чем менять данные, перезагрузи модуль!!!</b></small></p>"); 

 // когда перегрузка закончилась, показать таймер отчета до включения устройства
}