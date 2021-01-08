#define GPIO_BTN 4
#define GPIO_LED1 12
#define GPIO_LED2 13
#define GPIO_LED3 15
#define GPIO_LED4 2

#define BTN_GLITCH_TIME 50
#define BTN_SHORT_PRESS_TIMEOUT 300

#define millis() (unsigned long) (micros() / 1000ULL)

uint8_t short_press_count = 0;
uint32_t long_press_time = 0;
uint8_t btn_state = 0;


os_timer_t button_read_timer;
os_timer_t short_pressed_timer;

char str[64] = "";
void ICACHE_FLASH_ATTR short_press_timer_cb(void *arg)
{
    uint8_t *cnt = (uint8_t *)arg;
	os_sprintf(str, "short pressed %d", *cnt);	
	
	if ( *cnt == 1 )
	{
		digitalWrite( GPIO_LED1, !digitalRead(GPIO_LED1) );
	}
	else if ( *cnt == 2 )
	{
		digitalWrite( GPIO_LED2, !digitalRead(GPIO_LED2) );
	}
	else if ( *cnt == 3 )
	{
		digitalWrite( GPIO_LED3, !digitalRead(GPIO_LED3) );
	}

	
}

void ICACHE_FLASH_ATTR reschedule_short_press_timer(uint8_t *arg)
{
 	os_timer_disarm(&short_pressed_timer);
	os_timer_setfn(&short_pressed_timer, (os_timer_func_t *)short_press_timer_cb, (void *)arg);
	os_timer_arm(&short_pressed_timer, BTN_SHORT_PRESS_TIMEOUT, 0); 
}

void ICACHE_FLASH_ATTR button_read_cb(void *arg) 
{
    static uint8_t level = 0;
	level = digitalRead(GPIO_BTN);
	static uint32_t pressed_time = 0;
	static uint32_t prev_pressed_time = 0;
    static uint32_t ms = 0;
    static uint32_t released_time = 0;
    static uint32_t hold_time = 0;
    static uint32_t wait_time = 0;
	
	ms = millis();
	
    if ( level == 0 && !btn_state && ( (ms - pressed_time) > BTN_GLITCH_TIME))
    {
        btn_state = 1;
        pressed_time = ms;
    }
    else if ( level == 1 && btn_state && ((ms - pressed_time) > BTN_GLITCH_TIME))
    {
        hold_time = ms - pressed_time;
        released_time = millis();
		
		btn_state = 0;
		if ( short_press_count == 0 ) short_press_count = 1;
		
        if ( hold_time < BTN_SHORT_PRESS_TIMEOUT )
        {
            wait_time = pressed_time - prev_pressed_time;
            if ( wait_time < BTN_SHORT_PRESS_TIMEOUT )
            {
                short_press_count++;
            } 
            else 
            {
                short_press_count = 1;
            } 	
			reschedule_short_press_timer(&short_press_count);
		}
		else		
		{
            short_press_count = 1;
            long_press_time = hold_time;		
			
            if ( long_press_time < 1000 ) {
				os_sprintf(str, "short pressed %d, hold time %d ms", short_press_count, hold_time);
				digitalWrite( GPIO_LED1, !digitalRead(GPIO_LED1) );
            }
            else if ( long_press_time > 2000 ) {
				os_sprintf(str, "long pressed 2 sec (%d)", long_press_time);
				digitalWrite( GPIO_LED2, !digitalRead(GPIO_LED2) );
            } 
            else if (long_press_time > 1500) {
				os_sprintf(str, "long pressed 1,5 sec (%d)", long_press_time);
				digitalWrite( GPIO_LED3, !digitalRead(GPIO_LED3) );
            } 
            else if (long_press_time > 1000) {
				os_sprintf(str, "long pressed 1 sec (%d)", long_press_time);
				digitalWrite( GPIO_LED4, !digitalRead(GPIO_LED4) );
            }			
		}
        prev_pressed_time = pressed_time;
        pressed_time = ms;
        ms = millis();		
	}	
}

void ICACHE_FLASH_ATTR startfunc()
{
	
    // выполняется один раз при старте модуля.
 	os_timer_disarm(&button_read_timer);
	os_timer_setfn(&button_read_timer, (os_timer_func_t *)button_read_cb, NULL);
	os_timer_arm(&button_read_timer, 10, 1);   

}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
    // выполнение кода каждую 1 секунду
    if(timersrc%30==0)
    {
        // выполнение кода каждые 30 секунд
    }
}

void webfunc(char *pbuf) 
{
    os_sprintf(HTTPBUFF,"<br>press count: %d", short_press_count);
    os_sprintf(HTTPBUFF,"<br>str: %s", str);
}