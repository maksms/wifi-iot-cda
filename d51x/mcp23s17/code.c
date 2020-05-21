
// https://github.com/MetalPhreak/ESP8266_SPI_Driver
// https://www.eevblog.com/forum/microcontrollers/esp8266-native-spi-hardware-driver/
// https://github.com/MetalPhreak/ESP8266_MCP23S17
//***********************  SPI DRIVER: spi.h ***************************************************
#define SPI 0
#define HSPI 1

#define SPI_CLK_USE_DIV 0
#define SPI_CLK_80MHZ_NODIV 1

#define SPI_BYTE_ORDER_HIGH_TO_LOW 1
#define SPI_BYTE_ORDER_LOW_TO_HIGH 0

#ifndef CPU_CLK_FREQ //Should already be defined in eagle_soc.h
#define CPU_CLK_FREQ 80*1000000
#endif

#define SPI_CLK_PREDIV 10
#define SPI_CLK_CNTDIV 2
#define SPI_CLK_FREQ CPU_CLK_FREQ/(SPI_CLK_PREDIV*SPI_CLK_CNTDIV) // 80 / 20 = 4 MHz

void spi_init(uint8 spi_no);
void spi_mode(uint8 spi_no, uint8 spi_cpha,uint8 spi_cpol);
void spi_init_gpio(uint8 spi_no, uint8 sysclk_as_spiclk);
void spi_clock(uint8 spi_no, uint16 prediv, uint8 cntdiv);
void spi_tx_byte_order(uint8 spi_no, uint8 byte_order);
void spi_rx_byte_order(uint8 spi_no, uint8 byte_order);
uint32 spi_transaction(uint8 spi_no, uint8 cmd_bits, uint16 cmd_data, uint32 addr_bits, uint32 addr_data, uint32 dout_bits, uint32 dout_data, uint32 din_bits, uint32 dummy_bits);

//Expansion Macros
#define spi_busy(spi_no) READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR

#define spi_txd(spi_no, bits, data) spi_transaction(spi_no, 0, 0, 0, 0, bits, (uint32) data, 0, 0)
#define spi_tx8(spi_no, data)       spi_transaction(spi_no, 0, 0, 0, 0, 8,    (uint32) data, 0, 0)
#define spi_tx16(spi_no, data)      spi_transaction(spi_no, 0, 0, 0, 0, 16,   (uint32) data, 0, 0)
#define spi_tx32(spi_no, data)      spi_transaction(spi_no, 0, 0, 0, 0, 32,   (uint32) data, 0, 0)

#define spi_rxd(spi_no, bits) spi_transaction(spi_no, 0, 0, 0, 0, 0, 0, bits, 0)
#define spi_rx8(spi_no)       spi_transaction(spi_no, 0, 0, 0, 0, 0, 0, 8,    0)
#define spi_rx16(spi_no)      spi_transaction(spi_no, 0, 0, 0, 0, 0, 0, 16,   0)
#define spi_rx32(spi_no)      spi_transaction(spi_no, 0, 0, 0, 0, 0, 0, 32,   0)

#define REG_SPI_BASE(i)  (0x60000200-i*0x100)

#define SPI_CMD(i)                            (REG_SPI_BASE(i)  + 0x0)
#define SPI_FLASH_READ (BIT(31)) //From previous SDK
#define SPI_FLASH_WREN (BIT(30)) //From previous SDK
#define SPI_FLASH_WRDI (BIT(29)) //From previous SDK
#define SPI_FLASH_RDID (BIT(28)) //From previous SDK
#define SPI_FLASH_RDSR (BIT(27)) //From previous SDK
#define SPI_FLASH_WRSR (BIT(26)) //From previous SDK
#define SPI_FLASH_PP (BIT(25)) //From previous SDK
#define SPI_FLASH_SE (BIT(24)) //From previous SDK
#define SPI_FLASH_BE (BIT(23)) //From previous SDK
#define SPI_FLASH_CE (BIT(22)) //From previous SDK
#define SPI_FLASH_DP (BIT(21)) //From previous SDK
#define SPI_FLASH_RES (BIT(20)) //From previous SDK
#define SPI_FLASH_HPM (BIT(19)) //From previous SDK
#define SPI_USR (BIT(18))

#define SPI_ADDR(i)                           (REG_SPI_BASE(i)  + 0x4)

#define SPI_CTRL(i)                           (REG_SPI_BASE(i)  + 0x8)
#define SPI_WR_BIT_ORDER (BIT(26))
#define SPI_RD_BIT_ORDER (BIT(25))
#define SPI_QIO_MODE (BIT(24))
#define SPI_DIO_MODE (BIT(23))
#define SPI_TWO_BYTE_STATUS_EN (BIT(22)) //From previous SDK
#define SPI_WP_REG (BIT(21)) //From previous SDK
#define SPI_QOUT_MODE (BIT(20))
#define SPI_SHARE_BUS (BIT(19)) //From previous SDK
#define SPI_HOLD_MODE (BIT(18)) //From previous SDK
#define SPI_ENABLE_AHB (BIT(17)) //From previous SDK
#define SPI_SST_AAI (BIT(16)) //From previous SDK
#define SPI_RESANDRES (BIT(15)) //From previous SDK
#define SPI_DOUT_MODE (BIT(14))
#define SPI_FASTRD_MODE (BIT(13))

