/*при сборке добавить 2 переменные в настройках КК valdes[0]-очередь valdes[1]=прием*/
#define GPIO_switch 13
#define GPIO_button_minus 12
#define GPIO_button_plus 14
uint8_t tik1_button_minus = 0;
uint8_t tik1_button_plus = 0;
uint8_t tik2_button_minus = 0;
uint8_t tik2_button_plus = 0;
uint8_t flag_button_minus = 0;
uint8_t flag_button_plus = 0;
bool sost_button_minus = 0;
bool sost_button_plus = 0;
bool flag_sost_switch_reception = 0;
char display0[16];
char display1[16];
static os_timer_t esp_timer1;
read_kod()
{
	if (digitalRead(GPIO_button_minus) != sost_button_minus)
	{
		flag_button_minus++;
		sost_button_minus = digitalRead(GPIO_button_minus);
	}
	if (tik1_button_minus <= 3)
	{
		if (1 < flag_button_minus)
		{
			flag_button_minus = 1;
			tik1_button_minus = 0;
			tik2_button_minus = 0;
		}
		if (flag_button_minus == 1 && !digitalRead(GPIO_button_minus))
			tik1_button_minus++;
	}
	if (3 <= tik1_button_minus)
	{
		if (tik2_button_minus <= 20)
		{
			if (tik2_button_minus == 0 && digitalRead(GPIO_switch) && valdes[0] > 1)
			{
				valdes[0]--;
				os_sprintf(datasms, "номер очереди %d", valdes[0]);
				sendtelegramm();
			}
			tik2_button_minus++;
		}
		else
		{
			tik1_button_minus = 0;
			tik2_button_minus = 0;
			flag_button_minus = 0;
		}
	}
	if (digitalRead(GPIO_button_plus) != sost_button_plus)
	{
		flag_button_plus++;
		sost_button_plus = digitalRead(GPIO_button_plus);
	}
	if (tik1_button_plus <= 3)
	{
		if (1 < flag_button_plus)
		{
			flag_button_plus = 1;
			tik1_button_plus = 0;
			tik2_button_plus = 0;
		}
		if (flag_button_plus == 1 && !digitalRead(GPIO_button_plus))
			tik1_button_plus++;
	}
	if (3 <= tik1_button_plus)
	{
		if (tik2_button_plus <= 20)
		{
			if (tik2_button_plus == 0 && digitalRead(GPIO_switch))
			{
				valdes[0]++;
				if (valdes[0] > 50)
					valdes[0] = 1;
				os_sprintf(datasms, "номер очереди %d", valdes[0]);
				sendtelegramm();
			}
			tik2_button_plus++;
		}
		else
		{
			tik1_button_plus = 0;
			tik2_button_plus = 0;
			flag_button_plus = 0;
		}
	}
}
void ICACHE_FLASH_ATTR startfunc()
{
	valdes[0] = 1;
	os_timer_disarm(&esp_timer1);
	os_timer_setfn(&esp_timer1, (os_timer_func_t *)read_kod, NULL);
	os_timer_arm(&esp_timer1, 100, 1); //опрос раз в 100мс
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t timersrc)
{
	if (!flag_sost_switch_reception && digitalRead(GPIO_switch))
	{
		os_sprintf(datasms, "Врач принимает");
		sendtelegramm();
	}
	if (flag_sost_switch_reception && !digitalRead(GPIO_switch))
	{
		os_sprintf(datasms, "Прием окончен");
		sendtelegramm();
	}
	valdes[1] = flag_sost_switch_reception = digitalRead(GPIO_switch);
	os_sprintf(display0, "%s", digitalRead(GPIO_switch) ? "PRIEM" : "NET PRIEMA");
	os_sprintf(display1, "%d", valdes[0]);
	LCD_print(0, display0);
	LCD_print(1, display1);
}
void webfunc(char *pbuf)
{
	os_sprintf(HTTPBUFF, "<br>Номер в очереди: %d<br>%s", valdes[0], digitalRead(GPIO_switch) ? "Врач принимает" : "Прием окончен");
	os_sprintf(HTTPBUFF, "<meta http-equiv='refresh' content='2'>");
}
