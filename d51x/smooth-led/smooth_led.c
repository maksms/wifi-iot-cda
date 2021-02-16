
// Настройки: Led channel, Valdes number
// Кол-во переменных: 2

#define LED_CHANNEL_CFG_INDEX 0
#define VALDES_CFG_INDEX 1

#define LED_CHANNEL  sensors_param.cfgdes[LED_CHANNEL_CFG_INDEX]
#define VALDES_INDEX sensors_param.cfgdes[VALDES_CFG_INDEX]
#define TARGET_DUTY valdes[VALDES_INDEX]

#define CHECK_DUTY_INTERVAL 20
#define BRIGHTNESS_STEPS 255
#define BRIGHTNESS_STEP_DELAY 20 

int32_t prev_target_duty = 0;

os_timer_t read_duty_cb; 
os_timer_t tmr_pwm; 


void ICACHE_FLASH_ATTR led_off_cb(uint32_t target_duty)
{
	uint32_t duty = pwm_get_duty_iot( LED_CHANNEL );
	if ( duty > target_duty )
	{
		duty--;
		
		pwm_set_duty_iot( duty, LED_CHANNEL);
		pwm_start_iot();		
		
		os_timer_disarm(&tmr_pwm);
		os_timer_setfn(&tmr_pwm, (os_timer_func_t *) led_off_cb, target_duty);
		os_timer_arm(&tmr_pwm, BRIGHTNESS_STEP_DELAY, 0);
	}	
}

void ICACHE_FLASH_ATTR led_on_cb(uint32_t target_duty)
{
	uint32_t duty = pwm_get_duty_iot( LED_CHANNEL );
	if ( duty < target_duty )
	{
		duty++;
		
		pwm_set_duty_iot( duty, LED_CHANNEL);
		pwm_start_iot();		
		
		os_timer_disarm(&tmr_pwm);
		os_timer_setfn(&tmr_pwm, (os_timer_func_t *) led_on_cb, target_duty);
		os_timer_arm(&tmr_pwm, BRIGHTNESS_STEP_DELAY, 0);
	}
}

void ICACHE_FLASH_ATTR check_duty(uint32_t channel)
{
	if ( TARGET_DUTY == prev_target_duty ) return;
	prev_target_duty = TARGET_DUTY;
	
	uint32_t duty = pwm_get_duty_iot( channel );	
	
	if ( duty < TARGET_DUTY ) 
	{
		// плавно поднимаем яркость
		led_on_cb( TARGET_DUTY );
	} else {
		// плавно понижаем яркость
		led_off_cb( TARGET_DUTY );
	}
}

void ICACHE_FLASH_ATTR startfunc()
{
	os_timer_disarm(&read_duty_cb);
	os_timer_setfn(&read_duty_cb, (os_timer_func_t *) check_duty, LED_CHANNEL);
	os_timer_arm(&read_duty_cb, CHECK_DUTY_INTERVAL, 1);	
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
	
}


void webfunc(char *pbuf) 
{
	os_sprintf(HTTPBUFF, "target duty = %d<br>", TARGET_DUTY );
	os_sprintf(HTTPBUFF, "prev targer duty = %d<br>", prev_target_duty );
}