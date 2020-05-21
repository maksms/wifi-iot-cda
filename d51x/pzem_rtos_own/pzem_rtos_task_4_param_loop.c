/*
	на основе https://github.com/olehs/PZEM004T
	
	таск запущен без таймера, в таске крутятся операции чтения параметров в цикле
	через каждые 3 чтения тока читается либо напряжение, либо мощность
	расход считается каждый 120-ый раз
	2 чтения - примерно 1 сек
*/

#include "driver/uart.h"

#define MQTTD

//#define DEBUG
#define FW_VER_NUM "3.7.4"

#ifdef DEBUG
#define FW_VER FW_VER_NUM  " debug"
#else
#define FW_VER FW_VER_NUM
#endif	

#define DELAYED_START					60   //sec

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

#define BUF_SIZE (1024)      

#define millis() (unsigned long) (esp_timer_get_time() / 1000ULL) 
#define pauseTask(delay)  (vTaskDelay(delay / portTICK_PERIOD_MS))

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
	char payload[MQTT_PAYLOAD_BUF];
	uint32_t mqtt_send_interval_sec = MQTT_SEND_INTERVAL;

	static TimerHandle_t mqtt_send_timer;	
	void vMqttSendTimerCallback( TimerHandle_t xTimer );
#endif


float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;

uint8_t delayed_counter = DELAYED_START;

static TimerHandle_t system_start_timer;
void vSystemStartTimerCallback( TimerHandle_t xTimer );

// uart0
void send_buffer(const uint8_t *buffer, uint8_t len);
uint8_t read_buffer(uint8_t *buffer, uint8_t cnt);

uint8_t pzem_enabled = 0;
void read_electro_task( void * pvParameters );
void read_voltage();
void read_current();
void read_power();
void read_energy();

uint8_t pzem_crc(uint8_t *data, uint8_t sz);
void pzem_send (uint8_t *addr, uint8_t cmd);
uint8_t pzem_read(uint8_t resp, uint8_t *data);
float pzem_voltage(uint8_t *addr);
float pzem_current(uint8_t *addr);
float pzem_power(uint8_t *addr);
float pzem_energy(uint8_t *addr);

#ifdef DEBUG
void userlog(const char *fmt, ...) {
	char *str = (char *) malloc(100);
	memset(str, 100, 0);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(str, 100, fmt, args);
    va_end(args);
	uart_write_bytes(UART_NUM_1, str, len);
	free(str);
	str = NULL;
}
#endif

static void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
	
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 100, NULL);	
    
#ifdef DEBUG	
	uart_param_config(UART_NUM_1, &uart_config);
	uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 100, NULL);	
	os_install_putc1(userlog);
#endif	
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

void startfunc(){
	// выполняется один раз при старте модуля.
	 uart_init();	  
#ifdef DEBUG	 
	 userlog("\nFiwrmware: %s \n", FW_VER);
#endif 
	get_config_values(0);
	 // запуск таймера, чтобы мой основной код начал работать через Х секунд после старта, чтобы успеть запустить прошивку
	system_start_timer = xTimerCreate("system start timer", pdMS_TO_TICKS( DELAYED_START * 1000 ), pdFALSE, 0, vSystemStartTimerCallback);

#ifdef DEBUG	
	if ( system_start_timer == NULL ) {
		userlog("FAIL: Timer system_start_timer was not created \n");
	} else {
		userlog("PASS: Timer system_start_timer was created \n");
	}
#endif

BaseType_t b = xTimerStart( system_start_timer, 0);

#ifdef DEBUG	
	if ( b != pdPASS ) {
		userlog("FAIL: the timer system_start_timer couldn't start\n");
	} else {
		userlog("PASS: the timer system_start_timer was started\n");
	}
#endif	
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	get_config_values(1);

	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
	
	if ( delayed_counter > 0 ) { 
		delayed_counter--;
#ifdef DEBUG		
		userlog("countdown: %d\n", delayed_counter);
#endif		
	}

	pauseTask(1000);
}

