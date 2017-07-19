/**
MISO - PC7
MOSI - PC6
SCLK - PC5
CSn -  PD4 настроить как выход, установить в 0 для передачи по SPI интерфейсу
*/
#define GDO0_PIN 	GPIO_PIN_3 			
#define CSn_PIN 	GPIO_PIN_4
#define SCLK_PIN    	GPIO_PIN_5
#define MOSI_PIN   	GPIO_PIN_6
#define MISO_PIN   	GPIO_PIN_7

//--------- Регистры CC1101 ----------------
#define SRES 		0x30
#define SRX 		0x34
#define STX 		0x35
#define SNOP 		0x3D

#define IOCFG2		0x00
#define IOCFG1    0x01
#define IOCFG0    0x02
#define FIFOTHR   0x03
#define SYNC1     0x04
#define SYNC0     0x05
#define PKTLEN    0x06
#define PKTCTRL1  0x07
#define PKTCTRL0  0x08
#define ADDR      0x09
#define CHANNR    0x0A
#define FSCTRL1   0x0B
#define FSCTRL0   0x0C
#define FREQ2     0x0D
#define FREQ1     0x0E
#define FREQ0     0x0F
#define MDMCFG4   0x10
#define MDMCFG3   0x11
#define MDMCFG2   0x12
#define MDMCFG1   0x13
#define MDMCFG0   0x14
#define DEVIATN   0x15
#define MCSM2     0x16
#define MCSM1     0x17
#define MCSM0     0x18
#define FOCCFG    0x19
#define BSCFG     0x1A
#define AGCTRL2   0x1B
#define AGCTRL1   0x1C
#define AGCTRL0   0x1D
#define WOREVT1   0x1E
#define WOREVT0   0x1F
#define WORCTRL   0x20
#define FREND1    0x21
#define FREND0    0x22
#define FSCAL3    0x23
#define FSCAL2    0x24
#define FSCAL1    0x25
#define FSCAL0    0x26
#define RCCTRL1   0x27
#define RCCTRL0   0x28
#define FSTEST    0x29
#define PTEST     0x2A
#define AGCTEST   0x2B
#define TEST2     0x2C
#define TEST1     0x2D
#define TEST0     0x2E
//-------------------
#define periodusec 	125		// mcs период
//-------- настройки к датчику DS18B20 ---------------------
#define timer 10 										// время в секундах.
#define DS_BIT 	  GPIO_PIN_3 			// pin 3

#define keyT	11000

char config_433_ook[]=
{
IOCFG2, 	0x0D,
IOCFG0, 	0x00,
FIFOTHR, 	0x67,    // аттенюатор по приему на 12дБ
PKTCTRL0,	0x32,
FSCTRL1, 	0x06,
FSCTRL0, 	0x00,
FREQ2, 		0x10,
FREQ1, 		0xB0,
FREQ0, 		0x71,
MDMCFG4, 	0xA7,
MDMCFG3, 	0x32,
MDMCFG2, 	0x30,
MDMCFG1, 	0x22,
MDMCFG0, 	0xF8,
MCSM2, 		0x07,
MCSM1, 		0x30,
MCSM0, 		0x18,
AGCTRL2, 	0x04,
AGCTRL1, 	0x00,
AGCTRL0, 	0x92,
FREND1, 	0xB6,
FREND0, 	0x11,
FSCAL3, 	0xE9,
FSCAL2, 	0x2A,
FSCAL1, 	0x00,
FSCAL0, 	0x1F,
TEST2, 		0x81,
TEST1, 		0x35,
TEST0, 		0x09 };

