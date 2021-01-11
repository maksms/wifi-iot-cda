
/*
*  v.3 
*
*/

/*
adc - control backlight
mode - show other data from Interpreter via valdes:

valdes[0] - режим экрана:
    0 - основной режим - часы с днем недели
    1 - дата (через КК)
    2 - температура в комнате (через КК или Interpreter)
    3 - температура на улице  (через КК или Interpreter)
    4 - прогноз на завтра     (через Interpreter)
---------------------------------------------------------------------
cfgdes[1] - время отображения доп. экранов, сек, если 0 - не отображать
valdes[1]
---------------------------------------------------------------------
cfgdes[2] - эффект на экране часов
valdes[2]   0 - нет, 
            1 - бегущая горизонтальная точка слева направо, 
            2 - бегущая горизонтальная точка слева направо, потом справа налево
            3 - бегущая вертикальная точка снизу вверх
            4 - бегущая вертикальная точка снизу вверх, потом сверху вних
            5 - растущая вертикальная палка, обнуление и снова
            6 - растущая-убывающая вертикальная палка
*/


// valdes[3] - температура на улице
// valdes[4] - температура в комнате



#define FW_VER "3.0"


#define WORK_MODE           valdes[0]
#define MODE_SPEED          sensors_param.cfgdes[0]
#define MODE_DELAY          sensors_param.cfgdes[1]
#define ANIMATE             sensors_param.cfgdes[2]

#define TEMP_STREET         valdes[3]
#define TEMP_BEDROOM        valdes[4]
#define TEMP_HOME           valdes[5]

#define MATRIX_COUNT 8

char* day_of_week[7] = {"пн","вт","ср","чт","пт","сб","вс"};
char* text_month[12] = {"янв","фев","мар","апр","май","июн","июл","авг","сен","окт","ноя","дек"};

uint8_t mode = 0; // 0 - time, 1 - date, 2 - bedroom temp
                    
#define matrix_clear_row(section, row) maxOne(section, row, 0);

void ICACHE_FLASH_ATTR matrix_clear(uint8_t section)
{
    uint8_t row = 0;
    for(row=1;row<9;row++) maxOne(section, row, 0);
}

void ICACHE_FLASH_ATTR matrix_print_dot(uint8_t section, uint8_t row, uint8_t col) 
{
    // 1 - 1, 2 - 2, 3 - 4, 4 - 8, 5 - 16, 6 - 32, 7 - 64, 8 - 128
    uint8_t pos = 0;
    pos = ( col == 0 ) ? 1 : 2<<(col-1);
    maxOne(section, row, pos);
}

void ICACHE_FLASH_ATTR matrix_print_line(uint8_t row, uint8_t start, uint8_t length) 
{   
    uint8_t idx = 0;
    uint8_t col = 0;
    uint8_t  matrix_number = 0;
    for ( idx = start; idx < (start+length); idx++) {
        if ( idx > 8*MATRIX_COUNT ) return;
       matrix_number = (start / 8) + 1;
       col = idx - (MATRIX_COUNT * (matrix_number-1) );
       maxOne(matrix_number, row, 2<<(col-1));
    }    
}


void ICACHE_FLASH_ATTR print_time_with_day(uint8_t blynk) {
    // time:  00:00
    char data[20];
    os_sprintf(data,"%s  %02d%s%02d   ", day_of_week[time_loc.dow], time_loc.hour, blynk ? ":" : " ", time_loc.min);
    MATRIX_print (data, 1 , 0);
}

void ICACHE_FLASH_ATTR print_date_text_month() {
    // date:  dd.mm.yy
    char data[20];
    os_sprintf(data," %02d %s %02d   ", time_loc.day, text_month[time_loc.month-1], time_loc.year );
    //time_loc.dow
    MATRIX_print (data, 1 , 0);
}

int8_t ICACHE_FLASH_ATTR round_temp(int32_t temp) 
{
    if ( temp%10 < -5) {
        return (-1 + temp/10);
    } 
    else if  ( temp%10 < 5) {
        return temp/10;
    }
    return (1 + temp/10);
}

