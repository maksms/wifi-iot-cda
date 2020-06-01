/***********************************************************
  * @file    /main.c
  * @author  Rapid
  * @version V0.2
  * @date    30-Июль-2018
  * @brief   Main program body
	* датчик температуры на аналоге 8 для проверки шлюза LoRa
  **********************************************************
  */
#include "main.h"
#include "stm8l15x.h"
#include "string.h"    
#include "stdio.h"
#include "stdlib.h"


unsigned int Frame_Counter_Tx = 0x0000;

static const unsigned char S_Table[16][16] = 
{
  {0x63,0x7C,0x77,0x7B,0xF2,0x6B,0x6F,0xC5,0x30,0x01,0x67,0x2B,0xFE,0xD7,0xAB,0x76},
  {0xCA,0x82,0xC9,0x7D,0xFA,0x59,0x47,0xF0,0xAD,0xD4,0xA2,0xAF,0x9C,0xA4,0x72,0xC0},
  {0xB7,0xFD,0x93,0x26,0x36,0x3F,0xF7,0xCC,0x34,0xA5,0xE5,0xF1,0x71,0xD8,0x31,0x15},
  {0x04,0xC7,0x23,0xC3,0x18,0x96,0x05,0x9A,0x07,0x12,0x80,0xE2,0xEB,0x27,0xB2,0x75},
  {0x09,0x83,0x2C,0x1A,0x1B,0x6E,0x5A,0xA0,0x52,0x3B,0xD6,0xB3,0x29,0xE3,0x2F,0x84},
  {0x53,0xD1,0x00,0xED,0x20,0xFC,0xB1,0x5B,0x6A,0xCB,0xBE,0x39,0x4A,0x4C,0x58,0xCF},
  {0xD0,0xEF,0xAA,0xFB,0x43,0x4D,0x33,0x85,0x45,0xF9,0x02,0x7F,0x50,0x3C,0x9F,0xA8},
  {0x51,0xA3,0x40,0x8F,0x92,0x9D,0x38,0xF5,0xBC,0xB6,0xDA,0x21,0x10,0xFF,0xF3,0xD2},
  {0xCD,0x0C,0x13,0xEC,0x5F,0x97,0x44,0x17,0xC4,0xA7,0x7E,0x3D,0x64,0x5D,0x19,0x73},
  {0x60,0x81,0x4F,0xDC,0x22,0x2A,0x90,0x88,0x46,0xEE,0xB8,0x14,0xDE,0x5E,0x0B,0xDB},
  {0xE0,0x32,0x3A,0x0A,0x49,0x06,0x24,0x5C,0xC2,0xD3,0xAC,0x62,0x91,0x95,0xE4,0x79},
  {0xE7,0xC8,0x37,0x6D,0x8D,0xD5,0x4E,0xA9,0x6C,0x56,0xF4,0xEA,0x65,0x7A,0xAE,0x08},
  {0xBA,0x78,0x25,0x2E,0x1C,0xA6,0xB4,0xC6,0xE8,0xDD,0x74,0x1F,0x4B,0xBD,0x8B,0x8A},
  {0x70,0x3E,0xB5,0x66,0x48,0x03,0xF6,0x0E,0x61,0x35,0x57,0xB9,0x86,0xC1,0x1D,0x9E},
  {0xE1,0xF8,0x98,0x11,0x69,0xD9,0x8E,0x94,0x9B,0x1E,0x87,0xE9,0xCE,0x55,0x28,0xDF},
  {0x8C,0xA1,0x89,0x0D,0xBF,0xE6,0x42,0x68,0x41,0x99,0x2D,0x0F,0xB0,0x54,0xBB,0x16}
};

unsigned char NwkSkey[16] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
unsigned char AppSkey[16] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
unsigned char DevAddr[4]  = { 0x2F, 0x2F, 0x2F, 0x2F };

/*
MISO - PB7
MOSI - PB6
SCLK - PB5
CSn -  PB4 настроить как выход, установить в 0 для передачи по SPI интерфейсу
*/

#define CSn_PIN 	  GPIO_Pin_4			//порт PB4
#define SCLK_PIN    GPIO_Pin_5			//порт PB5
#define MOSI_PIN   	GPIO_Pin_6			//порт PB6
#define MISO_PIN   	GPIO_Pin_7			//порт PB7

//------------- незадействованные пины ---------------------

#define PA0   			GPIO_Pin_0			//порт PA0 - незадействован, цепь SWIM
#define PA1					GPIO_Pin_1
#define PA2   			GPIO_Pin_2			//порт PA2 - незадействован
#define PA3   			GPIO_Pin_3			//порт PA3 - незадействован

#define PB0   			GPIO_Pin_0			//порт PB0 - незадействован
#define PB1   			GPIO_Pin_1			//порт PB1 - незадействован
#define PB2   			GPIO_Pin_2			//порт PB2 - незадействован
#define PB3   			GPIO_Pin_3			//порт PB3 - незадействован

#define PC0   			GPIO_Pin_0			//порт PC0 - незадействован
#define PC1   			GPIO_Pin_1			//порт PC0 - незадействован
#define PC4   			GPIO_Pin_4			//порт PC0 - незадействован
#define PC6   			GPIO_Pin_6			//порт PC6 - незадействован в прошивке, идет на DIO_2

#define DS_ON   		GPIO_Pin_0			//порт PD0 - незадействован

//-------- настройки к датчику DS18B20 ---------------------
#define DS_BIT 	  	GPIO_Pin_5 			// порт PC5

