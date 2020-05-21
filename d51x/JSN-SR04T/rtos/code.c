#include "driver/uart.h"

#define MQTTD

#define FW_VER "1.2"
	
#define DELAYED_START		60   //sec
#define UART_READ_TIMEOUT	1000  // влияет на результаты чтения из юсарт
#define SONAR_READ_DELAY 	1000

#define COMMAND				0x55
#define RESPONSE_SIZE 		4

#define BUF_SIZE (128)      


#define pauseTask(delay)  (vTaskDelay(delay / portTICK_PERIOD_MS))

#ifdef MQTTD
	#define MQTT_SEND_INTERVAL 10 // sec
	#define MQTT_TOPIC_DISTANCE	"distance"
	#define MQTT_PAYLOAD_BUF 20
	//MQTT_Client* mqtt_client;    //for non os sdk
	char payload[MQTT_PAYLOAD_BUF];
	uint32_t mqtt_send_interval_sec = MQTT_SEND_INTERVAL;

	static TimerHandle_t mqtt_send_timer;	
	void vMqttSendTimerCallback( TimerHandle_t xTimer );
#endif


uint16_t distance = 0;
uint8_t sonar_enabled = 0;
uint16_t sonar_read_delay = SONAR_READ_DELAY;
uint8_t mm_cm = 0; // 0 - mm, 1 - cm

uint8_t delayed_counter = DELAYED_START;

static TimerHandle_t system_start_timer;
//static volatile os_timer_t system_start_timer; 
void vSystemStartTimerCallback( TimerHandle_t xTimer );

// uart0
void send_buffer(const uint8_t *buffer, uint8_t len);
uint8_t read_buffer(uint8_t *buffer, uint8_t cnt);

void read_distance_task( void * pvParameters );

void sonat_send ();
uint16_t sonar_read();

static void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
	
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 100, NULL, 0);	
}

void get_config_values() {   
	sonar_enabled = (sensors_param.cfgdes[0]  > 0) ? 1 : 0;  // читать данные sonar
	sonar_read_delay = (sensors_param.cfgdes[0] < 100) ? SONAR_READ_DELAY : sensors_param.cfgdes[0];	

#ifdef MQTTD	
	mqtt_send_interval_sec = (sensors_param.cfgdes[1] == 0) ? sensors_param.mqttts : sensors_param.cfgdes[1];		
#endif	
	mm_cm = (sensors_param.cfgdes[2] > 0) ? 1 : 0;  
}

void startfunc(){
	// выполняется один раз при старте модуля.
	uart_init();	  
	get_config_values();
	 // запуск таймера, чтобы мой основной код начал работать через Х секунд после старта, чтобы успеть запустить прошивку
	system_start_timer = xTimerCreate("system start timer", pdMS_TO_TICKS( DELAYED_START * 1000 ), pdFALSE, 0, vSystemStartTimerCallback);
	BaseType_t b = xTimerStart( system_start_timer, 0);
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	get_config_values();

	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
	
	if ( delayed_counter > 0 ) { 
		delayed_counter--;	
	}
	pauseTask(1000);
}

void vSystemStartTimerCallback( TimerHandle_t xTimer ){	

#ifdef DEBUG
	userlog("\n%s\n", __func__);
#endif	
	xTaskCreate(read_distance_task, "read_distance_task", 2048, NULL, 5, NULL); 

#ifdef MQTTD
	mqtt_send_timer = xTimerCreate("mqtt send timer", pdMS_TO_TICKS( mqtt_send_interval_sec * 1000 ), pdTRUE, 0, vMqttSendTimerCallback);
	xTimerStart( mqtt_send_timer, 0);
#endif

}

void sonar_send () {
	uint8_t *bytes = malloc(1);
	bytes[0] = COMMAND;
	send_buffer(bytes, 1);
}

uint16_t sonar_read() {
	uint16_t res = 0;
	
	uint8_t *buf = (uint8_t *) malloc(RESPONSE_SIZE);
	uint8_t len = read_buffer(buf, RESPONSE_SIZE);

	// validate
	if ( len ==  RESPONSE_SIZE && buf[0] == 0xFF) 
	{
		// check crc
	    uint16_t crc = (buf[0] + buf[1] + buf[2]) & 0xFF;
      	if ( crc == buf[3] ) {
        	res = ( (buf[1] << 8 ) + buf[2]);
      	} 
	}

	free(buf);
	buf = NULL;
	return res;
}


void read_distance_task( void * pvParameters ){
	for(;;){
		if ( !sonar_enabled ) { 
			vTaskDelete(NULL);
			return;
		}

		sonar_send();
		distance = sonar_read();
		uart_flush(UART_NUM_0);
		pauseTask(sonar_read_delay);
	}
	vTaskDelete(NULL);
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
	if ( sensors_param.mqtten != 1 ) return;
	memset(payload, 0, MQTT_PAYLOAD_BUF);
	if ( mm_cm ) {
		os_sprintf(payload,"%d.%d", (int)distance, 		(int)(distance*10) % 10);
	} else {
		os_sprintf(payload,"%d", distance);
	}
	//new mqtt client
	MQTT_Publish(MQTT_TOPIC_DISTANCE, payload, os_strlen(payload), 2, 0, 1);
	// old mqtt client
	//MQTT_Publish(client, MQTT_TOPIC_DISTANCE, payload, os_strlen(payload), 2, 0, 1);
}	
#endif


void webfunc(char *pbuf) {

	if ( delayed_counter > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных счетчика осталось %d секунд", delayed_counter);
	}

	if ( mm_cm ) {
		os_sprintf(HTTPBUFF,"<br><b>Расcтояние:</b> %d.%d см", 	(uint16_t)distance/10, 		(uint16_t)(distance % 10));
	} else {
		os_sprintf(HTTPBUFF,"<br><b>Расcтояние:</b> %d мм", 	distance);
	}
	
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
}