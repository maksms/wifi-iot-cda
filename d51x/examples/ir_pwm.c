static volatile os_timer_t timer_read_ir; 

#define BRIGHTNESS_STEPS 20
static uint8_t brightness_03[BRIGHTNESS_STEPS] = {0,1,2,3,4,6,8,12,16,22,29,39,51,67,86,109,136,170,209,255};  
									// 0-100%, средняя, S=(S1+S2)/2
									
static uint8_t brightness_steps_value;


void ICACHE_FLASH_ATTR pwm_up(){
	//uint8_t val = pwm_state(0);
	//if (val<255) val++;
	//PWM_ALL_SET(0, val, 0);
	if (brightness_steps_value < BRIGHTNESS_STEPS-1) brightness_steps_value++;
	PWM_ALL_SET(0, brightness_03[brightness_steps_value], 0);
}

void ICACHE_FLASH_ATTR pwm_down(){
	//uint8_t val = pwm_state(0);
	//if (val>0 && val<=255) val--;
	//PWM_ALL_SET(0, val, 0);
	if (brightness_steps_value > 0 ) brightness_steps_value--;
	PWM_ALL_SET(0, brightness_03[brightness_steps_value], 0);
}

void ICACHE_FLASH_ATTR read_ir_cb() {

	switch ( IR_KEYSND ) {
		case 284117685:  //up
			pwm_up();
			break;
		case 284099325:  //down
			pwm_down();
			break;		
	}
}

void ICACHE_FLASH_ATTR
startfunc(){
// выполняется один раз при старте модуля.

		brightness_steps_value = 0;
		os_timer_disarm(&timer_read_ir);	
		os_timer_setfn(&timer_read_ir, (os_timer_func_t *) read_ir_cb, NULL);
		os_timer_arm(&timer_read_ir, 300, 1);
}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
// выполнение кода каждую 1 секунду

if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}
}

void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br>IR Key: %d", IR_KEYSND); // вывод данных на главной модуля
}