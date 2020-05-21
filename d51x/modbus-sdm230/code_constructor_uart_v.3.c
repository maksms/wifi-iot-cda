#include "driver/uart.h"

#define FW_VER "3.1"

#define LOG_UART_NUM 					UART_NUM_0  // UART_NUM_1 ????
#define DELAYED_START					40   //sec

#define MAX_MILLIS_TO_WAIT             	500
#define FRAMESIZE                    	9 
#define SDM_REPLY_BYTE_COUNT          	0x04 
#define SDM_ADDR                      	0x01
#define SDM_RGST_INPUT                	0x04
#define SDM_RGST_HOLD               	0x03
#define SDM_REPLY_BYTE_COUNT            0x04 

#define SDM_NO_COMMAND					0xFFFF
#define SDM_VOLTAGE                     0x0000                              //V
#define SDM_CURRENT                     0x0006                              //A
#define SDM_POWER                       0x000C                              //W
#define SDM_TOTAL_ACTIVE_ENERGY         0x0156                              //kwh
#define SDM_CURRENT_RESETTABLE_TOTAL_ACTIVE_ENERGY   0x0180                  //kWh     ONLY SDM230
 
 
#define BUF_SIZE (1024)           //?????? 
#define RD_BUF_SIZE (BUF_SIZE)    //?????? 

#define millis() (unsigned long) (esp_timer_get_time() / 1000ULL) 
#define pauseTask(delay)  (vTaskDelay(delay / portTICK_PERIOD_MS))

							
uint8_t delayed_counter = DELAYED_START;

typedef enum  {
	NO_COMMAND,
	VOLTAGE,
	CURRENT,
	POWER,
	TOTAL_ACTIVE_ENERGY,
	CURRENT_RESETTABLE_TOTAL_ACTIVE_ENERGY
} SDMCommand; 

typedef enum {
	SDM_ERROR,	
	SDM_IDLE,	// можно отправлять команду
	SDM_SENDING,  		// данные отправлены и ждем ответ
	SDM_READY			// данные прочитаны
} SDMState;

static SDMCommand eSDMCommand = NO_COMMAND;
static SDMState eSDMState = SDM_IDLE;

uint32_t voltage = 0;
uint32_t current = 0;
uint32_t power = 0;
uint32_t consumption_total = 0;
uint32_t consumption_total_resettable = 0;


uint32_t cmd_time = 0;
uint32_t prev_time = 0;

static QueueHandle_t uart0_queue;
static QueueHandle_t sdm_data_queue;

static TimerHandle_t system_start_timer;


uint16_t calculateCRC(uint8_t *array, uint8_t num);
void vSystemStartTimerCallback( TimerHandle_t xTimer );
static void uart_event_task(void *pvParameters);   // получение данных в uart по прерыванию
static void sdm_parse_task(void *pvParameters);   // читает очередь и парсит данные
static void sdm_command_task(void *pvParameters);   // отправить команду в sdm
   
void sdm_parse_data(uint8_t *buf);   // парсим sdm230 данные
void sdm_cmd_send(uint8_t cmd);

static void uart_init() {
	    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
	
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 100, &uart0_queue);	
	
	
}


void startfunc(){
	 uart_init();	  
	 
	 // запуск таймера, чтобы мой основной код начал работать через Х секунд после старта, чтобы успеть запустить прошивку
	 system_start_timer = xTimerCreate("system start timer", pdMS_TO_TICKS( DELAYED_START * 1000 ), pdFALSE, 0, vSystemStartTimerCallback);
	 xTimerStart( system_start_timer, 0);
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
	
	if ( delayed_counter > 0 ) { 
		delayed_counter--; 
	} else {
		xTimerStop( system_start_timer, 0 );
		system_start_timer = NULL;
	}
	
	//ESP_LOGI(__func__, "timersrc : %d", timersrc);
	//vTaskDelay(1000 / portTICK_PERIOD_MS);
	pauseTask(1000);
}

void vSystemStartTimerCallback( TimerHandle_t xTimer ){
	// запуск основных действий по работе с SDM230
	 
	// Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
	
	// create queue for sdm data received from uart
	sdm_data_queue = xQueueCreate(10, FRAMESIZE);
	
	xTaskCreate(sdm_parse_task, "sdm data parse", 2048, NULL, 5, NULL);
	xTaskCreate(sdm_command_task, "send command to sdm", 2048, NULL, 5, NULL);

}