#define SPI_CTRL1(i)                          (REG_SPI_BASE (i) + 0xC) //From previous SDK. Removed _FLASH_ from name to match other registers.
#define SPI_CS_HOLD_DELAY 0x0000000F //Espressif BBS
#define SPI_CS_HOLD_DELAY_S 28 //Espressif BBS
#define SPI_CS_HOLD_DELAY_RES 0x00000FFF //Espressif BBS
#define SPI_CS_HOLD_DELAY_RES_S 16 //Espressif BBS
#define SPI_BUS_TIMER_LIMIT 0x0000FFFF //From previous SDK
#define SPI_BUS_TIMER_LIMIT_S 0 //From previous SDK


#define SPI_RD_STATUS(i)                         (REG_SPI_BASE(i)  + 0x10)
#define SPI_STATUS_EXT 0x000000FF //From previous SDK
#define SPI_STATUS_EXT_S 24 //From previous SDK
#define SPI_WB_MODE 0x000000FF //From previous SDK
#define SPI_WB_MODE_S 16 //From previous SDK
#define SPI_FLASH_STATUS_PRO_FLAG (BIT(7)) //From previous SDK
#define SPI_FLASH_TOP_BOT_PRO_FLAG (BIT(5)) //From previous SDK
#define SPI_FLASH_BP2 (BIT(4)) //From previous SDK
#define SPI_FLASH_BP1 (BIT(3)) //From previous SDK
#define SPI_FLASH_BP0 (BIT(2)) //From previous SDK
#define SPI_FLASH_WRENABLE_FLAG (BIT(1)) //From previous SDK
#define SPI_FLASH_BUSY_FLAG (BIT(0)) //From previous SDK

#define SPI_CTRL2(i)                           (REG_SPI_BASE(i)  + 0x14)
#define SPI_CS_DELAY_NUM 0x0000000F
#define SPI_CS_DELAY_NUM_S 28
#define SPI_CS_DELAY_MODE 0x00000003
#define SPI_CS_DELAY_MODE_S 26
#define SPI_MOSI_DELAY_NUM 0x00000007
#define SPI_MOSI_DELAY_NUM_S 23
#define SPI_MOSI_DELAY_MODE 0x00000003  //mode 0 : posedge; data set at positive edge of clk
										//mode 1 : negedge + 1 cycle delay, only if freq<10MHz ; data set at negitive edge of clk
										//mode 2 : Do not use this mode.
#define SPI_MOSI_DELAY_MODE_S 21
#define SPI_MISO_DELAY_NUM 0x00000007
#define SPI_MISO_DELAY_NUM_S 18
#define SPI_MISO_DELAY_MODE 0x00000003
#define SPI_MISO_DELAY_MODE_S 16
#define SPI_CK_OUT_HIGH_MODE 0x0000000F
#define SPI_CK_OUT_HIGH_MODE_S 12
#define SPI_CK_OUT_LOW_MODE 0x0000000F
#define SPI_CK_OUT_LOW_MODE_S 8
#define SPI_HOLD_TIME 0x0000000F
#define SPI_HOLD_TIME_S 4
#define SPI_SETUP_TIME 0x0000000F
#define SPI_SETUP_TIME_S 0

#define SPI_CLOCK(i)                          (REG_SPI_BASE(i)  + 0x18)
#define SPI_CLK_EQU_SYSCLK (BIT(31))
#define SPI_CLKDIV_PRE 0x00001FFF
#define SPI_CLKDIV_PRE_S 18
#define SPI_CLKCNT_N 0x0000003F
#define SPI_CLKCNT_N_S 12
#define SPI_CLKCNT_H 0x0000003F
#define SPI_CLKCNT_H_S 6
#define SPI_CLKCNT_L 0x0000003F
#define SPI_CLKCNT_L_S 0

