//заранее вбить имена роутеров и пороли в 9,10 строке
//при сборке ввести переменных 2, а в поле настройки 0
uint32_t ip_Gw = 0;
void alt_ssid_mode()
{
	wifi_mode_t currentMode;
	wifi_config_t sta_config;
	esp_wifi_get_config(WIFI_IF_STA, &sta_config);
	os_sprintf(sta_config.sta.ssid, "%s", (valdes[1] == 0) ? "SSID_1" : "SSID_2"); //имя роутера
	os_sprintf(sta_config.sta.password, "%s", (valdes[1] == 0) ? "PASSWORD_1" : "PASSWORD_2");	//пароль роутера
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));	 //применить параметры роутера
	esp_wifi_stop();
	ESP_ERROR_CHECK(esp_wifi_start());
	printf("alt_ssid_mode\n");
}

void startfunc()
{
	valdes[0] = valdes[1] = 0;
}

void timerfunc(uint32_t timersrc)
{
	tcpip_adapter_ip_info_t ipdata;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipdata);
	os_memcpy(&ip_Gw, &ipdata.gw, 4);
	if (ip_Gw == 0)
	{
		valdes[0]++;
	}
	if ((valdes[0] % 60) == 0 && valdes[0])
	{
		valdes[0]++;
		valdes[1]++;
        	if (valdes[1] > 1)
		{
			valdes[1] = 0;
		}
        	alt_ssid_mode(); //двинем на следущий роутер
	}
	if (valdes[0] == 1100)
	{
		esp_restart();
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void webfunc(char *pbuf)
{
}
