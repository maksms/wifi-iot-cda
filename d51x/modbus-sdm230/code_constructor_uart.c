#define FW_VER "2.19"

#define MAX_MILLIS_TO_WAIT             	500
#define FRAMESIZE                    	9 
#define SDM_REPLY_BYTE_COUNT          	0x04 
#define SDM_ADDR                      	0x01
#define SDM_RGST_INPUT                	0x04
#define SDM_RGST_HOLD               	0x03
#define SDM_REPLY_BYTE_COUNT            0x04 

#define SDM_VOLTAGE                     0x0000                              //V
#define SDM_CURRENT                     0x0006                              //A
#define SDM_POWER                       0x000C                              //W
#define SDM_TOTAL_ACTIVE_ENERGY         0x0156                              //kwh
#define SDM_CURRENT_RESETTABLE_TOTAL_ACTIVE_ENERGY   0x0180                  //kWh     ONLY SDM230
 
#define millis() (unsigned long) (esp_timer_get_time() / 1000ULL) 
 
float readVal(uint16_t reg, uint8_t addr);
uint16_t calculateCRC(uint8_t *array, uint8_t num);
 
 uint32_t task_cnt = 0;
 
uint32_t voltage = 0;
uint32_t current = 0;
uint32_t power = 0;
uint32_t consumption_total = 0;
uint32_t consumption_total_resettable = 0;
//uint32_t working_hours = 0;

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;

uint8_t len_data;
uint8_t ready = 0;
uint32_t x_time = 0;
uint32_t cmd_time = 0;
//float res = 0;


static void get_sdm_data(){
	task_cnt++;
	x_time = millis();
	//while (1) {
		voltage = readVal(SDM_VOLTAGE, SDM_ADDR);
		vTaskDelay(50 / portTICK_PERIOD_MS);
		
		current = readVal(SDM_CURRENT, SDM_ADDR);
		vTaskDelay(50 / portTICK_PERIOD_MS);
		
		power =  readVal(SDM_POWER, SDM_ADDR);
		vTaskDelay(50 / portTICK_PERIOD_MS);
		
		consumption_total = readVal(SDM_TOTAL_ACTIVE_ENERGY, SDM_ADDR);
		vTaskDelay(50 / portTICK_PERIOD_MS);
		
		consumption_total_resettable = readVal(SDM_CURRENT_RESETTABLE_TOTAL_ACTIVE_ENERGY, SDM_ADDR);
		//xTaskCreate(read_buff, "uart_read_buff", 1024*2, NULL /*pvParameters */, 12, NULL);
		//read_buff
		//ready = read_buff(1, 1, &res);
		cmd_time = millis() - x_time;
		//break;
		//delay(500);
	//}
	vTaskDelete(NULL);
	
}


static uint8_t read_buff(uint8_t addr, uint8_t reg, float *value)
{
	//return 0;
	int8_t result = -1;
	uart_event_t event;
    uint8_t *dtmp = (uint8_t *) malloc(RD_BUF_SIZE);
	float val = 0;
	for (;;) {
		if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) { 
            bzero(dtmp, RD_BUF_SIZE);
			result = event.type;
			
			 switch (event.type) { 
				case UART_DATA:
								len_data = uart_read_bytes(UART_NUM_0, dtmp, event.size, portMAX_DELAY);
								//((uint8_t*)&val)[3]= dtmp[3]; ((uint8_t*)&val)[2]= dtmp[4];	((uint8_t*)&val)[1]= dtmp[5];	((uint8_t*)&val)[0]= dtmp[6];
								//*value = val;
								
								// или вариант, так лучше
								 ((uint8_t*)value)[3]= dtmp[3]; 
								 ((uint8_t*)value)[2]= dtmp[4];	
								 ((uint8_t*)value)[1]= dtmp[5];	
								 ((uint8_t*)value)[0]= dtmp[6];
								break;
				case UART_FIFO_OVF:
								uart_flush_input(UART_NUM_0);
								xQueueReset(uart0_queue);
								break;	
				case UART_BUFFER_FULL:
								uart_flush_input(UART_NUM_0);
								xQueueReset(uart0_queue);
								break;
                case UART_PARITY_ERR:
								break;
                case UART_FRAME_ERR:
								break;
                default:
								break;								
			 }
		}	

		break;
	}
	
    free(dtmp);
    dtmp = NULL;
    //vTaskDelete(NULL); 	
	return result;
}

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
    //uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 100, NULL);	
}

