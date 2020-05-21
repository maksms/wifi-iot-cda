	//#include "driver/uart.h"
	#include "../moduls/uart_register.h"
	#include "../moduls/uart.h"
	#include "../moduls/uart.c" // ??????

	#define MQTTD

	//#define DEBUG
	#define FW_VER_NUM "1.3"



	#define millis() (uint32_t) (micros() / 1000ULL) 

	#ifdef DEBUG
	static char logstr[100];
	#define FW_VER FW_VER_NUM  ".4 debug"
	#else
	#define FW_VER FW_VER_NUM
	#endif	

	#define DELAYED_START					10   //sec

	#define UART_READ_TIMEOUT					1000  // влияет на результаты чтения из юсарт

	#define CUT_OFF_INCORRECT_VALUE			// если ток превышает 100А, напряжение 400В (или 0В), мощность 25 кВт, то текущему значению присваивается предыдущее
	#define PZEM_PAUSE_TASK 	20

	#define CMD_VOLTAGE 		0xB0
	#define RESP_VOLTAGE 		0xA0
	#define CMD_CURRENT 		0xB1
	#define RESP_CURRENT 		0xA1
	#define CMD_POWER   		0xB2
	#define RESP_POWER   		0xA2
	#define CMD_ENERGY  		0xB3
	#define RESP_ENERGY  		0xA3

	typedef  uint8_t PZEMAddress[4] ;
	PZEMAddress pzem_addr = {192, 168, 1, 1};

	typedef struct PZEMCommand {
		uint8_t command;
		uint8_t addr[4];
		uint8_t data;
		uint8_t crc;
	} PZEMCommand;


	#define RESPONSE_SIZE sizeof(PZEMCommand)
	#define RESPONSE_DATA_SIZE RESPONSE_SIZE - 2

#ifdef MQTTD
	#define MQTT_SEND_INTERVAL 10 // sec
	#define VOLTAGE_MQTT_TOPIC_PARAM	"pmv"
	#define CURRENT_MQTT_TOPIC_PARAM	"pmc"
	#define POWER_MQTT_TOPIC_PARAM		"pmw"
	#define ENERGY_MQTT_TOPIC_PARAM		"pmwh"
	#define MQTT_PAYLOAD_BUF 20
	MQTT_Client* mqtt_client;    //for non os sdk
	char payload[MQTT_PAYLOAD_BUF];
	uint32_t mqtt_send_interval_sec = MQTT_SEND_INTERVAL;

	static volatile os_timer_t mqtt_send_timer;	
	void mqtt_send_cb();
#endif


	uint8_t pzem_enabled = 1;
	uint8_t command = 0;
	float voltage = 0;
	float current = 0;
	float power = 0;
	float energy = 0;

	uint8_t delayed_counter = DELAYED_START;
	static volatile os_timer_t read_electro_timer;
	static volatile os_timer_t system_start_timer;

	void system_start_cb( );
	void read_electro_cb();	

	// uart0
	void send_buffer(uint8_t *buffer, uint8_t len);
	void read_buffer();
	
	void pzem_send (uint8_t *addr, uint8_t cmd);
	uint8_t pzem_crc(uint8_t *data, uint8_t sz);

	void request_voltage(uint8_t *addr);
	float read_voltage(uint8_t *buffer, uint8_t cnt);

	void request_current(uint8_t *addr);
	float read_current(uint8_t *buffer, uint8_t cnt);

	void request_power(uint8_t *addr);
	float read_power(uint8_t *buffer, uint8_t cnt);

	void request_energy(uint8_t *addr);
	float read_energy(uint8_t *buffer, uint8_t cnt);

	
	//uart

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
			switch ( command ) {
				case CMD_VOLTAGE:
					v = read_voltage(rx_buf, i);
					#ifdef CUT_OFF_INCORRECT_VALUE
						voltage = ( v == 0 || v > 400) ? voltage : v;
					#else
						voltage = ( v == 0 ) ? voltage : v;
					#endif
					#ifdef DEBUG
						os_bzero(logstr, 100);
						os_sprintf(logstr, "[%d] voltage: %d.%d \n", millis(), (int)voltage, 		(int)(voltage*10) % 10);
						uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif

					break;
				case CMD_CURRENT:
					v = read_current(rx_buf, i);
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
				case CMD_POWER:
					v = read_power(rx_buf, i);
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
				case CMD_ENERGY:
					v = read_energy(rx_buf, i);
					energy = ( v == 0) ? energy : v;
					#ifdef DEBUG
						os_bzero(logstr, 100);
						os_sprintf(logstr, "[%d] energy: %d.%d \n", millis(), (int)energy, 		(int)(energy*100) % 100);
						uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif

					break;
				default:
					break;
			}
			command = 0;
			i = 0;		
			break;
		}	
	}
}


