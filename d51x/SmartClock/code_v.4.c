
/*
*  v.4 
*  по кругу отображаем строки из конструктора строк
*  строка 1 отображается с задержкой Основная задержка
*  остальные строки отображаются с задержкой Доп. задержка
*  автозамена своих параметров:
*      _WTIME_ - день недели + часы без секунд, пример, "пн 22:56"
*      _MDATE_ - дата, месяц буквами, "21 фев 2021"
*/

/*
параметры: MainDelay,AddDelay,Animate

adc - control backlight
---------------------------------------------------------------------
cfgdes[0] - время отображения основного. экрана, сек, строка 1 конструктора строк 
cfgdes[1] - время отображения доп. экранов, сек, если 0 - не отображать
cfgdes[2] - эффект на экране часов
            0 - нет, 
            1 - бегущая горизонтальная точка слева направо, 
            2 - бегущая горизонтальная точка слева направо, потом справа налево
            3 - бегущая вертикальная точка снизу вверх
            4 - бегущая вертикальная точка снизу вверх, потом сверху вних
            5 - растущая вертикальная палка, обнуление и снова
            6 - растущая-убывающая вертикальная палка	
*/

#define FW_VER "4.0"

#define MAIN_DELAY         sensors_param.cfgdes[0]
#define ADD_DELAY          sensors_param.cfgdes[1]
#define ANIMATE             sensors_param.cfgdes[2]
#define RUN_SPEED             sensors_param.cfgdes[3]


#define MATRIX_COUNT 8

char* day_of_week[7] = {"пн","вт","ср","чт","пт","сб","вс"};
char* text_month[12] = {"янв","фев","мар","апр","май","июн","июл","авг","сен","окт","ноя","дек"};

int8_t mode = 0; // 0 - default, 1 - time, 2 - date, 3 - bedroom temp
            



void ICACHE_FLASH_ATTR repl_wtime(char *buf)
{
    //os_strcpy(buf, "mo 14:34");
	static uint8_t blynk = 1;
	blynk = 1 - blynk;
    os_sprintf(buf,"%s  %02d%s%02d   ", day_of_week[time_loc.dow], time_loc.hour, blynk ? ":" : " ", time_loc.min);
	
}

void ICACHE_FLASH_ATTR repl_mdate(char *buf)
{
    //os_strcpy(buf, "23 jan 2021");
	os_sprintf(buf," %02d %s %02d   ", time_loc.day, text_month[time_loc.month-1], time_loc.year );
}

static uint16_t str_line = 0;
	static uint16_t cnt = 1;
	static int step = 0;
	
typedef void (* repl_func)(char *); 

typedef struct repl_params {
		const char *name;
		repl_func func;
} repl_params_t;

#define REPL_PARAMS_CNT 2
repl_params_t REPLACE_PARAMS[REPL_PARAMS_CNT] = {
	 { "_WTIME_", repl_wtime}		// пн  22:34
	,{ "_MDATE_", repl_mdate}     // 21 фев 2021
};

void ICACHE_FLASH_ATTR replace_param(char *str, const char *repl_str, repl_func replfun)
{
    char r[100];
    char *astr = (char *)os_strstr(str, repl_str);
    if ( astr != NULL )
    {
        os_memset(r, 0, 100);
        int pos = astr - str;
        os_strncpy(r, str, pos);
        
        char *buf = (char *)os_zalloc(20);
        replfun(buf);
        os_strcat(r, buf);
        os_strcat(r, str + pos + os_strlen(repl_str) );
        os_memset(str, 0, 100);
        os_strcpy(str,r);
        os_free(buf);
    }     
}

void ICACHE_FLASH_ATTR matrix_print_dot(uint8_t section, uint8_t row, uint8_t col) 
{
    // 1 - 1, 2 - 2, 3 - 4, 4 - 8, 5 - 16, 6 - 32, 7 - 64, 8 - 128
    uint8_t pos = 0;
    pos = ( col == 0 ) ? 1 : 2<<(col-1);
    maxOne(section, row, pos);
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

void ICACHE_FLASH_ATTR startfunc(){
// выполняется один раз при старте модуля.
}



void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду
    
// mtrxsec=2			// Line print: 2 sec.
// mtrxspeed=120		// Speed print: 120 ms.
// mtinterv=3			// Interval: 3 min.
// replacesens(uint8_t num, char *buf)		в buf положит содержимое номера строки из конструктора строк
// maxlines - кол-во строк в конструкторе строк
// sensors_param.strrep[i]  номер строки из конструктора строк
// valdesen - кол-во глобальных переменных конструктора кода

	

	

	
	
	if ( step < MAIN_DELAY-1 || MAIN_DELAY == 0)
	{
		// берем первую строку из конструктора строк
		str_line = 0;
	}
	else  if (  step == MAIN_DELAY-1 ) 
	{
		// задержка отображения первой строки прошла, берем следующую строку конструктора строк 2,3,4 и т.д.
		cnt++;	
		
		// если строка в конструкторе строк пустая, пропустим ее, берем следующую
		while ( sensors_param.strrep[cnt] == NULL || os_strcmp(sensors_param.strrep[cnt], "") == 0)
		{
				cnt++;			
				if ( cnt >= maxlines ) break;
		}
	}
	else
	{
		str_line = cnt;
	}
	
	step++;
	
	if (  step == (MAIN_DELAY+ADD_DELAY)  ) step = 0;
	

	if ( cnt >= maxlines ) cnt = 1;

	uint8_t k;
	char *str = (char *)os_zalloc(100);
	replacesens(str_line, str);	// замена стандартных параметров в строке средствами прошивки
	
	for ( k = 0; k < REPL_PARAMS_CNT; k++)
	{
		// теперь замена своих параметров
		replace_param( str, REPLACE_PARAMS[k].name, REPLACE_PARAMS[k].func);
	}
		
	MATRIX_print (str, 1 , 0);	// выводим постоянно раз в секунду
	os_free(str);	
	
  // print running dot
  switch ( ANIMATE )
  {
      case 1: horizontal_running_dot(1, 60); break;
      case 2: vertical_running_dot(8, 8, 8); break;
      case 3: vertical_bar(8, 8, 8); break;
      default:
        break;
  }
}

void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF,"<br><b>Версия прошивки:</b> %s", FW_VER);
}