#define SPI_USER(i)                           (REG_SPI_BASE(i)  + 0x1C)
#define SPI_USR_COMMAND (BIT(31))
#define SPI_USR_ADDR (BIT(30))
#define SPI_USR_DUMMY (BIT(29))
#define SPI_USR_MISO (BIT(28))
#define SPI_USR_MOSI (BIT(27))
#define SPI_USR_DUMMY_IDLE (BIT(26)) //From previous SDK
#define SPI_USR_MOSI_HIGHPART (BIT(25))
#define SPI_USR_MISO_HIGHPART (BIT(24))
#define SPI_USR_PREP_HOLD (BIT(23)) //From previous SDK
#define SPI_USR_CMD_HOLD (BIT(22)) //From previous SDK
#define SPI_USR_ADDR_HOLD (BIT(21)) //From previous SDK
#define SPI_USR_DUMMY_HOLD (BIT(20)) //From previous SDK
#define SPI_USR_DIN_HOLD (BIT(19)) //From previous SDK
#define SPI_USR_DOUT_HOLD (BIT(18)) //From previous SDK
#define SPI_USR_HOLD_POL (BIT(17)) //From previous SDK
#define SPI_SIO (BIT(16))
#define SPI_FWRITE_QIO (BIT(15))
#define SPI_FWRITE_DIO (BIT(14))
#define SPI_FWRITE_QUAD (BIT(13))
#define SPI_FWRITE_DUAL (BIT(12))
#define SPI_WR_BYTE_ORDER (BIT(11))
#define SPI_RD_BYTE_ORDER (BIT(10))
#define SPI_AHB_ENDIAN_MODE 0x00000003 //From previous SDK
#define SPI_AHB_ENDIAN_MODE_S 8 //From previous SDK
#define SPI_CK_OUT_EDGE (BIT(7))
#define SPI_CK_I_EDGE (BIT(6))
#define SPI_CS_SETUP (BIT(5))
#define SPI_CS_HOLD (BIT(4))
#define SPI_AHB_USR_COMMAND (BIT(3)) //From previous SDK
#define SPI_FLASH_MODE (BIT(2))
#define SPI_AHB_USR_COMMAND_4BYTE (BIT(1)) //From previous SDK
#define SPI_DOUTDIN (BIT(0)) //From previous SDK

//AHB = http://en.wikipedia.org/wiki/Advanced_Microcontroller_Bus_Architecture ?


#define SPI_USER1(i)                          (REG_SPI_BASE(i) + 0x20)
#define SPI_USR_ADDR_BITLEN 0x0000003F
#define SPI_USR_ADDR_BITLEN_S 26
#define SPI_USR_MOSI_BITLEN 0x000001FF
#define SPI_USR_MOSI_BITLEN_S 17
#define SPI_USR_MISO_BITLEN 0x000001FF
#define SPI_USR_MISO_BITLEN_S 8
#define SPI_USR_DUMMY_CYCLELEN 0x000000FF
#define SPI_USR_DUMMY_CYCLELEN_S 0

#define SPI_USER2(i)                          (REG_SPI_BASE(i)  + 0x24)
#define SPI_USR_COMMAND_BITLEN 0x0000000F
#define SPI_USR_COMMAND_BITLEN_S 28
#define SPI_USR_COMMAND_VALUE 0x0000FFFF
#define SPI_USR_COMMAND_VALUE_S 0

#define SPI_WR_STATUS(i)                          (REG_SPI_BASE(i)  + 0x28)
 //previously defined as SPI_FLASH_USER3. No further info available.

#define SPI_PIN(i)                            (REG_SPI_BASE(i)  + 0x2C)
#define SPI_IDLE_EDGE (BIT(29))
#define SPI_CS2_DIS (BIT(2))
#define SPI_CS1_DIS (BIT(1))
#define SPI_CS0_DIS (BIT(0))

#define SPI_SLAVE(i)                          (REG_SPI_BASE(i)  + 0x30)
#define SPI_SYNC_RESET (BIT(31))
#define SPI_SLAVE_MODE (BIT(30))
#define SPI_SLV_WR_RD_BUF_EN (BIT(29))
#define SPI_SLV_WR_RD_STA_EN (BIT(28))
#define SPI_SLV_CMD_DEFINE (BIT(27))
#define SPI_TRANS_CNT 0x0000000F
#define SPI_TRANS_CNT_S 23
#define SPI_SLV_LAST_STATE 0x00000007 //From previous SDK
#define SPI_SLV_LAST_STATE_S 20 //From previous SDK
#define SPI_SLV_LAST_COMMAND 0x00000007 //From previous SDK
#define SPI_SLV_LAST_COMMAND_S 17 //From previous SDK
#define SPI_CS_I_MODE 0x00000003 //From previous SDK
#define SPI_CS_I_MODE_S 10 //From previous SDK
#define SPI_TRANS_DONE_EN (BIT(9))
#define SPI_SLV_WR_STA_DONE_EN (BIT(8))
#define SPI_SLV_RD_STA_DONE_EN (BIT(7))
#define SPI_SLV_WR_BUF_DONE_EN (BIT(6))
#define SPI_SLV_RD_BUF_DONE_EN (BIT(5))
#define SLV_SPI_INT_EN   0x0000001f
#define SLV_SPI_INT_EN_S 5
#define SPI_TRANS_DONE (BIT(4))
#define SPI_SLV_WR_STA_DONE (BIT(3))
#define SPI_SLV_RD_STA_DONE (BIT(2))
#define SPI_SLV_WR_BUF_DONE (BIT(1))
#define SPI_SLV_RD_BUF_DONE (BIT(0))