uint8_t get_config_values(uint8_t r) {   // return 0 - no need reinitialize, 1 - need reinitialize
	uint8_t reinit = 0;
	//reinit =  r && (pzem_enabled != sensors_param.cfgdes[0]);  // данные изменились
	pzem_enabled = (sensors_param.cfgdes[0] > 0) ? 1 : 0;  // читать данные pzem

#ifdef MQTTD	
	//reinit = r && (mqtt_send_interval_sec != sensors_param.cfgdes[1]);
	mqtt_send_interval_sec = (sensors_param.cfgdes[1] == 0) ? sensors_param.mqttts : sensors_param.cfgdes[1];		
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

	// запуск таймера, чтобы мой основной код начал работать через Х секунд после старта, чтобы успеть запустить прошивку
	os_timer_disarm(&system_start_timer);
	os_timer_setfn(&system_start_timer, (os_timer_func_t *)system_start_cb, NULL);
	os_timer_arm(&system_start_timer, DELAYED_START * 1000, 0);
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
		
	if ( delayed_counter > 0 ) { 
		delayed_counter--;	
	}	
}

void system_start_cb( ){
	os_timer_disarm(&read_electro_timer);
	os_timer_setfn(&read_electro_timer, (os_timer_func_t *)read_electro_cb, NULL);
	os_timer_arm(&read_electro_timer, 20, 0); // будет рестартовать сам себя

	mqtt_client = (MQTT_Client*) &mqttClient;
	os_timer_disarm(&mqtt_send_timer);
	os_timer_setfn(&mqtt_send_timer, (os_timer_func_t *)mqtt_send_cb, NULL);
	os_timer_arm(&mqtt_send_timer, mqtt_send_interval_sec * 1000, 1);

	command = 0;
}

void pzem_send (uint8_t *addr, uint8_t cmd) {
	PZEMCommand pzem;
	pzem.command = cmd;
	uint8_t i;
	for ( i = 0; i < sizeof(pzem.addr); i++) pzem.addr[i] = addr[i];
	pzem.data = 0;
	uint8_t *bytes = (uint8_t*)&pzem;
	pzem.crc = pzem_crc(bytes, sizeof(pzem) - 1);
	send_buffer(bytes, sizeof(pzem));
}

void request_voltage(uint8_t *addr) {
	if ( !pzem_enabled ) return;
	pzem_send(addr, CMD_VOLTAGE);
	//delay(500);
}

void request_current(uint8_t *addr) {
	if ( !pzem_enabled ) return;
	pzem_send(addr, CMD_CURRENT);
	//delay(500);
}

void request_power(uint8_t *addr) {
	if ( !pzem_enabled ) return;
	pzem_send(addr, CMD_POWER);
	//delay(500);
}

void request_energy(uint8_t *addr) {
	if ( !pzem_enabled ) return;
	pzem_send(addr, CMD_ENERGY);
	//delay(500);
}

float read_voltage(uint8_t *buffer, uint8_t cnt) {
	uint8_t i;
	uint8_t data[RESPONSE_DATA_SIZE];
	float value = 0;
	if ( cnt ==  RESPONSE_SIZE && buffer[0] == RESP_VOLTAGE && buffer[6] ==  pzem_crc(buffer, cnt-1) ) {
		for ( i = 0; i < RESPONSE_DATA_SIZE; i++) data[i] = buffer[1 + i];
		value = (data[0] << 8) + data[1] + ( data[2] / 10.0);
	}
	return value;
}
	
float read_current(uint8_t *buffer, uint8_t cnt) {
	uint8_t i;
	uint8_t data[RESPONSE_DATA_SIZE];
	float value = 0;
	if ( cnt ==  RESPONSE_SIZE && buffer[0] == RESP_CURRENT && buffer[6] ==  pzem_crc(buffer, cnt-1) ) {
		for ( i = 0; i < RESPONSE_DATA_SIZE; i++) data[i] = buffer[1 + i];
		value = (data[0] << 8) + data[1] + (data[2] / 100.0);
	}
	return value;
}

