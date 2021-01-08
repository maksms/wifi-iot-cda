#define FW_VER "1.2"

#define MCP23017_INTB_PIN 4     // pin esp
#define MCP23017_INTA_PIN 5     // pin esp


#define MCP23017_GPIO0   1 << 0     //0x0001
#define MCP23017_GPIO1   1 << 1     //0x0002
#define MCP23017_GPIO2   1 << 2     //0x0004
#define MCP23017_GPIO3   1 << 3     //0x0008
#define MCP23017_GPIO4   1 << 4     //0x0010
#define MCP23017_GPIO5   1 << 5     //0x0020
#define MCP23017_GPIO6   1 << 6     //0x0040
#define MCP23017_GPIO7   1 << 7     //0x0080
#define MCP23017_GPIO8   1 << 8     //0x0100
#define MCP23017_GPIO9   1 << 9     //0x0200
#define MCP23017_GPIO10  1 << 10    //0x0400
#define MCP23017_GPIO11  1 << 11    //0x0800
#define MCP23017_GPIO12  1 << 12    //0x1000
#define MCP23017_GPIO13  1 << 13    //0x2000
#define MCP23017_GPIO14  1 << 14    //0x4000
#define MCP23017_GPIO15  1 << 15    //0x8000

#define IODIRA      0x00    // регистр, указыващий направления портов output/input
#define IODIRB      0x01
#define IPOLA       0x02    // Input polarity инверсия ног только для входов
#define IPOLB       0x03
#define GPINTENA    0x04    // прерывания на ногах
#define GPINTENB    0x05
#define DEFVALA     0x06    // дефолтные значения ног, прерывание сработает, если на ноге сигнал отличается от дефолтного
#define DEFVALB     0x07
#define INTCONA     0x08    // условия сработки прерывания на ногах
#define INTCONB     0x09
#define IOCONA      0x0A    // конфигурационный регистр
#define IOCONB      0x0B
#define GPPUA       0x0C    // подтяжка ног 100к
#define GPPUB       0x0D
#define INTFA       0x0E    // регистр флагов прерываний, покажет на какой ноге было прерывание
#define INTFB       0x0F
#define INTCAPA     0x10    // покажет что было на ноге в момент прерывания на этой ноге
#define INTCAPB     0x11
#define GPIOA       0x12    // состояние ног, когда было прерывание на ноге может уже быть другое значение и надо читать INTCAP, если работаем с прерываниями
#define GPIOB       0x13
#define OLATA       0x14    
#define OLATB       0x15

typedef void (*func_cb)(void *arg);

//#define ENCODER_ROTATE_DEBOUNCE_TIME 50 // msec

#define MCP23017_ENCODER_CLK_PIN    MCP23017_GPIO10    
#define MCP23017_ENCODER_DT_PIN     MCP23017_GPIO9
#define MCP23017_ENCODER_BTN_PIN    MCP23017_GPIO11

#define MCP23017_ENCODER_BTN_INTR_TYPE GPIO_PIN_INTR_POSEDGE  

typedef enum {
	ENCODER_ROTATE_ZERO,
	ENCODER_ROTATE_LEFT,    
    ENCODER_ROTATE_RIGHT	
} encoder_direction_t;

typedef struct mcp23017_encoder {
    uint16_t mcp_pin_dt;
    uint16_t mcp_pin_clk;
    uint16_t mcp_pin_btn;
    uint8_t intr_a_pin;
    uint8_t intr_b_pin;
    encoder_direction_t direction;
    int32_t position;
    func_cb left;
    func_cb right;
    func_cb press;
    func_cb release;
} mcp23017_encoder_t;


mcp23017_encoder_t mcp23017_encoder;

#define millis() (unsigned long) (esp_timer_get_time() / 1000ULL)


void encoder_turn_left(void *arg){
    
}

void encoder_turn_right(void *arg){
    
}

void encoder_button_push(void *arg){
    uint8_t st = GPIO_ALL_GET(208);
    GPIO_ALL(208, !st );
}

void encoder_button_release(void *arg){
    uint8_t st = GPIO_ALL_GET(212);
    GPIO_ALL(212, !st );
}

void encoder_rotate_isr_handler(uint16_t val) {
    uint8_t level_clk = val & mcp23017_encoder.mcp_pin_clk ? 1 : 0;
    uint8_t level_dt = val & mcp23017_encoder.mcp_pin_dt ? 1 : 0;

    static uint8_t prev_dt = 0;
    if ( !level_dt && prev_dt ) {
        if ( level_clk != level_dt ) {
            mcp23017_encoder.direction = ENCODER_ROTATE_RIGHT;
            mcp23017_encoder.position++;
            mcp23017_encoder.right(NULL);
        } else {
            mcp23017_encoder.direction = ENCODER_ROTATE_LEFT;
            mcp23017_encoder.position--;
            mcp23017_encoder.left(NULL);
        }
    }
    prev_dt = level_dt; 
}

