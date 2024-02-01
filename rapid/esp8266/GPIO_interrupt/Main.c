char Buf[31];
uint8_t count_bit;
uint8_t EN=14;
uint8_t SCK=13;
uint8_t DATA=12;
uint8_t b;

os_timer_t DebounceTimer;
LOCAL void input_intr_handler(void *arg);

void ICACHE_FLASH_ATTR debounce_timer(void *arg)              // Защитный таймер
{
	ETS_GPIO_INTR_DISABLE();
	gpio_pin_intr_state_set(GPIO_ID_PIN(EN), GPIO_PIN_INTR_NEGEDGE);        // Вкл. прерывания на линии EN (по переходу 1 -> 0)
  gpio_pin_intr_state_set(GPIO_ID_PIN(SCK), GPIO_PIN_INTR_DISABLE);        // Запрещаем прерывания от SCK  
  count_bit=0; 
  ETS_GPIO_INTR_ENABLE();
      
}
LOCAL void input_intr_handler(void *arg)    // Процедура обраб. прерываний
{
      uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);               // Читаем состояние GPIO (нас интересуют EN
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);  // Очистка
      ETS_GPIO_INTR_DISABLE();     // Откл.прерываний
  
             if (gpio_status & BIT(EN))     // тут проверяем, какой пин вызвал прерывание, делаем обработку
                     {
                          gpio_pin_intr_state_set(GPIO_ID_PIN(EN), GPIO_PIN_INTR_DISABLE);        // Запрещаем прерывания от EN                 
                          gpio_pin_intr_state_set(GPIO_ID_PIN(SCK), GPIO_PIN_INTR_POSEDGE);     // Тут разрешаем прерывания SCK (по переходу 0 -> 1)      
                          os_timer_arm(&DebounceTimer, 20, 0); // Установка  защитного таймера на 20мс
                          GPIO_OUTPUT_SET(GPIO2, 0);
                     }
             if (gpio_status & BIT(SCK))
                     {
                       if (GPIO_INPUT_GET(GPIO_ID_PIN(DATA))==1)
                         { 
				                   b=0b00000001; 
				                 }
			                 else
					               {
				                  b=0b00000000;
					               }
              Buf[count_bit>>3] = (Buf[count_bit>>3]<< 1) | b; 
              count_bit++;    
  if(count_bit==0xFF)  gpio_pin_intr_state_set(GPIO_ID_PIN(SCK), GPIO_PIN_INTR_DISABLE);   
                     }
           ETS_GPIO_INTR_ENABLE();   // разрешаем прерывания
}

void ICACHE_FLASH_ATTR
startfunc()
 { 
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO12_U, FUNC_GPIO12);    //
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO13_U, FUNC_GPIO13);    //
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO14_U, FUNC_GPIO14);    // 
  gpio_output_set(0, 0, 0, GPIO_ID_PIN(EN));
  gpio_output_set(0, 0, 0, GPIO_ID_PIN(SCK));
  gpio_output_set(0, 0, 0, GPIO_ID_PIN(DATA));

  ETS_GPIO_INTR_DISABLE();  // Откл. глоб.прерываний
	ETS_GPIO_INTR_ATTACH(input_intr_handler, NULL);  // Подкл. процедуры обраб. прерываний
	gpio_pin_intr_state_set(GPIO_ID_PIN(EN), GPIO_PIN_INTR_NEGEDGE);      // Вкл.прерываний на линии EN (по переходу 0 -> 1)
  gpio_pin_intr_state_set(GPIO_ID_PIN(SCK), GPIO_PIN_INTR_DISABLE);        // Запрещаем прерывания от SCK  
	ETS_GPIO_INTR_ENABLE();          // Вкл.глоб.прерываний
	os_timer_disarm(&DebounceTimer); // Таймер
	os_timer_setfn(&DebounceTimer, &debounce_timer, 0);           
        
 }

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc)
 {
   
}

void webfunc(char *pbuf) 
{ 
  
}