float read_power(uint8_t *buffer, uint8_t cnt) {
	uint8_t i;
	uint8_t data[RESPONSE_DATA_SIZE];
	float value = 0;
	if ( cnt ==  RESPONSE_SIZE && buffer[0] == RESP_POWER && buffer[6] ==  pzem_crc(buffer, cnt-1) ) {
		for ( i = 0; i < RESPONSE_DATA_SIZE; i++) data[i] = buffer[1 + i];
		value = (data[0] << 8) + data[1];
	}
	return value;
}

float read_energy(uint8_t *buffer, uint8_t cnt) {
	uint8_t i;
	uint8_t data[RESPONSE_DATA_SIZE];
	float value = 0;
	if ( cnt ==  RESPONSE_SIZE && buffer[0] == RESP_ENERGY && buffer[6] ==  pzem_crc(buffer, cnt-1) ) {
		for ( i = 0; i < RESPONSE_DATA_SIZE; i++) data[i] = buffer[1 + i];
		value = ((uint32_t)data[0] << 16) + ((uint16_t)data[1] << 8) + data[2];
	}
	return value;
}

void read_electro_cb(){
	static uint8_t el_cnt = 1;
	if ( pzem_enabled && command == 0) {	
		// можно писать в uart
		int m = el_cnt % 4;
		if ( el_cnt == 120 ) {
			command = CMD_ENERGY;
			request_energy(pzem_addr);
		} else if ( m != 0) {
			command = CMD_CURRENT;
			request_current(pzem_addr);
		} else {
			int d = el_cnt / 4;
			int mm = d % 2;
			if ( mm == 1 ) {
				command = CMD_VOLTAGE;
				request_voltage(pzem_addr);
			} else {
				command = CMD_POWER;
				request_power(pzem_addr);
			}				
		}	

		os_delay_us(100);	
		system_soft_wdt_feed();

		el_cnt++;  // увеличим счетчик
		if ( el_cnt > 120 ) el_cnt = 1;
	}
	os_timer_disarm(&read_electro_timer);
	os_timer_setfn(&read_electro_timer, (os_timer_func_t *)read_electro_cb, NULL);
	os_timer_arm(&read_electro_timer, 100, 0);		
}

uint8_t pzem_crc(uint8_t *data, uint8_t sz) {
	uint16_t crc = 0;
	uint8_t i;
	for(i=0; i<sz; i++)
		crc += *data++;
	return (uint8_t)(crc & 0xFF);	
}


#ifdef MQTTD
void mqtt_send_cb() {
	if ( sensors_param.mqtten != 1 ) return;


	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d.%d", (int)voltage, 		(int)(voltage*10) % 10);
	MQTT_Publish(mqtt_client, VOLTAGE_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(PZEM_PAUSE_TASK);

	#ifdef DEBUG
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] mqtt: %s:   %s \n", millis(), VOLTAGE_MQTT_TOPIC_PARAM, payload);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d.%d", (int)current, 		(int)(current*100) % 100);
	MQTT_Publish(mqtt_client, CURRENT_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(PZEM_PAUSE_TASK);

	#ifdef DEBUG
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] mqtt: %s:   %s \n", millis(), CURRENT_MQTT_TOPIC_PARAM, payload);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", (int)power);
	MQTT_Publish(mqtt_client, POWER_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(PZEM_PAUSE_TASK);

	#ifdef DEBUG
		os_bzero(logstr, 100);
		os_sprintf(logstr, "[%d] mqtt: %s:   %s \n", millis(), POWER_MQTT_TOPIC_PARAM, payload);
		uart1_tx_buffer(logstr, os_strlen(logstr));
	#endif

	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", (int)energy);
	MQTT_Publish(mqtt_client, ENERGY_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(PZEM_PAUSE_TASK);

	#ifdef DEBUG
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

	os_sprintf(HTTPBUFF,"<br><b>Напряжение:</b> %d.%d В", 	(int)voltage, 		(int)(voltage*10) % 10);
	os_sprintf(HTTPBUFF,"<br><b>Сила тока:</b> %d.%d А", 	(int)current, 		(int)(current*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Мощность:</b> %d Вт", 	(int)power);
	os_sprintf(HTTPBUFF,"<br><b>Расход:</b> %d.%d Вт*ч", 	(int)energy, 		(int)(energy*100) % 100);	
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
}