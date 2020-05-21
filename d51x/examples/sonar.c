#define GPIO_SENSOR_1 4	// D6
#define GPIO_TRIG_1	  1 // TX
#define GPIO_PUMP 15	
#define LEVEL_TANK_MIN 10 // 10 см

#define LEVEL_TANK_MAX 90 // 90 см

#define SYSTEM_STARTED_DELAY 30000
#define SONAR_READ_DELAY 200

static volatile os_timer_t timer_system_start;  			// таймер задержки начала работы кода подсветки
static volatile os_timer_t timer_read_sonar;  			// таймер чтения данных с сенсоров

static volatile uint32_t distance_1;
static volatile uint32_t duration_1;

uint16_t level_min, level_max;

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

void ICACHE_FLASH_ATTR hc_sr_read_cb(uint8_t i) {
	switch (i) {
		case 1: hc_sr_read_1(); break;
	}
}

void ICACHE_FLASH_ATTR system_started_callback(void) {
	os_timer_disarm(&timer_read_sonar);
	os_timer_setfn(&timer_read_sonar, (os_timer_func_t *) hc_sr_read_cb, 1);
	os_timer_arm(&timer_read_sonar, SONAR_READ_DELAY, 1);  
}

void ICACHE_FLASH_ATTR startfunc(){
// выполняется один раз при старте модуля.
	os_timer_setfn(&timer_system_start, (os_timer_func_t *) system_started_callback, NULL);
	os_timer_arm(&timer_system_start, SYSTEM_STARTED_DELAY, 0);  //начинаем все через 10 сек после старта
}

void get_options() {

    level_min = (sensors_param.cfgdes[0] > LEVEL_TANK_MAX ) ? LEVEL_TANK_MIN : sensors_param.cfgdes[0];
    level_max = (sensors_param.cfgdes[1] < level_min ) ? LEVEL_TANK_MAX : sensors_param.cfgdes[1];
}


void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {

    get_options();

	if(timersrc%5==0){
		// выполнение кода каждые 5 секунд

        if (distance_1 < level_min )  digitalWrite(GPIO_PUMP, 1);
        else if ( distance_1 > level_max ) digitalWrite(GPIO_PUMP, 0);
	}
	
	if(timersrc%30==0){
	// выполнение кода каждые 30 секунд
	}
	
}

void webfunc(char *pbuf) {

   os_sprintf(HTTPBUFF,"<b>distance_1:</b>%d см \t duration_1: %d микросек<br>", distance_1, duration_1); 

}