//----------------------------------------------------------
#define periodusec 	33		// mcs период
#define TIM4_US_PERIOD 96

//----------------------------------------------------------
void init_SPI(void);
void delay_6us (volatile uint32_t us);
void init_gpio(void);
void init_CPU(void);
void init_TIM4(void);
void init_RTC (void);					
void setup_lora(void);
volatile uint32_t count;

//---------------- Объявление функций LoRa -----------------
void Encrypt_Payload(unsigned char *Data, unsigned char Data_Length, unsigned int Frame_Counter, unsigned char Direction);
void Calculate_MIC(unsigned char *Data, unsigned char *Final_MIC, unsigned char Data_Length, unsigned int Frame_Counter, unsigned char Direction);
void RFM_Send_Package(unsigned char *RFM_Tx_Package, unsigned char Package_Length);
void AES_Encrypt(unsigned char *Data, unsigned char *Key);
void Generate_Keys(unsigned char *K1, unsigned char *K2);
void XOR(unsigned char *New_Data,unsigned char *Old_Data);
void AES_Add_Round_Key(unsigned char *Round_Key, unsigned char (*State)[4]);
void Shift_Left(unsigned char *Data);
void AES_Shift_Rows(unsigned char (*State)[4]);
void AES_Mix_Collums(unsigned char (*State)[4]);
void AES_Calculate_Round_Key(unsigned char Round, unsigned char *Round_Key);
void RFM_Init(void);
unsigned char AES_Sub_Byte(unsigned char Byte);

void OneWireReset(void);
void OneWireOutByte(uint8_t d);
uint8_t OneWireInByte(void);

@far @interrupt void RTC_CSSLSE_IRQHandler(void)
  {
		RTC_ClearITPendingBit(RTC_IT_WUT); //Сбрасываем флаг прерывания
  }  

@far @interrupt void TIM4_UPD_OVF_TRG_IRQHandler(void)
{  
  if(count != 0x00)
  {
    count--;
  }
	  TIM4_ClearITPendingBit(TIM4_IT_Update);
}

void halt_init(void)
{
	GPIO_Init (GPIOB, MOSI_PIN, GPIO_Mode_Out_PP_High_Fast);	// Output push-pull, high level, 10MHz 
	GPIO_Init (GPIOB, CSn_PIN, GPIO_Mode_Out_PP_High_Fast); 	// Output push-pull, high level, 10MHz 
	GPIO_Init (GPIOB, SCLK_PIN, GPIO_Mode_Out_PP_High_Fast); 	// Output push-pull, high level, 10MHz 
	GPIO_Init (GPIOC, DS_BIT, GPIO_Mode_Out_PP_Low_Fast); 	// Пин DS_BIT настроен как выход
}


void halt_deinit(void)
{
	GPIO_Init (GPIOB, SCLK_PIN, GPIO_Mode_Out_PP_High_Fast); 	// Output push-pull, high level, 10MHz 
	GPIO_Init (GPIOB, MOSI_PIN, GPIO_Mode_Out_PP_High_Fast);	// Output push-pull, high level, 10MHz 
	GPIO_Init (GPIOB, CSn_PIN, GPIO_Mode_Out_PP_High_Fast); 	// Output push-pull, high level, 10MHz 
}

//------ Инициализация часов реального времени (RTC)--------
	void init_RTC(void)
	{
		CLK_PeripheralClockConfig(CLK_Peripheral_RTC, ENABLE);			// Разрешаем тактирование RTC модуля
		CLK_RTCClockConfig(CLK_RTCCLKSource_LSI, CLK_RTCCLKDiv_64); 	// Enable RTC clock 
		RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16); 				//time_step = 38кГц/64/16 = 27мс
		RTC_ITConfig(RTC_IT_WUT,ENABLE);
	}
	
	//------------- Переключение HSI-LSI------------------------
void switch_HSI_LSI(void)
	{
		CLK_LSICmd(ENABLE); 															// включение LSI генератора 38кГц (set LSION)
		CLK_SYSCLKSourceSwitchCmd(ENABLE);								// Enables switching the system clock (bit SWEN)
			while(CLK_GetFlagStatus(CLK_FLAG_LSIRDY)==RESET){};	// Ждем готовности источника тактирования
		CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_LSI); 		// choose LSI
			while(CLK_GetFlagStatus(CLK_FLAG_SWBSY)==SET){};		// Ждем готовности переключения 
		CLK_HaltConfig(CLK_Halt_FastWakeup,ENABLE);
	} 
	//------------- Инициализация таймера TIM4 ---------------
void init_TIM4(void)
	{
	  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
	  TIM4_DeInit();	
	}
  //------- Инициализация SPI интерфейса ---------------------
void init_SPI(void) 
	{
		CLK_PeripheralClockConfig (CLK_Peripheral_SPI1,ENABLE); // Разрешаем тактирование SPI модуля
		SPI_Init(SPI1,SPI_FirstBit_MSB, SPI_BaudRatePrescaler_16,//Старший бит первым, SPI frequency = f(CPU)/2 
		SPI_Mode_Master, SPI_CPOL_Low, SPI_CPHA_1Edge,  				// Режим мастера, Clock to 0 в режиме ожидания,
		SPI_Direction_2Lines_FullDuplex, SPI_NSS_Soft, 0);
		
		SPI_Cmd(SPI1,ENABLE);   // Активируем SPI модуль
	}

