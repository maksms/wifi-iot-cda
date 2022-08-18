uint32_t sec1_ticker = 0;
uint32_t sec30_ticker = 0;

void startfunc(){// выполняется один раз при старте модуля.
}

void timerfunc(uint32_t  timersrc) {// выполнение кода каждую 1 секунду

sec1_ticker++;

if(timersrc%30==0){// выполнение кода каждые 30 секунд
 sec30_ticker++;
}
 delay(1000); // обязательная строка, минимальное значение для RTOS систем- 10мс
}
void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"1-секундных запусков кода %d:", ticker); // вывод данных на главной модуля
os_sprintf(HTTPBUFF,"30-секундных запусков кода%d:", ticker); // вывод данных на главной модуля
}