#define SPI_SLAVE1(i)                         (REG_SPI_BASE(i)  + 0x34)
#define SPI_SLV_STATUS_BITLEN 0x0000001F
#define SPI_SLV_STATUS_BITLEN_S 27
#define SPI_SLV_STATUS_FAST_EN (BIT(26)) //From previous SDK
#define SPI_SLV_STATUS_READBACK (BIT(25)) //From previous SDK
#define SPI_SLV_BUF_BITLEN 0x000001FF
#define SPI_SLV_BUF_BITLEN_S 16
#define SPI_SLV_RD_ADDR_BITLEN 0x0000003F
#define SPI_SLV_RD_ADDR_BITLEN_S 10
#define SPI_SLV_WR_ADDR_BITLEN 0x0000003F
#define SPI_SLV_WR_ADDR_BITLEN_S 4
#define SPI_SLV_WRSTA_DUMMY_EN (BIT(3))
#define SPI_SLV_RDSTA_DUMMY_EN (BIT(2))
#define SPI_SLV_WRBUF_DUMMY_EN (BIT(1))
#define SPI_SLV_RDBUF_DUMMY_EN (BIT(0))



#define SPI_SLAVE2(i)  (REG_SPI_BASE(i)  + 0x38)
#define SPI_SLV_WRBUF_DUMMY_CYCLELEN  0X000000FF
#define SPI_SLV_WRBUF_DUMMY_CYCLELEN_S 24
#define SPI_SLV_RDBUF_DUMMY_CYCLELEN  0X000000FF
#define SPI_SLV_RDBUF_DUMMY_CYCLELEN_S 16
#define SPI_SLV_WRSTR_DUMMY_CYCLELEN  0X000000FF
#define SPI_SLV_WRSTR_DUMMY_CYCLELEN_S  8
#define SPI_SLV_RDSTR_DUMMY_CYCLELEN  0x000000FF
#define SPI_SLV_RDSTR_DUMMY_CYCLELEN_S 0

#define SPI_SLAVE3(i)                         (REG_SPI_BASE(i)  + 0x3C)
#define SPI_SLV_WRSTA_CMD_VALUE 0x000000FF
#define SPI_SLV_WRSTA_CMD_VALUE_S 24
#define SPI_SLV_RDSTA_CMD_VALUE 0x000000FF
#define SPI_SLV_RDSTA_CMD_VALUE_S 16
#define SPI_SLV_WRBUF_CMD_VALUE 0x000000FF
#define SPI_SLV_WRBUF_CMD_VALUE_S 8
#define SPI_SLV_RDBUF_CMD_VALUE 0x000000FF
#define SPI_SLV_RDBUF_CMD_VALUE_S 0

//Previous SDKs referred to these following registers as SPI_C0 etc.

#define SPI_W0(i) 							(REG_SPI_BASE(i) +0x40)
#define SPI_W1(i) 							(REG_SPI_BASE(i) +0x44)
#define SPI_W2(i) 							(REG_SPI_BASE(i) +0x48)
#define SPI_W3(i) 							(REG_SPI_BASE(i) +0x4C)
#define SPI_W4(i) 							(REG_SPI_BASE(i) +0x50)
#define SPI_W5(i) 							(REG_SPI_BASE(i) +0x54)
#define SPI_W6(i) 							(REG_SPI_BASE(i) +0x58)
#define SPI_W7(i) 							(REG_SPI_BASE(i) +0x5C)
#define SPI_W8(i) 							(REG_SPI_BASE(i) +0x60)
#define SPI_W9(i) 							(REG_SPI_BASE(i) +0x64)
#define SPI_W10(i) 							(REG_SPI_BASE(i) +0x68)
#define SPI_W11(i) 							(REG_SPI_BASE(i) +0x6C)
#define SPI_W12(i) 							(REG_SPI_BASE(i) +0x70)
#define SPI_W13(i) 							(REG_SPI_BASE(i) +0x74)
#define SPI_W14(i) 							(REG_SPI_BASE(i) +0x78)
#define SPI_W15(i) 							(REG_SPI_BASE(i) +0x7C)

 // +0x80 to +0xBC could be SPI_W16 through SPI_W31?

 // +0xC0 to +0xEC not currently defined.

#define SPI_EXT0(i)                           (REG_SPI_BASE(i)  + 0xF0) //From previous SDK. Removed _FLASH_ from name to match other registers.
#define SPI_T_PP_ENA (BIT(31)) //From previous SDK
#define SPI_T_PP_SHIFT 0x0000000F //From previous SDK
#define SPI_T_PP_SHIFT_S 16 //From previous SDK
#define SPI_T_PP_TIME 0x00000FFF //From previous SDK
#define SPI_T_PP_TIME_S 0 //From previous SDK

#define SPI_EXT1(i)                          (REG_SPI_BASE(i)  + 0xF4) //From previous SDK. Removed _FLASH_ from name to match other registers.
#define SPI_T_ERASE_ENA (BIT(31)) //From previous SDK
#define SPI_T_ERASE_SHIFT 0x0000000F //From previous SDK
#define SPI_T_ERASE_SHIFT_S 16 //From previous SDK
#define SPI_T_ERASE_TIME 0x00000FFF //From previous SDK
#define SPI_T_ERASE_TIME_S 0 //From previous SDK

