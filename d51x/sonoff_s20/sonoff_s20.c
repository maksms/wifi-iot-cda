#define LED_BLUE 13
#define RELAY_AND_LED_RED 12
#define THERM_NUMBER 1

static os_timer_t esp_timer;
uint8_t last_button_state;		// предыдущее состояние кнопки
uint8_t last_term_state;		// предыдущее состояние термостата

MQTT_Client* client = (MQTT_Client*) &mqttClient;
char payload[20];

void ICACHE_FLASH_ATTR read_gpio()
{
	uint8_t state = GPIO_ALL_GET(LED_BLUE); 
	if ( (last_button_state != state)) { // нажимали?
		last_button_state = state;			// запомнили новое состояние
		sensors_param.termo[THERM_NUMBER-1][0]= state;	// включили/выключили термостат
		
		os_sprintf(payload,"%d", state);
		MQTT_Publish(client, "thermo_en1", payload, os_strlen(payload), 2, 0, 1);

		if ( state ) GPIO_ALL(RELAY_AND_LED_RED, 0);
	}
	
	// термостат могли выключить и не кнопкой, а через веб-интерфейс
	state = sensors_param.termo[THERM_NUMBER-1][0];
	if ( state != last_term_state ) {
		last_term_state = state;
		GPIO_ALL(LED_BLUE,state);	// зажгли/погасили синий светодиод
		if ( state == 0 ) GPIO_ALL(RELAY_AND_LED_RED, 1); // если выключили термостат, включим реле, чтобы было питание, и конвектор сам управлял температурой
	}
			
		
		

}

void ICACHE_FLASH_ATTR
startfunc(){
// выполняется один раз при старте модуля.

last_button_state = GPIO_ALL_GET(LED_BLUE);
last_term_state=sensors_param.termo[THERM_NUMBER-1][0];
GPIO_ALL(LED_BLUE, last_term_state);	
if (last_term_state == 0) GPIO_ALL(RELAY_AND_LED_RED, 1);


os_timer_disarm(&esp_timer);
os_timer_setfn(&esp_timer, (os_timer_func_t *) read_gpio, NULL);
os_timer_arm(&esp_timer, 500, 1);

}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
// выполнение кода каждую 1 секунду


if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}
}

void webfunc(char *pbuf) {
//os_sprintf(HTTPBUFF,"<br>test"); // вывод данных на главной модуля
os_sprintf(HTTPBUFF,"<br><b>Термостат:</b> %s", sensors_param.termo[THERM_NUMBER-1][0] ? "ВКЛ" : "ВЫКЛ"); // вывод данных на главной модуля
}