void init_CPU(void)
	{  
		CLK_DeInit(); 				// Сброс настроек
		CLK_HSICmd(ENABLE);		// Внутренний источник, 16 МГц
		CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1); // устанавливаем предделитель на 4  
		CLK_PeripheralClockConfig (CLK_Peripheral_BOOTROM,DISABLE);// Disable Boot ROM clock
	}
	//----- Инициализация портов интерфейса SPI -------------
void init_gpio(void) 
{
		GPIO_DeInit(GPIOA);
		GPIO_Init (GPIOA, PA0, GPIO_Mode_In_PU_No_IT);  //Перекл. все неиспольз. пины на вход с подтяжкой
		GPIO_Init (GPIOA, PA1, GPIO_Mode_In_FL_No_IT);  //Перекл. все неиспольз. пины на вход с подтяжкой
		GPIO_Init (GPIOA, PA2, GPIO_Mode_Out_PP_Low_Fast);  //Перекл. все неиспольз. пины на вход с подтяжкой
		GPIO_Init (GPIOA, PA3, GPIO_Mode_Out_PP_Low_Fast);  //Перекл. все неиспольз. пины на вход с подтяжкой
		
		GPIO_DeInit(GPIOB);
	  GPIO_Init (GPIOB, PB0, GPIO_Mode_Out_PP_Low_Fast);  //Перекл. все неиспольз. пины на вход с подтяжкой
		GPIO_Init (GPIOB, PB1, GPIO_Mode_Out_PP_Low_Fast);  //Перекл. все неиспольз. пины на вход с подтяжкой
		GPIO_Init (GPIOB, PB2, GPIO_Mode_Out_PP_Low_Fast);  //Перекл. все неиспольз. пины на вход с подтяжкой
		GPIO_Init (GPIOB, PB3, GPIO_Mode_Out_PP_Low_Fast);  //Перекл. все неиспольз. пины на вход с подтяжкой
		
	  GPIO_Init (GPIOB, CSn_PIN, GPIO_Mode_Out_PP_High_Fast); 	// Output push-pull, high level, 10MHz 
	  GPIO_Init (GPIOB, SCLK_PIN, GPIO_Mode_Out_PP_High_Fast); 	// Output push-pull, high level, 10MHz 
    GPIO_Init (GPIOB, MOSI_PIN, GPIO_Mode_Out_PP_High_Fast);	// Output push-pull, high level, 10MHz 
   	GPIO_Init (GPIOB, MISO_PIN, GPIO_Mode_In_PU_No_IT);				// Input floating, no external interrupt 
		
		GPIO_DeInit(GPIOC);
		GPIO_Init (GPIOC, PC0, GPIO_Mode_Out_OD_Low_Slow);  //Перекл. все неиспольз. пины на вход с подтяжкой
		GPIO_Init (GPIOC, PC1, GPIO_Mode_Out_OD_Low_Slow);  //Перекл. все неиспольз. пины на вход с подтяжкой
	  GPIO_Init (GPIOC, PC4, GPIO_Mode_Out_PP_High_Fast);  //Перекл. все неиспольз. пины на вход с подтяжкой
		GPIO_Init (GPIOC, PC6, GPIO_Mode_Out_PP_High_Fast);  //Перекл. все неиспольз. пины на вход с подтяжкой
		
		GPIO_DeInit(GPIOD);
		GPIO_Init (GPIOD, DS_ON, GPIO_Mode_Out_PP_Low_Fast);  //Перекл. все неиспольз. пины на вход с подтяжкой
}

//--------- 6-ти микросекундная задержка на таймере 4 ---------
void delay_6us(volatile uint32_t us)
{  
		TIM4_Cmd(DISABLE);       // stop
		TIM4_TimeBaseInit(TIM4_Prescaler_2, TIM4_US_PERIOD);
		TIM4_ClearFlag(TIM4_FLAG_Update);
		TIM4_ITConfig(TIM4_IT_Update, ENABLE);
		count = us>>1;
		TIM4_Cmd(ENABLE);       // let's go
		while(count!=0);
		TIM4_Cmd(DISABLE);       // stop
}

