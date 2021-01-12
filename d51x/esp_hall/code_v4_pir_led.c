#define FW_VER "4.0"

// Длительность, Уровень яркости, Мин. освещенность, Задержка выкл PIR, PIR switch VGPIO


/*
*   v.4 changes:


*/
// TODO: 
// для "Датчик Движения 1" можно задать gpio или pwm, можно задать время таймера на отключение, сработает при пропаже сигнала (нет движения)
// для "Датчик Движения 2" можно задать gpio или pwm, можно задать время таймера на отключение, сработает при пропаже сигнала (нет движения)
// для "Датчик Геркон 1" можно задать gpio или pwm, можно задать время таймера на отключение, сработает при пропаже сигнала (контакт закрыт)
// для "Датчик Геркон 2" можно задать gpio или pwm, можно задать время таймера на отключение, сработает при пропаже сигнала (контакт закрыт)

// уровень яркости ленты в обычном режиме 
// уровень яркости ленты в ночном режиме (задается время с и время по), лучше через интерпретер и установку valdes - и время с и время по и урвовень днем и ночью будет региулироваться

#define GPIO_LED_STRIP 13
//#define GPIO_REED_SW_1 4
//#define GPIO_REED_SW_2 5
#define GPIO_PIR_1 4
//#define GPIO_PIR_1 12
//#define GPIO_PIR_2 14
#define PWM_LED 0

#define GPIO_BTN_1 222
#define GPIO_BTN_2 223
#define GPIO_BTN_3 227

#define GPIO_LED_NIGHT 224

#define GPIO_LED_PIR_1 15
//#define GPIO_LED_PIR_1 220
#define GPIO_LED_PIR_2 221

#define NO_GPIO 255
//#define GPIO_LED_GREEN 226
//#define GPIO_LED_RED 225

#define READ_SENSORS_INTERVAL 20
//#define READ_BUTTONS_INTERVAL 300


#define LUMINOSITY_STEP 20
#define LBRIGHTNESS_STEP_DELAY 20
#define LIGHT_DELAY_STEP 5

#define MIN_LIGHT_DELAY 5
#define MAX_LIGHT_DELAY 50

#define PIR_DELAY_OFF 30000

#define CFG_SENSOR_FUNC_IDX				0
#define CFG_PIR1_FUNC					sensors_param.cfgdes[0]			// функция для PIR1, 0-255: GPIO, 1000-1004: PWM
#define CFG_PIR1_DELAY					sensors_param.cfgdes[1]			// задержка выключения pir1 (запускается таймер)

#define CFG_PIR2_FUNC					sensors_param.cfgdes[2]			// функция для PIR2, 0-255: GPIO, 1000-1004: PWM
#define CFG_PIR2_DELAY					sensors_param.cfgdes[3]			// задержка выключения pir2 (запускается таймер)

#define CFG_RSW1_FUNC					sensors_param.cfgdes[4]			// функция для Reed Switch 1, 0-255: GPIO, 1000-1004: PWM
#define CFG_RSW1_DELAY					sensors_param.cfgdes[5]			// задержка выключения ReedSwitch1 (запускается таймер)

#define CFG_RSW2_FUNC					sensors_param.cfgdes[6]			// функция для Reed Switch 2, 0-255: GPIO, 1000-1004: PWM
#define CFG_RSW2_DELAY					sensors_param.cfgdes[7]			// задержка выключения ReedSwitch2  (запускается таймер)

#define CFG_BRIGHTNESS_LEVEL_MIN		sensors_param.cfgdes[8]			// минимальная яркость в заданное (ночное время), выставляем через интерпретер valdes[0]

#define CFG_BRIGTHNESS_STEP_DELAY		sensors_param.cfgdes[9]

#define VALDES_IS_DARK					valdes[0]


//#define BRIGHTNESS_STEPS 20
//static uint8_t brightness[BRIGHTNESS_STEPS] = {0,1,2,3,4,5,7,9,12,16,21,28,37,48,64,84,111,147,194,255};  
#define BRIGHTNESS_STEPS 32
static uint8_t brightness[BRIGHTNESS_STEPS] = {0,1,2,3,5,8,12,16,21,26,32,38,45,52,60,68,76,85,95,105,115,125,136,148,160,172,185,198,211,225,239,255};