#define SPI_EXT2(i)                           (REG_SPI_BASE(i)  + 0xF8) //From previous SDK. Removed _FLASH_ from name to match other registers.
#define SPI_ST 0x00000007 //From previous SDK
#define SPI_ST_S 0 //From previous SDK

#define SPI_EXT3(i)                           (REG_SPI_BASE(i)  + 0xFC)
#define SPI_INT_HOLD_ENA 0x00000003
#define SPI_INT_HOLD_ENA_S 0

//

#define PIN_GPIO_MOSI    13
#define PIN_GPIO_MISO    12
#define PIN_GPIO_CLK     14
#define PIN_GPIO_CS      15

#define PORTA0  0x00
#define PORTB0  0x01
#define PORT0   0x02

#define PORTA1  0x04
#define PORTB1  0x05
#define PORT1   0x06

#define PORTA2  0x08
#define PORTB2  0x09
#define PORT2   0x0A

#define PORTA3  0x0C
#define PORTB3  0x0D
#define PORT3   0x0E

#define PORTA4  0x10
#define PORTB4  0x11
#define PORT4   0x12

#define PORTA5  0x14
#define PORTB5  0x15
#define PORT5   0x16

#define PORTA6  0x18
#define PORTB6  0x19
#define PORT6   0x1A

#define PORTA7  0x1C
#define PORTB7  0x1D
#define PORT7   0x1E

#define PORTA   PORTA0
#define PORTB   PORTB0
#define PORT    PORT0

#define MCP23S17_GPIO0   0x0001
#define MCP23S17_GPIO1   0x0002
#define MCP23S17_GPIO2   0x0004
#define MCP23S17_GPIO3   0x0008
#define MCP23S17_GPIO4   0x0010
#define MCP23S17_GPIO5   0x0020
#define MCP23S17_GPIO6   0x0040
#define MCP23S17_GPIO7   0x0080
#define MCP23S17_GPIO8   0x0100
#define MCP23S17_GPIO9   0x0200
#define MCP23S17_GPIO10  0x0400
#define MCP23S17_GPIO11  0x0800
#define MCP23S17_GPIO12  0x1000
#define MCP23S17_GPIO13  0x2000
#define MCP23S17_GPIO14  0x4000
#define MCP23S17_GPIO15  0x8000

#define IODIRA      0x00
#define IODIRB      0x01
#define IPOLA       0x02
#define IPOLB       0x03
#define GPINTENA    0x04
#define GPINTENB    0x05
#define DEFVALA     0x06
#define DEFVALB     0x07
#define INTCONA     0x08
#define INTCONB     0x09
#define IOCONA      0x0A
#define IOCONB      0x0B
#define GPPUA       0x0C
#define GPPUB       0x0D
#define INTFA       0x0E
#define INTFB       0x0F
#define INTCAPA     0x10
#define INTCAPB     0x11
#define GPIOA       0x12
#define GPIOB       0x13
#define OLATA       0x14
#define OLATB       0x15

#define IODIR_CTRL   0x00
#define IPOL_CTRL    0x02
#define GPINTEN_CTRL 0x04
#define DEFVAL_CTRL  0x06
#define INTCON_CTRL  0x08
#define IOCON_CTRL   0x0A
#define GPPU_CTRL    0x0C
#define INTF_CTRL    0x0E
#define INTCAP_CTRL  0x10
#define GPIO_CTRL    0x12
#define OLAT_CTRL    0x14

#define INTPOL  0x02
#define ODR     0x04
#define HAEN    0x08
#define DISSLW  0x10
#define SEQOP   0x20
#define MIRROR  0x40
#define BANK    0x80


// ***************** SPI DRIVER: spi.c *******************************************
void spi_init(uint8 spi_no){
	
	if(spi_no > 1) return; //Only SPI and HSPI are valid spi modules. 

	spi_init_gpio(spi_no, SPI_CLK_USE_DIV);
	spi_clock(spi_no, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
	spi_tx_byte_order(spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW);
	spi_rx_byte_order(spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW); 

	SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CS_SETUP|SPI_CS_HOLD);
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_FLASH_MODE);

}

void spi_mode(uint8 spi_no, uint8 spi_cpha,uint8 spi_cpol){
	if(!spi_cpha == !spi_cpol) {
		CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_CK_OUT_EDGE);
	} else {
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CK_OUT_EDGE);
	}

	if (spi_cpol) {
		SET_PERI_REG_MASK(SPI_PIN(spi_no), SPI_IDLE_EDGE);
	} else {
		CLEAR_PERI_REG_MASK(SPI_PIN(spi_no), SPI_IDLE_EDGE);
	}
}