/*
*****************************************************************************************
* Description : Function contstructs a LoRaWAN package and sends it
*
* Arguments   : *Data pointer to the array of data that will be transmitted
*               Data_Length nuber of bytes to be transmitted
*               Frame_Counter_Up  Frame counter of upstream frames
*****************************************************************************************
*/
void LORA_Send_Data(unsigned char *Data, unsigned char Data_Length, unsigned int Frame_Counter_Tx)
{
  unsigned char i; //Define variables
  unsigned char Direction = 0x00; //Direction of frame is up

  unsigned char RFM_Data[64];
  unsigned char RFM_Package_Length;

  unsigned char MIC[4];
  unsigned char Mac_Header = 0x40;  //Unconfirmed data up

  unsigned char Frame_Control = 0x00;
  unsigned char Frame_Port = 0x01;

  Encrypt_Payload(Data, Data_Length, Frame_Counter_Tx, Direction);	//Encrypt the data

  RFM_Data[0] = Mac_Header;	//Build the Radio Package

  RFM_Data[1] = DevAddr[3];
  RFM_Data[2] = DevAddr[2];
  RFM_Data[3] = DevAddr[1];
  RFM_Data[4] = DevAddr[0];

  RFM_Data[5] = Frame_Control;

  RFM_Data[6] = (Frame_Counter_Tx & 0x00FF);
  RFM_Data[7] = ((Frame_Counter_Tx >> 8) & 0x00FF);

  RFM_Data[8] = Frame_Port;

  RFM_Package_Length = 9;	//Set Current package length

  for(i = 0; i < Data_Length; i++)	//Load Data
  {
    RFM_Data[RFM_Package_Length + i] = Data[i];
  }

  RFM_Package_Length = RFM_Package_Length + Data_Length;	//Add data Lenth to package length
  
  Calculate_MIC(RFM_Data, MIC, RFM_Package_Length, Frame_Counter_Tx, Direction);	//Calculate MIC

  for(i = 0; i < 4; i++)			//Load MIC in package
  {
    RFM_Data[i + RFM_Package_Length] = MIC[i];
  }
	
  RFM_Package_Length = RFM_Package_Length + 4;	//Add MIC length to RFM package length
 
  RFM_Send_Package(RFM_Data, RFM_Package_Length);	//Send Package
}
/*
*****************************************************************************************
* Description : Funtion that writes a register from the RFM
*
* Arguments   : RFM_Address Address of register to be written
*         RFM_Data    Data to be written
*****************************************************************************************
*/
void RFM_Write(unsigned char RFM_Address, unsigned char RFM_Data)
{
	GPIO_ResetBits (GPIOB,CSn_PIN); 					// Устанавливаем CSn в низкий уровень
		SPI_SendData(SPI1,RFM_Address | 0x80);		//Send Addres with MSB 1 to make it a writ command
	while ((SPI1->SR & (u8)SPI_FLAG_TXE) == RESET) { ; } /* Ждём очистки регистра SR */
		SPI_SendData(SPI1,RFM_Data);		//Send Addres with MSB 1 to make it a writ command
//	while ((SPI1->SR & (u8)SPI_FLAG_TXE) == RESET) { ; } /* Ждём очистки регистра SR */
    delay_6us (1);
	  GPIO_SetBits (GPIOB,CSn_PIN); 					// Устанавливаем CSn в высокий уровень
}
/*
*****************************************************************************************
* Description: Function used to initialize the RFM module on startup
*****************************************************************************************
*/
void RFM_Init()
{
  RFM_Write(0x01,0x00);		// Переводим RFM в режим Sleep
  RFM_Write(0x01,0x80);		//Set RFM in LoRa mode
  RFM_Write(0x01,0x81);		//Set RFM in Standby mode wait on mode ready
  /*
  while (digitalRead(DIO5) == LOW)
  {
  }
  */
  delay_6us(500);
  //Set carrair frequency
  // 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B  433.300 MHz =70991897 =  0x6C5333
  RFM_Write(0x06,0xD9);
  RFM_Write(0x07,0x06);
  RFM_Write(0x08,0x8B);

  RFM_Write(0x09,0x88); //PA pin disable, Pout=+10.8dB
	RFM_Write(0x0B,0x2B); //ограничение по току 100мА

  RFM_Write(0x1D,0x72);	//BW = 125 kHz, Coding rate 4/5, Explicit header mode

  RFM_Write(0x1E,0xA4);	//Spreading factor 7, PayloadCRC On

  RFM_Write(0x1F,0x25);	//Rx Timeout set to 37 symbols

  RFM_Write(0x20,0x00);	//Preamble length set to 8 symbols
  RFM_Write(0x21,0x08);	//0x0008 + 4 = 12

  RFM_Write(0x26,0x0C); //Low datarate optimization off AGC auto on

  RFM_Write(0x39,0x34);//Set LoRa sync word

  RFM_Write(0x33,0x27);//Set IQ to normal values
  RFM_Write(0x3B,0x1D);

  //Set FIFO pointers
  RFM_Write(0x0E,0x80);  //TX base adress
  RFM_Write(0x0F,0x00);	//Rx base adress

  RFM_Write(0x01,0x00);  //Switch RFM to sleep
}
/*
*****************************************************************************************
* Description : Function for sending a package with the RFM
*
* Arguments   : *RFM_Tx_Package Pointer to arry with data to be send
*               Package_Length  Length of the package to send
*****************************************************************************************
*/

void RFM_Send_Package(unsigned char *RFM_Tx_Package, unsigned char Package_Length)
{
  unsigned char i;
  unsigned char RFM_Tx_Location = 0x00;

  RFM_Write(0x01,0x81);	//Set RFM in Standby mode wait on mode ready
  /*
  while (digitalRead(DIO5) == LOW)
  {
  }
  */
  delay_6us(500);
  
  RFM_Write(0x40,0x40);  //Switch DIO0 to TxDone

  //Set carrair frequency
  // 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B  =7099205 =  0x6C5345
  RFM_Write(0x06,0xD9);
  RFM_Write(0x07,0x06);
  RFM_Write(0x08,0x8B);

  //SF7 BW 125 kHz
	 RFM_Write(0x1D,0x72); //125 kHz 4/5 coding rate explicit header mode
	 
  RFM_Write(0x1E,0xA4); //SF7 CRC On
 
  RFM_Write(0x26,0x04); //Low datarate optimization off AGC auto on
	
  RFM_Write(0x33,0x27);  //Set IQ to normal values
  RFM_Write(0x3B,0x1D);

  RFM_Write(0x22,Package_Length);  //Set payload length to the right length

  //Get location of Tx part of FiFo
  //RFM_Tx_Location = RFM_Read(0x0E);

  //Set SPI pointer to start of Tx part in FiFo
  //RFM_Write(0x0D,RFM_Tx_Location);
  RFM_Write(0x0D,0x80); // hardcoded fifo location according RFM95 specs

  //Write Payload to FiFo
  for (i = 0;i < Package_Length; i++)
  {
    RFM_Write(0x00,*RFM_Tx_Package);
    RFM_Tx_Package++;
  }
	
  RFM_Write(0x01,0x83);  //Switch RFM to Tx

//while (GPIO_ReadInputDataBit(GPIOC,DS_BIT)==0)	//  while(digitalRead(DIO0) == LOW)//Wait for TxDone
//  {
//  }

delay_6us(130000);
  RFM_Write(0x01,0x00);  //Switch RFM to sleep
	delay_6us(1000);
}