static uint8_t light_delay = 25; // msec, длительность свечения каждой ступени яркости

typedef void (* func_cb)(void *args);  

typedef enum {
	  USER_SENSOR_PIR1		// level = 1
//	, USER_SENSOR_PIR2		// level = 1
//	, USER_SENSOR_RSW1		// level = 0
//	, USER_SENSOR_RSW2		// level = 0
	, USER_SENSOR_MAX
} user_sensor_type_e;

typedef enum {
	USER_FUNC_GPIO,
	USER_FUNC_PWM
} user_sensor_func_e;

typedef struct {
	uint8_t gpio;
	uint8_t signal_gpio;
	uint8_t prev_state;				// предыдущее состояние
	user_sensor_func_e func;		// тип функции сенсора - gpio или pwm
	uint16_t pin_ch;				// номер gpio или номер channel pwm
	uint8_t level;					// уровень, при котором состояние ON
	char *topic;
	uint32_t tmr_delay;				// через какое время сработает таймер при закрытии контакта / пропадании сигнала
	func_cb tmr_cb;						// callback таймера
	void *tmr_args;						// аргументы коллбека
	func_cb on;						// callback при ON
	void *on_args;						// аргументы коллбека
	func_cb off;						// callback при OFF
	void *off_args;						// аргументы коллбека
	os_timer_t tmr;					// сам таймер
} contact_sensor_t;

contact_sensor_t _sensors[USER_SENSOR_MAX] = {
	  {GPIO_PIR_1,		GPIO_LED_PIR_1,	0,	USER_FUNC_GPIO,	NO_GPIO,	0,	"pir-1",	0,	NULL,	NULL,	NULL,	NULL, 	NULL, 	NULL,	0}
	//, {GPIO_PIR_2,		GPIO_LED_PIR_2,	0,	USER_FUNC_GPIO,	NO_GPIO,	1,	"pir-2",	0,	NULL,	NULL,	NULL,	NULL, 	NULL, 	NULL,	0}
	//, {GPIO_REED_SW_1,	NO_GPIO,		0,	USER_FUNC_GPIO,	NO_GPIO,	0,	"rsw-1",	0,	NULL,	NULL,	NULL,	NULL, 	NULL, 	NULL,	0}
	//, {GPIO_REED_SW_2,	NO_GPIO,		0,	USER_FUNC_GPIO,	NO_GPIO,	0,	"rsw-2",	0,	NULL,	NULL,	NULL,	NULL, 	NULL, 	NULL,	0}
};

static volatile os_timer_t timer_read_sensors; 
static volatile os_timer_t timer_pwm; 

static uint8_t is_dark = 0;

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
		state = ( _sensors[i].level ) ? state : !state;
		if ( state == _sensors[i].prev_state ) continue;  // состояние не менялось
	
		//uint8_t _state = ( _sensors[i].level ) ? state : !state;
		
		// состояние изменилось, выполнить callback
		if ( state ) 
		{
			if ( _sensors[i].on != NULL )
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
		
		mqtt_send_int(_sensors[i].topic, state);
		_sensors[i].prev_state = state;
		
		// set signal led gpio
		//if ( _sensors[i].signal_gpio != NO_GPIO )
		//	GPIO_ALL( _sensors[i].signal_gpio, state );		
	}
	
}


void ICACHE_FLASH_ATTR gpio_on_cb(void *args)
{
	uint16_t *gpio = (uint16_t *)args;
	GPIO_ALL(*gpio, 1);
}

void ICACHE_FLASH_ATTR gpio_off_cb(void *args)
{
	uint16_t *gpio = (uint16_t *)args;
	GPIO_ALL(*gpio, 0);	
}

void ICACHE_FLASH_ATTR led_on_cb(void *args)
{
	uint16_t *ch = (uint16_t *)(args);
	uint8_t duty = pwm_get_duty_iot( *ch );

	// получить ближайшее максимальное значение по таблице
	uint8_t i;
	for ( i=0; i < BRIGHTNESS_STEPS; i++)
	{
		if ( brightness[i] <= duty ) continue;
		else break;
	}
	
	if ( i == BRIGHTNESS_STEPS ) return;
	
	if ( brightness[i] == duty )
		pwm_set_duty_iot( brightness[i+1], *ch);
	else	
		pwm_set_duty_iot( brightness[i], *ch);
		
	pwm_start_iot();
	
	os_timer_disarm(&timer_pwm);
	os_timer_setfn(&timer_pwm, (os_timer_func_t *) led_on_cb, ch);
	os_timer_arm(&timer_pwm, light_delay, 0);
}