void spi_init_gpio(uint8 spi_no, uint8 sysclk_as_spiclk){

//	if(spi_no > 1) return; //Not required. Valid spi_no is checked with if/elif below.

	uint32 clock_div_flag = 0;
	if(sysclk_as_spiclk){
		clock_div_flag = 0x0001;	
	} 

	if(spi_no==SPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005|(clock_div_flag<<8)); //Set bit 8 if 80MHz sysclock required
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);	
	}else if(spi_no==HSPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105|(clock_div_flag<<9)); //Set bit 9 if 80MHz sysclock required
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2); //GPIO12 is HSPI MISO pin (Master Data In)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2); //GPIO14 is HSPI CLK pin (Clock)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2); //GPIO15 is HSPI CS pin (Chip Select / Slave Select)
	}

}

void spi_clock(uint8 spi_no, uint16 prediv, uint8 cntdiv){
	
	if(spi_no > 1) return;

	if((prediv==0)|(cntdiv==0)){

		WRITE_PERI_REG(SPI_CLOCK(spi_no), SPI_CLK_EQU_SYSCLK);

	} else {
	
		WRITE_PERI_REG(SPI_CLOCK(spi_no), 
					(((prediv-1)&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)|
					(((cntdiv-1)&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)|
					(((cntdiv>>1)&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)|
					((0&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S));
	}

}

void spi_tx_byte_order(uint8 spi_no, uint8 byte_order){

	if(spi_no > 1) return;

	if(byte_order){
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
	}
}

void spi_rx_byte_order(uint8 spi_no, uint8 byte_order){

	if(spi_no > 1) return;

	if(byte_order){
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
	}
}

uint32 spi_transaction(uint8 spi_no, uint8 cmd_bits, uint16 cmd_data, uint32 addr_bits, uint32 addr_data, uint32 dout_bits, uint32 dout_data,
				uint32 din_bits, uint32 dummy_bits){

	if(spi_no > 1) return 0;  //Check for a valid SPI 

	//code for custom Chip Select as GPIO PIN here

	while(spi_busy(spi_no)); //wait for SPI to be ready	

//########## Enable SPI Functions ##########//
	//disable MOSI, MISO, ADDR, COMMAND, DUMMY in case previously set.
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY);

	//enable functions based on number of bits. 0 bits = disabled. 
	//This is rather inefficient but allows for a very generic function.
	//CMD ADDR and MOSI are set below to save on an extra if statement.
//	if(cmd_bits) {SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_COMMAND);}
//	if(addr_bits) {SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_ADDR);}
	if(din_bits) {SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MISO);}
	if(dummy_bits) {SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_DUMMY);}
//########## END SECTION ##########//

//########## Setup Bitlengths ##########//
	WRITE_PERI_REG(SPI_USER1(spi_no), ((addr_bits-1)&SPI_USR_ADDR_BITLEN)<<SPI_USR_ADDR_BITLEN_S | //Number of bits in Address
									  ((dout_bits-1)&SPI_USR_MOSI_BITLEN)<<SPI_USR_MOSI_BITLEN_S | //Number of bits to Send
									  ((din_bits-1)&SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S |  //Number of bits to receive
									  ((dummy_bits-1)&SPI_USR_DUMMY_CYCLELEN)<<SPI_USR_DUMMY_CYCLELEN_S); //Number of Dummy bits to insert
//########## END SECTION ##########//

//########## Setup Command Data ##########//
	if(cmd_bits) {
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_COMMAND); //enable COMMAND function in SPI module
		uint16 command = cmd_data << (16-cmd_bits); //align command data to high bits
		command = ((command>>8)&0xff) | ((command<<8)&0xff00); //swap byte order
		WRITE_PERI_REG(SPI_USER2(spi_no), ((((cmd_bits-1)&SPI_USR_COMMAND_BITLEN)<<SPI_USR_COMMAND_BITLEN_S) | command&SPI_USR_COMMAND_VALUE));	
	}
//########## END SECTION ##########//

//########## Setup Address Data ##########//
	if(addr_bits){
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_ADDR); //enable ADDRess function in SPI module
		WRITE_PERI_REG(SPI_ADDR(spi_no), addr_data<<(32-addr_bits)); //align address data to high bits
	}
	

//########## END SECTION ##########//	

//########## Setup DOUT data ##########//
	if(dout_bits) {
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI); //enable MOSI function in SPI module
	//copy data to W0
	if(READ_PERI_REG(SPI_USER(spi_no))&SPI_WR_BYTE_ORDER) {
		WRITE_PERI_REG(SPI_W0(spi_no), dout_data<<(32-dout_bits));
	} else {

		uint8 dout_extra_bits = dout_bits%8;

		if(dout_extra_bits){
			//if your data isn't a byte multiple (8/16/24/32 bits)and you don't have SPI_WR_BYTE_ORDER set, you need this to move the non-8bit remainder to the MSBs
			//not sure if there's even a use case for this, but it's here if you need it...
			//for example, 0xDA4 12 bits without SPI_WR_BYTE_ORDER would usually be output as if it were 0x0DA4, 
			//of which 0xA4, and then 0x0 would be shifted out (first 8 bits of low byte, then 4 MSB bits of high byte - ie reverse byte order). 
			//The code below shifts it out as 0xA4 followed by 0xD as you might require. 
			WRITE_PERI_REG(SPI_W0(spi_no), ((0xFFFFFFFF<<(dout_bits - dout_extra_bits)&dout_data)<<(8-dout_extra_bits) | (0xFFFFFFFF>>(32-(dout_bits - dout_extra_bits)))&dout_data));
		} else {
			WRITE_PERI_REG(SPI_W0(spi_no), dout_data);
		}
	}
	}