//-------------------------------------------------
#include "stm8s.h"
#include "main.h"
//-------------------------------------------------
void init_SPI(void);
void delay(volatile uint32_t count);
void init_gpio(void);
void cc1101_init(void);
void init_CPU(void);
void sendRC(unsigned long data);
void OneWireReset(void);
void OneWireOutByte(uint8_t d);
uint8_t OneWireInByte(void);
//-----------------------------------------------------
main()
{
	//--- Объявление переменных для работы с DS18B20 --
  int tempds,HighByte, TReading;
  uint8_t  LowByte, SignBit;
	
	init_CPU();
	init_gpio(); // Инициализация портов ввода-вывода
	init_SPI();
	cc1101_init(); // Настроить трансивер на 433.920 модуляция AM
	

//-------------------------------------------------
//------ Начальная установка для ds18b20 ----------
    GPIO_Init (GPIOC, DS_BIT, GPIO_MODE_OUT_OD_HIZ_FAST); 	// Пин DS_BIT настроен как выход
    GPIO_WriteHigh(GPIOD,GDO0_PIN);		 											
 // GPIO_Init (GPIOC, DS_BIT, GPIO_MODE_IN_FL_NO_IT);					// Пин DS_BIT настроен как вход
//-------------------------------------------------
	while (1) 
	{ // sendRC(777777);  //Тестовая отправка
		 
//------------- Код чтения ds18b20 ----------------         
		OneWireReset();
		OneWireOutByte(0xcc);								// Команда пропуск ROM
		OneWireOutByte(0x44);  							// Команда начала конвертирования температуры (0x44)
																				//на время преобразования
		GPIO_WriteHigh(GPIOC,DS_BIT);				// Поднять пин DS_BIT (включение подтяжки)
		GPIO_Init (GPIOC, DS_BIT,GPIO_MODE_OUT_OD_HIZ_FAST);								// Пин DS_BIT настроен как выход
    		delay(1000000);
	//	GPIO_Init (GPIOC, DS_BIT, GPIO_MODE_IN_FL_NO_IT);								// Пин DS_BIT настроен как вход
	//	GPIO_WriteLow (GPIOC,DS_BIT);				// Сбросить пин DS_BIT (отключение подтяжки)
		OneWireReset();								// Процедура инициализации на шине
		OneWireOutByte(0xcc);					// Команда пропуск ROM
		OneWireOutByte(0xbe);					// Команда чтения памяти Read Scratchpad (0xBE)

		LowByte = OneWireInByte();
		HighByte = OneWireInByte();
     
		TReading = (HighByte << 8) + LowByte;
		SignBit = TReading & 0x8000;  
		if (SignBit)
			{
			TReading = (TReading ^ 0xffff) + 1; 
			}
				// tempds = (6 * TReading) + TReading / 4; 
       			 sendRC(((6 * TReading) + TReading / 4)/10+500+keyT); // отправляем данные			
	     			 // sendRC(TReading); // отправляем данные			
			delay(0x1E8F5C);	//Тестовая задержка				
	}
}
//------------------------------------------------
void init_CPU(void)
	{  
		CLK_DeInit(); 				// Сброс настроек
		CLK_HSICmd(ENABLE);		// Внутренний источник, 16 МГц
		CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1); // предделитель может варьироваться от 1 до 8. 4 = 4 МГц
	}

void init_SPI(void) 
	{
		enableInterrupts();																				// Активируем прерывания
		
		SPI_Init(SPI_FIRSTBIT_MSB, SPI_BAUDRATEPRESCALER_2,	 // Старший бит первым, SPI frequency = frequency(CPU)/2 
		SPI_MODE_MASTER, SPI_CLOCKPOLARITY_LOW, SPI_CLOCKPHASE_1EDGE,  // Режим мастера, Clock to 0 в режиме ожидания,
		SPI_DATADIRECTION_2LINES_FULLDUPLEX, SPI_NSS_SOFT, 0);
		
		//CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, ENABLE);  												// Активируем прерывание по приему байта
		SPI_Cmd(ENABLE);   // Активируем SPI модуль
	}

//----- Инициализация портов интерфейса SPI -------------
void init_gpio(void) 
{
	GPIO_Init (GPIOD, GDO0_PIN, GPIO_MODE_OUT_PP_HIGH_FAST); 	// Output push-pull, high level, 10MHz 
	GPIO_WriteHigh(GPIOD,GDO0_PIN);														// Поднимаем в единицу
	GPIO_Init (GPIOD, CSn_PIN, GPIO_MODE_OUT_PP_HIGH_FAST); 	// Output push-pull, high level, 10MHz 
	GPIO_Init (GPIOC, SCLK_PIN, GPIO_MODE_OUT_PP_HIGH_FAST); 	// Output push-pull, high level, 10MHz 
  	GPIO_Init (GPIOC, MOSI_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);	// Output push-pull, high level, 10MHz 
	GPIO_Init (GPIOC, MISO_PIN, GPIO_MODE_IN_FL_NO_IT);				// Input floating, no external interrupt 
}

