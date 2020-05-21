	#include "../moduls/uart_register.h"
	#include "../moduls/uart.h"
	#include "../moduls/uart.c"



//#define userlog(f_, ...) (printf((f_), ##__VA_ARGS__))

static char log_str[100];

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

void startfunc(){
    uart_init(BIT_RATE_9600);	
	uart_config(UART1);
	uart_div_modify(UART1,	UART_CLK_FREQ	/BIT_RATE_9600);
    os_install_putc1((void *)uart1_tx_buffer);   
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	
	os_bzero(logstr, 100);
    os_sprintf(logstr, "[%d] test: %d.%d \n", timersrc, (int)timersrc/10, 		(int)(timersrc) % 100);
	uart1_tx_buffer(logstr, os_strlen(logstr));
	
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}

}

void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF,"<br>test");
}