//########## END SECTION ##########//

//########## Begin SPI Transaction ##########//
	SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);
//########## END SECTION ##########//

//########## Return DIN data ##########//
	if(din_bits) {
		while(spi_busy(spi_no));	//wait for SPI transaction to complete
		
		if(READ_PERI_REG(SPI_USER(spi_no))&SPI_RD_BYTE_ORDER) {
			return READ_PERI_REG(SPI_W0(spi_no)) >> (32-din_bits); //Assuming data in is written to MSB. TBC
		} else {
			return READ_PERI_REG(SPI_W0(spi_no)); //Read in the same way as DOUT is sent. Note existing contents of SPI_W0 remain unless overwritten! 
		}

		return 0; //something went wrong
	}
//########## END SECTION ##########//

	//Transaction completed
	return 1; //success
}


//********************************************************************************



//******************************* MCP23S17.h ***************************
#define SPI_DEV HSPI

void mcp23s17_init();
void mcp23s17_REG_SET(uint8 ctrl_reg, uint8 port, uint16 value);
uint16 mcp23s17_REG_GET(uint8 ctrl_reg, uint8 port);
void mcp23s17_REG_SET_MASK(uint8 ctrl_reg, uint8 port, uint16 value, uint16 bitmask);

#define sGPIO_SET(port, value) mcp23s17_REG_SET(OLAT_CTRL, port, value)
//#define sGPIO_SET(port, value) mcp23s17_REG_SET(GPIO_CTRL, port, value)
#define sGPIO_SET_MASK(port, value, bitmask) mcp23s17_REG_SET_MASK(OLAT_CTRL, port, value, bitmask)
#define sGPIO_SET_PIN(port, pin, value) mcp23s17_REG_SET_MASK(OLAT_CTRL, port, value<<(pin-1), 1<<(pin-1))

#define sGPIO_GET(port) mcp23s17_REG_GET(OLAT_CTRL, port)

#define sGPIO_READ(port) mcp23s17_REG_GET(GPIO_CTRL, port)
//**********************************************************************


//************************* MCP23S17.c **********************************
void mcp23s17_init(){
	//init SPI bus
	spi_init_gpio(SPI_DEV, SPI_CLK_USE_DIV);
	spi_clock(SPI_DEV, 4, 2); //10MHz
	spi_tx_byte_order(SPI_DEV, SPI_BYTE_ORDER_HIGH_TO_LOW);
	spi_rx_byte_order(SPI_DEV, SPI_BYTE_ORDER_HIGH_TO_LOW); 

	SET_PERI_REG_MASK(SPI_USER(SPI_DEV), SPI_CS_SETUP|SPI_CS_HOLD);
	CLEAR_PERI_REG_MASK(SPI_USER(SPI_DEV), SPI_FLASH_MODE);

	//Enable hardware addressing & sequential addressing on all devices
	mcp23s17_REG_SET(IOCON_CTRL, PORTA, SEQOP|HAEN); 
	
}

void mcp23s17_REG_SET(uint8 ctrl_reg, uint8 port, uint16 value){
	
	uint8 cmd = (0x20|(port>>2))<<1; //0b0100[Address][WRITE]

	if (port & 0x02){
		spi_transaction(SPI_DEV, 8, cmd, 8, ctrl_reg, 16, value, 0, 0);
	} else {
		spi_transaction(SPI_DEV, 8, cmd, 8, ctrl_reg+(port&0x01), 8, value, 0, 0);	
	}	
}

uint16 mcp23s17_REG_GET(uint8 ctrl_reg, uint8 port){
	
	uint8 cmd = ((0x20|(port>>2))<<1)|0x01; //0b0100[Address][READ]

	if (port & 0x02){
		return spi_transaction(SPI_DEV, 8, cmd, 8, ctrl_reg, 0, 0, 16, 0);
	} else {
		return (uint16) spi_transaction(SPI_DEV, 8, cmd, 8, ctrl_reg+(port&0x01), 0, 0, 8, 0);	
	}	
}

void mcp23s17_REG_SET_MASK(uint8 ctrl_reg, uint8 port, uint16 value, uint16 bitmask) {
    uint16 current_value = ~bitmask & mcp23s17_REG_GET(ctrl_reg, port);
    uint16 set_value = bitmask & value;
	mcp23s17_REG_SET(ctrl_reg, port, current_value|set_value);
}



