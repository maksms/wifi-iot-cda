#include "../moduls/uart_register.h"
#include "../moduls/uart.h"
#include "../moduls/uart.c" // ??????

//#define DEBUG

#define FW_VER "1.6"

#define DELAYED_START					60   //sec
#define UART_READ_TIMEOUT					1000  // влияет на результаты чтения из юсарт
#define SONAR_READ_DELAY 	1000

#define millis() (uint32_t) (micros() / 1000ULL)

#define COMMAND				0x55
#define RESPONSE_SIZE 		4

#ifdef DEBUG
	static char logstr[100];
#endif

#if mqtte || mqttjsone
	#define MQTT_SEND_INTERVAL 10 // sec
	#define MQTT_TOPIC_DISTANCE	"distance"
	#define MQTT_TOPIC_FAIL	"fail"
	#define MQTT_PAYLOAD_BUF 20
	MQTT_Client* mqtt_client;    //for non os sdk
	//char payload[MQTT_PAYLOAD_BUF];
	uint32_t mqtt_send_interval_sec = MQTT_SEND_INTERVAL;

	static volatile os_timer_t mqtt_send_timer;	
	void mqtt_send_cb();
#endif


uint8_t delayed_counter = DELAYED_START;

	
uint8_t uart_ready = 1;
uint8_t fail = 1;
uint8_t swap_uart = 0;

uint32_t distance = 0;
uint32_t prev_distance = 0;
uint8_t sonar_enabled = 0;
uint16_t sonar_read_delay = SONAR_READ_DELAY;
uint8_t mm_cm = 0; // 0 - mm, 1 - cm

uint32_t distance_min = 200;
uint32_t distance_max = 4000;
uint16_t correction = 50;

static volatile os_timer_t read_sonar_timer;
static volatile os_timer_t system_start_timer;

void ICACHE_FLASH_ATTR system_start_cb( );
void ICACHE_FLASH_ATTR read_sonar_cb();	

void ICACHE_FLASH_ATTR send_buffer(uint8_t *buffer, uint8_t len);
void read_buffer();



uint32_t remap(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// UART1 TX GPIO2 Enable output debug
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

void debug_print_distance() {
	os_bzero(logstr, 100);
	os_sprintf(logstr, "distance: %d \t fail: %d \t freemem: %d\n", distance, fail, system_get_free_heap_size());
	uart1_tx_buffer(logstr, os_strlen(logstr));	
}
#endif




void ICACHE_FLASH_ATTR send_buffer(uint8_t *buffer, uint8_t len){
	uart0_tx_buffer(buffer, len);
}

void validate_distance(uint32_t dist)
{
	static uint32_t prev = 0;
	static uint8_t count = 0;
	static uint32_t sum = 0;
	static uint32_t min = 0;
	static uint32_t max = 0;
	

	
	if (correction == 0 ) return;
	
					
	if ( count == 0 ) 
	{
		min = dist;
		max = dist;
		
		#ifdef DEBUG
			os_bzero(logstr, 100);
			os_sprintf(logstr, "start measurments \n");
			uart1_tx_buffer(logstr, os_strlen(logstr));	
		#endif	
	}				
					
	if ( count < 10 ) {
		// накапливаем значения, суммируем

		#ifdef DEBUG
		
			os_bzero(logstr, 100);
			os_sprintf(logstr, "%02d. distance: %d\n", count+1, distance);
			uart1_tx_buffer(logstr, os_strlen(logstr));		
		#endif	

			
		sum += dist;
		max = ( dist > max ) ? dist : max;
		min = ( dist < min ) ? dist : min;
		count++;
	} else {
		// count = 10 - накопили 10 показаний
		uint32_t avg = sum / count; 
		fail = 0;
		//if ( max - min > correction) 
		if ( max - avg > correction || avg - min > correction) 
		{
			fail = 1;
		}			

		
					#ifdef DEBUG
						 
							os_bzero(logstr, 100);
							os_sprintf(logstr, "end measurments: \n");
							uart1_tx_buffer(logstr, os_strlen(logstr));	
							

						os_bzero(logstr, 100);
						os_sprintf(logstr, "min: %d \t max: %d \t avg: %d \t fail: %d \t \n\n\n", min, max, avg, fail);
						uart1_tx_buffer(logstr, os_strlen(logstr));	
					
					
					#endif	

							max = 0;
		min = 1000;
		count = 0;
		sum = 0;
	}
	

					
	prev = dist;
}

void read_buffer(){	
	static char rx_buf[125];
	static uint8_t len = 0;

	uint32_t ts = micros();
	
	WRITE_PERI_REG(UART_INT_CLR(UART0),UART_RXFIFO_FULL_INT_CLR);
	WRITE_PERI_REG(UART_INT_CLR(UART0),UART_RXFIFO_TOUT_INT_CLR);
	while ( READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S) 
			&& ( micros() - ts < UART_READ_TIMEOUT*1000)) 
	{
		WRITE_PERI_REG(0X60000914, 0x73); //WTD
		uint8_t read = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		rx_buf[len] = read; // buffer[i++] = read;
		len++;
		ts = micros();
		
		
		#ifdef DEBUG1
			os_bzero(logstr, 100);
			os_sprintf(logstr, "%02X ", read);
			uart1_tx_buffer(logstr, os_strlen(logstr));
		#endif
	
		system_soft_wdt_feed();
		if ( RESPONSE_SIZE == len) {
			// что то прочитали
			// validate		
			if ( rx_buf[0] == 0xFF) 
			{
				// check crc
				uint16_t crc = (rx_buf[0] + rx_buf[1] + rx_buf[2]) & 0xFF;
				if ( crc == rx_buf[3] ) {
					fail = 0;
					distance = ( (rx_buf[1] << 8 ) + rx_buf[2]);
					valdes[0] = distance;
					validate_distance(distance);
				}
			}				
			len = 0;
			uart_ready = 1;			
			break;			
		}
	}				
}