void Encrypt_Payload(unsigned char *Data, unsigned char Data_Length, unsigned int Frame_Counter, unsigned char Direction)
{
  unsigned char i = 0x00;
  unsigned char j;
  unsigned char Number_of_Blocks = 0x00;
  unsigned char Incomplete_Block_Size = 0x00;

  unsigned char Block_A[16];

  Number_of_Blocks = Data_Length / 16;  //Calculate number of blocks
  Incomplete_Block_Size = Data_Length % 16;
  if(Incomplete_Block_Size != 0)
  {
    Number_of_Blocks++;
  }

  for(i = 1; i <= Number_of_Blocks; i++)
  {
    Block_A[0] = 0x01;
    Block_A[1] = 0x00;
    Block_A[2] = 0x00;
    Block_A[3] = 0x00;
    Block_A[4] = 0x00;

    Block_A[5] = Direction;

    Block_A[6] = DevAddr[3];
    Block_A[7] = DevAddr[2];
    Block_A[8] = DevAddr[1];
    Block_A[9] = DevAddr[0];

    Block_A[10] = (Frame_Counter & 0x00FF);
    Block_A[11] = ((Frame_Counter >> 8) & 0x00FF);

    Block_A[12] = 0x00; //Frame counter upper Bytes
    Block_A[13] = 0x00;

    Block_A[14] = 0x00;

    Block_A[15] = i;

    
    AES_Encrypt(Block_A,AppSkey); //Calculate S //original
    
	
    if(i != Number_of_Blocks)//Check for last block
    {
      for(j = 0; j < 16; j++)
      {
        *Data = *Data ^ Block_A[j];
        Data++;
      }
    }
    else
    {
      if(Incomplete_Block_Size == 0)
      {
        Incomplete_Block_Size = 16;
      }
      for(j = 0; j < Incomplete_Block_Size; j++)
      {
        *Data = *Data ^ Block_A[j];
        Data++;
      }
    }
  }
}

