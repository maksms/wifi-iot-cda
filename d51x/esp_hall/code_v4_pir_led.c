#define FW_VER "4.1"

// Настройки: PIR1 GPIO, active level, func type, func GPIO/CH,OFF delay, Signal GPIO, NightMode,
// Настройки: PIR2 GPIO, active level, func type, func GPIO/CH,OFF delay, Signal GPIO, NightMode,
// Настройки: DryCont1 GPIO, active level, func type, func GPIO/CH,OFF delay, Signal GPIO, NightMode,
// Настройки: DryCont2 GPIO, active level, func type, func GPIO/CH,OFF delay, Signal GPIO, NightMode,
// Настройки: Led Night GPIO, PwmStepSpeed

// Настройки: PIR1 GPIO, active level, func type, func GPIO/CH,OFF delay, Signal GPIO, NightMode,PIR2 GPIO, active level, func type, func GPIO/CH,OFF delay, Signal GPIO, NightMode,DryCont1 GPIO, active level, func type, func GPIO/CH,OFF delay, Signal GPIO, NightMode,DryCont2 GPIO, active level, func type, func GPIO/CH,OFF delay, Signal GPIO, NightMode, Led Night GPIO,PwmStepSpeed


/*
*   v.4 changes:
		* перевел в концепцию 4 сухих контакта с возможностью для каждого задать: 
			- gpio сухого контакта 
			- активный уровень gpio контакта
			- функцию, которую выполняет контакт:
						0 - вкл/выкл gpio
						1 - вкл/выкл pwm (плавно с текущего duty до указанного, см. ниже)
						2 - переключить gpio
			- номер gpio или канала pwm, который будет управляться сигналом с контакта
			- задержку выключения gpio или канала
			- контакт включает указанный gpio или канал pwm только в темноте (с датчика освещенности)
			- номер gpio для включения сигнального светодиода, что контакт сработал (при выключении задержка не используется, см. выше)
			
		* возможность задать мин и макс освещенность, при которой осуществляется определение темно/светло (задается в нтерпретере)
		* возможность задать время начала и окончания ночного режима, при котором яркость минимальна (время и значение яркости задается через интерпретер)
	
		//TODO: через конструктор строк задать конфигурацию сенсора ( t:g;v:221;d:1;s:220 )
*/


#define MQTT_TOPIC_NIGHT_MODE 			"night-mode"

#define NO_GPIO 255
//#define GPIO_LED_GREEN 226
//#define GPIO_LED_RED 225

#define READ_SENSORS_INTERVAL 20


#define CFG_SENSOR_FUNC_IDX				0
#define CFG_SENSOR_DATA_SIZE			7

#define CFG_PIR1_GPIO					sensors_param.cfgdes[0]			// 12		
#define CFG_PIR1_LEVEL 					sensors_param.cfgdes[1]			// 1	//уровень срабатывания
#define CFG_PIR1_FUNC					sensors_param.cfgdes[2]			// функция для PIR1, 0: GPIO, 1: PWM; 2: toggle GPIO
#define CFG_PIR1_PIN_CH					sensors_param.cfgdes[3]			// номер gpio или канала pwm
#define CFG_PIR1_DELAY					sensors_param.cfgdes[4]			// задержка выключения pir1 (запускается таймер)
#define CFG_PIR1_SIGNAL_GPIO			sensors_param.cfgdes[5]			// 220 // gpio для сигнализации сработки датчика
#define CFG_PIR1_NIGHT_MODE				sensors_param.cfgdes[6]			// вызов on-callback только в темноте


#define CFG_PIR2_GPIO					sensors_param.cfgdes[7]			// 14
#define CFG_PIR2_LEVEL					sensors_param.cfgdes[8]			// 1	//уровень срабатывания
#define CFG_PIR2_FUNC					sensors_param.cfgdes[9]			// функция для PIR1, 0: GPIO, 1: PWM
#define CFG_PIR2_PIN_CH					sensors_param.cfgdes[10]			// номер gpio или канала pwm
#define CFG_PIR2_DELAY					sensors_param.cfgdes[11]			// задержка выключения pir2 (запускается таймер)
#define CFG_PIR2_SIGNAL_GPIO			sensors_param.cfgdes[12]		// 221	// gpio для сигнализации сработки датчика
#define CFG_PIR2_NIGHT_MODE				sensors_param.cfgdes[13]			// вызов on-callback только в темноте

