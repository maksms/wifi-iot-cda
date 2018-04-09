#define GPIO_INPUT_IO_0     21
#define GPIO_INPUT_IO_1     22
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

static void IRAM_ATTR push_isr_handler()
{
  valdes[0]++;
}

static void IRAM_ATTR rotare_isr_handler()
{
  if(digitalRead(23)){valdes[1]--;}
  else{valdes[1]++;}
}

void startfunc(){ // выполняется один раз при старте модуля.
valdes[0]=0;
valdes[1]=0;

char data[30];
os_sprintf(data,"Push: %d",valdes[0]);
LCD_print(0,data);
os_sprintf(data,"Rotare: %d",valdes[1]);
LCD_print(1,data);

gpio_config_t io_conf;
//disable interrupt
io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
//interrupt of rising edge
io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
//bit mask of the pins, use GPIO4/5 here
io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
//set as input mode
io_conf.mode = GPIO_MODE_INPUT;
//enable pull-up mode
//io_conf.pull_up_en = 1;
gpio_config(&io_conf);

     //изменения назначения intrrupt типа на один пин
    //gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_NEGEDGE);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, push_isr_handler, NULL);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, rotare_isr_handler, NULL);
}

void timerfunc(uint32_t  timersrc) {  // выполнение кода каждую 1 секунду
static int32_t push = 0;
static int32_t rotare = 0;
if(push!=valdes[0]){
  char data[30];
  os_sprintf(data,"Push: %d",valdes[0]);
  LCD_print(0,data);
}
if(rotare!=valdes[1]){
  char data[30];
  os_sprintf(data,"Rotare: %d",valdes[1]);
  LCD_print(1,data);
}



if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}

 vTaskDelay(500 / portTICK_PERIOD_MS);
}

void webfunc(char *pbuf) { // вывод данных на главной модуля
os_sprintf(HTTPBUFF,"Push: %d<br>",valdes[0]);
os_sprintf(HTTPBUFF,"Rotare: %d<br>",valdes[1]);
}