static void send_buff(uint8_t *data, uint8_t len) {

	uart_write_bytes(UART_NUM_0, (const char *) data, len);
	// if func_res >= OK else FAIL (-1)
}

void startfunc(){
	 uart_init();
	  
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}

	if(timersrc%3==0){
		xTaskCreate(get_sdm_data, "get_sdm_data", 1024*2, NULL /*pvParameters */, 12, NULL);
	
	}
	
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}


void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF,"<br><b>task count:</b> %d", task_cnt);
	os_sprintf(HTTPBUFF,"<br><b>time:</b> %d", cmd_time);
	os_sprintf(HTTPBUFF,"<br><b>len data:</b> %d", len_data);
	os_sprintf(HTTPBUFF,"<br><b>ready:</b> %d", ready);
//	os_sprintf(HTTPBUFF,"<br><b>value:</b> %d.%d", (uint8_t)res, (int)(res*100) % 100);
	
		os_sprintf(HTTPBUFF,"<br><b>voltage:</b> %d.%d", (uint8_t)voltage, (int)(voltage*100) % 100);
			os_sprintf(HTTPBUFF,"<br><b>current:</b> %d.%d", (uint8_t)current, (int)(current) % 100);
				os_sprintf(HTTPBUFF,"<br><b>power:</b> %d.%d", (uint8_t)power, (int)(power*100) % 100);
					os_sprintf(HTTPBUFF,"<br><b>consumption_total:</b> %d.%d", (uint8_t)consumption_total, (int)(consumption_total*100) % 100);
						os_sprintf(HTTPBUFF,"<br><b>consumption_total_resettable:</b> %d.%d", (uint8_t)consumption_total_resettable, (int)(consumption_total_resettable*100) % 100);
						
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
	
}


 
float readVal(uint16_t reg, uint8_t addr) {
  uint16_t temp;
  uint8_t sdm_buff[FRAMESIZE] = {addr, SDM_RGST_INPUT, 0, 0, 0, 0x02, 0, 0, 0};
  float result = 0;

  sdm_buff[2] = reg >> 8;  //high 
  sdm_buff[3] = reg & 0xFF;  //low

  temp = calculateCRC(sdm_buff, FRAMESIZE - 3);                                   //calculate out crc only from first 6 bytes

  sdm_buff[6] = temp & 0xFF; //low
  sdm_buff[7] = temp >> 8; //high
  
  // переключить pin - RTS в high  digitalWrite(RTS_pin, HIGH)  в том случае если он используется
  // delay(2)
  //send to uart0
  int8_t st = uart_write_bytes(UART_NUM_0, (const char *) sdm_buff, FRAMESIZE-1) ;
vTaskDelay(50 / portTICK_PERIOD_MS);
	//if ( st < 0 ) { return 0; } // не удалась отправка в uart
	//delay(2);	
	//TODO: flush TX buffer 
	uart_flush_input(UART_NUM_0);
	// переключить pin - RTS в low  digitalWrite(RTS_pin, LOW)  в том случае если он используется
	
	//resptime = millis() + MAX_MILLIS_TO_WAIT;
	
	// читаем порт
    //st = uart_read_bytes(UART_NUM_0, sdm_buff, FRAMESIZE, portMAX_DELAY);
	read_buff(addr, reg, &result);
	uart_flush(UART_NUM_0);
	return result;
	//if ( st < 0 ) { return 0; }
	/*
	if (sdm_buff[0] == addr && sdm_buff[1] == SDM_RGST_INPUT && sdm_buff[2] == SDM_REPLY_BYTE_COUNT) {
		if ((calculateCRC(sdm_buff, FRAMESIZE - 2)) == ((sdm_buff[8] << 8) | sdm_buff[7])) {
          ((uint8_t*)&res)[2]= sdm_buff[4];
          ((uint8_t*)&res)[3]= sdm_buff[3];
          ((uint8_t*)&res)[1]= sdm_buff[5];
          ((uint8_t*)&res)[0]= sdm_buff[6];	
		   return res;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
	*/
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