void vSystemStartTimerCallback( TimerHandle_t xTimer ){
#ifdef DEBUG
	userlog("\n%s\n", __func__);
#endif	
	xTaskCreate(read_electro_task, "read_electro_task", 2048, NULL, 5, NULL); 

#ifdef MQTTD
	mqtt_send_timer = xTimerCreate("mqtt send timer", pdMS_TO_TICKS( mqtt_send_interval_sec * 1000 ), pdTRUE, 0, vMqttSendTimerCallback);
	xTimerStart( mqtt_send_timer, 0);
#endif

}

void pzem_send (uint8_t *addr, uint8_t cmd) {
	PZEMCommand pzem;
	pzem.command = cmd;
	for ( uint8_t i = 0; i < sizeof(pzem.addr); i++) pzem.addr[i] = addr[i];
	pzem.data = 0;
	uint8_t *bytes = (uint8_t*)&pzem;
	pzem.crc = pzem_crc(bytes, sizeof(pzem) - 1);
	send_buffer(bytes, sizeof(pzem));
}

uint8_t pzem_read(uint8_t resp, uint8_t *data) {
	uint8_t res = 0;
	uint8_t *buf = (uint8_t *) malloc(RESPONSE_SIZE);
	uint8_t len = read_buffer(buf, RESPONSE_SIZE);
	if ( len ==  RESPONSE_SIZE && buf[0] == resp && buf[6] == pzem_crc(buf, len-1)) {
		for ( uint8_t i = 0; i < RESPONSE_DATA_SIZE; i++) data[i] = buf[1 + i];
		res = 1;
	}	
	free(buf);
	buf = NULL;
	return res;
}

float pzem_voltage(uint8_t *addr) {
	uint8_t data[RESPONSE_DATA_SIZE];
	pzem_send(addr, CMD_VOLTAGE);
	uint8_t res = pzem_read( RESP_VOLTAGE, &data);
	pauseTask(10);
	float value = (res > 0 ) ? (data[0] << 8) + data[1] + ( data[2] / 10.0) : 0;
	return value;
}

float pzem_current(uint8_t *addr) {
	uint8_t data[RESPONSE_DATA_SIZE];
	pzem_send(addr, CMD_CURRENT);
	uint8_t res = pzem_read( RESP_CURRENT, &data);
	pauseTask(10);
	float value = (res > 0 ) ? (data[0] << 8) + data[1] + (data[2] / 100.0) : 0;
	return value;
}

float pzem_power(uint8_t *addr) {
	uint8_t data[RESPONSE_DATA_SIZE];
	pzem_send(addr, CMD_POWER);
	uint8_t res = pzem_read( RESP_POWER, &data);
	pauseTask(10);
	float value = (res > 0 ) ? (data[0] << 8) + data[1] : 0;
	return value;
}

float pzem_energy(uint8_t *addr) {
	uint8_t data[RESPONSE_DATA_SIZE];
	pzem_send(addr, CMD_ENERGY);
	uint8_t res = pzem_read( RESP_ENERGY, &data);
	pauseTask(10);
	float value = (res > 0 ) ? ((uint32_t)data[0] << 16) + ((uint16_t)data[1] << 8) + data[2] : 0;
	return value;
}

void read_voltage(){
	if ( !pzem_enabled ) return;	
	float v = pzem_voltage(pzem_addr);

	#ifdef CUT_OFF_INCORRECT_VALUE
		voltage = ( v == 0 || v > 400) ? voltage : v;
	#else
		voltage = ( v == 0 ) ? voltage : v;
	#endif

#ifdef DEBUG
	if ( voltage >= 0) userlog("%d \t\t voltage: %d.%d V\n", millis(), (int)voltage,  ( (int) (voltage*10) % 10));		
#endif	
	pauseTask(PZEM_PAUSE_TASK );
}

void read_current(){
	if ( !pzem_enabled ) return;
	float v = pzem_current(pzem_addr);

	#ifdef CUT_OFF_INCORRECT_VALUE
		current = ( v == 0 || v > 100) ? current : v;
	#else
		current = ( v == 0) ? current : v;		
	#endif

#ifdef DEBUG	
	if ( current >= 0) userlog("%d \t\t current: %d.%d A\n", millis(), (int)current,  ( (int) (current*100) % 100));		
#endif	
	pauseTask( PZEM_PAUSE_TASK );
}

