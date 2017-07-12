uint32_t current_value; 	       	// Инициализация  текущего значения счетчика
uint32_t previous_value;	       	// Инициализация предыдущего значения счетчика
uint32_t counter_overflows;   		// Инициализация счетчика переполнений
uint32_t division_ratio;	      	// Инициализация коэффициента деления
uint32_t initial_indication;       	// Инициализация начального показания счетчика
uint32_t div1;	                	// Инициализация коэффициента деления
uint32_t init1;             		// Инициализация начального показания счетчика
uint32_t counter1_print; 	       	// Значение показаний счетчика 1
uint32_t counter2_print; 	       	// Значение показаний счетчика 2
uint8_t value_1,value_2, value_3;	// Переменные value_1,value_2, value_3

void ICACHE_FLASH_ATTR
startfunc()
{  
// выполняется один раз при старте модуля
 }

PCF8583_read_my(uint8_t adr)
 {
i2c_start();				   		// Устанавливаем на шине Start состояние
i2c_writeByte(adr*2);		   		// отправляем байт  0b101000yх (отклик любой RTC)
     if(i2c_getAck()){return(-1);}  // slave не откликнулся
i2c_writeByte(01);		           	// устанавливаем регистр для чтения
     if(i2c_getAck()){return(-1);}  // slave not ack
i2c_stop();			           		// Формируем на шине Stop состояние
// ------------ читаем текущее значение (current_value) ----------------
i2c_start(); 							// Устанавливаем на шине Start состояние 
i2c_writeByte(adr*2+1);
     if(i2c_getAck()){return(-1);} 		// slave не откликнулся
uint8_t Val=0;							// Инициализация вспомогательной переменной
Val= i2c_readByte(); 					// ACK
current_value=(Val&15) + (Val/16)*10; 	// Поразрядное логическое "И" +
value_1=Val;
i2c_setAck(0);
Val= i2c_readByte(); 					//ACK
current_value=current_value+ (Val&15)*100 + (Val/16)*1000;
value_2=Val;
i2c_setAck(0);
Val= i2c_readByte(); 					//ACK
current_value=current_value+ (Val&15)*10000 + (Val/16)*100000;
value_3=Val;
i2c_setAck(0);

// -------- читаем предыдущее значение (previous_value) ------------
Val= i2c_readByte(); 					//ACK
previous_value=(Val&15) + (Val/16)*10; 
i2c_setAck(0);
Val= i2c_readByte(); 					//ACK
previous_value=previous_value+ (Val&15)*100 + (Val/16)*1000;
i2c_setAck(0);
Val= i2c_readByte(); 					//ACK
previous_value=previous_value+ (Val&15)*10000 + (Val/16)*100000;
i2c_setAck(0);

//---- читаем содержимое счетчика переполнений (counter_overflows)----
Val = i2c_readByte(); 					//ACK
counter_overflows = Val;
i2c_setAck(0);

//----- читаем содержимое коэффициента деления (division_ratio)32-bit  ------
// пока не используем, просто читаем
Val = i2c_readByte(); 					//ACK 
division_ratio=Val; 					// Первые 8 бит всунули в 4 байтовое слово
i2c_setAck(0);
Val = i2c_readByte(); //ACK 
division_ratio=division_ratio+(Val<<8); // Вторые 8 бит всунули в 4 байтовое слово
i2c_setAck(0);
Val = i2c_readByte(); //ACK 
division_ratio=division_ratio+(Val<<16);// третьи 8 бит всунули в 4 байтовое слово
i2c_setAck(0);
Val = i2c_readByte(); //ACK 
division_ratio=division_ratio+(Val<<24);// Четвертые 8 бит всунули в 4 байтовое слово
i2c_setAck(0);

//---- читаем начальное показание счетчика (initial_indication) ----
// пока не используем, просто читаем
Val = i2c_readByte(); 					//ACK 
initial_indication=Val; 				// Первые 8 бит всунули в 4 байтовое слово
i2c_setAck(0);
Val = i2c_readByte(); 					//ACK 
initial_indication=initial_indication+(Val<<8); // Вторые 8 бит всунули в 4 байтовое слово
i2c_setAck(0);
Val = i2c_readByte(); 					//ACK 
initial_indication=initial_indication+(Val<<16); // третьи 8 бит всунули в 4 байтовое слово
i2c_setAck(0);
Val = i2c_readByte(); 					//ACK 
initial_indication=initial_indication+(Val<<24); // Четвертые 8 бит всунули в 4 байтовое слово
//---------------------------------------------------------------------------
i2c_setAck(1);
i2c_stop();
//return current_value;
}

