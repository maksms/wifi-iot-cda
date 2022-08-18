uint32_t ticker_1s = 0;
uint32_t ticker_30s = 0;

void startfunc(){// выполняется один раз при старте модуля.
}

void timerfunc(uint32_t  timersrc) {// выполнение кода каждую 1 секунду

ticker_1s++;

if(timersrc%30==0){// выполнение кода каждые 30 секунд
 ticker_30s++;
}
 delay(1000); // обязательная строка, минимальное значение для RTOS систем- 10мс
}
void webfunc(char *pbuf) {

os_sprintf(HTTPBUFF, "<b>Тестовый модуль<br>"); // вывод данных на главной модуля
os_sprintf(HTTPBUFF, "<b>-------------------------------------------------<br>");
os_sprintf(HTTPBUFF, "<b>1-секундных запусков кода: %d<br>", ticker_1s);
os_sprintf(HTTPBUFF, "<b>30-секундных запусков кода: %d<br>", ticker_30s);

}
