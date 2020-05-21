#define FW_VER "1.2"

uint32_t voltage = 0;
uint32_t current = 0;
uint32_t power = 0;
uint32_t consumption_total = 0;
uint32_t consumption_total_resettable = 0;
//uint32_t working_hours = 0;

void startfunc(){
	// выполняется один раз при старте модуля.
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
	
	
		voltage = mbval[0][0];
		current = mbval[0][1];
		power = mbval[0][2];
		consumption_total = mbval[1][0];
		consumption_total_resettable = mbval[2][0];
		//working_hours = mbval[3][0];
	
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}


void webfunc(char *pbuf) {
	
	if ( sensors_param.cfgdes[0] > 0 ) {
	char tmp[10];
	
	tofloatmb(tmp, 3, voltage); 
	os_sprintf(HTTPBUFF,"<b>Напряжение, В: </b>%s", tmp);
	
	tofloatmb(tmp, 3, current); 
	os_sprintf(HTTPBUFF,"<br><b>Ток, А: </b>%s", tmp );
	
	tofloatmb(tmp, 3, power); 
	os_sprintf(HTTPBUFF,"<br><b>Мощность, Вт: </b>%s", tmp);
	
	tofloatmb(tmp, 3, consumption_total); 
	os_sprintf(HTTPBUFF,"<br><b>Общий счетчик, кВт*ч: </b>%s", tmp );
	
	tofloatmb(tmp, 3, consumption_total_resettable);
	os_sprintf(HTTPBUFF,"<br><b>Обнуляемый счетчик, кВт*ч: </b>%s", tmp);
	
	//tofloatmb(tmp, 3, working_hours);
	//os_sprintf(HTTPBUFF,"<br><b>Время работы, ч: </b>%s", tmp );
	}
	
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
	
}