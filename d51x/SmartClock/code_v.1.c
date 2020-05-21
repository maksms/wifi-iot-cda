#define FW_VER "1.9"

#define MATRIX_COUNT 8
/*
       1 2 3 4 5 6 7 8
    1: * * * * * * * *          
    2: * * * * * * * *
    3: * * * * * * * * 
    4: * * * * * * * * 
    5: * * * * * * * * 
    6: * * * * * * * * 
    7: * * * * * * * * 
    8: * * * * * * * * 
*/

/*
    каждая буква и цифра занимают 5 точек
    разделитель между символами - 1 точка
    текст пишется пропуская 1-ую колонку
*/

typedef  uint8_t bitmap_t[8*MATRIX_COUNT][8];
bitmap_t bitmap; 

char* day_of_week[7] = {"пн","вт","ср","чт","пт","сб","вс"};
char* text_month[12] = {"янв","фев","мар","апр","май","июн","июл","авг","сен","окт","ноя","дек"};

uint8_t mode = 0; // 0 - time, 1 - date, 2 - bedroom temp
                    
void ICACHE_FLASH_ATTR matrix_clear_row(uint8_t section, uint8_t row){
    maxOne(section, row, 0);
}

void ICACHE_FLASH_ATTR matrix_clear(uint8_t section){
    uint8_t row = 0;
    for(row=1;row<9;row++) maxOne(section, row, 0);
}

void ICACHE_FLASH_ATTR matrix_print_dot(uint8_t section, uint8_t row, uint8_t col) {
    // 1 - 1, 2 - 2, 3 - 4, 4 - 8, 5 - 16, 6 - 32, 7 - 64, 8 - 128
    uint8_t pos = 0;
    pos = ( col == 0 ) ? 1 : 2<<(col-1);
    maxOne(section, row, pos);
}

void ICACHE_FLASH_ATTR matrix_print_line(uint8_t row, uint8_t start, uint8_t length) {
    
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


void ICACHE_FLASH_ATTR print_time(uint8_t blynk) {
    // time:  00:00
    char data[5];
    os_sprintf(data,"%02d%s%02d", time_loc.hour, blynk ? ":" : " ", time_loc.min);
    MATRIX_print (data, 1 , 0);
}

void ICACHE_FLASH_ATTR print_time_with_day(uint8_t blynk) {
    // time:  00:00
    char data[9];
    os_sprintf(data,"%s  %02d%s%02d   ", day_of_week[time_loc.dow], time_loc.hour, blynk ? ":" : " ", time_loc.min);
    MATRIX_print (data, 1 , 0);
}

void ICACHE_FLASH_ATTR print_date() {
    // date:  dd.mm.yy
    char data[8];
    os_sprintf(data,"%02d.%02d.%02d", time_loc.day, time_loc.month, time_loc.year);
    //time_loc.dow
    MATRIX_print (data, 1 , 0);
}

void ICACHE_FLASH_ATTR print_date_text_month() {
    // date:  dd.mm.yy
    char data[9];
    os_sprintf(data,"%02d %s %02d", time_loc.day, text_month[time_loc.month-1], time_loc.year);
    //time_loc.dow
    MATRIX_print (data, 1 , 0);
}

int8_t ICACHE_FLASH_ATTR round_temp(int32_t temp) {
    if ( temp%10 < 5) {
        return temp/10;
    } 
    return temp = 1 + temp/10;
}

void ICACHE_FLASH_ATTR print_temp_bedroom(uint8_t scroll) {
    char data[50];
    os_sprintf(data,"Спальня %d     ", round_temp(vsens[2][0]));
    MATRIX_print (data, 1 , scroll);
}

void ICACHE_FLASH_ATTR print_temp_house(uint8_t scroll) {
    char data[50];
    os_sprintf(data,"Дома %d     ", round_temp(vsens[1][0]));
    MATRIX_print (data, 1 , scroll);
}

void ICACHE_FLASH_ATTR print_temp_street(uint8_t scroll) {
    char data[50];
    os_sprintf(data,"Улица %s%d     ", vsens[0][0] < 0 ? " " : "+", round_temp(vsens[0][0]));
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

void print_bitmap(bitmap_t *bitmap) {

}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду

uint32_t val = mode;
    val = valdes[0];
    if (val != mode) {
        mode = val;
    }

    /*
        в авто-режиме:
            отображаем часы каждую секунду
            отображаем дату каждую 10 мин, на 2 сек
            отображаем температуру в комнате каждую 2-ю минуту на 2 сек
            отображаем температуру на улице каждые каждую 3-ю минуту на 2 сек
    */



    if(timersrc%5==0){
    // выполнение кода каждые 5 секунд
    }

    if(timersrc%30==0){
    // выполнение кода каждые 30 секунд
    }
    
    mode = 0;
    static int i = 0;
    if(i==60 || i==61) { // выполнение кода каждые 2 мин (120 секунд)
        mode = 2;
    }  
    if(i==120 || i==121) { // выполнение кода каждые 3 мин (180 секунд)
        mode = 4;
    }
    if(i==180 || i==181) { // выполнение кода каждые 10 мин (600 секунд)
        mode = 1;
    } 
    i++;
    if (i==182) i=2;    

    switch (mode) {
        //case 0: print_time(timersrc%2); break;
        case 0: print_time_with_day(timersrc%2); break;
        //case 1: print_date(); break;
        case 1: print_date_text_month(); break;
        case 2: print_temp_bedroom(0); break;  // спальня
        case 3: print_temp_house(0); break;  // дом
        case 4: print_temp_street(0); break;  // улица
    }

  // print running dot
    horizontal_running_dot(1, 60);
    //vertical_running_dot(8, 8, 8);

/*
     static uint8_t i = 1;
    vertical_bar(3, 5, i);
    i++;
    if (i>8) i=1;   
*/

/*
0: 1
1: 2
2: 4
3: 8
4: 16
5: 32
6: 64
7: 128
*/
//maxOne(8, 8, 1 + 4 + 16 + 64);
}

void webfunc(char *pbuf) {
    
    char data[50];
    switch (mode) {
        case 0: os_sprintf(data,"Время"); break;
        case 1: os_sprintf(data,"Дата"); break;
        case 2: os_sprintf(data,"Температура в комнате %d", vsens[2][0]); break;
        case 3: os_sprintf(data,"Дома %d", vsens[1][0]); break;
        case 4: os_sprintf(data,"Улица %d", vsens[0][0]); break;
        

    }
    os_sprintf(HTTPBUFF,"<br><b>Режим: </b> %d - %s", mode, data);

    os_sprintf(HTTPBUFF,"<br><b>Версия прошивки:</b> %s", FW_VER);

}

