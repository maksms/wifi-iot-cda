
uint32_t some_var;      // переменная, участвующая в логике, в которую вычитывается значение из опций, в котторую приходит значение из valdes
                        // из которой сохраняется значение в опцию, из которой сохраняется значение в valdes и оправляется по mqtt/http

#define MQTTD
#ifdef MQTTD
MQTT_Client* mqtt_client;
static char payload[20];
#endif


void ICACHE_FLASH_ATTR startfunc() {
    // выполняется один раз при старте модуля.
    #ifdef MQTTD
        mqtt_client = (MQTT_Client*) &mqttClient;
    #endif
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду
    uint32_t val = 0;    // временная переменная, в которую прочитаем значение из опции или valdes
    uint8_t need_save = 0;

    // сначала смотрим опцию, поменялось ли там значение
    val =  (sensors_param.cfgdes[0] > 10 || sensors_param.cfgdes[0] < 1) ? 10 : sensors_param.cfgdes[0];  // разлиные проверки, можно убрать
    if ( val != some_var ) {  // значение изменилось, в опции новое значение
        some_var = val;
        valdes[0] = some_var;
        // чтобы все корректно работало и значене не поменялось обратно через mqtt, нужно отправить новое значение
        #ifdef MQTTD
        if ( sensors_param.mqtten && mqtt_client != NULL) {
	        os_memset(payload, 0, 20);
	        os_sprintf(payload,"%d", val);
	        MQTT_Publish(mqtt_client, "valuedes0", payload, os_strlen(payload), 2, 0, 1);
	        os_delay_us(20);
        }
        #endif  
    }

    // теперь смотрим valdes, поменялось ли там значение
    val = ( valdes[0] > 10 ) ? 10 : valdes[0]; // различные проверки, можно убрать
    if ( val != some_var ) {
        // прилетело новое значение, сохранить
        some_var = val;
        sensors_param.cfgdes[0] = some_var;    // в опцию пропишем новое значени
        need_save = 1;                          // флаг, что надо сохранить изменени опции во флеш
    }

    if ( need_save ) SAVEOPT;
}