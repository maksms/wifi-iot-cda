//назначаем пинам имена
const uint8_t MOTION_MANAGER = 36;
const uint8_t MOTION_ZAL = 39;
const uint8_t GERKON_DVER = 35;
const uint8_t GERKON_KASSA = 33;
const uint8_t ZAMOK = 17;
const uint8_t SIRENA = 18;
const uint8_t LED = 27;
const uint8_t BEEP = 32;
const uint8_t ENERGY = 25;
const uint8_t DYM = 15;
const uint8_t RELAY_DYM = 26;

//упрощаем упрощаемое
#define gpio_read GPIO_ALL_GET
#define gpio_write GPIO_ALL

/*================================================================================================================================================*/
/*================================================================================================================================================*/

//=========   ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ     ===========================================================================================================
uint8_t current_mode = 112; //текущий режим работы, 111 - закрыто, 112 - открыто, 113 - тревога
uint8_t sendmode; //что именно отправить? 100 - только sms, 101 - только телеграм, 102 - и телеграм, и смс, 200 - отправка не требуется
bool masterkey_ok;
bool simplekey_ok;
bool key_error;
uint32_t wiegandkey;
uint32_t key_error_info;
uint32_t uptime_second;


bool text_mode_open_flag;
bool text_mode_close_flag;
bool text_mode_autoclose_flag;
bool text_mode_warning_flag;
bool text_door_open_flag;
bool text_door_close_flag;
bool text_money_close_flag;
bool text_money_open_flag;
bool text_siren_on_flag;
bool text_siren_off_flag;
bool text_smoke_on_flag;
bool text_smoke_off_flag;
bool text_energy_on_flag;
bool text_energy_off_flag;
bool text_internet_on_flag;
bool text_internet_off_flag;
bool text_motion_zal_on_flag;
bool text_motion_zal_off_flag;
bool text_motion_manager_on_flag;
bool text_motion_manager_off_flag;
bool text_relay_dym_off_flag;
bool text_relay_dym_on_flag;
bool text_inet_off_flag;
bool text_inet_on_flag;


#define masterkey1 sensors_param.cfgdes[1]
#define masterkey2 sensors_param.cfgdes[2]
#define simplekey1 sensors_param.cfgdes[3]
#define simplekey2 sensors_param.cfgdes[4]
#define simplekey3 sensors_param.cfgdes[5]
#define opentime sensors_param.cfgdes[6]
#define closetime sensors_param.cfgdes[7]
#define simplekeyopentime sensors_param.cfgdes[8]
#define simplekeyclosetime sensors_param.cfgdes[9]


//=========   НАСТРОЙКИ   =========================================================================================================================
#define offtime sensors_param.cfgdes[0] //время для автопостановки на охрану, минуты
#define warning_offtime 5 //время для возврата в режим охраны после сработки

//тексты сообщений
const char *text_mode_open = "Текст";
const char *text_mode_close = "Текст";
const char *text_mode_autoclose = "Текст";
const char *text_mode_warning = "Текст";
const char *text_door_open = "Текст";
const char *text_door_close = "Текст";
const char *text_money_close = "Текст";
const char *text_money_open = "Текст";
const char *text_siren_on = "Текст";
const char *text_siren_off = "Текст";
const char *text_smoke_on = "Текст";
const char *text_smoke_off = "Текст";
const char *text_energy_on = "Текст";
const char *text_energy_off = "Текст";
const char *text_internet_on = "Текст";
const char *text_internet_off = "Текст";
const char *text_motion_zal_on = "Текст";
const char *text_motion_zal_off = "Текст";
const char *text_motion_manager_on = "Текст";
const char *text_motion_manager_off = "Текст";
const char *text_relay_dym_off = "Текст";
const char *text_relay_dym_on = "Текст";
const char *text_inet_off = "Текст";
const char *text_inet_on = "Текст";




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
uint8_t auto_alarm_offtime_min;
uint8_t auto_alarm_offtime_sec;
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
	auto_alarm_offtime_min = offtime - motion_off_min - 1;
	auto_alarm_offtime_sec = 60 - motion_off_sec;
}





//=========   ФУНКЦИИ   =========================================================================================================================

