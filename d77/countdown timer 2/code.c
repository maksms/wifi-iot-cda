#define		GPIOUPR		5    //гпио управление таймером
uint32_t  timersrc_save = 0; //сюда сохраняем текущее состояние счетчика прошивки
uint32_t  timersrc_save_sost = 0;//переменная состояния обратного отсчета
bool flag = 0;//флаг запуска отсчета
void ICACHE_FLASH_ATTR startfunc()
{
	digitalWrite(GPIOUPR, 0); //ставим на старте гпио в 0
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
	if (sensors_param.cfgdes[0] < 0 || 120 < sensors_param.cfgdes[0])//диапазон значений таймера
	{
		sensors_param.cfgdes[0] = 90; 
	}
	if (valdes[0] == 1)//если таймер не запущен и включили гпио
	{
		valdes[0] = 0;
		flag = 1;//то поднимаем флаг
		digitalWrite(GPIOUPR, 1);//включаем гпио
		timersrc_save = timersrc;//и запоминаем значение таймера прошивки
	}
	if (flag == 1)
	{
		timersrc_save_sost = timersrc - timersrc_save;//вычисляем разницу между текущим состоянием таймера прошивки и его сохраненным значением
		if (timersrc_save_sost == sensors_param.cfgdes[0])//если соответствует запрошенному 
		{
			flag = 0;//опускае флаг
			digitalWrite(GPIOUPR, 0);//выключаем гпио
		}
	}
}

void webfunc(char *pbuf) 
{
	if (flag == 1)//если таймер работает
	{
		os_sprintf(HTTPBUFF,"<br>осталось %d сек. ", (sensors_param.cfgdes[0] - timersrc_save_sost)); // вывод данных на главной модуля
		os_sprintf(HTTPBUFF + os_strlen(HTTPBUFF), "<meta http-equiv='refresh' content='2'>");//обновляем страничку раз в 2 сек
	}
}