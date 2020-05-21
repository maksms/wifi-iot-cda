#define STAIRS_COUNT 12
#define STAIRS_TOP 6
#define STAIRS_BOTTOM 6

#define GPIO_STAIRS_BOTTOM_START 220
#define GPIO_STAIRS_TOP_START 224

#define GPIO_SENSOR_1 4	//D6
#define GPIO_TRIG_1	  1
	
#define GPIO_SENSOR_2 5 //D2
#define GPIO_TRIG_2	  3

#define GPIO_SWITCH_STATE_MANUAL_MODE 16
#define GPIO_SWITCH_MODE 6

#define SYSTEM_STARTED_DELAY 5000

#define DELAY_ON_NEXT 650				// задержка включения/выключения последующих ступеней по направлению движения - sensors_param.cfgdes[2];
#define DELAY_ON_FIRST 50				// задержка включения первой ступени по направлению движения - sensors_param.cfgdes[1];
#define DELAY_LIGHT 4000				// длительность свечения ступени - sensors_param.cfgdes[0];
#define DELAY_BLOCK_RESET 3000			// задержка снятия блокировки - sensors_param.cfgdes[4]; 
#define DELAY_IGNORE 1500				// длительность игнорирования изменения состояния сенсоров - sensors_param.cfgdes[3];

#define VER_CODE 45

static volatile os_timer_t timer_system_start;  			// таймер задержки начала работы кода подсветки
static volatile os_timer_t timer_read_sensors;  			// таймер чтения данных с сенсоров
static volatile os_timer_t timer_read_sonar[2];  			// таймер чтения данных с сенсоров
static volatile os_timer_t timer_block_reset;  				// таймер сброса блокировки для включения подсветки
static volatile os_timer_t timer_sensor_ignore[2];  		// таймеры сброса флага игнорирования чтения данных с сенсоров
static volatile os_timer_t timer_process_lights;  			// таймер управления состоянием подсветки
static volatile os_timer_t timer_stairs[STAIRS_COUNT];      // таймеры включения/выключения подсветки ступенек

static volatile uint8_t gpio_stairs[STAIRS_COUNT];			// массив номеров gpio ступенек

//******* options ****
static volatile uint32_t delay_on_next;  	// мсек, задержка включения/выключения ступеньки sensors_param.cfgdes[2];
static volatile uint32_t delay_on_first;  	// мсек, задержка включения первой ступеньки  по направлению sensors_param.cfgdes[1];
static volatile uint32_t light_time;    	// мсек, время подсветки ступеньки   sensors_param.cfgdes[0];
static volatile uint32_t ignore_time;  		// мсек, время игнорирования чтения данных с датчиков   sensors_param.cfgdes[3];
static volatile uint32_t delay_block;  	// мсек, задержка начала выключения ступеней после включения первой   sensors_param.cfgdes[4];


static volatile uint8_t direction = 2;
static volatile uint8_t mode = 0;	// 0 - manual, 1 - auto	   переключение через gpio6    отправка по mqtt valdes[0]
static volatile uint8_t state = 0;  // 0 - выкл, 1 - вкл - для mode == manual, переключение через gpio16 
static volatile uint8_t prev_state = 0;  
static volatile uint8_t block = 0;  // 0 - не блокировано - можно вкл/выключать, 1 - блокировано, нельзя вкл / выкл 
static volatile uint8_t sensor_ignore[2] = {0, 0};  // 0 - не блокировано - можно вкл/выключать, 1 - блокировано, нельзя вкл / выкл 
							// используется только для mode = auto
							
static volatile uint8_t sensor1 = 0;  // 0 - нет сигнала, 1 - есть сигнал - для тестов через gpio4 (output)
static volatile uint8_t sensor1_prev = 0;  // 0 - нет сигнала, 1 - есть сигнал - для тестов через gpio4 (output)
static volatile int8_t sensor1_count = 0;  // 0 - нет сигнала, 1 - есть сигнал - для тестов через gpio12 (output)
static volatile uint8_t sensor2 = 0;  // 0 - нет сигнала, 1 - есть сигнал - для тестов через gpio12 (output)
static volatile uint8_t sensor2_prev = 0;  // 0 - нет сигнала, 1 - есть сигнал - для тестов через gpio12 (output)
static volatile int8_t sensor2_count = 0;  // 0 - нет сигнала, 1 - есть сигнал - для тестов через gpio12 (output)

static volatile uint32_t distance_1, distance_2;
static volatile uint32_t duration_1, duration_2;

MQTT_Client* client;
char payload[20];



