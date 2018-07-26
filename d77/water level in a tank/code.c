/*
	в конструкторе кода перед сборкой прошивки в окно "Глобальные переменные" вставить 1 ,а 
    в окно "Количество настроек" этот текст без кавычек : 
	"минимум воды - мм,максимум воды - мм"
*/
void ICACHE_FLASH_ATTR startfunc()
{ 
	valdes[0] = 0;
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
	sonar_read();
	if(sensors_param.cfgdes[0] < 0 || 1000 < sensors_param.cfgdes[0])
		sensors_param.cfgdes[0] = 100;
	if(sensors_param.cfgdes[1] < 0 || 1000 < sensors_param.cfgdes[1])
		sensors_param.cfgdes[1] = 500;
	valdes[0] = (readsonar <= sensors_param.cfgdes[0])?100:(sensors_param.cfgdes[1] <= readsonar)?0:((readsonar - sensors_param.cfgdes[0]) / ((sensors_param.cfgdes[1] - sensors_param.cfgdes[0]) / 100));
}

void webfunc(char *pbuf) 
{
	os_sprintf(HTTPBUFF,"<br>уровень: %d%",valdes[0]); // вывод данных на главной модуля
}