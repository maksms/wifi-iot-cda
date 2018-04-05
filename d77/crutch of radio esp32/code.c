/*
	в конструкторе кода перед сборкой прошивки в окно "Глобальные переменные" вставить 2 ,а 
    в окно "Количество настроек" этот текст без кавычек : "
	
	стартовая станция,стартовая громкость,время до старта радио после перезагрузки,время через которое вернет станцию при обрыве,предел памяти,время сброса по памяти
	
	"
*/
uint32_t counter_of_time_of_free_memory = 0;
void startfunc()
{
	valdes[0] = valdes[1] = 0;	
}
void timerfunc(uint32_t timersrc)
{
	if (sensors_param.cfgdes[0] < 0 || 20 < sensors_param.cfgdes[0])
		sensors_param.cfgdes[0] = 0; //стартовая станция
	if (sensors_param.cfgdes[1] < 0 || 50 < sensors_param.cfgdes[1])
		sensors_param.cfgdes[1] = 0; //стартовая громкость
	if (sensors_param.cfgdes[2] < 0 || 180 < sensors_param.cfgdes[2])
		sensors_param.cfgdes[2] = 5; //время до старта радио после перезагрузки
	if (sensors_param.cfgdes[3] < 0 || 180 < sensors_param.cfgdes[3])
		sensors_param.cfgdes[3] = 5; //время через которое вернет станцию при обрыве
	if (sensors_param.cfgdes[4] < 1000 || 200000 < sensors_param.cfgdes[4])
		sensors_param.cfgdes[4] = 120000; //предел памяти
	if (sensors_param.cfgdes[5] < 60 || 200000 < sensors_param.cfgdes[5])
		sensors_param.cfgdes[5] = 300; //время сброса по памяти
	if (curradio && valdes[0] != curradio)
		valdes[0] = curradio; //сохраняем состояние
	if (curradio !=0 && system_get_free_heap_size() > sensors_param.cfgdes[4])
		counter_of_time_of_free_memory ++;
	else
		counter_of_time_of_free_memory = 0;
	if (counter_of_time_of_free_memory > sensors_param.cfgdes[5])
		system_restart();
	if (!curradio && valdes[0] && pt2257_vol_state)
		valdes[1]++;
	if (valdes[1] > sensors_param.cfgdes[3])
	{
		os_sprintf(urlbuf, "%d", valdes[0]);
		radio_control();
		valdes[1] = 0;
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}
void webfunc(char *pbuf)
{	
}