void ICACHE_FLASH_ATTR load_options() {

	light_time = sensors_param.cfgdes[0];
	delay_on_next = sensors_param.cfgdes[2];	
	delay_on_first = sensors_param.cfgdes[1];
	ignore_time = sensors_param.cfgdes[3];
	delay_block = sensors_param.cfgdes[4];
	

	if ( light_time < 1 || light_time > 30000) light_time = DELAY_LIGHT;
	if ( ignore_time < 1 || ignore_time > 30000) ignore_time = DELAY_IGNORE;
	if ( delay_on_next < 1 || delay_on_next > 10000) delay_on_next = delay_on_next;
	if ( delay_on_first < 1 || delay_on_first > 10000) delay_on_first = DELAY_ON_FIRST;	
	if ( delay_block < 1 || delay_block > 10000) delay_block = DELAY_BLOCK_RESET;	
}

uint32_t millis() {
		return micros() / 1000;
}


uint32_t pulseIn(uint8_t pin, uint8_t level)
{
  uint8_t i = 0;
  uint32_t start, startImp, finishImp, res;
  start =  micros(); //millis
  res = 1000;
  startImp =  micros();
  finishImp =  micros();
  
  do {
    if (digitalRead(pin)==level){
      i = 1;
      startImp =  micros();
    }
  } while((i==0)&&( (micros() - start)<50*1000));
  
  i = 0;
  do {
    if (digitalRead(pin)!=level){
      i = 1;
      finishImp =  micros();
	  res = finishImp - startImp;
	  if ( res > 30*1000 ) return 10000;
    }
  //} while((i==0)&&((millis() - start)<1000));
  } while((i==0)&&((micros() - start)<300*1000));
 
  return res;
}



void ICACHE_FLASH_ATTR hc_sr_read_1() {
	// посылаем с промежутком 50 миллисекунд импульсы длительностью 10 микросекунд для запуска внутреннего микроконтроллера датчика HC-SR04
	digitalWrite(GPIO_TRIG_1, 0);
	delayMicroseconds(2);
	digitalWrite(GPIO_TRIG_1, 1);
	delayMicroseconds(10);
	digitalWrite(GPIO_TRIG_1, 0);
	duration_1 = pulseIn( GPIO_SENSOR_1, 1);
	distance_1 = duration_1 / 58.2;	
}

void ICACHE_FLASH_ATTR hc_sr_read_2() {
	// посылаем с промежутком 50 миллисекунд импульсы длительностью 10 микросекунд для запуска внутреннего микроконтроллера датчика HC-SR04
	digitalWrite(GPIO_TRIG_2, 0);
	delayMicroseconds(2);
	digitalWrite(GPIO_TRIG_2, 1);
	delayMicroseconds(10);
	digitalWrite(GPIO_TRIG_2, 0);
	duration_2 = pulseIn( GPIO_SENSOR_2, 1);
	distance_2 = duration_2 / 58.2;	 // *  0.393701
}

void ICACHE_FLASH_ATTR hc_sr_read_cb(uint8_t i) {
	switch (i) {
		case 1: hc_sr_read_1(); break;
		case 2: hc_sr_read_2(); break;
	}
}

void ICACHE_FLASH_ATTR init_stairs() {
	uint8_t i;
	for (i = 0;  i < STAIRS_COUNT; i++) {
		if ( i < STAIRS_BOTTOM )
			gpio_stairs[i] = GPIO_STAIRS_BOTTOM_START + i;
		else
			gpio_stairs[i] = GPIO_STAIRS_TOP_START + i - STAIRS_BOTTOM;
	}	
}

void ICACHE_FLASH_ATTR get_mode() {
	if ( GPIO_ALL_GET(GPIO_SWITCH_MODE) == 1) { 
		GPIO_ALL(GPIO_SWITCH_MODE, 0);
		mode++;
		mode = mode&1;		
		
		os_sprintf(payload,"%d", mode);
		MQTT_Publish(client, "stairs_mode", payload, os_strlen(payload), 2, 0, 1);
	}
}

void ICACHE_FLASH_ATTR block_reset_cb() {
	block = 0;
}

void ICACHE_FLASH_ATTR ignore_reset_cb(uint8_t i) {
	sensor_ignore[i] = 0;
}

void ICACHE_FLASH_ATTR do_light_off(uint8_t stair_num) {
	GPIO_ALL(gpio_stairs[stair_num], 0); 
	os_timer_disarm(&timer_stairs[stair_num]);	
}