void read_power(){
	if ( !pzem_enabled ) return;
	float v = pzem_power(pzem_addr);

	#ifdef CUT_OFF_INCORRECT_VALUE
		power = ( v == 0 || v > 25000) ? power : v;	
	#else
		power = ( v == 0) ? power : v;
	#endif

#ifdef DEBUG		
	if ( power >= 0) userlog("%d \t\t power: %d.%d\ Wh\n", millis(), (int)power,  ( (int) (power*100) % 100));		
#endif		
	pauseTask( PZEM_PAUSE_TASK );
}

void read_energy(){
	if ( !pzem_enabled ) return;
	float v = pzem_energy(pzem_addr);	
	energy = ( v == 0) ? energy : v;
#ifdef DEBUG		
	if ( energy >= 0) userlog("%d \t\t energy: %d.%d Wt*h\n", millis(), (int)energy,  ( (int) (energy*100) % 100));		
#endif
	pauseTask( PZEM_PAUSE_TASK );
}

void read_electro_task( void * pvParameters ){
	for(;;){
		if ( !pzem_enabled ) { 
			vTaskDelete(NULL);
			return;
		}
		//userlog("%s\n", __func__);
		for ( uint8_t i=1;i<=120;i++) {
			int m = i % 4;
			if ( i == 120 ) {
				read_energy();
				continue;
			}
			if ( m != 0) {
				read_current();
			} else {
				int d = i / 4;
				int mm = d % 2;
				
				if ( mm == 1 ) {
					read_voltage();
				} else {
					read_power();
				}				
			}			
		}
		pauseTask(50);
	}
	vTaskDelete(NULL);
}

uint8_t pzem_crc(uint8_t *data, uint8_t sz) {
    uint16_t crc = 0;
    for(uint8_t i=0; i<sz; i++)
        crc += *data++;
    return (uint8_t)(crc & 0xFF);	
}

void send_buffer(const uint8_t *buffer, uint8_t len){
	uart_write_bytes(UART_NUM_0, (const char *) buffer, len);
}

uint8_t read_buffer(uint8_t *buffer, uint8_t cnt){
	int8_t result = 0;
	result = uart_read_bytes(UART_NUM_0, buffer, cnt, UART_READ_TIMEOUT / portTICK_RATE_MS );
	if 	( result < 0 ) { result = 0;	}
	return result;
}


#ifdef MQTTD
void vMqttSendTimerCallback( TimerHandle_t xTimer ) {
	//userlog("%s\n", __func__);
	if ( sensors_param.mqtten != 1 ) return;

//userlog("enabled send mqtt\n");
	memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d.%d", (int)voltage, 		(int)(voltage*10) % 10);
	MQTT_Publish(client, VOLTAGE_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	pauseTask(20);

	memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d.%d", (int)current, 		(int)(current*100) % 100);
	MQTT_Publish(client, CURRENT_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	pauseTask(20);

	memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", (int)power);
	MQTT_Publish(client, POWER_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	pauseTask(20);

	memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", (int)energy);
	MQTT_Publish(client, ENERGY_MQTT_TOPIC_PARAM, payload, os_strlen(payload), 2, 0, 1);
	pauseTask(20);
}	
#endif


void show_countdown(uint8_t cnt) {
	if ( cnt > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных счетчика осталось %d секунд", cnt);
	}
}

void webfunc(char *pbuf) {
	show_countdown( delayed_counter );
	os_sprintf(HTTPBUFF,"<br><b>Напряжение:</b> %d.%d В", 	(int)voltage, 		(int)(voltage*10) % 10);
	os_sprintf(HTTPBUFF,"<br><b>Сила тока:</b> %d.%d А", 	(int)current, 		(int)(current*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Мощность:</b> %d Вт", 	(int)power);
	os_sprintf(HTTPBUFF,"<br><b>Расход:</b> %d.%d Вт*ч", 	(int)energy, 		(int)(energy*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
}