void ModeHandler() { //что делать в том или ином режиме
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

	if(current_mode == 113)
	{
		gpio_write(LED, 1);
		gpio_write(ZAMOK, 1);
	}
}

void SendHandler() { //отправляем уведомления

	/*  read_24cxx(0x50, 100, (uint8_t *)&text_mode_open_state, 1);
	    read_24cxx(0x50, 110, (uint8_t *)&text_mode_close_state, 1);
	    read_24cxx(0x50, 120, (uint8_t *)&text_mode_autoclose_state, 1);
	    read_24cxx(0x50, 130, (uint8_t *)&text_mode_warning_state, 1);
	    read_24cxx(0x50, 140, (uint8_t *)&text_door_open_state, 1);
	    read_24cxx(0x50, 150, (uint8_t *)&text_door_close_state, 1);
	    read_24cxx(0x50, 160, (uint8_t *)&text_money_open_state, 1);
	    read_24cxx(0x50, 170, (uint8_t *)&text_money_close_state, 1);
	*/

	if(sendmode == 100) {
		//  отправляем смс
		sms_send(sensors_param.tel, datasms);
	}
	else
		if(sendmode == 101) {
			//  отправляем телеграм
			sendtelegramm();
		}
		else
			if(sendmode == 102) {
				//  отправляем смс и телеграм
				sendtelegramm();
				sms_send(sensors_param.tel, datasms);
			}
			else {
				sendmode = 200;
			}
	sendmode = 200;
}

void SendSMS() {
	sendmode = 100;
	SendHandler();
}

void SendTelegram() {
	sendmode = 101;
	SendHandler();
}

void SendAll() {
	sendmode = 102;
	SendHandler();
}

void AutoAlarm() {  //автоматическая постановка на охрану, если нет движения в течении времени offtime

	if(motion_off_sec_all == (offtime * 60) && current_mode != 111)
	{
		current_mode = 111;
		os_sprintf(datasms, text_mode_autoclose);
		SendAll();
		sendmode = 200;
	}
}

void WiegandHandler() {
	if(wiegandsiteCode != 0 && wiegandserialNumber != 0) {
		wiegandkey = (wiegandsiteCode * 100000 + wiegandserialNumber);
		if(wiegandkey == masterkey1 || wiegandkey == masterkey2)
			masterkey_ok = 1;
		else
			masterkey_ok = 0;
		if(wiegandkey == simplekey1 || wiegandkey == simplekey2 || wiegandkey == simplekey3)
			simplekey_ok = 1;
		else
			simplekey_ok = 0;
		if(wiegandkey != masterkey1 && wiegandkey != masterkey2 && wiegandkey != simplekey1 && wiegandkey != simplekey2 && wiegandkey != simplekey3)
		{
			key_error = 1;
			key_error_info = wiegandkey;
		}
		else
		{
			key_error = 0;
			key_error_info = 0;
		}
	}


	if((masterkey_ok == 1 || (simplekey_ok == 1 && (time_loc.hour >= simplekeyopentime && time_loc.hour <= simplekeyclosetime))) && current_mode == 111) {
		current_mode = 112;
		masterkey_ok = 0;
		simplekey_ok = 0;
		wiegandsiteCode = 0;
		wiegandserialNumber = 0;
	}

	if((masterkey_ok == 1 || simplekey_ok == 1) && current_mode == 112) {
		current_mode = 111;
		masterkey_ok = 0;
		simplekey_ok = 0;
		wiegandsiteCode = 0;
		wiegandserialNumber = 0;
	}

	if((masterkey_ok == 1 || simplekey_ok == 1) && current_mode == 113) {
		current_mode = 111;
		masterkey_ok = 0;
		simplekey_ok = 0;
		wiegandsiteCode = 0;
		wiegandserialNumber = 0;
	}
}

void InternetWatchDogSend() {

	if(uptime_second > 40 && !pingprint && !text_inet_off_flag) { //если нет интернета
		os_sprintf(datasms, text_inet_off);
		SendAll();
		text_inet_off_flag = 1;
		text_inet_on_flag = 0;
	}

	if(uptime_second > 40 && pingprint && !text_inet_on_flag) { //если интернет появился
		os_sprintf(datasms, text_inet_on);
		SendAll();
		text_inet_on_flag = 1;
		text_inet_off_flag = 0;
	}
}