void ICACHE_FLASH_ATTR do_light_on(uint8_t  stair_num) {
	GPIO_ALL(gpio_stairs[stair_num], 1);
	os_timer_disarm(&timer_stairs[stair_num]);	
	if ( mode ) { //AUTO
		// выставим таймер на выключение
		os_timer_disarm(&timer_stairs[stair_num]);
		os_timer_setfn(&timer_stairs[stair_num], (os_timer_func_t *) do_light_off, stair_num);
		os_timer_arm(&timer_stairs[stair_num], light_time, 0); 
	}	
}

void ICACHE_FLASH_ATTR toggle_stair(uint8_t __stair_num, uint8_t __state, uint32_t __delay) {
	if ( __state ) {
		os_timer_disarm(&timer_stairs[__stair_num]);
		os_timer_setfn(&timer_stairs[__stair_num], (os_timer_func_t *) do_light_on, __stair_num);
		os_timer_arm(&timer_stairs[__stair_num], __delay, 0);  		
	} else {
		os_timer_disarm(&timer_stairs[__stair_num]);
		os_timer_setfn(&timer_stairs[__stair_num], (os_timer_func_t *) do_light_off, __stair_num);
		os_timer_arm(&timer_stairs[__stair_num], __delay, 0); 		
	}
}

void ICACHE_FLASH_ATTR turn_on_from_bottom_to_top() {
	uint32_t _delay;
	uint8_t i;
	block = 1;
	for ( i = 0; i < STAIRS_COUNT; i++ ) 
	{
		_delay = (i == 0) ? delay_on_first : delay_on_next * i;
		toggle_stair(i, 1, _delay); 
	}	
}

void ICACHE_FLASH_ATTR turn_on_from_top_to_bottom() {
	uint32_t _delay;
	uint8_t i;
	block = 1;
	for ( i = STAIRS_COUNT; i > 0; i--) 
	{
		_delay = (i == STAIRS_COUNT) ? delay_on_first : delay_on_next * (STAIRS_COUNT - i);
		toggle_stair(i-1, 1, _delay);		
	}	
}

void ICACHE_FLASH_ATTR turn_off_from_bottom_to_top() {
	uint32_t _delay;
	uint8_t i;
	if ( block ) return;
	for ( i = 0; i < STAIRS_COUNT; i++ ) 
	{
		_delay = (i == 0) ? delay_on_first : delay_on_next * i;
		toggle_stair(i, 0, _delay);  
	}		
}

void ICACHE_FLASH_ATTR turn_off_from_top_to_bottom() {
	uint32_t _delay;
	uint8_t i;
	if ( block ) return;
	for ( i = (STAIRS_COUNT - 1); i >= 0; i--) 
	{
		_delay = (i == 0) ? delay_on_first : delay_on_next * i;
		toggle_stair(i, 0, _delay);
	}		
}

void ICACHE_FLASH_ATTR turn_on_to_center() {
	uint32_t _delay;
	uint8_t i, j;
	block = 1;
	for ( i = 0; i < STAIRS_BOTTOM; i++  ) 
	{
		_delay = (i == 0) ? delay_on_first : delay_on_next * i;
		j = STAIRS_COUNT - i - 1;		
		toggle_stair(i, 1, _delay);
		toggle_stair(j, 1, _delay);
	}	
}

void ICACHE_FLASH_ATTR turn_off_to_center() {
	uint32_t _delay;
	uint8_t i, j;
	block = 1;
	for ( i = 0; i < STAIRS_BOTTOM; i++  ) 
	{
		_delay = (i == 0) ? delay_on_first : delay_on_next * i;
		j = STAIRS_COUNT - i - 1;		
		toggle_stair(i, 0, _delay);
		toggle_stair(j, 0, _delay);
	}	
}

void ICACHE_FLASH_ATTR turn_on_from_center() {
	uint32_t _delay;
	uint8_t i, j, k;
	if ( block ) return;
	for ( i = 0 ; i < STAIRS_BOTTOM; i++  ) 
	{
		_delay = (i == 0) ? delay_on_first : delay_on_next * i;
		k = STAIRS_BOTTOM - i - 1;
		j = STAIRS_BOTTOM + i;
		toggle_stair(k, 1, _delay);
		toggle_stair(j, 1, _delay);
	}	
}

void ICACHE_FLASH_ATTR turn_off_from_center() {
	uint32_t _delay;
	uint8_t i, j, k;
	if ( block ) return;
	for ( i = 0 ; i < STAIRS_BOTTOM; i++  ) 
	{
		_delay = (i == 0) ? delay_on_first : delay_on_next * i;
		k = STAIRS_BOTTOM - i - 1;
		j = STAIRS_BOTTOM + i;
		toggle_stair(k, 0, _delay);
		toggle_stair(j, 0, _delay);
	}	
}


