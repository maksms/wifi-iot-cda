/* termometr 24_10_2021_1  SDK 1.3.0  ESP8266: Скомпилирована новая прошивка с: 1-wire DS18B20, Коррекция датчиков, Время и NTP, GPIO, Поддержка календаря, Конструктор кода, Конструктор main page, Обновление OTA */
#define ADDLISTSENS {201,LS_MODE_TEMP|LSENSFL2|LSENS32BIT,"pikovaya","pikovaya",&pikovaya,NULL},{202,LS_MODE_TEMP|LSENSFL2|LSENS32BIT,"pikovayaold","pikovayaold",&pikovayaold,NULL},
uint8_t  flag_temp = 0;
uint8_t  save_hour = 0;
uint8_t  save_min = 0;
uint8_t  save_sec = 0;
uint8_t  save_day = 0;
uint8_t  save_month = 0;
uint8_t  save_dow = 0;
uint8_t  timer_flag = 0;
uint16_t save_year = 0;
int32_t save_temp = 0;
uint8_t  save_flag_temp = 0;
uint8_t  save_save_hour = 0;
uint8_t  save_save_min = 0;
uint8_t  save_save_sec = 0;
uint8_t  save_save_day = 0;
uint8_t  save_save_month = 0;
uint8_t  save_save_dow = 0;
uint8_t  save_timer_flag = 0;
uint16_t save_save_year = 0;
int32_t save_save_temp = 0;
int32_t save_temp_doble = 0;
int32_t pikovaya = 0;
int32_t pikovayaold = 0;
uint32_t timersrc_save = 0;
void ICACHE_FLASH_ATTR startfunc()
{
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t timersrc)
{
	int32_t temp = ((data1wire[0] != 0 && data1wire[0] != 8500  && data1wire[0] != 25500) ? data1wire[0] : temp);
	if (temp == save_temp_doble && time_loc.day > 0 && flag_temp != 0)
	{
		timer_flag +=1;
	}
	if (flag_temp != 0  && timer_flag > 30)
	{
		flag_temp = 0;
		timer_flag = 0;
	}
	if (save_temp > 0 && temp > save_temp_doble && time_loc.day > 0)
	{
		flag_temp = 1;
	}
	if (save_temp > 0 && temp > (save_temp_doble + 18) && time_loc.day > 0)
	{
		if((timersrc - timersrc_save) > 60)
		{
			pikovayaold = save_save_temp = save_temp;
			save_save_hour = save_hour;
			save_save_min = save_min;
			save_save_sec = save_sec;
			save_save_day = save_day;
			save_save_month = save_month;
			save_save_year = save_year;
			save_save_dow = save_dow;
			timersrc_save = timersrc;
		}
		save_temp = 0;
	}
	if (temp < save_temp_doble && time_loc.day > 0)
	{
		flag_temp = 2;
	}
	if (temp != save_temp_doble && time_loc.day > 0)
	{
		save_temp_doble = temp;
	}
	if (temp > save_temp && time_loc.day > 0)
	{
		pikovaya = save_temp = temp;
		save_hour = time_loc.hour;
		save_min = time_loc.min;
		save_sec = time_loc.sec;
		save_day = time_loc.day;
		save_month = time_loc.month;
		save_year = time_loc.year;
		save_dow = time_loc.dow;
	}
}
void webfunc(char *pbuf)
{
	os_sprintf(HTTPBUFF, "<pre><center><H2>в данный момент %s%02d.%02d",(data1wire[0] > 3680)?"<font color=\"red\">":"", data1wire[0] / 100, data1wire[0] % 100);
	os_sprintf(HTTPBUFF, "%s °C", (data1wire[0] > 3680)?"</font>":"");
	os_sprintf(HTTPBUFF, "<br><br>%s",(flag_temp == 1)? "растет":(flag_temp == 2) ? "падает":"");
	if (save_temp > 0)
	{
		os_sprintf(HTTPBUFF, "<br><hr><br><br>в %02d:%02d:%02d %s %02d.%02d.20%02d<br><br>было", save_hour, save_min, save_sec, (save_dow == 0)?"ПН":(save_dow == 1)?"ВТ":(save_dow == 2)? "СР":(save_dow == 3)? "ЧТ":(save_dow == 4)? "ПТ":(save_dow == 5)? "СБ":"ВС", save_day, save_month, save_year);
		os_sprintf(HTTPBUFF, " %s%02d.%02d", (save_temp > 3680)?"<font color=\"red\">":"", save_temp / 100, save_temp % 100);
		os_sprintf(HTTPBUFF, "%s °C", (save_temp > 3680)?"</font>":"");
	}
	if (save_save_temp > 0)
	{
		os_sprintf(HTTPBUFF, "<br><br><br><hr><br><br>в %02d:%02d:%02d %s %02d.%02d.20%02d<br><br>было", save_save_hour, save_save_min, save_save_sec, (save_save_dow == 0)?"ПН":(save_save_dow == 1)?"ВТ":(save_save_dow == 2)? "СР":(save_save_dow == 3)? "ЧТ":(save_save_dow == 4)? "ПТ":(save_save_dow == 5)? "СБ":"ВС", save_save_day, save_save_month, save_save_year);
		os_sprintf(HTTPBUFF, " %s%02d.%02d", (save_save_temp > 3680)?"<font color=\"red\">":"", save_save_temp / 100, save_save_temp % 100);
		os_sprintf(HTTPBUFF, "%s °C", (save_save_temp > 3680)?"</font>":"");
	}
	os_sprintf(HTTPBUFF, "</H2></center></pre><meta http-equiv='refresh' content='2'>");
}