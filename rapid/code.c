#define SRES 0x30
#define SRX 0x34
#define STX 0x35
#define SNOP 0x3D

#define IOCFG2     0x00
#define IOCFG1     0x01
#define IOCFG0     0x02
#define FIFOTHR    0x03
#define SYNC1      0x04
#define SYNC0      0x05
#define PKTLEN     0x06
#define PKTCTRL1   0x07
#define PKTCTRL0   0x08
#define ADDR       0x09
#define CHANNR     0x0A
#define FSCTRL1    0x0B
#define FSCTRL0    0x0C
#define FREQ2      0x0D
#define FREQ1      0x0E
#define FREQ0      0x0F
#define MDMCFG4    0x10
#define MDMCFG3    0x11
#define MDMCFG2    0x12
#define MDMCFG1    0x13
#define MDMCFG0    0x14
#define DEVIATN    0x15
#define MCSM2      0x16
#define MCSM1      0x17
#define MCSM0      0x18
#define FOCCFG     0x19
#define BSCFG      0x1A
#define AGCTRL2    0x1B
#define AGCTRL1    0x1C
#define AGCTRL0    0x1D
#define WOREVT1    0x1E
#define WOREVT0    0x1F
#define WORCTRL    0x20
#define FREND1     0x21
#define FREND0     0x22
#define FSCAL3     0x23
#define FSCAL2     0x24
#define FSCAL1     0x25
#define FSCAL0     0x26
#define RCCTRL1    0x27
#define RCCTRL0    0x28
#define FSTEST     0x29
#define PTEST      0x2A
#define AGCTEST    0x2B
#define TEST2      0x2C
#define TEST1      0x2D
#define TEST0      0x2E

char config_433_ook[]={
IOCFG0, 0x7F,
IOCFG2, 0x0D,
FIFOTHR, 0x67,    // аттенюатор по приему на 12дБ
PKTCTRL0,0x32,
FSCTRL1, 0x06,
FSCTRL0, 0x00,
FREQ2, 0x10,
FREQ1, 0xB0,
FREQ0, 0x71,
MDMCFG4, 0xA7,
MDMCFG3, 0x32,
MDMCFG2, 0x30,
MDMCFG1, 0x22,
MDMCFG0, 0xF8,
MCSM2, 0x07,
MCSM1, 0x30,
MCSM0, 0x18,
AGCTRL2, 0x04,
AGCTRL1, 0x00,
AGCTRL0, 0x92,
FREND1, 0xB6,
FREND0, 0x11,
FSCAL3, 0xE9,
FSCAL2, 0x2A,
FSCAL1, 0x00,
FSCAL0, 0x1F,
TEST2, 0x81,
TEST1, 0x35,
TEST0, 0x09
};
void cc1101_init(void) { // Функция инициализации трансивера CC1101
PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
gpio_output_set(0,0,BIT15,0);
GPIO_OUTPUT_SET(15, 0); // Установить CSn в лог.0
os_delay_us(500); // Ждем 500мкс
SPI_WriteByte(SRES); // Отправка строба сброс
os_delay_us(100); // Ждем 100мкс
int8_t qt;
for (qt=0;qt<sizeof(config_433_ook);qt++) SPI_WriteByte(config_433_ook[qt]);
SPI_WriteByte(STX); //Отправка строба приема // SPI_ReadByte(SNOP);
GPIO_OUTPUT_SET(15, 1); // Установить CSn в лог.1
}

void ICACHE_FLASH_ATTR
startfunc(){
// start funs
SPI_Init();
cc1101_init(); // Настроить трансивер на 433.920 модуляция AM
}

void ICACHE_FLASH_ATTR
timerfunc(uint32_t timersrc) {

// timer funs every 30 second

if(timersrc%30==0)
{
cc1101_init(); // Настроить трансивер на 433.920 модуляция AM
}

}

void webfunc(char *pbuf) {

//os_sprintf(HTTPBUFF,"Hello world");

}