static void uart_event_task(void *pvParameters){
	uart_event_t event;   
	size_t buffered_size; 
	uint8_t *dtmp = (uint8_t *) malloc(RD_BUF_SIZE); 
	for (;;) {
		// Waiting for UART event. 
		if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) { 
			bzero(dtmp, RD_BUF_SIZE);
			
			switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(UART_NUM_0, dtmp, event.size, portMAX_DELAY);
                    // send buffer to Queue to parse SDM data
					cmd_time = millis() - prev_time;
					if ( event.size == FRAMESIZE ) {
						xQueueSend( sdm_data_queue, ( void * ) dtmp, ( TickType_t ) 0 );
						eSDMState = SDM_IDLE;
					}
                    break;
                case UART_FIFO_OVF:
                    //ESP_LOGI(TAG, "hw fifo overflow");
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(uart0_queue);
					eSDMState = SDM_ERROR;
                    break;

                case UART_BUFFER_FULL:
                    //ESP_LOGI(TAG, "ring buffer full");
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(uart0_queue);
					eSDMState = SDM_ERROR;
                    break;
                case UART_PARITY_ERR:
                    //ESP_LOGI(TAG, "uart parity error");
					eSDMState = SDM_ERROR;
                    break;
                case UART_FRAME_ERR:
                    //ESP_LOGI(TAG, "uart frame error");
					eSDMState = SDM_ERROR;
                    break;				
                // Others
                default:
                    //ESP_LOGI(TAG, "uart event type: %d", event.type);
					eSDMState = SDM_ERROR;
                    break;				
			}
		}
		taskYIELD();
	}
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);	
}
   
static void sdm_parse_task(void *pvParameters) {
	uint8_t *dtmp = (uint8_t *) malloc(FRAMESIZE);
	for (;;) {
		bzero(dtmp, FRAMESIZE);
		xQueueReceive( sdm_data_queue, (void *)dtmp, (portTickType)portMAX_DELAY);		
		sdm_parse_data(&dtmp);
		taskYIELD();
	}
	free(dtmp);
    dtmp = NULL;
	vTaskDelete(NULL);	
}	
   
void sdm_parse_data(uint8_t *buf) {
	float result = 0;
	if ((calculateCRC(buf, FRAMESIZE - 2)) != ((buf[8] << 8) | buf[7])) return;
	
	((uint8_t*)&result)[2]= buf[4];
	((uint8_t*)&result)[3]= buf[3];
	((uint8_t*)&result)[1]= buf[5];
	((uint8_t*)&result)[0]= buf[6];
		  	  
	switch (eSDMCommand) {
		case VOLTAGE:	voltage = result;
			break;
		case CURRENT:	current = result;
			break;	
		case POWER:		power = result;
			break;
		case TOTAL_ACTIVE_ENERGY:	consumption_total = result;
			break;
		case CURRENT_RESETTABLE_TOTAL_ACTIVE_ENERGY:	consumption_total_resettable = result;
			break;
		default:
			break;
	}
}

static void sdm_command_task(void *pvParameters){
	for (;;) {
		eSDMCommand = VOLTAGE;
		eSDMState = SDM_IDLE;
		prev_time = millis();
		sdm_cmd_send(SDM_VOLTAGE);
		
		
		
		pauseTask(1000);
	}
	vTaskDelete(NULL);
}

void sdm_cmd_send(uint8_t cmd){
	uint8_t sdm_buff[FRAMESIZE] = {SDM_ADDR, SDM_RGST_INPUT, 0, 0, 0, 0x02, 0, 0, 0};
	sdm_buff[2] = cmd >> 8;  //high 
	sdm_buff[3] = cmd & 0xFF;  //low
	uint16_t temp = calculateCRC(sdm_buff, FRAMESIZE - 3);
	sdm_buff[6] = temp & 0xFF; //low
	sdm_buff[7] = temp >> 8; //high
  
	int8_t st = uart_write_bytes(UART_NUM_0, (const char *) sdm_buff, FRAMESIZE-1) ;

}

void show_countdown(uint8_t cnt) {
	if ( cnt > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных SDM230 осталось %d секунд", cnt);
	}
}

void webfunc(char *pbuf) {
	
	show_countdown( delayed_counter );
	if ( delayed_counter < 1 ) {
		os_sprintf(HTTPBUFF,"<br><b>Command time:</b> %d", cmd_time);
		os_sprintf(HTTPBUFF,"<br><b>voltage:</b> %d.%d", (uint8_t)voltage, (int)(voltage*100) % 100);
		os_sprintf(HTTPBUFF,"<br><b>current:</b> %d.%d", (uint8_t)current, (int)(current) % 100);
		os_sprintf(HTTPBUFF,"<br><b>power:</b> %d.%d", (uint8_t)power, (int)(power*100) % 100);
		os_sprintf(HTTPBUFF,"<br><b>consumption_total:</b> %d.%d", (uint8_t)consumption_total, (int)(consumption_total*100) % 100);
		os_sprintf(HTTPBUFF,"<br><b>consumption_total_resettable:</b> %d.%d", (uint8_t)consumption_total_resettable, (int)(consumption_total_resettable*100) % 100);
	}					
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
	
}


uint16_t calculateCRC(uint8_t *array, uint8_t num){
  uint16_t _crc, _flag;
  _crc = 0xFFFF;
  for (uint8_t i = 0; i < num; i++) {
    _crc = _crc ^ array[i];
    for (uint8_t j = 8; j; j--) {
      _flag = _crc & 0x0001;
      _crc >>= 1;
      if (_flag)
        _crc ^= 0xA001;
    }
  }
  return _crc;	  
}