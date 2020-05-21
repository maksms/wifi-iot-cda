#define FW_VER "2.0.5"

#define GPIO_LED_STRIP 13
#define GPIO_REED_SW_1 4
#define GPIO_REED_SW_2 5
#define GPIO_PIR_1 12
#define GPIO_PIR_2 14
#define PWM_LED 0

#define GPIO_BTN_1 222
#define GPIO_BTN_2 223
#define GPIO_BTN_3 227

#define GPIO_LED_NIGHT 224

#define GPIO_LED_PIR_1 220
#define GPIO_LED_PIR_2 221

//#define GPIO_LED_GREEN 226
//#define GPIO_LED_RED 225

#define READ_SENSORS_INTERVAL 300
//#define READ_BUTTONS_INTERVAL 300

#define BRIGHTNESS_STEPS 20
#define LUMINOSITY_STEP 20
#define LIGHT_DELAY_STEP 5

#define MIN_LIGHT_DELAY 5
#define MAX_LIGHT_DELAY 50

#define PIR_DELAY_OFF 30000

MQTT_Client* client;
static char payload[20];
static uint8_t allow_mqtt_send = 0;

static uint8_t brightness_01[BRIGHTNESS_STEPS] = {0,1,2,3,4,5,7,9,12,16,21,28,37,48,64,84,111,147,194,255};  
									// 0-100%, логарифмическая, S1 = k * ln(R), k=18.04, R - значение ШИМ (0-255)

static uint8_t brightness_02[BRIGHTNESS_STEPS] = {0,1,2,3,4,7,11,16,23,31,42,54,69,87,107,130,156,186,219,255};  
									// 0-100%, степенная, S2 = k * R^n, k=16.06, n=0.33,  R - значение ШИМ (0-255)
									
static uint8_t brightness_03[BRIGHTNESS_STEPS] = {0,1,2,3,4,6,8,12,16,22,29,39,51,67,86,109,136,170,209,255};  
									// 0-100%, средняя, S=(S1+S2)/2
									
static uint8_t brightness_steps_value;
									
static uint8_t idx = 0;  // текущий индекс яркости 0-19
static uint8_t light_delay = 25; // msec, длительность свечения каждой ступени яркости
static uint8_t brightness_type = 1;  // 0 - простое включение, 1 - логарифмическая, 2 - степенная, 3 - апроксимированная по 1 и 2

static uint8_t door_state = 0;    // valdes[0]
static uint8_t prev_door_state = 0;

static uint8_t window_state = 0;    //valdes[1]
static uint8_t prev_window_state = 0;

static uint8_t pir1_state = 0;     // valdes[2]
static uint8_t prev_pir1_state = 0;
static uint8_t pir2_state = 0;    // valdes[3]
static uint8_t prev_pir2_state = 0;

static uint8_t pir_state = 0;  // общее состояние датчиков движения
static uint32_t vgpio = 255;  // vgpio для включения по датчикам движения
static uint8_t vgpio_state = 0;  // vgpio состояние локальное

static uint16_t luminosity = 0; // 0 - 1024
static uint16_t min_luminosity = 0; // 0 - 1024
static uint8_t prev_luminosity_state = 0;
static uint8_t allow_herkon1_turn_on_led = 1;

static uint8_t is_dark = 0;
static uint32_t pir_delay_off_time = 0;
static uint8_t pir_ignore_on = 0;
static uint8_t allow_save_opt = 0;

								
static volatile os_timer_t timer_read_sensors; 
static volatile os_timer_t timer_pwm; 
static volatile os_timer_t timer_pir_off; 




void ICACHE_FLASH_ATTR set_brightness(uint8_t state){
	uint8_t is_exit = 0;
	if ( state == 1 ) {
		// включаем
		idx++;
		//if (idx > BRIGHTNESS_STEPS-1 ) {
		if (idx > brightness_steps_value-1 ) {
			//idx = BRIGHTNESS_STEPS-1;
			idx = brightness_steps_value-1;
			is_exit = 1;
		}
	} else {
		// выключаем
		idx--;
		//if (idx > BRIGHTNESS_STEPS ) {
		if (idx > brightness_steps_value ) {
			idx = 0;
			is_exit = 1;
		}	
	}
	
	if ( is_exit == 1) {
			os_timer_disarm(&timer_pwm);
		return;
	}
	
	// выставляем яркость
	uint8_t tpwm;
	switch (brightness_type) {
		case 1: tpwm = brightness_01[idx]; break;
		case 2: tpwm = brightness_02[idx]; break;
		case 3: tpwm = brightness_03[idx]; break;
	}
	analogWrite(PWM_LED, tpwm);	
}

void ICACHE_FLASH_ATTR stop_led() {
	os_timer_disarm(&timer_pwm);		
}

void ICACHE_FLASH_ATTR start_led(uint8_t state) {
	stop_led();
	
	os_timer_setfn(&timer_pwm, (os_timer_func_t *) set_brightness, state);
	os_timer_arm(&timer_pwm, light_delay, 1);		
}