void get_config_values() {   
	sonar_enabled = (sensors_param.cfgdes[0]  > 0) ? 1 : 0;  // читать данные sonar
	sonar_read_delay = (sensors_param.cfgdes[0] < 100) ? SONAR_READ_DELAY : sensors_param.cfgdes[0];	

#if mqtte || mqttjsone	
	mqtt_send_interval_sec = (sensors_param.cfgdes[1] == 0) ? sensors_param.mqttts : sensors_param.cfgdes[1];		
#endif	
	mm_cm = (sensors_param.cfgdes[2] > 0) ? 1 : 0;  
	swap_uart = (sensors_param.cfgdes[3] > 0) ? 1 : 0;  
	
	distance_min = sensors_param.cfgdes[4];  
	distance_max = sensors_param.cfgdes[5];  
	correction = sensors_param.cfgdes[6];  
}

void ICACHE_FLASH_ATTR startfunc(){
	
	get_config_values();
	
	// выполняется один раз при старте модуля.
	uart_init(BIT_RATE_9600);	  
	
	if ( swap_uart ) {
		system_uart_swap();
	}
	
	ETS_UART_INTR_ATTACH(read_buffer, NULL);

	#ifdef DEBUG
		uart_config(UART1);
		uart_div_modify(UART1,	UART_CLK_FREQ	/BIT_RATE_115200);

		os_install_putc1((void *)uart1_tx_buffer);
	#endif

	// запуск таймера, чтобы мой основной код начал работать через Х секунд после старта, чтобы успеть запустить прошивку
	os_timer_disarm(&system_start_timer);
	os_timer_setfn(&system_start_timer, (os_timer_func_t *)system_start_cb, NULL);
	os_timer_arm(&system_start_timer, DELAYED_START * 1000, 0);
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	
	get_config_values();
	
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
		
	if ( delayed_counter > 0 ) { 
		delayed_counter--;	
	}	
}

void ICACHE_FLASH_ATTR system_start_cb( ){
	os_timer_disarm(&read_sonar_timer);
	os_timer_setfn(&read_sonar_timer, (os_timer_func_t *)read_sonar_cb, NULL);
	os_timer_arm(&read_sonar_timer, 20, 0); // будет рестартовать сам себя
	
#if mqtte || mqttjsone	
	mqtt_client = (MQTT_Client*) &mqttClient;
	os_timer_disarm(&mqtt_send_timer);
	os_timer_setfn(&mqtt_send_timer, (os_timer_func_t *)mqtt_send_cb, NULL);
	os_timer_arm(&mqtt_send_timer, mqtt_send_interval_sec * 1000, 1);
#endif
	
}

void ICACHE_FLASH_ATTR read_sonar_cb(){
	if ( uart_ready ) {
		// send command into uart
		uart_ready = 0;
		distance = 0; // ???? обнулять?
		fail = 1;
		uint8_t cmd = COMMAND;
		send_buffer(&cmd, 1);
		os_delay_us(50);
	}
	
	os_timer_disarm(&read_sonar_timer);
	os_timer_setfn(&read_sonar_timer, (os_timer_func_t *)read_sonar_cb, NULL);
	os_timer_arm(&read_sonar_timer, SONAR_READ_DELAY, 0);	
	
}

#if mqtte || mqttjsone
void mqtt_send_cb() 
{
	if ( sensors_param.mqtten != 1 ) return;
	//char *payload = (char *)os_malloc(MQTT_PAYLOAD_BUF);
	char payload[MQTT_PAYLOAD_BUF];
	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	if ( mm_cm ) {
		os_sprintf(payload,"%d.%d", (uint16_t)distance/10, 		(uint16_t)(distance % 10));
	} else {
		os_sprintf(payload,"%d", distance);
	}
	system_soft_wdt_feed();
	MQTT_Publish(mqtt_client, MQTT_TOPIC_DISTANCE, payload, os_strlen(payload), 2, 0, 1);
	
	system_soft_wdt_feed();
	os_memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload,"%d", fail);
	MQTT_Publish(mqtt_client, MQTT_TOPIC_FAIL, payload, os_strlen(payload), 2, 0, 1);
	//free(payload);
}
#endif

void webfunc(char *pbuf) {
	system_soft_wdt_feed();
	if ( delayed_counter > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных счетчика осталось %d секунд<br>", delayed_counter);
	}

	if ( mm_cm ) {
		os_sprintf(HTTPBUFF,"<b>Расcтояние:</b> %d.%d см", 	(uint16_t)distance/10, 		(uint16_t)(distance % 10));
	} else {
		os_sprintf(HTTPBUFF,"<b>Расcтояние:</b> %d мм", 	distance);
	}
	
	uint32_t distance_map = 100 - remap(distance, 0, distance_max, 0, 100);
	os_sprintf(HTTPBUFF,"<br><b>Заполнение:</b> %d %%", 	 ( distance_map > 100 ) ? 100 : distance_map);
	
	if ( fail == 1 ) {
		os_sprintf(HTTPBUFF,"<br>Ошибка чтения данных или бак переполнен");
	}
	
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
}