void ICACHE_FLASH_ATTR led_off_cb(void *args)
{
	uint16_t *ch = (uint16_t*)(args);
	uint8_t duty = pwm_get_duty_iot( *ch );
	
	// получить ближайшее минимальное значение по таблице
	uint8_t i;
	for ( i=BRIGHTNESS_STEPS-1; i >=0 ; i--)
	{
		if ( brightness[i] > duty ) continue;
		else break;
	}	
	if ( i == 0 ) return;
	
	pwm_set_duty_iot( brightness[i-1], *ch);
	pwm_start_iot();
	
	os_timer_disarm(&timer_pwm);
	os_timer_setfn(&timer_pwm, (os_timer_func_t *) led_off_cb, ch);
	os_timer_arm(&timer_pwm, light_delay, 0);	
}

void ICACHE_FLASH_ATTR load_options()
{
	// читаем настройки
	uint8_t i;
	for ( i = 0; i < USER_SENSOR_MAX; i++ )
	{
		_sensors[i].func = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*2] / 1000;
		_sensors[i].pin_ch = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*2] % 1000;
		_sensors[i].tmr_delay = sensors_param.cfgdes[CFG_SENSOR_FUNC_IDX + i*2 + 1];
		if ( _sensors[i].func == USER_FUNC_GPIO )
		{
			_sensors[i].on = gpio_on_cb;
			_sensors[i].on_args = &_sensors[i].pin_ch;
			
			if ( _sensors[i].tmr_delay == 0 )
			{
				_sensors[i].off = gpio_off_cb;
				_sensors[i].off_args = &_sensors[i].pin_ch;
				_sensors[i].tmr_cb == NULL;
			} else {
				_sensors[i].tmr_cb = gpio_off_cb;
				_sensors[i].tmr_args = &_sensors[i].pin_ch;
				_sensors[i].off = NULL;
			}		
		}
		else if ( _sensors[i].func == USER_FUNC_PWM )
		{
			_sensors[i].on = led_on_cb;
			_sensors[i].on_args = &_sensors[i].pin_ch;
			
			if ( _sensors[i].tmr_delay == 0 )
			{
				_sensors[i].off = led_off_cb;
				_sensors[i].off_args = &_sensors[i].pin_ch;							
				_sensors[i].tmr_cb == NULL;
			} else {
				_sensors[i].tmr_cb = led_off_cb;
				_sensors[i].tmr_args = &_sensors[i].pin_ch;							
				_sensors[i].off == NULL;				
			}	
		}
	}	
	
	light_delay = CFG_BRIGTHNESS_STEP_DELAY < 10 ? LBRIGHTNESS_STEP_DELAY : CFG_BRIGTHNESS_STEP_DELAY;
}

void ICACHE_FLASH_ATTR startfunc()
{
	// выполняется один раз при старте модуля.
	uint8_t i;
	for (i=220; i < 228; i++ ) {
		GPIO_ALL(i, 0);
	}
	
	load_options();
	
	os_timer_disarm(&timer_read_sensors);
	os_timer_setfn(&timer_read_sensors, (os_timer_func_t *) read_sensors_cb, NULL);
	os_timer_arm(&timer_read_sensors, READ_SENSORS_INTERVAL, 1);
	
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	if( timersrc % 10 == 0 ){  // каждые 10 сек
		load_options();
		mqtt_send_int("night-mode", is_dark);
	} 
	
	
	// через интерпретер с учетом гистерезиса при чтении adc выставляем valdes[0] // VALDES_IS_DARK // 
	GPIO_ALL(GPIO_LED_NIGHT, VALDES_IS_DARK );  // подсвечиваем
	if ( VALDES_IS_DARK != is_dark )
	{
		is_dark = VALDES_IS_DARK;
		mqtt_send_int("night-mode", is_dark);
	}		
}



void webfunc(char *pbuf) {
	
}