void ICACHE_FLASH_ATTR switch_led(uint8_t state){
	stop_led();	
	if ( (is_dark == 0) && (state == 1) ) return;
	if ( brightness_type == 0 ) {
		// выставляем яркость
		analogWrite(PWM_LED, (state == 1) ? 255 : 0);
	} else {
		start_led( state );	
	}	
}

void ICACHE_FLASH_ATTR process_door(){
	if (allow_herkon1_turn_on_led == 0) return;  // не реагировать на открытие двери (на геркон 1)
	if ( door_state == prev_door_state ) return; // состояние не изменилось, ничего не делаем

	switch_led( door_state);  // поджигаем/гасим ленту
	if ( allow_mqtt_send ) {
		os_sprintf(payload,"%d", door_state);
		MQTT_Publish(client, "reed-switch-1", payload, os_strlen(payload), 2, 0, 1);
	}
	prev_door_state = door_state;	
}


void ICACHE_FLASH_ATTR process_window(){
	if ( window_state == prev_window_state ) return; // состояние не изменилось, ничего не делаем
	if ( allow_mqtt_send ) {
		os_sprintf(payload,"%d", window_state);
		MQTT_Publish(client, "reed-switch-2", payload, os_strlen(payload), 2, 0, 1);
	}	
	prev_window_state = window_state;
}

void ICACHE_FLASH_ATTR process_pir1(){
	if ( pir1_state == prev_pir1_state ) return; // состояние не изменилось, ничего не делаем
	GPIO_ALL( GPIO_LED_PIR_1, pir1_state);
	if ( allow_mqtt_send ) {
		os_sprintf(payload,"%d", pir1_state);
		MQTT_Publish(client, "pir-1", payload, os_strlen(payload), 2, 0, 1);
	}		
	prev_pir1_state = pir1_state;	
}

void ICACHE_FLASH_ATTR process_pir2(){
	if ( pir2_state == prev_pir2_state ) return; // состояние не изменилось, ничего не делаем
	GPIO_ALL( GPIO_LED_PIR_2, pir2_state);
	if ( allow_mqtt_send ) {
		os_sprintf(payload,"%d", pir2_state);
		MQTT_Publish(client, "pir-2", payload, os_strlen(payload), 2, 0, 1);
	}		
	prev_pir2_state = pir2_state;		
}


void ICACHE_FLASH_ATTR vgpio_on() {
	if ( pir_ignore_on ) return;
	if ( is_dark == 0 ) return;
	GPIO_ALL( vgpio, 1);
}

void ICACHE_FLASH_ATTR vgpio_off() {
	GPIO_ALL( vgpio, 0);
}

void ICACHE_FLASH_ATTR process_vgpio_switch(uint8_t _state){
	if ( _state ) {
		// включаем vgpio
		vgpio_on();
		
		// перезапустить таймер на выключение
		os_timer_disarm(&timer_pir_off);	
		os_timer_setfn(&timer_pir_off, (os_timer_func_t *) vgpio_off, NULL);
		os_timer_arm(&timer_pir_off, pir_delay_off_time, 0);	
	}
}


void ICACHE_FLASH_ATTR process_night_mode(uint8_t _state) {
	GPIO_ALL(GPIO_LED_NIGHT, _state );
	if ( _state != prev_luminosity_state) {
		// состояние изменилось
		if ( allow_mqtt_send ) {
			os_sprintf(payload,"%d", _state);
			MQTT_Publish(client, "night-mode", payload, os_strlen(payload), 2, 0, 1);
		}	
		prev_luminosity_state = _state;
	}
}

void ICACHE_FLASH_ATTR read_sensors(){
	door_state = !GPIO_ALL_GET(GPIO_REED_SW_1);
	window_state = !GPIO_ALL_GET(GPIO_REED_SW_2);
	pir1_state = GPIO_ALL_GET(GPIO_PIR_1);
	pir2_state = GPIO_ALL_GET(GPIO_PIR_2);
	luminosity = analogRead();
	
	is_dark = min_luminosity > luminosity;
	
	pir_state = pir1_state || pir2_state; // хотя бы один датчик сработал
	process_vgpio_switch(pir2_state);
	
	// TODO: переделать на таймеры одиночные
	process_door();
	process_window();
	process_pir1();
	process_pir2();
	process_night_mode(is_dark);
}

