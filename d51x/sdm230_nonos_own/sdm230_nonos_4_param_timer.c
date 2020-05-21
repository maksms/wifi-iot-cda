	//#include "driver/uart.h"
	#include "../moduls/uart_register.h"
	#include "../moduls/uart.h"
	#include "../moduls/uart.c" // ??????

	#define MQTTD

	//#define DEBUG
	#define FW_VER_NUM "1.3"


	#define ELECTRO_C20_V1_P1__E10

	#define millis() (uint32_t) (micros() / 1000ULL) 

	#ifdef DEBUG
	static char logstr[100];
	#define FW_VER FW_VER_NUM  ".2 debug"
	#else
	#define FW_VER FW_VER_NUM
	#endif	

	#define DELAYED_START					60   //sec

	#define UART_READ_TIMEOUT					1000  // влияет на результаты чтения из юсарт

	#define CUT_OFF_INCORRECT_VALUE			// если ток превышает 100А, напряжение 400В (или 0В), мощность 25 кВт, то текущему значению присваивается предыдущее
	#define SDM_PAUSE_TASK 	50
	#define MQTT_PAUSE_TASK 	50

	#define SDM_ADDR					0x0001

	#define SDM_NO_COMMAND				0xFFFF

	#define SDM_VOLTAGE 				0x0000
	#define SDM_CURRENT 				0x0006
	#define SDM_POWER   				0x000C
	#define SDM_ENERGY  				(uint16_t)0x0156
	#define SDM_ENERGY_RESETTABLE  		(uint16_t)0x0180

	#define high_byte(val) (uint8_t) ( val >> 8 )
	#define low_byte(val) (uint8_t) ( val & 0xFF )

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
	#define MQTT_PAYLOAD_BUF 20
	char payload[MQTT_PAYLOAD_BUF];
	uint32_t mqtt_send_interval_sec = MQTT_SEND_INTERVAL;
	MQTT_Client* mqtt_client;
	static volatile os_timer_t mqtt_send_timer;	
	void mqtt_send_cb();
#endif


	uint8_t sdm_enabled = 0;
	uint8_t sdm_task_delay = SDM_PAUSE_TASK;
	uint32_t command = SDM_NO_COMMAND;
	float voltage = 0;
	float current = 0;
	float power = 0;
	float energy = 0;
	float energy_resettable = 0;

	uint8_t delayed_counter = DELAYED_START;

	static volatile os_timer_t read_electro_timer;
	static volatile os_timer_t system_start_timer;

	void system_start_cb( );
	void read_electro_cb();	

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
	
#ifdef DEBUG
void ICACHE_FLASH_ATTR uart1_tx_buffer(uint8_t *buffer, uint8_t sz) {
	uint8_t i;
	for (i = 0; i < sz; i++) {
		while (true)
		{
			uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(UART1)) & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S);
			if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
				break;
			}
		}
		WRITE_PERI_REG(UART_FIFO(UART1) , buffer[i]);
	}
}
#endif

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
					#ifdef DEBUG
						os_bzero(logstr, 100);
						os_sprintf(logstr, "[%d] voltage: %d.%d \n", millis(), (int)voltage, 		(int)(voltage*100) % 100);
						uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif

					break;
				case SDM_CURRENT:
					#ifdef CUT_OFF_INCORRECT_VALUE
						current = ( v == 0 || v > 100) ? current : v;
					#else
						current = ( v == 0) ? current : v;		
					#endif
					#ifdef DEBUG
						os_bzero(logstr, 100);
						os_sprintf(logstr, "[%d] current: %d.%d \n", millis(), (int)current, 		(int)(current*100) % 100);
						uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif

					break;
				case SDM_POWER:
					#ifdef CUT_OFF_INCORRECT_VALUE
						power = ( v == 0 || v > 25000) ? power : v;	
					#else
						power = ( v == 0) ? power : v;
					#endif					
					#ifdef DEBUG
						os_bzero(logstr, 100);
						os_sprintf(logstr, "[%d] power: %d \n", millis(), (int)power);
						uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif

					break;
				case SDM_ENERGY:
					energy = ( v == 0) ? energy : v;
					#ifdef DEBUG
						os_bzero(logstr, 100);
						os_sprintf(logstr, "[%d] energy: %d.%d \n", millis(), (int)energy, 		(int)(energy*100) % 100);
						uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif

					break;
				case SDM_ENERGY_RESETTABLE:
					energy_resettable = ( v == 0) ? energy_resettable : v;
					#ifdef DEBUG
						os_bzero(logstr, 100);
						os_sprintf(logstr, "[%d] energy_resettable: %d.%d \n", millis(), (int)energy_resettable, 		(int)(energy_resettable*100) % 100);
						uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif
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


uint8_t get_config_values(uint8_t r) {   // return 0 - no need reinitialize, 1 - need reinitialize
	uint8_t reinit = 0;
	//reinit =  r && (pzem_enabled != sensors_param.cfgdes[0]);  // данные изменились
	sdm_enabled = (sensors_param.cfgdes[0] > 0) ? 1 : 0;  // читать данные sdm

	sdm_task_delay = (sensors_param.cfgdes[1] > 0) ? sensors_param.cfgdes[1] : SDM_PAUSE_TASK; 

#ifdef MQTTD	
	//reinit = r && (mqtt_send_interval_sec != sensors_param.cfgdes[1]);
	mqtt_send_interval_sec = (sensors_param.cfgdes[2] == 0) ? sensors_param.mqttts : sensors_param.cfgdes[2];		
#endif	

	return reinit;
}

