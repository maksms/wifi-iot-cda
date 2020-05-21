//#include "driver/uart.h"
	#include "../moduls/uart_register.h"
	#include "../moduls/uart.h"
	#include "../moduls/uart.c" // ??????

	#define MQTTD

	#define DEBUG
	#define FW_VER_NUM "2.3"
	#ifdef DEBUG
	static char logstr[100];
	#define FW_VER FW_VER_NUM  ".42 debug"
	#else
	#define FW_VER FW_VER_NUM
	#endif	

	/*
	SDM Enable, SDM Task Delay, MQTT Send Interval, Max ток,Водонагреватель,Детская2,Детская1,Спальня,Кухня,Ванная,Гостинная1,Гостинная2
	sensors_param.cfgdes -  12
		cfgdes[0] - читать данные SDM
		cfgdes[1] - задержка чтения данных с SDM
		cfgdes[2] - время отправки данных с SDM по mqtt
		cfgdes[3] - превышение по току	
				// пример:   X YY ZZZ GGG,   
				//           X - надо вкл ( 1, 0),  
				//           YY - приоритет , 00 - 15, 
				//           ZZZ - 4ый октет IP адреса устройства
				//           GGG - gpio управления
				// 			112223014

		cfgdes[4] - время определения перегрузки, сек, control_current_delay
		cfgdes[5] - время определения отсутствия перегрузки, сек, control_load_on_delay
		
		cfgdes[6] - 1 водонагреватель
		cfgdes[7] - 2 конвектор у саши
		cfgdes[8] - 3 конвектор у тани
		cfgdes[9] - 4 конвектор в спальне
		cfgdes[10] - 5 конвектор на кухне
		cfgdes[11] - 6 конвектор в ванной
		cfgdes[12] - 7 гостинная 1
		cfgdes[13] - 8 гостинная 2

	*/

	#define ELECTRO_C20_V1_P1__E10

	#define START_DEVICES_OPT_IDX 6 // индекс опции в настройках, с которого начинается настройка устройств
	#define DEVICES_COUNT 8
	typedef unsigned char device_name_t[32];
	device_name_t devices_title[DEVICES_COUNT] = {"Водонагреватель", "Детская2", "Детская1", "Спальня", "Кухня", "Ванная", "Гостинная1", "Гостинная2"};


	#define CONTROL_CURRENT_DELAY 			500		// интервал определения перегрузки
	#define CONTROL_OVERLOAD_TIME 				200		// интервал длительности перегрузки
	#define CONTROL_LOAD_ON_DELAY 				5000		// интервал длительности перегрузки
	#define CURRENT_TRESHOLD 				275
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
	static volatile os_timer_t control_current_timer;


	uint8_t is_overload = 0;
	uint16_t current_treshold = CURRENT_TRESHOLD;					// valdes[0]
	uint32_t control_load_on_delay = CONTROL_LOAD_ON_DELAY;
	uint32_t control_current_delay = CONTROL_CURRENT_DELAY;

	uint8_t opt_saving = 0;
struct Gp {
    uint8_t pin;
    uint8_t state;
};

// gpio 0 - 255,  priority- gpio*1000,  need_on - gpio*100000
typedef struct Device {
		uint8_t priority;   // 0 - не используется, 1 - max, 8 - min
		uint8_t gpio;       // 0 - 255
		uint8_t ip;
		uint8_t off;
		uint8_t idx;
		uint8_t need_on;
		uint32_t t_overload;
} Device_t;


static Device_t devices[DEVICES_COUNT];


	void system_start_cb( );
	void read_electro_cb();	
	void control_current_cb();	

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
	#define OPT_GET_GPIO(val) (uint8_t) ( val % 1000 )
	#define OPT_GET_GPIO_IP(val) (uint8_t) ( val % 1000000 / 1000 )
	#define OPT_GET_GPIO_PRIORITY(val) (uint8_t) ( val % 100000000/ 1000000 )
	#define OPT_GET_GPIO_NEED_ON(val) (uint8_t) ( val / 100000000 )
	

	void sort_devices();
	void fill_devices();
	void turn_off_next_device();
	void turn_devices_on();

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
						os_sprintf(logstr, "[%d] energy: %d.%d \n", millis(), (uint32_t)energy, 		(uint32_t)(energy*100) % 100);
						uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif

					break;
				case SDM_ENERGY_RESETTABLE:
					energy_resettable = ( v == 0) ? energy_resettable : v;
					#ifdef DEBUG
						os_bzero(logstr, 100);
						os_sprintf(logstr, "[%d] energy_resettable: %d.%d \n", millis(), (uint32_t)energy_resettable, 		(uint32_t)(energy_resettable*100) % 100);
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