void ICACHE_FLASH_ATTR load_options() {
	light_delay = sensors_param.cfgdes[0];
	brightness_type = sensors_param.cfgdes[1];
	if ( brightness_type > 3 ) brightness_type = 0;
	
	allow_herkon1_turn_on_led = sensors_param.cfgdes[2];

	brightness_steps_value = sensors_param.cfgdes[4];
	if (brightness_steps_value < 1 || brightness_steps_value > BRIGHTNESS_STEPS ) brightness_steps_value = BRIGHTNESS_STEPS;
	
	min_luminosity = sensors_param.cfgdes[5];
	if (min_luminosity > 1024 ) min_luminosity = 0;	
	
	pir_delay_off_time = sensors_param.cfgdes[7];
	if ( pir_delay_off_time > 120000 || 
	    ( pir_delay_off_time > 1 && pir_delay_off_time < PIR_DELAY_OFF)) 
	{ 
		pir_delay_off_time = PIR_DELAY_OFF;	
		pir_ignore_on = 0;
	}
		
	pir_ignore_on = pir_delay_off_time == 0;
	
	vgpio = sensors_param.cfgdes[8];
	if (vgpio < 20 || vgpio > 255 ) {
		vgpio = 255;	
		pir_ignore_on = 1;
	}
	
	allow_mqtt_send = sensors_param.mqtten;
}

void ICACHE_FLASH_ATTR save_options() {

	sensors_param.cfgdes[0] = light_delay;
	sensors_param.cfgdes[1] = brightness_type;
	sensors_param.cfgdes[2] = allow_herkon1_turn_on_led;
	sensors_param.cfgdes[4] = brightness_steps_value;
	sensors_param.cfgdes[5] = min_luminosity;
	sensors_param.cfgdes[7] = pir_delay_off_time;
	sensors_param.cfgdes[8] = (vgpio > 255 || vgpio < 20) ? 255 : vgpio;
	SAVEOPT
	
}

void ICACHE_FLASH_ATTR startfunc(){
	// выполняется один раз при старте модуля.
	client = (MQTT_Client*) &mqttClient;
	prev_door_state = 0;
	door_state = 0;
    pir_delay_off_time  = PIR_DELAY_OFF;
	
	load_options();
	
	uint8_t i;
	for (i=220; i < 228; i++ ) {
		GPIO_ALL(i, 0);
	}
		
	os_timer_disarm(&timer_read_sensors);
	os_timer_setfn(&timer_read_sensors, (os_timer_func_t *) read_sensors, NULL);
	os_timer_arm(&timer_read_sensors, READ_SENSORS_INTERVAL, 1);
	
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	if( timersrc % 10 == 0 ){  // каждые 10 сек
		load_options();
		
		os_sprintf(payload,"%d", is_dark);
		MQTT_Publish(client, "night-mode", payload, os_strlen(payload), 2, 0, 1);
	} 
}



void webfunc(char *pbuf) {
	char *str;
	
	uint8_t max_bright;
	switch ( brightness_type ) {
		case 1: max_bright = brightness_01[brightness_steps_value-1]; break;
		case 2: max_bright = brightness_02[brightness_steps_value-1]; break;
		case 3: max_bright = brightness_03[brightness_steps_value-1]; break;
	}
	os_sprintf(HTTPBUFF,"<small><b>Макс. яркость:</b> %d (%d)</small><br>", max_bright, brightness_steps_value); 
	os_sprintf(HTTPBUFF,"<small><b>Мин. освещенность:</b> %d</small><br>", min_luminosity ); 
	
	switch (brightness_type) {
		case 0: str = "постоянная"; break;
		case 1: str = "логарифмическая"; break;
		case 2: str = "степенная"; break;
		case 3: str = "усредненная"; break;
	}	
	os_sprintf(HTTPBUFF,"<small><b>Тип подсветки:</b> %s (%d)</small><br>", str, brightness_type); 
	os_sprintf(HTTPBUFF,"<small><b>Задержка выкл. LED:</b> %d мсек</small><br>", light_delay); 
	os_sprintf(HTTPBUFF,"<small><b>Реагировать на открытие двери:</b> %s</small><br>", allow_herkon1_turn_on_led ? "ДА" : "НЕТ"); 
	os_sprintf(HTTPBUFF,"<small><b>Задержка выкл. PIR:</b> %d сек</small>&nbsp;0 - игнор<br>", pir_delay_off_time / 1000); 
	os_sprintf(HTTPBUFF,"<b>Датчики:</b><br>"); 
	os_sprintf(HTTPBUFF,"<small><table>"); 
	os_sprintf(HTTPBUFF,"<tr><th><b>Дверь:</b></th><th><b>Окно:</b></th><th><b>Движение 1:</b></th><th><b>Движение 2:</b></th><th><b>Ночь:</b></th>"); 
	os_sprintf(HTTPBUFF,"<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td>", door_state ? "Открыта" : "Закрыта",
																			window_state ? "Открыто" : "Закрыто",
																			pir1_state ? "ДА" : "НЕТ", 
																			pir2_state ? "ДА" : "НЕТ", 
																			is_dark ? "ДА" : "НЕТ"); 
	os_sprintf(HTTPBUFF,"</table></small>"); 
	
	os_sprintf(HTTPBUFF,"<br><small><b>Версия прошивки:</b> %s</small><br>", FW_VER); 
	

}