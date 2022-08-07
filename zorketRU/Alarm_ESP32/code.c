//назначаем пинам имена
#define MOTION_MANAGER 36
#define MOTION_ZAL 39
#define GERKON_DVER 35
#define GERKON_KASSA 33
#define ZAMOK 17
#define SIRENA 18
#define LED 27
#define BEEP 32
#define ENERGY 25
#define DYM 15
#define RELAY_DYM 26


//упрощаем упрощаемое
#define gpio_read GPIO_ALL_GET
#define gpio_write GPIO_ALL

//параметры связи
#define TEL_NUMBER 89871234567

//флаг отправки sms о постановке на охрану, 0 - отправка не требуется, 1 - отправка требуется
bool sms_state_alarm_on = 1;

//переменная с текстом для отправки
////uint32_t smstext = defaulttext;


void startfunc() { // выполняется один раз при старте модуля.
}

void timerfunc(uint32_t  timersrc) {// выполнение кода каждую 1 секунду

	//Светодиод на считывателе
	//Если на охране(1) - led синий(1), если снято с охраны - зеленый(0)
	if(gpio_read(ZAMOK) == 1) gpio_write(LED, 1);
	else gpio_write(LED, 0);

	/*Отправляем SMS о постановке/снятии с охраны
	    if(gpio_read(ZAMOK) == 1 && sms_state_alarm_on == 1)
	    {
	        smstext == "Test1";
	        send_sms(TEL_NUMBER,smstext);
	        delay(20);
	        sms_state_alarm_on = 0;
	    }
	    else
	    {
	        sms_state_alarm_on = 1;
	    }
	*/
	if(timersrc % 30 == 0) { // выполнение кода каждые 30 секунд
	}
	delay(10); // обязательная строка, минимальное значение для RTOS систем- 10мс
}
void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF, "<b>Система охранной сигнализации<br>"); // вывод данных на главной модуля
	os_sprintf(HTTPBUFF, "ТЕСТ "); // вывод данных на главной модуля
	os_sprintf(HTTPBUFF, "Текст<br>"); // вывод данных на главной модуля
}
