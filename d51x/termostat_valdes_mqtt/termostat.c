#define LED_BLUE 15
#define RELAY_AND_LED_RED 12
#define THERM_NUMBER 1
#define FW_VER "2.20"

#define LONG_PRESS_GPIO 6

#define READ_STATE_DELAY 50

/*
	для mqtt лучше включить разделные топики на запись
	кол-во переменных: 4
	кол-во настроек: 3 (Термостат состояние,Уставка x10, Гистерезис х10)
	1. Термостат:	0 - выкл, > 0 - вкл
	2. Уставка x10
	3. Гистерезис x10
	
	мин температура  = Уставка-Гистерезис
	макс температура = Уставка+Гистерезис
*/

// 2.20 переделка под термоголовку (убрал определение состояние нагрева по расходу тока) + текущая температура с любого датчика через interpreter/mqtt/get в valdes4 значение x10
// установка через interpreter:	valdesset(3,d2d[0][0])   - установили значение из _D2D0101_ - модуль 1 датчик 1
// GPIO_Key - долгое нажатие (GPIO6) включает-выключает термостат
// 2.10 переделка кода под GPIO_Keys и лампочку состояни термостата

#define THERMO_STATE_SAVE	sensors_param.cfgdes[0]
#define THERMO_SETPOINT_SAVE	sensors_param.cfgdes[1]
#define THERMO_HYSTERESIS_SAVE	sensors_param.cfgdes[2]

#if termoe
#define THERMO_STATE		sensors_param.termo[THERM_NUMBER-1][0]
#define THERMO_HYSTERESIS	sensors_param.termozn[THERM_NUMBER-1][1]-sensors_param.termozn[THERM_NUMBER-1][0]
#define THERMO_SETPOINT 	sensors_param.termozn[THERM_NUMBER-1][0
#else
#define THERMO_STATE		valdes[0]		
#define THERMO_HYSTERESIS	valdes[2]		
#define THERMO_SETPOINT 	valdes[1]		
#endif

#define THERMO_TEMP 		valdes[3]		// текущая температура, можно выставить либо по mqtt, либо через interpreter с любого датчика, в т.ч. d2d

#if termoe
#define MQTT_TERM_STATE 	"thermo_en1"
#define MQTT_TERM_SETPOINT 	"thermo_set1"
#define MQTT_TERM_HYST 		"thermo_hyst1"
#else
#define MQTT_TERM_STATE 	"valuedes0" 
#define MQTT_TERM_SETPOINT 	"valuedes1" 
#define MQTT_TERM_HYST 		"valuedes2" 
#endif

int state = 0;
int term_set = 0;
int term_hyst = 0;

#define THERMO_TEMP_MIN term_set-term_hyst
#define THERMO_TEMP_MAX term_set+term_hyst


void ICACHE_FLASH_ATTR load_config()
{
	THERMO_STATE = THERMO_STATE_SAVE;
	THERMO_SETPOINT = THERMO_SETPOINT_SAVE;
	THERMO_HYSTERESIS = THERMO_HYSTERESIS_SAVE;
	
	state = THERMO_STATE;
	term_set = THERMO_SETPOINT_SAVE;
	term_hyst = THERMO_HYSTERESIS;
}

void ICACHE_FLASH_ATTR term_send_mqtt(const char *topic, int value)
{
#if mqtte
	if ( !sensors_param.mqtten ) return;
	char payload[20];
	os_sprintf(payload,"%d", value);
	MQTT_Publish(&mqttClient, topic, payload, os_strlen(payload), 2, 0, 1);	
#endif
}


void ICACHE_FLASH_ATTR toggle_termo()
{
	state = !state;
	GPIO_ALL_M(RELAY_AND_LED_RED, 1); 
	
	THERMO_STATE_SAVE = state;
	THERMO_STATE = state;
	term_send_mqtt(MQTT_TERM_STATE, state);	
	SAVEOPT;			
	
	GPIO_ALL_M(LONG_PRESS_GPIO, 0); 	// сбросим флаг	
}

void ICACHE_FLASH_ATTR save_config()
{

	if ( THERMO_STATE_SAVE != state || THERMO_SETPOINT_SAVE != term_set || THERMO_HYSTERESIS_SAVE != term_hyst ) {

		state = THERMO_STATE_SAVE;
		THERMO_STATE = state;
		term_send_mqtt(MQTT_TERM_STATE, state);	
		
		term_set = THERMO_SETPOINT_SAVE;
		THERMO_SETPOINT = term_set;
		term_send_mqtt(MQTT_TERM_SETPOINT, term_set);		
		
		term_hyst = THERMO_HYSTERESIS_SAVE;
		THERMO_HYSTERESIS = term_hyst;
		term_send_mqtt(MQTT_TERM_HYST, term_hyst);		
		
	}
	
	if ( THERMO_STATE != state || THERMO_SETPOINT != term_set || THERMO_HYSTERESIS != term_hyst )
	{
		state = THERMO_STATE;
		THERMO_STATE_SAVE = state;
		
		term_set = THERMO_SETPOINT;
		THERMO_SETPOINT_SAVE = term_set;

		term_hyst = THERMO_HYSTERESIS;
		THERMO_HYSTERESIS_SAVE = term_hyst;
		
	
		SAVEOPT;
	}
}

void ICACHE_FLASH_ATTR term_process()
{
	if ( !state ) return;
	if ( THERMO_TEMP < THERMO_TEMP_MIN )
	{
		// on
		GPIO_ALL_M(RELAY_AND_LED_RED, 1); 
	}
	else if ( THERMO_TEMP > THERMO_TEMP_MAX ) 
	{
		// off
		GPIO_ALL_M(RELAY_AND_LED_RED, 0); 
	}
}

void ICACHE_FLASH_ATTR startfunc(){
	// выполняется один раз при старте модуля.

 	load_config();
	
	GPIO_ALL(LED_BLUE, THERMO_STATE); 

}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	save_config();
	
	if ( GPIO_ALL_GET(LONG_PRESS_GPIO) == 1 )
	{
		toggle_termo();
	}	
	
	GPIO_ALL(LED_BLUE, state);
	
	if(timersrc%40==0)
	{
		// выполнение кода каждые 40 секунд
		term_process();
	}
}

void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br><b>Температура:</b> %d.%d °C", THERMO_TEMP / 10, THERMO_TEMP % 10); 
os_sprintf(HTTPBUFF,"<br><b>Термостат:</b> %s", state ? "ВКЛ" : "ВЫКЛ"); 
os_sprintf(HTTPBUFF,"<br><b>Уставка:</b> %d.%d °C", term_set / 10, term_set % 10); 
os_sprintf(HTTPBUFF,"<br><b>Гистерезис:</b> %d.%d °C", term_hyst / 10, term_hyst % 10); 
os_sprintf(HTTPBUFF,"<br><small><b>Версия прошивки:</b> %s</small>", FW_VER); 
}