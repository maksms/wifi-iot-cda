// Реализация энкодера через прерывания.
// Фильтрация дребезга контактов аппаратная.
// GPIO21 для кнопки энкодера
// GPIO22 для поворотного диска (CLK)
// GPIO23 для поворотного диска (DATA)

#define GPIO_INPUT_IO_0     21 
#define GPIO_INPUT_IO_1     22 
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

// обработчики прерываний обязательно до первого упоминания в коде
static void IRAM_ATTR push_isr_handler() // обработчик прерывания кнопки
{
  valdes[0]++;
}

static void IRAM_ATTR rotare_isr_handler() // обработчик прерывания поворотного диска
{
  if(digitalRead(23)){valdes[1]--;}
  else{valdes[1]++;}
}

void startfunc(){ // выполняется один раз при старте модуля.
  valdes[0]=0;
  valdes[1]=0;
  // первое обновление дисплея
  char data[30];
  os_sprintf(data,"Push: %d",valdes[0]);
  LCD_print(0,data);
  os_sprintf(data,"Rotare: %d",valdes[1]);
  LCD_print(1,data);
  // настройка режимов GPIO
  gpio_config_t io_conf;
  // отключаем прерывания
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  // срабатывание прерывания по фронту
  io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
  // выбираем GPIO21 и GPIO22
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
  // режим выбранных портов на вход
  io_conf.mode = GPIO_MODE_INPUT;
  // включение внутренней подтяжки GPIO к питанию
  // io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  // изменение выбранного GPIO на срабатывание по спаду
  // gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_NEGEDGE);

  // инициализация сервиса прерывания GPIO
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  // добавляем срабатывание прерывания для GPIO21
  gpio_isr_handler_add(GPIO_INPUT_IO_0, push_isr_handler, NULL);
  // добавляем срабатывание прерывания для GPIO22
   gpio_isr_handler_add(GPIO_INPUT_IO_1, rotare_isr_handler, NULL);
}

void timerfunc(uint32_t  timersrc) {  // выполнение кода каждую 1 секунду
  // локальные нестираемые переменные
  static int32_t push = 0;
  static int32_t rotare = 0;
  // если нажимали кнопку энкодера, то изменения на дисплей
  if(push!=valdes[0]){
    char data[30];
    os_sprintf(data,"Push: %d",valdes[0]);
    LCD_print(0,data);
  }
  // если крутили энкодер, то изменения на дисплей
  if(rotare!=valdes[1]){
    char data[30];
    os_sprintf(data,"Rotare: %d",valdes[1]);
    LCD_print(1,data);
  }



  if(timersrc%30==0){ // выполнение кода каждые 30 секунд
    
  }
  // задержка функции timerfunc()
  vTaskDelay(500 / portTICK_PERIOD_MS); 
}

void webfunc(char *pbuf) { // вывод данных на главной модуля
  // можно увидеть и на вебморде, но не динамично
  os_sprintf(HTTPBUFF,"Push: %d<br>",valdes[0]);
  os_sprintf(HTTPBUFF,"Rotare: %d<br>",valdes[1]);
}