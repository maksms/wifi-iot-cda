#define FW_VER "1.16"

uint32_t voltage = 0;
uint32_t current = 0;
uint32_t power = 0;
uint32_t consumption_total = 0;
uint32_t consumption_total_resettable = 0;
//uint32_t working_hours = 0;

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;
uint8_t *dtmp;
uint8_t len_data;
uint8_t ready = 0;
uint32_t x_time = 0;
uint32_t cmd_time = 0;
float res = 0;

static void read_buff(void *pvParameters)
{
	/*
	uint8_t *data = (uint8_t *) malloc(BUF_SIZE); 
	    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        // Write data back to the UART
       
    }
	*/
	uart_event_t event;
	size_t buffered_size;
    dtmp = (uint8_t *) malloc(RD_BUF_SIZE);
	
	for (;;) {
		if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) { 
            bzero(dtmp, RD_BUF_SIZE);
            //ESP_LOGI(TAG, "uart[%d] event:", UART_NUM_0);
			 switch (event.type) { 
				case UART_DATA:
								len_data = uart_read_bytes(UART_NUM_0, dtmp, event.size, portMAX_DELAY);
								cmd_time = (unsigned long) (esp_timer_get_time() / 1000ULL) - x_time;
								
								ready = 1;
								/*
								dtmp[0] - address
								dtmp[1] - function
								dtmp[2] - byte count
								
								dtmp[3]..dtmp[6] - data
								
								dtmp[7]..dtmp[8] - crc
								*/
								res = 0;
								((uint8_t*)&res)[3]= dtmp[3];
								((uint8_t*)&res)[2]= dtmp[4];
								((uint8_t*)&res)[1]= dtmp[5];
								((uint8_t*)&res)[0]= dtmp[6];
								break;
				case UART_FIFO_OVF:
								uart_flush_input(UART_NUM_0);
								xQueueReset(uart0_queue);
								ready = 2;
								break;	
				case UART_BUFFER_FULL:
								uart_flush_input(UART_NUM_0);
								xQueueReset(uart0_queue);
								ready = 2;
								break;
                case UART_PARITY_ERR:
								ready = 2;
								break;
                case UART_FRAME_ERR:
								ready = 2;
								break;
                default:
								ready = 2;
								break;								
			 }
		}		
	}
	
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL); 	
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
}

static void send_buff(uint8_t *data, uint8_t len) {
	//uint8_t len = sizeof(data);
	ready = 0;
	x_time = (unsigned long) (esp_timer_get_time() / 1000ULL);
	uart_write_bytes(UART_NUM_0, (const char *) data, len);
}

void startfunc(){
	// выполняется один раз при старте модуля.
	 uart_init();
	 xTaskCreate(read_buff, "uart_read_buff", 1024*2, NULL /*pvParameters */, 12, NULL); 
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}

	if(timersrc%30==2){
		
		uint8_t send_buf[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x02, 0x71, 0xCB};
		uint8_t len = 8;
		
		send_buff(send_buf, len);
	}
	
	
		//voltage = mbval[0][0];
		//current = mbval[0][1];
		//power = mbval[0][2];
		//consumption_total = mbval[1][0];
		//consumption_total_resettable = mbval[2][0];
		//working_hours = mbval[3][0];
	
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}


void webfunc(char *pbuf) {
	
	//if ( sensors_param.cfgdes[0] > 0 ) {
	//char tmp[10];
	//tofloatmb(tmp, 3, res);
	//tofloatmb(tmp, 3, voltage); 
	//os_sprintf(HTTPBUFF,"<b>Напряжение, В: </b>%s", tmp);
	
	//tofloatmb(tmp, 3, current); 
	//os_sprintf(HTTPBUFF,"<br><b>Ток, А: </b>%s", tmp );
	
	//tofloatmb(tmp, 3, power); 
	//os_sprintf(HTTPBUFF,"<br><b>Мощность, Вт: </b>%s", tmp);
	
	//tofloatmb(tmp, 3, consumption_total); 
	//os_sprintf(HTTPBUFF,"<br><b>Общий счетчик, кВт*ч: </b>%s", tmp );
	
	//tofloatmb(tmp, 3, consumption_total_resettable);
	//os_sprintf(HTTPBUFF,"<br><b>Обнуляемый счетчик, кВт*ч: </b>%s", tmp);
	
	//tofloatmb(tmp, 3, working_hours);
	//os_sprintf(HTTPBUFF,"<br><b>Время работы, ч: </b>%s", tmp );
	//}

	os_sprintf(HTTPBUFF,"<br><b>time:</b> %d", cmd_time);
	os_sprintf(HTTPBUFF,"<br><b>len data:</b> %d", len_data);
	os_sprintf(HTTPBUFF,"<br><b>ready:</b> %d", ready);
	
	os_sprintf(HTTPBUFF,"<br><b>value:</b> %d.%d", (uint8_t)res, (int)(res*100) % 100);
	/*
	for (uint8_t i = 0;i<9;i++){
	os_sprintf(HTTPBUFF,"<br><b>data:</b> %02X", (char *)dtmp[i]);
	}
	*/
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
	
}