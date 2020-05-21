#include "driver/uart.h"

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
 
 
static void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
	uart_param_config(UART_NUM_0, &uart_config);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_driver_install(UART_NUM_0, 1024 * 2, 1024 * 2, 100, NULL);
    uart_driver_install(UART_NUM_1, 1024 * 2, 1024 * 2, 100, NULL);
   
    os_install_putc1(userlog);
}
 
 
void startfunc(){
 
	uart_init();
	
	char str[] = "test string";
	uint8_t i = 1;
	uint8_t k = 2;
	
	
    userlog("\n put string to uart1 \n");
    userlog("\n put string value to uart1: %s \n", str);
    userlog("\n put int value to uart1: %d \n", i);
    userlog("\n put 2 int value to uart1: %d %d \n", k, i);
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	
	userlog("%s   time: %d \n", __func__, timersrc);
	
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF,"<br>test");
}