void ICACHE_FLASH_ATTR startfunc(){



	// выполняется один раз при старте модуля.
	uart_init(BIT_RATE_9600);	  
	ETS_UART_INTR_ATTACH(read_buffer, NULL);

	#ifdef DEBUG
		uart_config(UART1);
		uart_div_modify(UART1,	UART_CLK_FREQ	/BIT_RATE_9600);

		os_install_putc1((void *)uart1_tx_buffer);
	#endif
	
	get_config_values(0);


	#ifdef DEBUG
		os_bzero(logstr, 100);
		os_sprintf(logstr, "Start module \n");
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

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

	#ifdef DEBUG
		os_bzero(logstr, 100);
		os_sprintf(logstr, "countdown: %d \n", delayed_counter);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

		delayed_counter--;	
	}	
}

void system_start_cb( ){

	#ifdef DEBUG
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] %s \n", millis(), __func__);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	os_timer_disarm(&read_electro_timer);
	os_timer_setfn(&read_electro_timer, (os_timer_func_t *)read_electro_cb, NULL);
	os_timer_arm(&read_electro_timer, sdm_task_delay, 0); // будет рестартовать сам себя

	mqtt_client = (MQTT_Client*) &mqttClient;
	os_timer_disarm(&mqtt_send_timer);
	os_timer_setfn(&mqtt_send_timer, (os_timer_func_t *)mqtt_send_cb, NULL);
	os_timer_arm(&mqtt_send_timer, mqtt_send_interval_sec * 1000, 1);

	command = SDM_NO_COMMAND;
}

void sdm_send (uint8_t addr, uint8_t fcode, uint32_t reg) {
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

	#ifdef DEBUG1
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] %s: ", millis(), __func__);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	#ifdef DEBUG1
		uint8_t k;
		for (k=0;k<sizeof(sdm);k++) {
			os_bzero(logstr, 100);
			os_sprintf(logstr, "%02X ", bytes[k]);
			uart1_tx_buffer(logstr, os_strlen(logstr));
		}
		os_bzero(logstr, 100);
		os_sprintf(logstr, "\n");
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

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
	if ( !sdm_enabled ) return;
	sdm_send(addr, 4, SDM_VOLTAGE);
}

void request_current(uint8_t addr) {
	if ( !sdm_enabled ) return;
	sdm_send(addr, 4, SDM_CURRENT);
}

void request_power(uint8_t addr) {
	if ( !sdm_enabled ) return;
	sdm_send(addr, 4, SDM_POWER);
}

void request_energy(uint8_t addr) {
	if ( !sdm_enabled ) return;
	sdm_send(addr, 4, SDM_ENERGY);
}

void request_energy_resettable(uint8_t addr) {
	if ( !sdm_enabled ) return;
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

void read_electro_cb(){

	#ifdef DEBUG1
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] %s: sdm_enabled: %d   command: %04X \n", millis(), __func__, sdm_enabled, command);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	static uint8_t el_cnt = 0;
	if ( sdm_enabled && command == SDM_NO_COMMAND) {	
		// можно писать в uart
		el_cnt++;  // увеличим счетчик

		#ifdef ELECTRO_C20_V1_P1__E10
			if ( el_cnt > 22 ) el_cnt = 1;		
			read_electro_params_c20vp__er_10sec(el_cnt);
		#else
			if ( el_cnt > 120 ) el_cnt = 1;
			read_electro_params_c3_v1_c3_p1__e120(el_cnt);
		#endif

		os_delay_us(500);	
		system_soft_wdt_feed();
	}
	os_timer_disarm(&read_electro_timer);
	os_timer_setfn(&read_electro_timer, (os_timer_func_t *)read_electro_cb, NULL);
	os_timer_arm(&read_electro_timer, sdm_task_delay, 0);		
}

#ifdef MQTTD
void mqtt_send_cb() {
	if ( sensors_param.mqtten != 1 ) return;

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d.%d", (int)voltage, 		(int)(voltage*10) % 10);
	MQTT_Publish(mqtt_client, VOLTAGE_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);

	#ifdef DEBUG1
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] mqtt: %s:   %s \n", millis(), VOLTAGE_MQTT_TOPIC_PARAM, payload);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d.%d", (int)current, 		(int)(current*100) % 100);
	MQTT_Publish(mqtt_client, CURRENT_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);

	#ifdef DEBUG1
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] mqtt: %s:   %s \n", millis(), CURRENT_MQTT_TOPIC_PARAM, payload);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", (int)power);
	MQTT_Publish(mqtt_client, POWER_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);

	#ifdef DEBUG1
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] mqtt: %s:   %s \n", millis(), POWER_MQTT_TOPIC_PARAM, payload);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", (int)energy);
	MQTT_Publish(mqtt_client, ENERGY_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);

	#ifdef DEBUG1
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] mqtt: %s:   %s \n", millis(), ENERGY_MQTT_TOPIC_PARAM, payload);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	system_soft_wdt_feed();
}	
#endif

void webfunc(char *pbuf) {

	if ( delayed_counter > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных счетчика осталось %d секунд", delayed_counter);
	}
	os_sprintf(HTTPBUFF,"<br><b>Напряжение:</b> %d.%d В", 	(uint8_t)voltage, 		(uint8_t)(voltage*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Сила тока:</b> %d.%d А", 	(uint8_t)current, 		(uint8_t)(current*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Мощность:</b> %d.%d Вт", 	(uint16_t)power,   		(uint8_t)(power*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Расход1:</b> %d.%d кВт*ч", 	(uint32_t)energy, 		(uint8_t)(energy*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Расход2:</b> %d.%d кВт*ч", 	(uint32_t)energy_resettable, 		(uint8_t)(energy_resettable*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
}