//----------- Функция инициализации трансивера CC1101 -------
void cc1101_init(void) 
	{ 
	uint8_t cnt;
	// Придется повыпендриваться, смотри даташит 19.1.2 Manual Reset
		GPIO_WriteLow (GPIOD,CSn_PIN); 	// Устанавливаем CSn в низкий уровень
		delay(10); // сюда вставить задержку не менее 20мкс
	  SPI_SendData(SRES);							// Отправка строба сброс
		delay(150); // Ждем 500мкс
		for (cnt=0;cnt<sizeof(config_433_ook);cnt++) 
			{
			SPI_SendData(config_433_ook[cnt]);
			while ((SPI->SR & (u8)SPI_FLAG_TXE) == RESET) { ; } /* Ждём очистки регистра DR */
			}
		delay(150); // сюда вставить задержку (какую, не знаю пока)
		SPI_SendData(STX); 	//Отправка строба "передача" 
		delay(10); 										// SPI_ReadByte(SNOP);
		GPIO_WriteHigh(GPIOD,CSn_PIN); 	// Устанавливаем на выводе CSn высокий уровень
}
//-------- Функция отправки RemoteSwitch(двоичный протокол) --------------
void sendRC(unsigned long data) // , unsigned short pin 531 440 max 
	{    
	//	unsigned short repeats = 1 << (((unsigned long)data >> 20) & 7);
	 	unsigned short repeats = 5;
	
		unsigned long dataBase4 = 0; 
		uint8_t i,iy;
		unsigned short int j;
		data |= 3L << 20;  										// поразрядное ИЛИ
		data = data & 0xfffff; 								//truncate to 20 bit
	for (i=0; i<20; i++) 
		{
		  dataBase4<<=1;
		  dataBase4|=(data%2);
		  data/=2;														// эквивалентно data=data/2
		}
		// repeats=20;
	for (j=0;j<repeats;j++) 
		{
		  data=dataBase4; 
			
	for (iy=0; iy<20; iy++) 
		{ 
			switch (data & 1) 
				{
					case 0:
						GPIO_WriteLow (GPIOD,GDO0_PIN);				// установить в 0 пин RC_BIT порта B			
						delay(periodusec);
						GPIO_WriteHigh(GPIOD,GDO0_PIN);				// установить в 1 пин RC_BIT порта B  PD3    GDO0_PIN
						delay(periodusec*3);
					break; 
					case 1: 
						GPIO_WriteLow (GPIOD,GDO0_PIN);
						delay(periodusec*3);
						GPIO_WriteHigh(GPIOD,GDO0_PIN);
						delay(periodusec);
					break;
				} 
			data>>=1;
		}
	GPIO_WriteLow(GPIOD,GDO0_PIN);
	delay(periodusec);
	GPIO_WriteHigh(GPIOD,GDO0_PIN);
	delay(periodusec*31);
	} 
}
/*
@far @interrupt void IRQ_Handler_SPI(void)
{
		int res;
		SPI_ClearITPendingBit(SPI_IT_RXNE);
		res = SPI_ReceiveData();
		++res;
}
*/
void delay(volatile uint32_t count) 
{
for ( ; count > 0; --count);
}

/* Пример чтения датчика DS18B20 .
Пример подключения ОДНОГО датчика на ОДИН вывод м/к !! 
размер кода чуть больше 300 байт. 
Взято с http://homes-smart.ru/index.php/oborudovanie/datchiki/ds18b20
 */
 void OneWireReset() 
	{
    	 	GPIO_Init (GPIOC, DS_BIT,GPIO_MODE_OUT_OD_HIZ_FAST);			// Пин DS_BIT настроен как выход
		GPIO_WriteLow (GPIOC,DS_BIT);											// Сбросить пин DS_BIT (отключение подтяжки)
     		delay(500);							// Задержка 500мкс
		GPIO_Init (GPIOC, DS_BIT, GPIO_MODE_IN_FL_NO_IT);	// Отпускаем линию (пин DS_BIT настроен как вход)
     		delay(500);							// Задержка 500мкс
	}
//-------------------------------------------------------
void OneWireOutByte(uint8_t d) 
{
   uint8_t n;
   for(n=8; n!=0; n--)
		{
			if ((d & 0x01) == 1) 
				{	//--- отправляем лог.1 в линию 1-wire
					GPIO_Init (GPIOC, DS_BIT, GPIO_MODE_OUT_OD_HIZ_FAST);			// Пин DS_BIT настроен как выход
					GPIO_WriteLow (GPIOC,DS_BIT);			// Сбросить пин DS_BIT (отключение подтяжки)
				 	delay(5);									// Задержка 5мкс
					GPIO_WriteHigh (GPIOC,DS_BIT);				// Пин DS_BIT настроен как вход
					delay(60);								// Задержка 60мкс
				}
      else
				{	//--- отправляем лог.0 в линию 1-wire
				  	GPIO_Init (GPIOC, DS_BIT, GPIO_MODE_OUT_OD_HIZ_FAST);		// Пин DS_BIT настроен как выход
					GPIO_WriteLow (GPIOC,DS_BIT);			// Сбросить пин DS_BIT (отключение подтяжки)
					delay(60);								// Задержка 60мкс
					GPIO_WriteHigh (GPIOC,DS_BIT);		
				}
      d=d>>1; 
   }
}
//-------------------------------------------------
uint8_t OneWireInByte() 
{
    uint8_t d, n,b;
    for (n=0; n<8; n++)
			{
				GPIO_Init (GPIOC, DS_BIT, GPIO_MODE_OUT_OD_HIZ_FAST);
				GPIO_WriteLow (GPIOC,DS_BIT);			// Сбросить пин DS_BIT (отключение подтяжки)
        			delay(5);													// Задержка 5мкс
				GPIO_Init (GPIOC, DS_BIT, GPIO_MODE_IN_FL_NO_IT);							// Пин DS_BIT настроен как вход
      //  delay(5);													// Задержка 5мкс
			//	b = (GPIO_ReadInputPin(GPIOC,DS_BIT)!= 0);	// читаем значение пина DS_BIT
        if (GPIO_ReadInputPin(GPIOC,DS_BIT)!=0)
					{
					b=0b00000001;
				  	}
					else
					{
					b=0b00000000;
					}
				delay(60);												// Задержка 50мкс
				d = (d >> 1) | (b<<7);
			}
    return (d);
}
//------------------------------------------------------