void ICACHE_FLASH_ATTR turn_on_stairs(uint8_t __direction) {
	block = 1;
	
	uint32_t _delay;
	uint8_t i;
		
	switch ( __direction ) {
		case 0:
			turn_on_from_top_to_bottom();
			break;
		case 1:
			turn_on_from_bottom_to_top();
			break;
		case 2:
			turn_on_to_center();
			break;
		case 3:
			turn_on_from_center();
			break;
		
	}
}

void ICACHE_FLASH_ATTR turn_off_stairs(uint32_t __direction) {
	if ( block ) return;
	
	uint32_t _delay;
	uint8_t i;
		
	switch ( __direction ) {
		case 0:
			turn_off_from_top_to_bottom();
			break;
		case 1:
			turn_off_from_bottom_to_top();
			break;
		case 2:
			turn_off_to_center();
			break;
		case 3:
			turn_off_from_center();
			break;
		
	}
}


void ICACHE_FLASH_ATTR start_timer_block_reset() {
	os_timer_disarm(&timer_block_reset);
	os_timer_setfn(&timer_block_reset, (os_timer_func_t *) block_reset_cb, NULL);
	os_timer_arm(&timer_block_reset, delay_block, 0);	
}

void ICACHE_FLASH_ATTR start_timer_ignore_reset(uint8_t i) {
	os_timer_disarm(&timer_sensor_ignore[i]);
	os_timer_setfn(&timer_sensor_ignore[i], (os_timer_func_t *) ignore_reset_cb, i);
	os_timer_arm(&timer_sensor_ignore[i], ignore_time, 0);	
}

void ICACHE_FLASH_ATTR process_lights_callback() {
		if ( mode ) {
			if ( !block && (sensor1 || sensor2) )
			{
				// ВКЛ, хоть один сенсор сработал
				if ( (sensor1 != sensor1_prev ) && sensor1) {					
					direction = 1;
					sensor_ignore[0] = 1;
					start_timer_ignore_reset(0);	
					turn_on_stairs( direction );
					sensor1_prev = sensor1;
				} else 
				if ((sensor2 != sensor2_prev ) && sensor2) {					
					sensor_ignore[1] = 1;
					start_timer_ignore_reset(1);					
					direction = 0;
					turn_on_stairs( direction );
					sensor2_prev = sensor2;
				} else {
					direction = 2;
					turn_on_stairs( direction );
				}
			} else {
				// ВЫКЛ, все сенсоры отключились	
				if ( sensor1 != sensor1_prev ) {
						start_timer_block_reset();
						direction = 1;						
						sensor1_prev = sensor1;
				}	else	
				if ( sensor2 != sensor2_prev ) {
						start_timer_block_reset();
						direction = 0;
						sensor2_prev = sensor2;
				} else {
					direction = 3;
				}
			}

		} else {
			if ( state != prev_state ) {
				if (state) {
					block = 1;
					direction = 2;
					turn_on_stairs( direction );
				} else {
					block = 0;
					direction = 3;
					turn_off_stairs( direction );
				}
				prev_state = state;
			}
		}
	
	
}

void ICACHE_FLASH_ATTR read_sensors_callback() {
	get_mode();
	if ( mode ) {
		uint8_t tmp1, tmp2;
		//tmp1 = GPIO_ALL_GET(GPIO_SENSOR_1);
		//tmp2 = GPIO_ALL_GET(GPIO_SENSOR_2);
		
		tmp1 = distance_1 < 20;
		tmp2 = distance_2 < 20;
		if ( tmp1 != sensor1 ) { // изменилось состояние
			if ( !sensor_ignore[0]) { 
				sensor1 = tmp1; 
				sensor1_count = 0;
			} else { 
				sensor1_count++; 
			}
		}
		
		if ( tmp2 != sensor2 ) { // изменилось состояние
			if ( !sensor_ignore[1]) { 
				sensor2 = tmp2; 
				sensor2_count = 0;
			} else { 
				sensor2_count++; 
			}
		}		

	} else {
		state = GPIO_ALL_GET(GPIO_SWITCH_STATE_MANUAL_MODE);
	}

}