#define CFG_DRY1_GPIO					sensors_param.cfgdes[14] 		// 4
#define CFG_DRY1_LEVEL 					sensors_param.cfgdes[15] 		// 0	// уровень срабатывания
#define CFG_DRY1_FUNC					sensors_param.cfgdes[16]		// функция для DRY1, 0: GPIO, 1: PWM
#define CFG_DRY1_PIN_CH					sensors_param.cfgdes[17]		// номер gpio или канала pwm
#define CFG_DRY1_DELAY					sensors_param.cfgdes[18]		// задержка выключения DryContact1 (запускается таймер)
#define CFG_DRY1_SIGNAL_GPIO			sensors_param.cfgdes[19]		// 255 // gpio для сигнализации сработки датчика
#define CFG_DRY1_NIGHT_MODE				sensors_param.cfgdes[20]		// вызов on-callback только в темноте

#define CFG_DRY2_GPIO					sensors_param.cfgdes[21] 		// 5
#define CFG_DRY2_LEVEL					sensors_param.cfgdes[22] 		// 0	//уровень срабатывания
#define CFG_DRY2_FUNC					sensors_param.cfgdes[23]		// функция для DRY2, 0: GPIO, 1: PWM
#define CFG_DRY2_PIN_CH					sensors_param.cfgdes[24]		// номер gpio или канала pwm
#define CFG_DRY2_DELAY					sensors_param.cfgdes[25]		// задержка выключения DryContact2  (запускается таймер)
#define CFG_DRY2_SIGNAL_GPIO			sensors_param.cfgdes[26]		// 255 // gpio для сигнализации сработки датчика			
#define CFG_DRY2_NIGHT_MODE				sensors_param.cfgdes[27]		// вызов on-callback только в темноте

#define GPIO_LED_NIGHT					sensors_param.cfgdes[28]		//224	// gpio для сигнализации темного времени
#define CFG_BRIGTHNESS_STEP_DELAY		sensors_param.cfgdes[29]		// скорость шага pwm

#define VALDES_IS_DARK					valdes[0]						// выставляется через interpreter по гистерезису освещенности
#define VALDES_MIN_NIGHT_BRIGTHNESS		valdes[1]						// выставляется через interpreter по времени

//#define BRIGHTNESS_STEPS 20
//static uint8_t brightness[BRIGHTNESS_STEPS] = {0,1,2,3,4,5,7,9,12,16,21,28,37,48,64,84,111,147,194,255};  
#define BRIGHTNESS_STEPS 32
static uint8_t brightness[BRIGHTNESS_STEPS] = {0,1,2,3,5,8,12,16,21,26,32,38,45,52,60,68,76,85,95,105,115,125,136,148,160,172,185,198,211,225,239,255};

#define BRIGHTNESS_STEP_DELAY 20 
static uint8_t light_delay = BRIGHTNESS_STEP_DELAY; // msec, длительность свечения каждой ступени яркости

typedef void (* func_cb)(void *args);  

typedef enum {
	  USER_SENSOR_PIR1		// level = 1
	, USER_SENSOR_PIR2		// level = 1
	, USER_SENSOR_RSW1		// level = 0
	, USER_SENSOR_RSW2		// level = 0
	, USER_SENSOR_MAX
} user_sensor_type_e;

typedef enum {
	USER_FUNC_GPIO,
	USER_FUNC_PWM,
	USER_FUNC_TOGGLE_GPIO
} user_sensor_func_e;

typedef struct {
	uint8_t state;
	uint8_t gpio;
	uint8_t active_level;
	uint8_t signal_gpio;
	user_sensor_func_e func;		// тип функции сенсора - gpio или pwm
	uint16_t pin_ch;				// номер gpio или номер channel pwm

	char *topic;
	uint32_t tmr_delay;				// через какое время сработает таймер при закрытии контакта / пропадании сигнала
	func_cb tmr_cb;						// callback таймера
	void *tmr_args;						// аргументы коллбека
	func_cb on;						// callback при ON
	void *on_args;						// аргументы коллбека
	uint8_t dark;						// on-callback сработает только в темноте
	func_cb off;						// callback при OFF
	void *off_args;						// аргументы коллбека
	os_timer_t tmr;					// сам таймер задержки
	os_timer_t tmr_pwm; 			// таймер для pwm функции (func)

} contact_sensor_t;

contact_sensor_t _sensors[USER_SENSOR_MAX];

static volatile os_timer_t timer_read_sensors; 

static uint8_t is_dark = 0;			// флан ночного режима по датчbre освещенности, при 1 - включаем gpio/pwm

static void ICACHE_FLASH_ATTR mqtt_send_int(const char *topic, int val)
{
	#if mqtte
	if ( !sensors_param.mqtten ) return;		
	char payload[10];
	os_sprintf(payload,"%d", val);
	MQTT_Publish(&mqttClient, topic, payload, os_strlen(payload), 0, 0, 0);
	#endif
}