void ICACHE_FLASH_ATTR mqtt_send_valdes(uint32_t val, uint8_t idx) {
	if ( sensors_param.mqtten != 1 ) return;
	if ( mqtt_client == NULL) return;
	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", val);
	MQTT_Publish(mqtt_client, "valuedes0", payload, os_strlen(payload), 2, 0, 1);
	os_delay_us(20);
	system_soft_wdt_feed();	
}

uint8_t ICACHE_FLASH_ATTR get_config_values(uint8_t r) {   // return 0 - no need reinitialize, 1 - need reinitialize



	uint8_t reinit = 0;
	//reinit =  r && (pzem_enabled != sensors_param.cfgdes[0]);  // данные изменились
	sdm_enabled = (sensors_param.cfgdes[0] > 0) ? 1 : 0;  // читать данные sdm

	sdm_task_delay = (sensors_param.cfgdes[1] > 0) ? sensors_param.cfgdes[1] : SDM_PAUSE_TASK; 

#ifdef MQTTD	
	//reinit = r && (mqtt_send_interval_sec != sensors_param.cfgdes[1]);
	mqtt_send_interval_sec = (sensors_param.cfgdes[2] == 0) ? sensors_param.mqttts : sensors_param.cfgdes[2];		
#endif	
	uint16_t prev_treshhold = current_treshold;
	current_treshold = (sensors_param.cfgdes[3] == 0 || sensors_param.cfgdes[3] > 300) ? CURRENT_TRESHOLD :  sensors_param.cfgdes[3];

	if ( prev_treshhold != current_treshold ) {
		// поменялось значение в настройках, надо обновить valdes[0]
		prev_treshhold = current_treshold;
		valdes[0] = current_treshold;
		#ifdef MQTTD
			mqtt_send_valdes(valdes[0], 0);
		#endif		
	} else {
		// значение не менялось, но могло поменяться в valdes[0]
		uint16_t tmp_current_treshold = valdes[0];  // получили по mqtt или через get, но здесь может быть и предыдущее значение
		if ( tmp_current_treshold > 0 && tmp_current_treshold != current_treshold ) {  
			// значение в valdes[0] отличается от текущего и в опциях
			current_treshold = tmp_current_treshold;
			sensors_param.cfgdes[3] = current_treshold;
			opt_saving = 1;
			SAVEOPT
			os_delay_us(500);
			system_soft_wdt_feed();
			opt_saving = 0;
		}
	}

	control_current_delay = (sensors_param.cfgdes[4] == 0) ? CONTROL_CURRENT_DELAY : sensors_param.cfgdes[4];
	control_load_on_delay = (sensors_param.cfgdes[5] == 0) ? CONTROL_LOAD_ON_DELAY : sensors_param.cfgdes[5]*1000;

	return reinit;
}

void ICACHE_FLASH_ATTR fill_devices(){
	uint8_t i;
	for (i = 0; i < DEVICES_COUNT; i++){
		uint32_t val = sensors_param.cfgdes[START_DEVICES_OPT_IDX + i];
		devices[i].gpio = OPT_GET_GPIO(val);
		devices[i].ip = OPT_GET_GPIO_IP(val);
		devices[i].priority = OPT_GET_GPIO_PRIORITY(val);
		devices[i].idx = i;
		devices[i].off = 0;
		devices[i].need_on = OPT_GET_GPIO_NEED_ON(val);
		devices[i].t_overload = 0;
	}
	sort_devices();
}

void ICACHE_FLASH_ATTR update_devices(){
	uint8_t i;
	for (i = 0; i < DEVICES_COUNT; i++){
		uint32_t val = sensors_param.cfgdes[START_DEVICES_OPT_IDX + i];
		devices[i].gpio = OPT_GET_GPIO(val);
		devices[i].ip = OPT_GET_GPIO_IP(val);
		devices[i].priority = OPT_GET_GPIO_PRIORITY(val);
		devices[i].idx = i;
		devices[i].need_on = OPT_GET_GPIO_NEED_ON(val);
	}
	sort_devices();
}