void encoder_push_isr_handler(uint16_t val) {
    uint8_t level = val & mcp23017_encoder.mcp_pin_btn ? 1 : 0;
    
    if ( 
            ( MCP23017_ENCODER_BTN_INTR_TYPE == GPIO_PIN_INTR_NEGEDGE  && level == 0) ||
            ( MCP23017_ENCODER_BTN_INTR_TYPE == GPIO_PIN_INTR_POSEDGE  && level == 1)
        ) 
    {
        mcp23017_encoder.press(NULL);
    }

    if ( 
            ( MCP23017_ENCODER_BTN_INTR_TYPE == GPIO_PIN_INTR_NEGEDGE  && level == 1) ||
            ( MCP23017_ENCODER_BTN_INTR_TYPE == GPIO_PIN_INTR_POSEDGE  && level == 0)
        ) 
    {
        mcp23017_encoder.release(NULL);
    }

}

void mcp23017_intr_handler()
{
    uint32_t gpio_st = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    ETS_GPIO_INTR_DISABLE();
    //if( gpio_st & BIT( MCP23017_INTB_PIN ) 
    if  ( 
            (mcp23017_encoder.intr_b_pin != 255 && gpio_st & BIT( mcp23017_encoder.intr_b_pin )) 
            || ( mcp23017_encoder.intr_a_pin != 255 && gpio_st & BIT( mcp23017_encoder.intr_a_pin ))
        )
    {
        uint16_t _int = MCPread_reg16(0, INTFA); // считываем данные с mcp23017
        uint16_t _cap = MCPread_reg16(0, INTCAPA); // считываем данные с mcp23017 // чтение снимка ножек при прерывании сбрасывает прерывание
        if ( _int & mcp23017_encoder.mcp_pin_dt ||  _int & mcp23017_encoder.mcp_pin_clk )  
        {
            encoder_rotate_isr_handler(_cap);
        }

        if ( _int & mcp23017_encoder.mcp_pin_btn  ) {
            encoder_push_isr_handler( _cap );
        }
    }

    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_st);
    ETS_GPIO_INTR_ENABLE();
}

void ICACHE_FLASH_ATTR mcp23017_encoder_init() {
    mcp23017_encoder.mcp_pin_clk = MCP23017_ENCODER_CLK_PIN;
    mcp23017_encoder.mcp_pin_dt = MCP23017_ENCODER_DT_PIN;
    mcp23017_encoder.mcp_pin_btn = MCP23017_ENCODER_BTN_PIN;
    mcp23017_encoder.intr_a_pin = 255; //MCP23017_INTA_PIN;
    mcp23017_encoder.intr_b_pin = MCP23017_INTB_PIN;

    mcp23017_encoder.direction = ENCODER_ROTATE_ZERO;
    mcp23017_encoder.position = 0;

    mcp23017_encoder.left = encoder_turn_left;
    mcp23017_encoder.right = encoder_turn_right;
    mcp23017_encoder.press = encoder_button_push;
    mcp23017_encoder.release = encoder_button_release;

   //#ifdef MCP23017_INTB_AVAILABLE
   //     PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
   //     PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
   // #endif

   // #ifdef MCP23017_INTA_AVAILABLE
   //     PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
   //     PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);
   // #endif

    // запретить gpio4 на output, т.е. сделать INPUT
    //gpio_output_set(0, 0, 0, (1 << MCP23017_INTB));  //GPIO_DIS_OUTPUT(gpio_no) 
    if ( mcp23017_encoder.intr_b_pin != 255 )
        GPIO_DIS_OUTPUT( mcp23017_encoder.intr_b_pin);


    if ( mcp23017_encoder.intr_a_pin != 255 )
        GPIO_DIS_OUTPUT( mcp23017_encoder.intr_a_pin);

    MCPwrite_reg16(0, GPINTENA, mcp23017_encoder.mcp_pin_clk | mcp23017_encoder.mcp_pin_dt | mcp23017_encoder.mcp_pin_btn); // 0b0000111000000000

    //MCPwrite_reg16(0, INTCONA, mcp23017_encoder.mcp_pin_clk | mcp23017_encoder.mcp_pin_dt);
    MCPwrite_reg16(0, INTCONA, 0);

    // условия срабатывания прерываний, если на пинах значение отличается от  заданного ниже (DEFVAL  = 1 )
    MCPwrite_reg16(0, DEFVALA, 0b1111111111111111);

    ETS_GPIO_INTR_DISABLE();  
    ETS_GPIO_INTR_ATTACH(mcp23017_intr_handler,NULL);
    
    if ( mcp23017_encoder.intr_b_pin != 255 )
        gpio_pin_intr_state_set(GPIO_ID_PIN(mcp23017_encoder.intr_b_pin),GPIO_PIN_INTR_NEGEDGE);
    

    if ( mcp23017_encoder.intr_a_pin != 255 )
        gpio_pin_intr_state_set(GPIO_ID_PIN(mcp23017_encoder.intr_a_pin),GPIO_PIN_INTR_NEGEDGE);

    ETS_GPIO_INTR_ENABLE();    
}

void ICACHE_FLASH_ATTR startfunc()
{
    mcp23017_encoder_init();   
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {


}

void webfunc(char *pbuf) {
    os_sprintf(HTTPBUFF,"<hr>");
    os_sprintf(HTTPBUFF,"<br>Encoder position: %d, direction: %s"
                        , mcp23017_encoder.position
                        , mcp23017_encoder.direction == 1 ? "left" : (mcp23017_encoder.direction == 2 ? "right" : "zero")
                        );
    os_sprintf(HTTPBUFF,"<br>Версия: %s", FW_VER );
}