uint16_t tik1_button = 0;
uint16_t tik2_button = 0;
uint8_t flag_button = 0;
uint8_t number_of_clicks1 = 0;
uint8_t number_of_clicks2 = 0;
bool sost_button = 1;
bool short_button = 0;
bool long_button = 0;
bool flag_sost_switch_reception = 0;
static os_timer_t esp_timer1;
read_kod()
{
	if (digitalRead(14) != sost_button)//ловлю изменение гпио
	{
		flag_button ++;
		sost_button = digitalRead(14);//запоминаю состояние гпио
	}
	if (flag_button == 1 && sost_button == 0)//если нажали кнопку считаю время нажатия
	{
		tik1_button ++;
	}
	if (flag_button > 1)//если дребезг или отпустили кнопку
	{
		if(sost_button == 1)//если кнопка отпущена 
		{
			if(tik1_button > 30 && tik1_button < 1500)//если время нажатия больше 30мс и меньше 1,5сек
			{
				short_button = 1;//то это короткое нажатие
			}
			if(tik1_button >= 1500)//если время нажатия больше 1,5сек
			{
				long_button = 1;//то это длинное нажатие
			}
		}
		tik1_button = 0;//обнуляем переменные если время составило меньше 30 мс , значит это дребезг
		flag_button = 0;//обнуляем переменные
	}	
	if(short_button == 1)//если короткое нажатие
	{
		number_of_clicks1 += 1;// суммируем нажатия
		short_button = 0;//обнуляем переменные
		tik2_button = 1;//запускаем счетчик времени для счета кол-ва нажатий
	}
	if (tik2_button > 0)//запускаем счетчик времени для счета кол-ва нажатий
	{
		tik2_button ++;
	}
	if (tik2_button > 300) // если в течении этого времени не была нажата кнопка
	{
		number_of_clicks2 = number_of_clicks1;//то перекладываем кол-во нажатий в переменную для обработки в основном цикле
		number_of_clicks1 = 0;//обнуляем переменные
		tik2_button = 0;//обнуляем переменные
	}
}	

void ICACHE_FLASH_ATTR startfunc()
{
	os_timer_disarm(&esp_timer1);
	os_timer_setfn(&esp_timer1, (os_timer_func_t *)read_kod, NULL);
	os_timer_arm(&esp_timer1, 1, 1);// 1мс , постоянная работа
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t timersrc)
{
	if (long_button == 1)
	{
		long_button = 0;//обнуляем переменные
		if (mtest == 15 && sensors_param.mqtten == 1)
		{
			MQTT_Client *client = (MQTT_Client *)&mqttClient;
			MQTT_Publish(client, "KEY_ALARM", "KEY_ALARM", 9, 2, 0, 1);//шлем KEY_ALARM
			mqttSEND();//вызываем отправку всех метрик мктт
		}
	}
	if (number_of_clicks2 > 0)
	{
		if (number_of_clicks2 == 1)//Одинарный щелчок
		{	
			if(GPIO_ALL_GET(200) == 1 || GPIO_ALL_GET(201) == 1 || GPIO_ALL_GET(202) == 1 || GPIO_ALL_GET(203) == 1 || GPIO_ALL_GET(204) == 1 || GPIO_ALL_GET(205) == 1 || GPIO_ALL_GET(206) == 1)//выключает Gpio200+Gpio201+Gpio202+Gpio203, Gpio204+Gpio205, Gpio206, если хотя бы один Gpio200 Gpio201 Gpio202 Gpio203 Gpio204 Gpio205 или Gpio206 включен
			{
				GPIO_ALL(200, 0);//выключает Gpio200, если хоть что то горит
				GPIO_ALL(201, 0);//выключает Gpio201, если хоть что то горит
				GPIO_ALL(202, 0);//выключает Gpio202, если хоть что то горит
				GPIO_ALL(203, 0);//выключает Gpio203, если хоть что то горит
				GPIO_ALL(204, 0);//выключает Gpio204, если хоть что то горит
				GPIO_ALL(205, 0);//выключает Gpio205, если хоть что то горит
				GPIO_ALL(206, 0);//выключает Gpio206, если хоть что то горит
			}
			else
			{
				GPIO_ALL(200, 1);//включает Gpio200, если все Gpio200+Gpio201+Gpio202+Gpio203 выключены
				GPIO_ALL(201, 1);//включает Gpio201, если все Gpio200+Gpio201+Gpio202+Gpio203 выключены
				GPIO_ALL(202, 1);//включает Gpio202, если все Gpio200+Gpio201+Gpio202+Gpio203 выключены
				GPIO_ALL(203, 1);//включает Gpio203, если все Gpio200+Gpio201+Gpio202+Gpio203 выключены
			}
		}
		if (number_of_clicks2 == 2)//Двойной щелчок
		{	
			if(GPIO_ALL_GET(204) == 1 || GPIO_ALL_GET(205) == 1)//если Gpio204 или Gpio205  включен
			{
				GPIO_ALL(204, 0);// Выключает Gpio204
				GPIO_ALL(205, 0);// Выключает Gpio205
			}
			else
			{
				GPIO_ALL(204, 1);//Включает Gpio204, если оба выключены
				GPIO_ALL(205, 1);//Включает Gpio205, если оба выключены
			}
		}
		if (number_of_clicks2 == 3)//Тройной щелчок
		{	
			if(GPIO_ALL_GET(206) == 1)//если Gpio206  включен
			{
				GPIO_ALL(206, 0);// Выключает
			}
			else
			{
				GPIO_ALL(206, 1);//Включает Gpio206, если он выключен
			}
		}
		if (mtest == 15 && sensors_param.mqtten == 1)//если мктт в норме и включено
		{
			mqttSEND();//вызываем отправку всех метрик мктт
		}
		number_of_clicks2 = 0;//обнуляем переменные
	}
	if (timersrc !=0 &&  600 < wfrc )// если обрывов связи больше 600 ,перезагрузим модуль иначе обычно он уже на связь не выходит
	{
		system_restart() ;
	}
}
void webfunc(char *pbuf)
{
}