// ------- Функция обновления предыдущего значения -------------------
PCF8583_upd_previous(uint8_t adr)
{
i2c_start();			        	// Устанавливаем на шине Start состояние
i2c_writeByte(adr*2);	        	// отправляем байт 0b101000yх (откл любой RTC)
  if(i2c_getAck()){return;} 		// slave не откликнулся
i2c_writeByte(04);		        	// устанавливаем адрес регистра 0x00 для чтения
  if(i2c_getAck()){return;}         // slave не откликнулся
i2c_writeByte(value_1);	        	// записываем по адресу 0x00 байт 0x20
	if(i2c_getAck()){return;}   	// slave не откликнулся
i2c_writeByte(value_2);	        	// записываем по адресу 0x00 байт 0x20
	if(i2c_getAck()){return;}   	// slave не откликнулся
i2c_writeByte(value_3);	        	// записываем по адресу 0x00 байт 0x20
  if(i2c_getAck()){} 	           	// slave не откликнулся
i2c_stop();			        		// Формируем на шине Stop состояние
}

PCF8583_upd_counter_overflows(uint8_t adr)
{
i2c_start();			     		// Устанавливаем на шине Start состояние
i2c_writeByte(adr*2);	     		// отправл байт на шину 0b101000yх (отк любой RTC)
  if(i2c_getAck()){return;}      	// slave не откликнулся
i2c_writeByte(07);		     		// устанавливаем адрес регистра 0x00 для чтения
  if(i2c_getAck()){return;}      	// slave не откликнулся
i2c_writeByte(counter_overflows);	// записываем по адресу 0x00 байт 0x20
  if(i2c_getAck()){} 			 	// slave не откликнулся
i2c_stop();							// Формируем на шине Stop состояние
}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc)
 {
      if(timersrc%10==0)  				// выполнение кода каждые 10 секунд
            { 
			
// ---------------- Логика работы счетчика  A -------------------
				PCF8583_read_my (0x50);
				
      if (current_value>previous_value)
            {       
				PCF8583_upd_previous(0x50);				// Перезапись предыдущего значения счетчика текущим 
            }
      else 
            { 
				if(current_value!=previous_value)
					{
						counter_overflows++;
						PCF8583_upd_previous(0x50);				// Перезапись предыдущего значения счетчика текущим 
						PCF8583_upd_counter_overflows(0x50);	// Перезапись счетчика переполнений counter_overflows
					}
			}
               init1=sensors_param.cfgdes[0];		// Читаем начальное значение из flash памяти esp
               div1=sensors_param.cfgdes[1];		// Читаем значение коэффициента деления из flash памяти esp
		if (div1==0) div1=1;   						// исключение деления на ноль
		counter1_print= init1+((current_value*10+999999*10*counter_overflows)/div1); 
		valdes[0] = counter1_print;
// ------ Счетчик B ----------------------------------------
				current_value=0;					// Обнуляем переменные
				previous_value=0;					// Обнуляем переменные
				counter_overflows=0;				// Обнуляем переменные
				
				PCF8583_read_my (0x51);
				
	if (current_value>previous_value)
            {       
				PCF8583_upd_previous(0x51);				// Перезапись предыдущего значения счетчика текущим 
            }
      else 
            { 
				if(current_value!=previous_value)
					{
						counter_overflows++;
						PCF8583_upd_previous(0x51);				// Перезапись предыдущего значения счетчика текущим 
						PCF8583_upd_counter_overflows(0x51);	// Перезапись счетчика переполнений counter_overflows
					}
			}
               init1=sensors_param.cfgdes[2];		// Читаем начальное значение из flash памяти esp
               div1=sensors_param.cfgdes[3];		// Читаем значение коэффициента деления из flash памяти esp
		if (div1==0) div1=1;   						// исключение деления на ноль
		counter2_print= init1+((current_value*10+999999*10*counter_overflows)/div1); 
			valdes[1] = counter2_print;
			}
}

void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"Горячая вода: <b>%s</b> ,", fltostr(counter1_print));
os_sprintf(HTTPBUFF,"<br>Показания счетчика A: %d",counter1_print);
os_sprintf(HTTPBUFF,"<br>Показания счетчика B: %d",counter2_print);
os_sprintf(HTTPBUFF,"<br>Текущее значение:%d",current_value);
os_sprintf(HTTPBUFF,"<br>Предыдущее значение:%d",previous_value);
os_sprintf(HTTPBUFF,"<br>Счетчик переполнений:%d",counter_overflows);
}