void ICACHE_FLASH_ATTR read_sensors_cb()
{
	// обработка состояния gpio
	// если состояние изменилось, то выполним коллбек в зависимости от состояния, а так же запустим таймер для выключенного состояния
	uint8_t i;
	for (i = 0; i < USER_SENSOR_MAX; i++ )
	{
		uint8_t state = GPIO_ALL_GET(_sensors[i].gpio);
		state = ( _sensors[i].active_level ) ? state : !state;
		if ( state == _sensors[i].state ) continue;  // состояние не менялось
	
		//uint8_t _state = ( _sensors[i].active_level ) ? state : !state;
		
		// состояние изменилось, выполнить callback
		if ( state ) 
		{
			if ( _sensors[i].on != NULL && ( !_sensors[i].dark || ( _sensors[i].dark && is_dark) ) )  // + включаем только в темноте, а выключаем в любое время
				_sensors[i].on( _sensors[i].on_args );
				
		} 
		else 
		{
			if ( _sensors[i].off != NULL )
				_sensors[i].off( _sensors[i].off_args );			
				
			if ( _sensors[i].tmr_delay > 0 && _sensors[i].tmr_cb != NULL )
			{
				// запустим таймер
				os_timer_disarm(&_sensors[i].tmr);		
				os_timer_setfn(&_sensors[i].tmr, (os_timer_func_t *) _sensors[i].tmr_cb, _sensors[i].tmr_args);
				os_timer_arm(&_sensors[i].tmr, _sensors[i].tmr_delay, 0);					
			}				
		}
		
		mqtt_send_int(_sensors[i].topic, state); // сразу же отправим данные в топик, только если состояние изменилось
		_sensors[i].state = state;
		
		// set signal led gpio
		if ( _sensors[i].signal_gpio != NO_GPIO )
			GPIO_ALL( _sensors[i].signal_gpio, state );		
	}
	
}

void ICACHE_FLASH_ATTR gpio_toggle_cb(void *args)
{
	contact_sensor_t *_sens = (contact_sensor_t *)args;
	uint8_t state = GPIO_ALL_GET(_sens->pin_ch);	
	GPIO_ALL(_sens->pin_ch, !state);	
}

void ICACHE_FLASH_ATTR gpio_on_cb(void *args)
{
	contact_sensor_t *_sens = (contact_sensor_t *)args;
	GPIO_ALL(_sens->pin_ch, 1);	
}

void ICACHE_FLASH_ATTR gpio_off_cb(void *args)
{
	contact_sensor_t *_sens = (contact_sensor_t *)args;
	GPIO_ALL(_sens->pin_ch, 0);	
}

void ICACHE_FLASH_ATTR led_on_cb(void *args)
{
	contact_sensor_t *_sens = (contact_sensor_t *)args;
	
	uint8_t duty = pwm_get_duty_iot( _sens->pin_ch );
			
	// получить ближайшее максимальное значение по таблице
	uint8_t i;
	for ( i=0; i < BRIGHTNESS_STEPS; i++)
	{
		if ( brightness[i] <= duty ) continue;
		else break;
	}
	
	if ( i == BRIGHTNESS_STEPS ) return;
	
	

	uint8_t new_duty;
	if ( brightness[i] == duty )
		new_duty = brightness[i+1];
	else	
		new_duty = 	brightness[i];
		
	if ( new_duty > VALDES_MIN_NIGHT_BRIGTHNESS )
		new_duty = VALDES_MIN_NIGHT_BRIGTHNESS;
		
	pwm_set_duty_iot( new_duty, _sens->pin_ch);
		
	pwm_start_iot();
	
	os_timer_disarm(&_sens->tmr_pwm);
	os_timer_setfn(&_sens->tmr_pwm, (os_timer_func_t *) led_on_cb, _sens);
	os_timer_arm(&_sens->tmr_pwm, light_delay, 0);
}

void ICACHE_FLASH_ATTR led_off_cb(void *args)
{
	contact_sensor_t *_sens = (contact_sensor_t *)args;
	uint8_t duty = pwm_get_duty_iot( _sens->pin_ch );
	
	// получить ближайшее минимальное значение по таблице
	uint8_t i;
	for ( i=BRIGHTNESS_STEPS-1; i >=0 ; i--)
	{
		if ( brightness[i] > duty ) continue;
		else break;
	}	
	if ( i == 0 ) return;
	
	pwm_set_duty_iot( brightness[i-1], _sens->pin_ch);
	pwm_start_iot();
	
	os_timer_disarm(&_sens->tmr_pwm);
	os_timer_setfn(&_sens->tmr_pwm, (os_timer_func_t *) led_off_cb, _sens);
	os_timer_arm(&_sens->tmr_pwm, light_delay, 0);	
}

