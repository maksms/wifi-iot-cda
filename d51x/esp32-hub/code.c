#define BLE_MQTT_SEND_MAC 1
void
startfunc(){
// выполняется один раз при старте модуля.
}

void
 timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	if(timersrc%30==0)
	{
	// выполнение кода каждые 30 секунд



	}

		if ( sensors_param.blecfg == 1 &&
		     sensors_param.mqtten  && 
			 (timersrc%sensors_param.mqttts == 0)
			) 
		{
			char topic[10];
			char payload[16];
			for (uint8_t i=0;i<bleSens;i++)
			{
				if (BleHubData[i].type != NULL) 
				{
					memset(topic, 0, 10);
					memset(payload, 0, 16);
					
					sprintf(topic,"bt%dtime",i+1);
					sprintf(payload,"%d",TIMESENDBT(i)); //BleHubData[i].time);
					MQTT_Publish(topic, payload, strlen(payload), 2, 0, 0);
					
					#if BLE_MQTT_SEND_MAC == 1 
					char ble_mac[6];
					memcpy(ble_mac, sensors_param.bleDev+i, 6);
					memset(payload, 0, 16);
					
					for (uint8_t k = 0; k<6;k++)
					{
						char t[2];
						sprintf(t, "%02X", ble_mac[k]);
						if ( k == 0 )
							strcpy(payload, t);
						else
							strcat(payload, t);
					}
					ESP_LOGW("BLE", "mac%d: %s", i+1, payload);	
					memset(topic, 0, 10);
					sprintf(topic,"bt%dmac",i+1);
					MQTT_Publish(topic, payload, strlen(payload), 2, 0, 0);
					#endif
				}
			} 
		}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void webfunc(char *pbuf) {

}