void ICACHE_FLASH_ATTR sort_devices()
{
	uint8_t i, j = 0;
	for ( i = 0; i < DEVICES_COUNT; i++) {
		for ( j = i + 1; j < DEVICES_COUNT; j++) {
			if (devices[i].priority < devices[j].priority) {
				struct Device dev;
				dev = devices[i];
				devices[i] = devices[j];
				devices[j] = dev;
			}
		}
	}
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
	fill_devices();

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

	if(timersrc%5==0){
		update_devices();
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

	os_timer_disarm(&control_current_timer);
	os_timer_setfn(&control_current_timer, (os_timer_func_t *)control_current_cb, NULL);
	os_timer_arm(&control_current_timer, control_current_delay, 0); // будет рестартовать сам себя


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
	os_sprintf(payload,"%d.%d", (uint16_t)current, 		(uint16_t)(current*100) % 100);
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
	//os_sprintf(payload,"%d", (int)energy);
	os_sprintf(payload,"%d.%d", (uint32_t)energy, 		(uint32_t)(energy*100) % 100);
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

void control_current_cb(){
	static uint32_t overload_ts = 0;
	uint8_t tmp_overload = 0;
	
	tmp_overload = current >= ((float)current_treshold / 10.0f);
	
	if ( tmp_overload != is_overload) {
		is_overload = tmp_overload;
		overload_ts = millis();  // запомнили время первого изменения статуса
	}	

	if ( is_overload ) {
		// перегрузка
		if (millis() - overload_ts > CONTROL_OVERLOAD_TIME) {
			// если превышение длилось более X мсек (CONTROL_LOAD_DELAY), выкл. девайс
			turn_off_next_device();
			overload_ts = millis();  // фиксируем время, чтобы потом начать новую проверку на длительность перегрузки
		} else {
			// перегрузка и прошло еще меньше чем Х мсек,
			// ничего не делаем
		}
	} else {
		// нет перегрузки
		if ( (millis() - overload_ts > control_load_on_delay )) {
			// прошло более Х сек после завершения перегрузки, включить устройства
			turn_devices_on();
			overload_ts = millis();
		}
	}

	os_timer_disarm(&control_current_timer);
	os_timer_setfn(&control_current_timer, (os_timer_func_t *)control_current_cb, NULL);
	os_timer_arm(&control_current_timer, control_current_delay, 0); // будет рестартовать сам себя


	
}


// Управлять можно gpio командой IP_ADRES/gpio?st=1&pin=12 - этим Вы установите на 12 GPIO логическую единицу.
static void ICACHE_FLASH_ATTR tcpclient_recon_cb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR  tcpclient_connect_cb(void *arg) {
	struct espconn *pespconn = (struct espconn *)arg;	
	espconn_regist_sentcb(pespconn, tcpclient_sent_cb);
	espconn_regist_disconcb(pespconn, tcpclient_discon_cb);
	
	char payload[512];

	struct Gp *gp = (struct Gp *)pespconn->reverse;
	os_sprintf(payload, "GET /gpio?pin=%d&st=%d", gp->pin, gp->state);
	os_sprintf(payload + os_strlen(payload), " HTTP/1.1\r\nHost: testdomen\r\nConnection: keep-alive\r\nAccept: */*\r\n\r\n");
	espconn_sent(pespconn, payload, strlen(payload));	
	os_free(gp);
}

static void ICACHE_FLASH_ATTR  control_remote_device(uint8_t ip4, uint8_t gpio, uint8_t state) {

	struct espconn *pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
	if (pCon == NULL) return;	

	pCon->type = ESPCONN_TCP;
	pCon->state = ESPCONN_NONE;
	pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	pCon->proto.tcp->local_port = espconn_port();
	pCon->proto.tcp->remote_port = 80; // порт
	char adrr[16];             //  ип
	os_sprintf(adrr, "192.168.2.%d", ip4); //запись в буфер  значений ип
	uint32_t ip = ipaddr_addr(adrr);        // сервер
	os_memcpy(pCon->proto.tcp->remote_ip, &ip, 4);

    struct Gp *gp = (struct Gp *)os_zalloc(sizeof(struct Gp));
    gp->pin = gpio;
    gp->state = state;
	pCon->reverse = gp;

	espconn_regist_connectcb(pCon, tcpclient_connect_cb); // функция отправки GET запроса
	espconn_regist_reconcb(pCon, tcpclient_recon_cb);
	espconn_connect(pCon);
}

void turn_off_next_device(){
	// выклюаем первое включенное устройство с наименьшим приоритетом
	uint8_t i = 0;
	for (i = 0; i < DEVICES_COUNT; i++) {
		if ( devices[i].gpio < 255 && 
		     devices[i].priority > 0 && 
			 devices[i].off == 0)  // не выключенное   // нужно как то определять статус - получать с устройства valdes (по mqtt) или самому запрашивать (get)
		{
			devices[i].off = 1;
			devices[i].t_overload = millis();  // фиксируем время выключения

			// выключить GPIO - devices_gpio[i]
			//GPIO_ALL(devices[i].gpio,0);  // используем только совместно с опцией VGPIO

			// отправляем get запрос на выключение
			control_remote_device(devices[i].ip, devices[i].gpio, 0);

			break;  // отключаем GPIO и вываливаемся из цикла, чтобы в следующей итерации выключить следующее gpio
		}
		
	}	
}

void turn_devices_on(){
	uint8_t i = 0;
	for ( i = DEVICES_COUNT; i > 0; i--) {
		if ( devices[i-1].gpio < 255 && 
			 devices[i-1].priority > 0 && 
			 devices[i-1].off == 1 &&
			 (millis() - devices[i-1].t_overload) > control_load_on_delay 
			) 
		{
			devices[i-1].off = 0; // сбросить флаг выключения
			devices[i-1].t_overload = 0;
			// выключить GPIO - devices_gpio[i]
			if (devices[i-1].need_on == 1) // включать обратно, только если это требуется
				//GPIO_ALL(devices[i-1].gpio,1); // если выключали gpio, то не факт, что его надо включать обратно
				control_remote_device(devices[i-1].ip, devices[i-1].gpio, 1);
				break; // ?????????
		}
		
	}
}

void webfunc(char *pbuf) {

	if ( delayed_counter > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных счетчика осталось %d секунд", delayed_counter);
	}
	
	if ( opt_saving ) os_sprintf(HTTPBUFF,"<p style='color: red;'><small><b>Идет сохранение настроек!</b></small></p>"); 
	//os_sprintf(HTTPBUFF,"<p><small><b>valdes[0]:</b> %d</small></p>", valdes[0]); 
	//os_sprintf(HTTPBUFF,"<p><small><b>sensros_param[3]:</b> %d</small></p>", sensors_param.cfgdes[3]); 
	//os_sprintf(HTTPBUFF,"<p><small><b>current tr:</b> %d</small></p>", current_treshold); 
	//os_sprintf(HTTPBUFF,"<p><small><b>Задержка определения перегрузки:</b> %d</small></p>", control_current_delay); 
	//os_sprintf(HTTPBUFF,"<p><small><b>Задержка после пропадания перегрузки:</b> %d</small></p>", control_load_on_delay); 
	
	os_sprintf(HTTPBUFF, "<table width='100%%' cellpadding='2' cellspacing='2' cols='2'>"
							"<tr>"
							"<td align='left'><b>Отсечка по току:</b> %d.%d A</td>"
							"<td align='right'><b>Перегрузка: </b><strong><span style='color: %s;'>%s</span></strong></td>"
							"</tr>"
						  "</table>", 
						  (uint16_t)current_treshold / 10, 		
						  (uint16_t)current_treshold % 10,		
						  is_overload ? "red" : "green", 
						  is_overload ? "ДА" : "НЕТ"
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

	
	os_sprintf(HTTPBUFF,"<br><small><table><tr><th>Устройство</th><th>Вкл</th><th>IP</th><th>GPIO</th><th>Приоритет</th><th>Включать</th><th>Code.Opt</th></tr>");
	uint8_t i;
	for ( i = 0; i < DEVICES_COUNT; i++) {
		uint8_t priority = devices[i].priority;
		uint8_t idx = devices[i].idx;
		uint8_t ip = devices[i].ip;
		uint8_t gpio = devices[i].gpio;
		uint8_t on = devices[i].need_on;
		uint8_t off = devices[i].off;
		os_sprintf(HTTPBUFF,"<tr %s><td><b>%s</b></td>"
								 "<td>%s</td>"
								 "<td>%d</td>"
								 "<td align='center'>%d</td>"
								 "<td align='center'>%d</td>"
								 "<td align='center'>%d</td>"
								 "<td align='center'>%d</td></tr>", 
							off ? "style='color: red;'" : "",
							devices_title[idx], 
							off ? "Off" : "On", 
							ip, gpio, priority, on, idx + START_DEVICES_OPT_IDX);	
	}
	os_sprintf(HTTPBUFF,"</table></small>");
	os_sprintf(HTTPBUFF,"<small><details><summary>Приоритеты (0 - выкл, 1 - max, 15 - min)</summary>");
	os_sprintf(HTTPBUFF,"<p>Если надо обратно включать GPIO, указать его + 100000000");
	os_sprintf(HTTPBUFF,"<br>GPIO: 0 - 255, IP: 2 - 255, priority: <gpio> + N00000</p></details></small>");

	os_sprintf(HTTPBUFF,"<p><small><b>Версия прошивки:</b> %s</small></p>", FW_VER); 
	os_sprintf(HTTPBUFF,"<p style='color: red;'><small><b>Прежде чем менять данные, перезагрузи модуль!!!</b></small></p>"); 

 // когда перегрузка закончилась, показать таймер отчета до включения устройства
}