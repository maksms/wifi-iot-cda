uint32_t ticker = 0;

void startfunc(){// выполняется один раз при старте модуля.
}

void timerfunc(uint32_t  timersrc) {// выполнение кода каждую 1 секунду

ticker++;

if(timersrc%30==0){// выполнение кода каждые 30 секунд
}
 delay(1000); // обязательная строка, минимальное значение для RTOS систем- 10мс
}
void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"Натикало %d", ticker); // вывод данных на главной модуля
}
