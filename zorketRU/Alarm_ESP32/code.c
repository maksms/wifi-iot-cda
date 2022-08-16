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

//переменная уведомления сиреной об открытии-закрытии замка, ячейка 60
//код 11 - "уведомление о закрытии уже прозвучало, пи не требуется"
//код 22 - "уведомление об открытии уже прозвучало, пи-пи не требуется"
uint8_t state_pipi;

//переменные состояний отправки

//замок, ячейка 50
//код 55 - "сообщение о закрытии уже отправлено, отправка не требуется"
//код 77 - "сообщение об открытии уже отправлено, отправка не требуется"
uint8_t state_zamok;

void startfunc() { // выполняется один раз при старте модуля.

}

void timerfunc(uint32_t  timersrc) {// выполнение кода каждую 1 секунду

	//Светодиод на считывателе
	//Если замок закрыт(1) - led синий(1), если замок открыт - зеленый(0), жестко привязываем цвет диода к состоянию замка
	if(gpio_read(ZAMOK) == 1 && gpio_read(LED) != 1)
		gpio_write(LED, 1);

	if(gpio_read(ZAMOK) == 0 && gpio_read(LED) != 0)
		gpio_write(LED, 0);

	//Пищим сиреной при открытии-закрытии замка
	read_24cxx(0x50, 60, (uint8_t *)&state_pipi, 1);
	
	if(gpio_read(ZAMOK) == 1 && state_pipi != 11)
	{
		gpio_write(SIRENA, 1);
		delay(10);
		gpio_write(SIRENA, 0);
		state_pipi = 11;
		write_24cxx(0x50, 60, (uint8_t *)&state_pipi, 1);
	}

	if(gpio_read(ZAMOK) == 0 && state_pipi == 11)
	{
		gpio_write(SIRENA, 1);
		delay(10);
		gpio_write(SIRENA, 0);
		delay(200);
		gpio_write(SIRENA, 1);
		delay(10);
		gpio_write(SIRENA, 0);
		state_pipi = 22;
		write_24cxx(0x50, 60, (uint8_t *)&state_pipi, 1);
	}


	//Отправляем SMS о изменении состояния замка
	read_24cxx(0x50, 50, (uint8_t *)&state_zamok, 1);
	
	if(gpio_read(ZAMOK) == 1 && state_zamok != 55)
	{
		char datasms[] = " #Магазин_замок_ЗАКРЫТ";
		sms_send(sensors_param.tel, datasms);
		//      sendtelegramm(datasms);
		state_zamok = 55;
		write_24cxx(0x50, 50, (uint8_t *)&state_zamok, 1);
	}

	if(gpio_read(ZAMOK) == 0 && state_zamok == 55)
	{
		char datasms[] = " #Магазин_замок_ОТКРЫТ";
		sms_send(sensors_param.tel, datasms);
		//      sendtelegramm(datasms);
		state_zamok = 77;
		write_24cxx(0x50, 50, (uint8_t *)&state_zamok, 1);
	}

	if(timersrc % 30 == 0) { // выполнение кода каждые 30 секунд
	}
	delay(10); // обязательная строка, минимальное значение для RTOS систем- 10мс
}
void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF, "<b>Система охранной сигнализации<br>"); // вывод данных на главной модуля
	os_sprintf(HTTPBUFF, "<b>Переменная state_zamok ячейка50 содержит uint8_t значение %d, а последнее sms справа %s<br>", state_zamok, datasms); // вывод данных на главной модуля
	os_sprintf(HTTPBUFF, "Текст строки 3<br>"); // вывод данных на главной модуля
}
