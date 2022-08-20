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

uint32_t ticker_1s = 0; //тикер запуска 1-секундного кода
uint32_t ticker_30s = 0; //тикер запуска 30-секундного кода

/*================================================================================================================================================*/
/*================================================================================================================================================*/


//=========   НАСТРОЙКИ   =========================================================================================================================
#define offtime sensors_param.cfgdes[0] //время для автопостановки на охрану, минуты
#define warning_offtime 5 //время для возврата в режим охраны после сработки
//=========   НАСТРОЙКИ   =========================================================================================================================


uint8_t current_mode = 112; //текущий режим работы, 111 - закрыто, 112 - открыто, 113 - тревога

//=========   ТАЙМЕРЫ   =========================================================================================================================
bool motion; //наличие движения в настоящий момент времени

TimerHandle_t motion_detect_timer; //создаем таймер присмотра за датчиками
void vMotionDetectTimerCallback(TimerHandle_t xTimer) { //следим за датчиками
	if(gpio_read(MOTION_MANAGER) == 0 || gpio_read(MOTION_ZAL) == 0 || gpio_read(GERKON_DVER) == 0) {
		motion = 1;
	}
	else {
		motion = 0;
	}
}

uint32_t motion_off_sec_all; //общее количество секунд без движения, сбрасывается после накопления ~18ч
uint8_t motion_off_sec;
uint8_t motion_off_min;
uint16_t motion_off_hour;

TimerHandle_t motion_off_timer; //создаем таймер отсутствия движения motion_off_timer
void vMotionOffTimerCallback(TimerHandle_t xTimer) { //функция обратного вызова таймера
	if(motion == 0) { //если нет движения - прибавляем секунду
		motion_off_sec_all++;
	}
	else {
		motion_off_sec_all = 0;//если было движение - сбрасываем на ноль
	}
	motion_off_sec = (motion_off_sec_all % 3600ul) % 60ul;
	motion_off_min = (motion_off_sec_all % 3600ul) / 60ul;
	motion_off_hour = (motion_off_sec_all / 3600ul);
}

//=========   ТАЙМЕРЫ   =========================================================================================================================




//=========   ФУНКЦИИ   =========================================================================================================================

void AutoAlarm() {  //автоматическая постановка на охрану, если нет движения в течении времени offtime
	if(motion_off_sec_all >= (offtime * 60) && current_mode == 112)
	{
		current_mode = 111;
	}
}

void ModeHandler(){ //что делать в том или ином режиме
	if(current_mode == 111)
	{
		gpio_write(LED, 1);
		gpio_write(ZAMOK, 1);
	}

	if(current_mode == 112)
	{
		gpio_write(LED, 0);
		gpio_write(ZAMOK, 0);
	}
}

void SendMessage(){ //отправляем уведомления
	
}
//=========   ФУНКЦИИ   =========================================================================================================================



void startfunc() {
	// выполняется один раз при старте модуля.
	
	// запуск таймера контроля датчиков, 2 раза в секунду
	motion_detect_timer = xTimerCreate("Motion Detect Timer", pdMS_TO_TICKS(1 * 500), pdTRUE, 0, vMotionDetectTimerCallback);
	BaseType_t c = xTimerStart(motion_detect_timer, 0); //запуск циклического таймера

	// запуск таймера отсутствия движения, запуск 1 раз в секунду
	motion_off_timer = xTimerCreate("Motion Off Timer", pdMS_TO_TICKS(1 * 1000), pdTRUE, 0, vMotionOffTimerCallback);
	BaseType_t b = xTimerStart(motion_off_timer, 0); //запуск циклического таймера
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	ticker_1s++; //тикер запусков кода
	AutoAlarm();
	ModeHandler();


	/*if(gpio_read(16) == 1) current_mode = 111;
	if(gpio_read(16) == 0) current_mode = 112;*/

	if(timersrc % 30 == 0) {
		// выполнение кода каждые 30 секунд
		ticker_30s++; //тикер запусков

	}

	delay(1000); // обязательная строка, минимальное значение для RTOS систем- 10мс
}
void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF, "<b>Система охранной сигнализации<br>"); // вывод данных на главной модуля
	os_sprintf(HTTPBUFF, "<b>-------------------------------------------------<br>");
	os_sprintf(HTTPBUFF, "<b>Движение отсутствует: %d сек<br>", motion_off_sec_all);
	os_sprintf(HTTPBUFF, "<b>Текущий режим: %d<br>", current_mode);
	os_sprintf(HTTPBUFF, "<b>   <br>");
	os_sprintf(HTTPBUFF, "<b>   <br>");
	os_sprintf(HTTPBUFF, "<b>Текущие настройки сигнализации:<br>");
	os_sprintf(HTTPBUFF, "<b>===============================<br>");
	os_sprintf(HTTPBUFF, "<b> Автопостановка при бездействии, мин: %d<br>", offtime);
	os_sprintf(HTTPBUFF, "<b>   <br>");
	os_sprintf(HTTPBUFF, "<b>   <br>");
	os_sprintf(HTTPBUFF, "<b>Отладочная информация:<br>");
	os_sprintf(HTTPBUFF, "<b>===============================<br>");
	os_sprintf(HTTPBUFF, "<b>1-секундных запусков кода: %d<br>", ticker_1s);
	os_sprintf(HTTPBUFF, "<b>30-секундных запусков кода: %d<br>", ticker_30s);
	os_sprintf(HTTPBUFF, "<b>wiegandserialNumber: %d<br>", wiegandserialNumber);
	os_sprintf(HTTPBUFF, "<b>wiegandsiteCode: %d<br>", wiegandsiteCode);
	os_sprintf(HTTPBUFF, "<b>motion_off_sec: %d<br>", motion_off_sec);
	os_sprintf(HTTPBUFF, "<b>motion_off_min: %d<br>", motion_off_min);
	os_sprintf(HTTPBUFF, "<b>motion_off_hour: %d<br>", motion_off_hour);

}
