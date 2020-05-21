#define page1_line1 "Boiler %s %6d.%1d°"
#define page1_line2 "Pump   %s Power %s"
#define page1_line3 "Hotcab %s %6d.%1d°"
#define page1_line4_1 "Hum %2d.%1d%%  Air %2d.%1d°"



#define ADC_divider_A 0,296
#define ADC_divider_mA 296
#define ADC_zero 128

#define MIN_CUR_PUMP 300
#define MIN_CUR_BOILER 300


int32_t gpio_boiler = 0;
int32_t gpio_hotcab = 0;
int32_t gpio_pump = 0;

int32_t adc0 = 0;  //boiler
int32_t adc1 = 0;  //pump
int32_t adc2 = 0;  //hotcab
//uint8_t refresh_time = 0;

void ICACHE_FLASH_ATTR update_LCD(uint32_t  timersrc) {
	
	char lcd_line_text[40] = "";
	int32_t temp = 0;
	
	adc0 = abs(ADCdata[1] - ADC_zero )*ADC_divider_mA ;
	adc1 = abs(ADCdata[3] - ADC_zero )*ADC_divider_mA ;
	adc2 = abs(ADCdata[0] - ADC_zero )*ADC_divider_mA ;
	

			// ***************** page 1, line 1 ******************************
			os_memset(lcd_line_text, 0, 40);	
			uint8_t pump_status = ( adc1 >= MIN_CUR_PUMP && GPIO_ALL_GET(gpio_pump) == 1);
			valdes[0] = pump_status;
			uint8_t boiler_status = ( adc0 >= MIN_CUR_BOILER && GPIO_ALL_GET(gpio_boiler) == 1);
			valdes[1] = boiler_status ;
			os_sprintf(lcd_line_text, page1_line2, pump_status ? "ON " : "OFF", (GPIO_ALL_GET(gpio_pump) == 1) ? "ON " : "OFF");
			LCD_print(0, lcd_line_text);	
	
			// ***************** page 1, line 2 ******************************
			os_memset(lcd_line_text, 0, 40);	
			temp = data1wire[0];  // dsw1
			os_sprintf(lcd_line_text, page1_line1, (GPIO_ALL_GET(gpio_boiler) == 1) ? "ON " : "OFF", (int)(temp / 10), (int)(temp % 10));
			LCD_print(1, lcd_line_text);		
	

			// ***************** page 1, line 3 ******************************
			os_memset(lcd_line_text, 0, 40);	
			temp = data1wire[1];  //dsw2 
			os_sprintf(lcd_line_text, page1_line3, (GPIO_ALL_GET(gpio_hotcab) == 1) ? "ON " : "OFF", (int)(temp / 10), (int)(temp % 10));
			LCD_print(2, lcd_line_text);		
			
			// ***************** page 1, line 4 ******************************
			os_memset(lcd_line_text, 0, 40);	
			temp = dht_t1;
			//if (show_min == 0) {
				uint16_t hum = dht_h1;
				os_sprintf(lcd_line_text, page1_line4_1, (int)(hum / 10), (int)(hum % 10), (int)(temp / 10), (int)(temp % 10)); 
			//} else {
			//	os_sprintf(lcd_line_text, page1_line4_2, time_loc.hour, time_loc.min, time_loc.day ,time_loc.month, time_loc.year, (int)(temp / 10), (int)(temp % 10)); 
			//}
			LCD_print(3, lcd_line_text);	
	
}


void ICACHE_FLASH_ATTR
startfunc(){
// выполняется один раз при старте модуля.
	gpio_boiler = 12; //sensors_param.cfgdes[1];
	gpio_hotcab = 14; //sensors_param.cfgdes[2];
	gpio_pump = 13; //sensors_param.cfgdes[3];
}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
// выполнение кода каждую 1 секунду

//if(timersrc%refresh_time ==0){
// выполнение кода каждые refresh_time  секунд
update_LCD(timersrc);
//}

if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}
}

void webfunc(char *pbuf) {
  //os_sprintf(HTTPBUFF,"<br>PCF8591 = AIN0: %d\tAIN1: %d\tAIN2: %d\tAIN3: %d", adc0, adc1, adc2, adc3 ); // вывод данных на главной модуля

}