void ICACHE_FLASH_ATTR load_options()
{
	// читаем настройки
	uint8_t i;
	for ( i = 0; i < USER_SENSOR_MAX; i++ )
	{

		_sensors[i].gpio = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*CFG_SENSOR_DATA_SIZE];
		_sensors[i].active_level = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*CFG_SENSOR_DATA_SIZE + 1];
		_sensors[i].func = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*CFG_SENSOR_DATA_SIZE + 2];
		_sensors[i].pin_ch = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*CFG_SENSOR_DATA_SIZE + 3];
		_sensors[i].tmr_delay = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*CFG_SENSOR_DATA_SIZE + 4];
		_sensors[i].signal_gpio = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*CFG_SENSOR_DATA_SIZE + 5];
		_sensors[i].dark = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*CFG_SENSOR_DATA_SIZE + 6];
		
		
		if ( _sensors[i].func == USER_FUNC_GPIO )
		{
			_sensors[i].on = gpio_on_cb;
			_sensors[i].on_args = &_sensors[i];
			
			if ( _sensors[i].tmr_delay == 0 )
			{
				_sensors[i].off = gpio_off_cb;
				_sensors[i].off_args = &_sensors[i];
				_sensors[i].tmr_cb == NULL;
			} else {
				_sensors[i].tmr_cb = gpio_off_cb;
				_sensors[i].tmr_args = &_sensors[i];
				_sensors[i].off = NULL;
			}		
		}
		else if ( _sensors[i].func == USER_FUNC_PWM )
		{
			_sensors[i].on = led_on_cb;
			_sensors[i].on_args = &_sensors[i];
			
			if ( _sensors[i].tmr_delay == 0 )
			{
				_sensors[i].off = led_off_cb;
				_sensors[i].off_args = &_sensors[i];							
				_sensors[i].tmr_cb == NULL;
			} else {
				_sensors[i].tmr_cb = led_off_cb;
				_sensors[i].tmr_args = &_sensors[i];							
				_sensors[i].off == NULL;				
			}	
		}
		else if ( _sensors[i].func == USER_FUNC_TOGGLE_GPIO )
		{
			_sensors[i].on = gpio_toggle_cb;
			_sensors[i].on_args = &_sensors[i];
			
			if ( _sensors[i].tmr_delay == 0 )
			{
				_sensors[i].off = gpio_toggle_cb;
				_sensors[i].off_args = &_sensors[i];
				_sensors[i].tmr_cb == NULL;
			} else {
				_sensors[i].tmr_cb = gpio_toggle_cb;
				_sensors[i].tmr_args = &_sensors[i];
				_sensors[i].off = NULL;
			}				
		}
	}	
	
	

	
	light_delay = CFG_BRIGTHNESS_STEP_DELAY < 10 ? BRIGHTNESS_STEP_DELAY : CFG_BRIGTHNESS_STEP_DELAY;
}

void ICACHE_FLASH_ATTR startfunc()
{
	// выполняется один раз при старте модуля.
	uint8_t i;
	for (i=220; i < 228; i++ ) {
		GPIO_ALL(i, 0);
	}
	
	load_options();
	
	_sensors[0].topic = "pir-1";
	_sensors[1].topic = "pir-2";
	_sensors[2].topic = "rsw-1";
	_sensors[3].topic = "rsw-2";
	
	os_timer_disarm(&timer_read_sensors);
	os_timer_setfn(&timer_read_sensors, (os_timer_func_t *) read_sensors_cb, NULL);
	os_timer_arm(&timer_read_sensors, READ_SENSORS_INTERVAL, 1);
	
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	load_options();
	
	if( timersrc % 10 == 0 ){  // каждые 10 сек
		
	} 
	
	if( timersrc % 60 == 0 ){  // каждые 60 сек
		mqtt_send_int(MQTT_TOPIC_NIGHT_MODE, is_dark);
	} 
	
	// через интерпретер с учетом гистерезиса при чтении adc выставляем valdes[0] // VALDES_IS_DARK // 
	GPIO_ALL(GPIO_LED_NIGHT, VALDES_IS_DARK );  // подсвечиваем
	if ( VALDES_IS_DARK != is_dark )
	{
		is_dark = VALDES_IS_DARK;
		mqtt_send_int(MQTT_TOPIC_NIGHT_MODE, is_dark); // отправка при изменениее
	}		
	
	// VALDES_MIN_NIGHT_BRIGTHNESS 
}



void webfunc(char *pbuf) {
	uint8_t i;
	for ( i = 0; i < USER_SENSOR_MAX; i++ )
	{
		os_sprintf(HTTPBUFF, "%s = %d<br>", _sensors[i].topic, _sensors[i].state );
	}		
}