void Calculate_MIC(unsigned char *Data, unsigned char *Final_MIC, unsigned char Data_Length, unsigned int Frame_Counter, unsigned char Direction)
{
  unsigned char i;
  unsigned char Block_B[16];
  
  unsigned char Key_K1[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  unsigned char Key_K2[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  //unsigned char Data_Copy[16];

  unsigned char Old_Data[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  unsigned char New_Data[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  
  
  unsigned char Number_of_Blocks = 0x00;
  unsigned char Incomplete_Block_Size = 0x00;
  unsigned char Block_Counter = 0x01;

  Block_B[0] = 0x49;  //Create Block_B
  Block_B[1] = 0x00;
  Block_B[2] = 0x00;
  Block_B[3] = 0x00;
  Block_B[4] = 0x00;

  Block_B[5] = Direction;

  Block_B[6] = DevAddr[3];
  Block_B[7] = DevAddr[2];
  Block_B[8] = DevAddr[1];
  Block_B[9] = DevAddr[0];

  Block_B[10] = (Frame_Counter & 0x00FF);
  Block_B[11] = ((Frame_Counter >> 8) & 0x00FF);

  Block_B[12] = 0x00; //Frame counter upper bytes
  Block_B[13] = 0x00;

  Block_B[14] = 0x00;
  Block_B[15] = Data_Length;

  Number_of_Blocks = Data_Length / 16;  //Calculate number of Blocks and blocksize of last block
  Incomplete_Block_Size = Data_Length % 16;

  if(Incomplete_Block_Size != 0)
  {
    Number_of_Blocks++;
  }

  Generate_Keys(Key_K1, Key_K2);

  //Preform Calculation on Block B0
	
  AES_Encrypt(Block_B,NwkSkey);	//Preform AES encryption

  for(i = 0; i < 16; i++)  //Copy Block_B to Old_Data
  {
    Old_Data[i] = Block_B[i];
  }

  //Preform full calculating until n-1 messsage blocks
  while(Block_Counter < Number_of_Blocks)
  {
    //Copy data into array
    for(i = 0; i < 16; i++)
    {
      New_Data[i] = *Data;
      Data++;
    }

    XOR(New_Data,Old_Data);		//Preform XOR with old data
    AES_Encrypt(New_Data,NwkSkey);	//Preform AES encryption

    //Copy New_Data to Old_Data
    for(i = 0; i < 16; i++)
    {
      Old_Data[i] = New_Data[i];
    }

    Block_Counter++;	//Raise Block counter
  }

  //Perform calculation on last block
  //Check if Datalength is a multiple of 16
  if(Incomplete_Block_Size == 0)
  {
    //Copy last data into array
    for(i = 0; i < 16; i++)
    {
      New_Data[i] = *Data;
      Data++;
    }

    XOR(New_Data,Key_K1);	//Preform XOR with Key 1
    XOR(New_Data,Old_Data);	//Preform XOR with old data
    
    //Preform last AES routine
    // read NwkSkey from PROGMEM
    AES_Encrypt(New_Data,NwkSkey);
  }
  else
  {
    //Copy the remaining data and fill the rest
    for(i =  0; i < 16; i++)
    {
      if(i < Incomplete_Block_Size)
      {
        New_Data[i] = *Data;
        Data++;
      }
      if(i == Incomplete_Block_Size)
      {
        New_Data[i] = 0x80;
      }
      if(i > Incomplete_Block_Size)
      {
        New_Data[i] = 0x00;
      }
    }

    XOR(New_Data,Key_K2);    //Preform XOR with Key 2
    XOR(New_Data,Old_Data);    //Preform XOR with Old data

    AES_Encrypt(New_Data,NwkSkey);    //Preform last AES routine
  }

  Final_MIC[0] = New_Data[0];
  Final_MIC[1] = New_Data[1];
  Final_MIC[2] = New_Data[2];
  Final_MIC[3] = New_Data[3];
}

void Generate_Keys(unsigned char *K1, unsigned char *K2)
{
  unsigned char i;
  unsigned char MSB_Key;

  AES_Encrypt(K1,NwkSkey);  //Encrypt the zeros in K1 with the NwkSkey

  //Create K1
  //Check if MSB is 1
  if((K1[0] & 0x80) == 0x80)
  {
    MSB_Key = 1;
  }
  else
  {
    MSB_Key = 0;
  }

  Shift_Left(K1);	//Shift K1 one bit left

  if(MSB_Key == 1)//if MSB was 1
  {
    K1[15] = K1[15] ^ 0x87;
  }
  
  for( i = 0; i < 16; i++) //Copy K1 to K2
  {
    K2[i] = K1[i];
  }

  if((K2[0] & 0x80) == 0x80)  //Check if MSB is 1
  {
    MSB_Key = 1;
  }
  else
  {
    MSB_Key = 0;
  }

  Shift_Left(K2);  //Shift K2 one bit left

  //Check if MSB was 1
  if(MSB_Key == 1)
  {
    K2[15] = K2[15] ^ 0x87;
  }
}

void Shift_Left(unsigned char *Data)
{
  unsigned char i;
  unsigned char Overflow = 0;
  //unsigned char High_Byte, Low_Byte;

  for(i = 0; i < 16; i++)
  {
    //Check for overflow on next byte except for the last byte
    if(i < 15)
    {
      //Check if upper bit is one
      if((Data[i+1] & 0x80) == 0x80)
      {
        Overflow = 1;
      }
      else
      {
        Overflow = 0;
      }
    }
    else
    {
      Overflow = 0;
    }
		
    Data[i] = (Data[i] << 1) + Overflow;    //Shift one left
  }
}

void XOR(unsigned char *New_Data,unsigned char *Old_Data)
{
  unsigned char i;

  for(i = 0; i < 16; i++)
  {
    New_Data[i] = New_Data[i] ^ Old_Data[i];
  }
}

/*
*****************************************************************************************
* Title         : AES_Encrypt
* Description  : 
*****************************************************************************************
*/
void AES_Encrypt(unsigned char *Data, unsigned char *Key)
{
  unsigned char Row, Column, Round = 0;
  unsigned char Round_Key[16];
	unsigned char State[4][4];

  //  Copy input to State arry
  for( Column = 0; Column < 4; Column++ )
  {
    for( Row = 0; Row < 4; Row++ )
    {
      State[Row][Column] = Data[Row + (Column << 2)];
    }
  }

  memcpy( &Round_Key[0], &Key[0], 16 ); //  Copy key to round key
	
  AES_Add_Round_Key( Round_Key, State );	//  Add round key

  for( Round = 1 ; Round < 10 ; Round++ )	//  Preform 9 full rounds with mixed collums
  {
    //  Perform Byte substitution with S table
    for( Column = 0 ; Column < 4 ; Column++ )
    {
      for( Row = 0 ; Row < 4 ; Row++ )
      {
        State[Row][Column] = AES_Sub_Byte( State[Row][Column] );
      }
    }

    AES_Shift_Rows(State);		//  Perform Row Shift

    AES_Mix_Collums(State);		//  Mix Collums

    AES_Calculate_Round_Key(Round, Round_Key);	//  Calculate new round key

    AES_Add_Round_Key(Round_Key, State);		//  Add the round key to the Round_key
  }

  //  Perform Byte substitution with S table whitout mix collums
  for( Column = 0 ; Column < 4 ; Column++ )
  {
    for( Row = 0; Row < 4; Row++ )
    {
      State[Row][Column] = AES_Sub_Byte(State[Row][Column]);
    }
  }
    
  AES_Shift_Rows(State); //  Shift rows

  AES_Calculate_Round_Key( Round, Round_Key ); //  Calculate new round key

  AES_Add_Round_Key( Round_Key, State );		//  Add round key

  //  Copy the State into the data array
  for( Column = 0; Column < 4; Column++ )
  {
    for( Row = 0; Row < 4; Row++ )
    {
      Data[Row + (Column << 2)] = State[Row][Column];
    }
  }
} // AES_Encrypt

/*
*****************************************************************************************
* Title         : AES_Add_Round_Key
* Description : 
*****************************************************************************************
*/
void AES_Add_Round_Key(unsigned char *Round_Key, unsigned char (*State)[4])
{
  unsigned char Row, Collum;

  for(Collum = 0; Collum < 4; Collum++)
  {
    for(Row = 0; Row < 4; Row++)
    {
      State[Row][Collum] ^= Round_Key[Row + (Collum << 2)];
    }
  }
} 

/*
*****************************************************************************************
* Title         : AES_Sub_Byte
* Description : 
*****************************************************************************************
*/
unsigned char AES_Sub_Byte(unsigned char Byte)
{
//  unsigned char S_Row,S_Collum;
//  unsigned char S_Byte;
//
//  S_Row    = ((Byte >> 4) & 0x0F);
//  S_Collum = ((Byte >> 0) & 0x0F);
//  S_Byte   = S_Table [S_Row][S_Collum];

  return S_Table [ ((Byte >> 4) & 0x0F) ] [ ((Byte >> 0) & 0x0F) ]; // original
  
} //    AES_Sub_Byte

/*
*****************************************************************************************
* Title         : AES_Shift_Rows
* Description : 
*****************************************************************************************
*/
void AES_Shift_Rows(unsigned char (*State)[4])
{
  unsigned char Buffer;

  //Store firt byte in buffer
  Buffer      = State[1][0];
  //Shift all bytes
  State[1][0] = State[1][1];
  State[1][1] = State[1][2];
  State[1][2] = State[1][3];
  State[1][3] = Buffer;

  Buffer      = State[2][0];
  State[2][0] = State[2][2];
  State[2][2] = Buffer;
  Buffer      = State[2][1];
  State[2][1] = State[2][3];
  State[2][3] = Buffer;

  Buffer      = State[3][3];
  State[3][3] = State[3][2];
  State[3][2] = State[3][1];
  State[3][1] = State[3][0];
  State[3][0] = Buffer;
}   //  AES_Shift_Rows

/*
*****************************************************************************************
* Title         : AES_Mix_Collums
* Description : 
*****************************************************************************************
*/
void AES_Mix_Collums(unsigned char (*State)[4])
{
  unsigned char Row,Collum;
  unsigned char a[4], b[4];
    
    
  for(Collum = 0; Collum < 4; Collum++)
  {
    for(Row = 0; Row < 4; Row++)
    {
      a[Row] =  State[Row][Collum];
      b[Row] = (State[Row][Collum] << 1);

      if((State[Row][Collum] & 0x80) == 0x80)
      {
        b[Row] ^= 0x1B;
      }
    }
        
    State[0][Collum] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
    State[1][Collum] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
    State[2][Collum] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
    State[3][Collum] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
  }
}   //  AES_Mix_Collums

/*
*****************************************************************************************
* Title       : AES_Calculate_Round_Key
* Description : 
*****************************************************************************************
*/
void AES_Calculate_Round_Key(unsigned char Round, unsigned char *Round_Key)
{
  unsigned char i, j, b, Rcon;
  unsigned char Temp[4];

    //Calculate Rcon
  Rcon = 0x01;
  while(Round != 1)
  {
    b = Rcon & 0x80;
    Rcon = Rcon << 1;
        
    if(b == 0x80)
    {
      Rcon ^= 0x1b;
    }
    Round--;
  }
    
  //  Calculate first Temp
  //  Copy laste byte from previous key and subsitute the byte, but shift the array contents around by 1.
    Temp[0] = AES_Sub_Byte( Round_Key[12 + 1] );
    Temp[1] = AES_Sub_Byte( Round_Key[12 + 2] );
    Temp[2] = AES_Sub_Byte( Round_Key[12 + 3] );
    Temp[3] = AES_Sub_Byte( Round_Key[12 + 0] );

  Temp[0] ^= Rcon;		//  XOR with Rcon

  //  Calculate new key
  for(i = 0; i < 4; i++)
  {
    for(j = 0; j < 4; j++)
    {
      Round_Key[j + (i << 2)]  ^= Temp[j];
      Temp[j]                   = Round_Key[j + (i << 2)];
    }
  }
}  

void main(void)
{
	int  HighByte,TReading; 
  uint8_t LowByte,SignBit;
	int  tempds;
	
	init_CPU();						  // Инициализация тактовой частоты микроконтроллера
	init_gpio(); 					  // Инициализация портов ввода-вывода в том числе  pinMode(NSS, OUTPUT);
	init_SPI();							// Инициализация SPI интерфейса
	init_TIM4();						// Инициализация TIM4
  init_RTC ();					  // Инициализация RTC
	enableInterrupts();
	
 	GPIO_SetBits (GPIOB,CSn_PIN); 	// Устанавливаем CSn в высокий уровень
	
  RFM_Init();											//Инициализация RFM модуля
  delay_6us(500); 

	while (1) 
		{ 
	
	  uint16_t temp;
	  uint8_t Data[8];
	  uint8_t Data_Length = 0x00;
		
		GPIO_SetBits(GPIOD, DS_ON);					// Подать питание на датчик ds18b20
		delay_6us(1000); 											// Задержка около 200 мкс
		
		//----------- Запуск преобразования ds18b20 ---------------
	  OneWireReset();											// Формируем сброс на шине
		OneWireOutByte(0xcc);								// Отправляем команду "пропуск ROM"
		OneWireOutByte(0x44);  							// Отправляем команду начала конвертирования температуры (0x44)
		GPIO_Init(GPIOC,DS_BIT,GPIO_Mode_In_FL_No_IT);	// Пин DS_BIT настроен как вход
															
    //----------- уходим на 1 секунду в halt режим -------------
	  halt_init();
		switch_HSI_LSI();										// переключаем тактирование
		RTC_SetWakeUpCounter(37);						// Прерывание через 1 секунду
		RTC_WakeUpCmd(ENABLE);							// Активируем RTC Wakeup модуль
		CLK_HSICmd(DISABLE);								// Отключаем Internal High Speed oscillator (HSI)
		CLK_HSEConfig(CLK_HSE_OFF);					// Отключаем External High Speed oscillator (HSE)
		PWR_UltraLowPowerCmd(ENABLE);				// Активируем режим Ultra Low Power 
	  halt();															// Уходим в режим очень глубокого сна
		
//-------------------- Просыпаемся -------------------------
		CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);	// Восстанавливаем частоту с Fclk/8 до Fclk
  	RTC_WakeUpCmd(DISABLE);								// Отключаем RTC Wakeup модуль (наверное нужо остановить)
//--------- Читаем показания температуры ds18b20 -----------
		OneWireReset();											// Формируем сброс на шине
		OneWireOutByte(0xcc);								// Отправляем команду пропуск ROM
		OneWireOutByte(0xbe);								// Отправляем команду  чтения памяти Read Scratchpad (0xBE)
		
		LowByte = OneWireInByte();					// Читаем младший байт
		HighByte = OneWireInByte();					// Читаем старший байт
		GPIO_ResetBits(GPIOD, DS_ON);														// снимаем питание с  ds18b20
		TReading = (HighByte << 8) + LowByte;
		SignBit = TReading & 0x8000;  
		if (SignBit)
			{
				TReading = (TReading ^ 0xffff) + 1; 
			} 
			tempds=((6 * TReading) + TReading / 4)/10;
	//Data_Length=sprintf(Data,"t:%d.%+d;", tempds/10,tempds%10);
	//Data_Length=sprintf(Data,"t:%f;", tempds);
	Data_Length=sprintf (Data,"t:%s%d.%d;",(tempds < 0 ? "-" : ""),abs(tempds/10),abs(tempds%10));
	
  //------ Выводим CC1101 из режима SLEEP в режим "Передача" ------
		halt_deinit();

	  LORA_Send_Data(Data, Data_Length, Frame_Counter_Tx);
		Frame_Counter_Tx++;
//------------------- уходим в halt режим ----------------------
   
	  halt_init(); 
		switch_HSI_LSI();											// переключаем тактирование
		RTC_SetWakeUpCounter(33300);					// Прерывание через 15 минут
		RTC_WakeUpCmd(ENABLE);								// Активируем RTC Wakeup модуль
		CLK_HSICmd(DISABLE);									// Отключаем Internal High Speed oscillator (HSI)
		CLK_HSEConfig(CLK_HSE_OFF);						// Отключаем External High Speed oscillator (HSE)
		PWR_UltraLowPowerCmd(ENABLE);					// Активируем режим Ultra Low Power
	  halt();																// Уходим в режим очень глубокого сна
		//------------------- Просыпаемся ------------------------------
		CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);	// Восстанавливаем частоту с Fclk/8 до Fclk
		RTC_WakeUpCmd(DISABLE);
		} 
	} 
	
	// ----------- функция сброса на шине 1-wire ------------
 void OneWireReset() 
	{
     GPIO_Init(GPIOC,DS_BIT,GPIO_Mode_Out_OD_HiZ_Fast);			// Пин DS_BIT настроен как открытый коллектор
		 GPIO_ResetBits(GPIOC,DS_BIT);													// Прижимаем шину к земле на 500мкс
		 delay_6us(84);																					// Задержка около 500 мкс
		 GPIO_Init(GPIOC,DS_BIT,GPIO_Mode_In_FL_No_IT);					// Настраиваем линию DS_BIT на вход
     delay_6us(84);																					// Задержка около  500 мкс
	}
	//-------------------------------------------------------
void OneWireOutByte(uint8_t d) 
{
   uint8_t n;
	 GPIO_Init (GPIOC, DS_BIT, GPIO_Mode_Out_OD_HiZ_Fast);			// Пин DS_BIT настроен как выход
   for(n=8; n!=0; n--)
		{
			if ((d & 0x01) == 1) 
				{	//--- формируем лог.1 на линии 1-wire
					GPIO_ResetBits (GPIOC,DS_BIT);					// Притягиваем пин DS_BIT к земле
				  delay_6us(1);														// Задержка 5мкс (6 мкс)
					GPIO_SetBits (GPIOC,DS_BIT);						// Отпускаем пин DS_BIT
					delay_6us(10);													// Задержка 60мкс
				}
      else
				{	//--- формируем лог.0 на линии 1-wire
					GPIO_ResetBits (GPIOC,DS_BIT);					// Притягиваем пин DS_BIT к земле
					delay_6us(10);													// Задержка 60мкс
					GPIO_SetBits (GPIOC,DS_BIT);						// 		
				}
      d=d>>1; 
   }
	 GPIO_Init(GPIOC,DS_BIT,GPIO_Mode_In_FL_No_IT);	// Настраиваем линию DS_BIT на вход
}
//-------------------------------------------------
uint8_t OneWireInByte() 
{
    uint8_t d, n,b;
    for (n=0; n<8; n++)
			{
				GPIO_Init (GPIOC, DS_BIT, GPIO_Mode_Out_OD_HiZ_Fast);	// Пин DS_BIT настроен как выход
				GPIO_ResetBits (GPIOC,DS_BIT);												// Притягиваем пин DS_BIT к земле
        delay_6us(1);																					// Задержка около 6мкс
				GPIO_Init (GPIOC, DS_BIT, GPIO_Mode_In_FL_No_IT);			// Настраиваем DS_BIT на вход
        if (GPIO_ReadInputDataBit(GPIOC,DS_BIT)!=0)
					{
						b=0b00000001;
				  }
					else
					{
						b=0b00000000;
					}
				delay_6us(10);							// Задержка около 60мкс
				d = (d >> 1) | (b<<7);
			}
    return (d);
		GPIO_Init(GPIOC,DS_BIT,GPIO_Mode_In_FL_No_IT);	// Настраиваем линию DS_BIT на вход
}
	