void ICACHE_FLASH_ATTR print_matrix(const char *template, int32_t value, uint8_t scroll)
{
    char data[50];
    os_sprintf(data,template, value);
    MATRIX_print (data, 1 , scroll);
}

void ICACHE_FLASH_ATTR print_temp_bedroom(uint8_t scroll) 
{
    char data[50];
    os_sprintf(data,"Спальня %d°    ", round_temp( TEMP_BEDROOM )); 
    MATRIX_print (data, 1 , scroll);
}

void ICACHE_FLASH_ATTR print_temp_house(uint8_t scroll) 
{
    char data[50];
    os_sprintf(data,"Дома %d°    ", round_temp( TEMP_HOME ));
    MATRIX_print (data, 1 , scroll);
}

void ICACHE_FLASH_ATTR print_temp_street(uint8_t scroll) 
{
    char data[50];
    int32_t street = (TEMP_STREET > 500 ) ? TEMP_STREET - 1000 : TEMP_STREET;
    os_sprintf(data,"Улица %s%d°    ", street < 5 ? " " : "+", round_temp( street ));
    MATRIX_print (data, 1 , scroll);

}

void ICACHE_FLASH_ATTR startfunc(){
// выполняется один раз при старте модуля.
}

void ICACHE_FLASH_ATTR horizontal_running_dot(uint8_t line, uint8_t limit) {
    static pos = 1;
    matrix_print_dot( pos / 8 + 1, line, pos % 8);
    pos++;
    if (pos > limit) pos = 1;
}

void ICACHE_FLASH_ATTR vertical_running_dot(uint8_t matrix, uint8_t col, uint8_t limit) {
    static row = 1;
    matrix_print_dot( matrix, row, col-1);
    row++;
    if (row > limit) row = 1;
}

void ICACHE_FLASH_ATTR vertical_bar(uint8_t matrix, uint8_t col, uint8_t height) {
    static row = 1;
    for ( row = 1; row <= height; row++) {
        matrix_print_dot( matrix, row, col-1);
    }    
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду

uint32_t val = mode;
    val = WORK_MODE;
    if (val != mode) {
        mode = val;
    }

    if(timersrc%5==0){
    // выполнение кода каждые 5 секунд
    }

    if(timersrc%30==0){
    // выполнение кода каждые 30 секунд
    }
    
    mode = 0;
    if ( MODE_SPEED > 10 && MODE_DELAY > 1 )
        {
        static int i = 0;
        if ( i >= MODE_SPEED && i <= ( MODE_SPEED + MODE_DELAY ) ) 
        { 
            mode = 2;
        }  
        else if ( i >= MODE_SPEED * 2 && i <= ( MODE_SPEED*2 + MODE_DELAY ) ) 
        {
            mode = 4;
        }
        else if ( i >= MODE_SPEED * 3 && i <= ( MODE_SPEED*3 + MODE_DELAY ) ) 
        { 
            mode = 1;
        }
        i++;
        if ( i == MODE_SPEED*3 + MODE_DELAY +1 ) i=0;    
    }

    switch (mode) {
        case 0: print_time_with_day(timersrc%2); break;
        case 1: print_date_text_month(); break;
        case 2: print_temp_bedroom(0); break;  // спальня
        case 3: print_temp_house(0); break;  // дом
        case 4: print_temp_street(0); break;  // улица
        default:
            print_time_with_day(timersrc%2); break;
    }
    

  // print running dot
  switch ( ANIMATE )
  {
      case 0: horizontal_running_dot(1, 60); break;
      case 1: vertical_running_dot(8, 8, 8); break;
      case 2: vertical_bar(8, 8, 8); break;
      default:
        horizontal_running_dot(1, 60); break;
  }
    
    //


}

void webfunc(char *pbuf) {

    os_sprintf(HTTPBUFF,"<br><b>Версия прошивки:</b> %s", FW_VER);

}