void ICACHE_FLASH_ATTR system_started_callback(void) {
	init_stairs();
	
	state = 1;
	block = 0;
	mode = 0;
	
	load_options();
	
	os_timer_setfn(&timer_read_sensors, (os_timer_func_t *) read_sensors_callback, NULL);
	os_timer_arm(&timer_read_sensors, 100, 1);  //каждые 50 мсек читаем сенсоры
	
	
	os_timer_disarm(&timer_system_start);

	
	os_timer_disarm(&timer_process_lights);
	os_timer_setfn(&timer_process_lights, (os_timer_func_t *) process_lights_callback, NULL);
	os_timer_arm(&timer_process_lights, 100, 1);  

	os_timer_disarm(&timer_read_sonar[0]);
	os_timer_setfn(&timer_read_sonar[0], (os_timer_func_t *) hc_sr_read_cb, 1);
	os_timer_arm(&timer_read_sonar[0], 300, 1);  

	os_timer_disarm(&timer_read_sonar[1]);
	os_timer_setfn(&timer_read_sonar[1], (os_timer_func_t *) hc_sr_read_cb, 2);
	os_timer_arm(&timer_read_sonar[1], 300, 1);  
	
	client = (MQTT_Client*) &mqttClient;
	os_sprintf(payload,"%d", mode);
	MQTT_Publish(client, "stairs_mode", payload, os_strlen(payload), 2, 0, 1);	
}


void ICACHE_FLASH_ATTR startfunc(){
// выполняется один раз при старте модуля.
	os_timer_setfn(&timer_system_start, (os_timer_func_t *) system_started_callback, NULL);
	os_timer_arm(&timer_system_start, SYSTEM_STARTED_DELAY, 0);  //начинаем все через 10 сек после старта
}


void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {

	if(timersrc%5==0){
		// выполнение кода каждые 5 секунд
		load_options();
	}
	
	if(timersrc%30==0){
	// выполнение кода каждые 30 секунд
		//save_options();
	}
	
	


}

void webfunc(char *pbuf) {
   
   os_sprintf(HTTPBUFF,"<b>distance_1:</b>%d см \t duration_1: %d микросек<br>", distance_1, duration_1); 
   os_sprintf(HTTPBUFF,"<b>distance_2:</b>%d см \t duration_1: %d микросек<br>", distance_2, duration_2); 
   os_sprintf(HTTPBUFF,"<b>Статус:</b><br>"); 
   os_sprintf(HTTPBUFF,"Режим работы: <b>%s</b> \t Состояние: <b>%s</b> <br>\t Блокировка: <b>%s</b>", mode ? "АВТО" : "РУЧНОЙ" , state ? "ВКЛ" : "ВЫКЛ", block ? "ДА" : "НЕТ"); 

   os_sprintf(HTTPBUFF,"<br>Датчик (верх): <b>%s</b> \t Датчик (низ): <b>%s</b>", sensor1 ? "ДА" : "НЕТ" , sensor2 ? "ДА" : "НЕТ"); 
   
   os_sprintf(HTTPBUFF,"<br><b>Конфигурация:</b>"); 
   os_sprintf(HTTPBUFF,"<br>Длительность подсветки: %d мсек \t (cfgdes[0])", light_time); 
   os_sprintf(HTTPBUFF,"<br>Задержка вкл. 1ой ступени: %d мсек \t (cfgdes[1])", delay_on_first); 
   os_sprintf(HTTPBUFF,"<br>Задержка вкл. ступеней: %d мсек \t (cfgdes[2])", delay_on_next); 
   os_sprintf(HTTPBUFF,"<br>Длит. игнор. сенсоров: %d мсек \t (cfgdes[3])", ignore_time); 
   os_sprintf(HTTPBUFF,"<br>Задержка блокировки: %d мсек \t (cfgdes[4])", delay_block); 
   

   os_sprintf(HTTPBUFF,"<br>Sensor1 count: %d", sensor1_count); // вывод данных на главной модуля
   os_sprintf(HTTPBUFF,"<br>Sensor2 count: %d", sensor2_count); // вывод данных на главной модуля
    switch (direction) {
	case 0: os_sprintf(HTTPBUFF,"<br>Направление: %s \t ", "0 - сверху вниз"); // вывод данных на главной модуля
			break;
	case 1: os_sprintf(HTTPBUFF,"<br>Направление: %s \t ", "1 - снизу вверх"); // вывод данных на главной модуля
			break;
	case 2: os_sprintf(HTTPBUFF,"<br>Направление: %s \t ", "2 - в центр"); // вывод данных на главной модуля
			break;
	case 3: os_sprintf(HTTPBUFF,"<br>Направление: %s \t ", "3 - из центра"); // вывод данных на главной модуля
			break;
   }  
   
   os_sprintf(HTTPBUFF,"<br>Версия: <b>%d</b>", VER_CODE ); // вывод данных на главной модуля
}