void StarterSend() { //на 10 секунде аптайма отправляем сообщения о включении прибора
	if(uptime_second == 30) {
		os_sprintf(datasms, " #Магазин_ПРИБОР_СИГНАЛИЗАЦИИ_запущен %u секунд назад", uptime_second);
		SendAll();
	}
}

void UpTimeSecondMath() {
	uptime_second = timer_uptime.day * 3600 * 24 + timer_uptime.hour * 3600 + timer_uptime.min * 60 + timer_uptime.sec; //время работы модуля в секундах
}


//=========   ОСНОВНОЙ КОД    ===================================================================================================================
void startfunc() {
	// выполняется один раз при старте модуля.

	// запуск таймера контроля датчиков, 1 раз в секунду
	motion_detect_timer = xTimerCreate("Motion Detect Timer", pdMS_TO_TICKS(1 * 1000), pdTRUE, 0, vMotionDetectTimerCallback);
	BaseType_t c = xTimerStart(motion_detect_timer, 0); //запуск циклического таймера

	// запуск таймера отсутствия движения, запуск 1 раз в секунду
	motion_off_timer = xTimerCreate("Motion Off Timer", pdMS_TO_TICKS(1 * 1000), pdTRUE, 0, vMotionOffTimerCallback);
	BaseType_t b = xTimerStart(motion_off_timer, 0); //запуск циклического таймера

}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	UpTimeSecondMath();
	StarterSend();
	AutoAlarm();
	ModeHandler();
	WiegandHandler();
	InternetWatchDogSend();




	if(timersrc % 30 == 0) {
		// выполнение кода каждые 30 секунд



	}
	delay(50); // обязательная строка, минимальное значение для RTOS систем- 10мс
}
void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF, "<b>Система охранной сигнализации<br>"); // вывод данных на главной модуля
	os_sprintf(HTTPBUFF, "<b>==============================================<br>");
	if(current_mode == 111) {
		os_sprintf(HTTPBUFF, "<b>Текущий режим: На охране (закрыто)<br>");
	}
	if(current_mode == 112) {
		os_sprintf(HTTPBUFF, "<b>Текущий режим: Снято с охраны (открыто)<br>");
	}
	if(current_mode == 113) {
		os_sprintf(HTTPBUFF, "<b>Текущий режим: ТРЕВОГА<br>");
	}
	os_sprintf(HTTPBUFF, "<b>Движение отсутствует: %d ч %d мин %d сек<br>", motion_off_hour, motion_off_min, motion_off_sec);
	if(current_mode != 111) {
		os_sprintf(HTTPBUFF, "<b>Авто-постановка на охрану через: %d мин %d сек<br>", auto_alarm_offtime_min, auto_alarm_offtime_sec);
	}
	os_sprintf(HTTPBUFF, "<b>   <br>");
	os_sprintf(HTTPBUFF, "<b>   <br>");
	os_sprintf(HTTPBUFF, "<b>   <br>");
	os_sprintf(HTTPBUFF, "<b>   <br>");
	if(key_error_info != 0) {
		os_sprintf(HTTPBUFF, "<b>Номер неизвестного ключа: %d<br>", key_error_info);
	}
	os_sprintf(HTTPBUFF, "<b>----------------------------------------------<br>");
	if(pingprint) {
		os_sprintf(HTTPBUFF, "<b>Интернет есть<br>");
	}
	else {
		os_sprintf(HTTPBUFF, "<b>Интернет отсутствует<br>");
	}
	os_sprintf(HTTPBUFF, "<b>uptime_second: %d<br>", uptime_second);
	//os_sprintf(HTTPBUFF,"<button type='button' onclick='func(112);repage()' style='width:130px;height:20px'><b>Снять с охраны</b></button>");
	//os_sprintf(HTTPBUFF,"<button type='button' onclick='func(111);repage()' style='width:130px;height:20px'><b>Поставить на охрану</b></button>");
	//os_sprintf(HTTPBUFF,"function func(current_mode){request.open('GET', 'gpio?st=2&pin='+pinSet, true);request.onreadystatechange = reqReadyStateChange;request.send();}");
}