#define B(bit_no)         (1 << (bit_no))
#define BIT_CLEAR(reg, bit_no)   (reg) &= ~B(bit_no)
#define BIT_SET(reg, bit_no)   (reg) |= B(bit_no)
#define BIT_CHECK(reg, bit_no)   ( (reg) & B(bit_no) )
#define BIT_TRIGGER(reg, bit_no)   (reg) ^= B(bit_no)

static volatile os_timer_t timer_read_gpio[7]; 
uint8_t gpio_state[7] = {0, 0, 0, 0, 0, 0, 0};
uint8_t gpio_num[7] = {0, 1, 2, 3, 4, 5, 6};

uint8_t GPIO_TEST[7] = {5, 4, 1, 3, 2, 0, 16};

#define FW_VER "21"

void read_gpio_cb(uint8_t idx) {
	if ( gpio_state[idx] == GPIO_ALL_GET(GPIO_TEST[idx])) return;
	gpio_state[idx] = GPIO_ALL_GET(GPIO_TEST[idx]);

	uint16_t bit;
	bit =  sGPIO_READ(PORT0);
	if ( gpio_state[idx] ) {
		// on
		sGPIO_SET(PORT0, BIT_SET( bit, gpio_num[idx] ));
	} else {
		// off
		sGPIO_SET(PORT0, BIT_CLEAR( bit,  gpio_num[idx] ));
	}
}

/*
unsigned char x = (1 << 2) | (1 << 3) | (1 << 7);
if (x & (1 << 2)) //{   во второй бит вписана единица  }
if (x & (1 << 3)) //{   в третий бит вписана единица  }
if (x & (1 << 7)) //{   в седьмой бит вписана единица  }

Чтобы записать единицу в бит n:					x |= (1 << n);
Чтобы записать ноль в бит n:					x &= ~(1 << n);
Если нужно инвертировать состояние бита:		x ^= (1 << n);
Если нужно определить, что в X на N-й позиции:	bool b = (bool((1 << n)  &  x))
Обнулить несколько битов можно и так:			x = x & (~((1<<3)|(1<<5)|(1<<6))); //обнуляем третий, пятый и шестой биты
*/
//***********************************************************************
void ICACHE_FLASH_ATTR startfunc(){
    // выполняется один раз при старте модуля.

    mcp23s17_init();
    mcp23s17_REG_SET(IODIR_CTRL, PORT0, 0x0000);

	uint8_t i;
	for ( i = 0; i < 7; i++) {
		gpio_state[i] = GPIO_ALL_GET(GPIO_TEST[i]);
	}

	for ( i = 0; i < 7; i++) {
		os_timer_disarm(&timer_read_gpio[i]);
		os_timer_setfn(&timer_read_gpio[i], (os_timer_func_t *) read_gpio_cb, i);
		os_timer_arm(&timer_read_gpio[i], 100, 1);	
	}

}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    static uint16 portval = 0x0000;

    //sGPIO_SET(PORT0, portval);
    //portval++;
    //os_delay_us(10000); //10ms delay so you can actually see it counting :D

    // выполнение кода каждую 1 секунду
    if(timersrc%30==0){
        // выполнение кода каждые 30 секунд
    }
}

void webfunc(char *pbuf) {
   // os_sprintf(HTTPBUFF,"<br>GPIO %d: %s", GPIO_TEST, (gpio_state) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"<br>MCP GPIO: </b>"); // вывод данных на главной модуля
    
    os_sprintf(HTTPBUFF,"<br>%d: %s", 1,  BIT_CHECK ( sGPIO_READ(PORT0), 0) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 2,  BIT_CHECK ( sGPIO_READ(PORT0), 1) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 3,  BIT_CHECK ( sGPIO_READ(PORT0), 2 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 4,  BIT_CHECK ( sGPIO_READ(PORT0), 3 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 5,  BIT_CHECK ( sGPIO_READ(PORT0), 4 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 6,  BIT_CHECK ( sGPIO_READ(PORT0), 5 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 7,  BIT_CHECK ( sGPIO_READ(PORT0), 6 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 8,  BIT_CHECK ( sGPIO_READ(PORT0), 7 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"<br>%d: %s", 9,  BIT_CHECK ( sGPIO_READ(PORT0), 8 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 10,  BIT_CHECK ( sGPIO_READ(PORT0), 9 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 11,  BIT_CHECK ( sGPIO_READ(PORT0), 10 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 12,  BIT_CHECK ( sGPIO_READ(PORT0), 11 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 13,  BIT_CHECK ( sGPIO_READ(PORT0), 12 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 14,  BIT_CHECK ( sGPIO_READ(PORT0), 13 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 15,  BIT_CHECK ( sGPIO_READ(PORT0), 14 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"   %d: %s", 16,  BIT_CHECK ( sGPIO_READ(PORT0), 15 ) ? "ON" : "OFF"); // вывод данных на главной модуля
    
    os_sprintf(HTTPBUFF,"<br>FW ver: %s", FW_VER); // вывод данных на главной модуля
}