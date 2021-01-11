#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
/* убрал лишнего из веб*/
#define FW_VER "2.2"
/*

через интерпретер вычисляем температуру с датчиков и VSENS (D2D) и записываем ее в valdes
0     | 1                    | 2          | 3              | 4           | 5                   | 6          | 7             | 8             | 9             | 10            | 11            |
Режим , Источник температуры , Период/сек , Гистерезис/x10 , Уставка/x10 , Задержка насоса/сек , Расписание , ЧЧММУСТ/x10-1 , ЧЧММУСТ/x10-2 , ЧЧММУСТ/x10-3 , ЧЧММУСТ/x10-4 , ЧЧММУСТ/x10-5 

используется 2 термостата

!!!!! для расписания доступно 5 временных интервалов, их можно увеличить (до 7) или уменьшить через CFG_TEMPSET_SCHEDULE_COUNT
!!!!! если используете расписание, то необходимо заполнять все временные интервалы и уставки
!!!!! заполнение всех временных интервалов должно идти последовательно по времени от меньшего к большему
!!!!! температура уставки так же должна быть заполнена
!!!!! если вам интервалов много, сократите их
элемент расписания формируется следующим образом
NHHMMTTT, пример, 10715230
где N = 1, номер расписания
    HH = 07 - 7 часов (ведущий 0 обязателен)
    MM = 15 - 15 минут, (если требуется указать 0 минут, то ведущий 0 обязателен, т.е. будет 00)
    TTT = 230 - уставка х10, т.е. 23.0
 

номер x / 10000000
остаток без номера  y = x % 10000000    = 04 00 230
часы y / 100000
остаток без часов z = y % 100000    = 00 230
минуты z / 1000
остаток без минут t = z % 1000

*/


#define GPIO_ON 1
#define GPIO_OFF 0

#define KOTEL1_GPIO 12
#define KOTEL2_GPIO 14 
#define KOTEL2_NIGHT_MODE_GPIO 16  // ESC режим для Протерма

#define NIGHT_MODE_START_TIME 23
#define NIGHT_MODE_END_TIME 7

#define THERMOSTAT_PERIOD 40
#define THERMOSTAT_TEMPSET 250
#define THERMOSTAT_HYSTERESIS 5

#define PUMP_DELAY 300

#define KOTEL1_NAME "Kotel1"
#define KOTEL2_NAME "Kotel2"

#define DIV_INDEX 10000000
#define DIV_HOUR  100000
#define DIV_MINTE 1000

typedef enum {
    KOTEL_NONE,
    KOTEL_1,
    KOTEL_2
} kotel_type_t;

kotel_type_t active_kotel = 0; // 0 - нет активного котла, 1 - дизельный, 2 - электрический

/*
* режим работы управления котлами:
*       0 - ручной, термостаты не включены, можно кнопками вкл/выкл gpio котлов
*       1 - авто, используются термостаты в автоматическом режиме, можно как с глобальной уставкой, так и по расписанию
*       2 - активный котел 1, используется термостат
*       3 - активный котел 2, используется термостат
*/
#define CFG_INDEX_WORK_MODE                     0   // sensors_param.cfgdes[0]

// источник температуры для термостата: 0 - внутренние расчеты, 1 - внешняя температура valdes4
#define CFG_INDEX_TEMP_SOURCE                   1   //  sensors_param.cfgdes[1]

// период срабатывания термостата в секундах
#define CFG_INDEX_THERMO_PERIOD                 2   //  sensors_param.cfgdes[2]

// гистерезис х10 (valdes1)
#define CFG_INDEX_THERMO_HYSTERESIS             3   //  sensors_param.cfgdes[3]

// глобальная уставка, принимаем из внешней системы (valdes2)
#define CFG_INDEX_THERMO_TEMPSET                4   //  sensors_param.cfgdes[4]

// задержка выключения насоса котла2, сек,      
// котел 1 не должен включаться столько времени, после выключения котла 2
#define CFG_INDEX_PUMP_DELAY                    5   //  sensors_param.cfgdes[5]

// расписание (0 - выкл, 1  - вкл) valdes3
#define CFG_INDEX_SCHEDULE_ENABLED              6   //  sensors_param.cfgdes[6]

// расписания
#define CFG_TEMPSET_SCHEDULE_START_IDX          7   // sensors_param.cfgdes[7]


// режим работы
#define VALDES_INDEX_WORK_MODE                  0   //  valdes[0]

#define VALDES_INDEX_TEMP_SOURCE                -1  
#define VALDES_INDEX_THERMO_PERIOD              -1 

// гистерезис
#define VALDES_INDEX_THERMO_HYSTERESIS          1   //  valdes[1] 

// глобальная уставка (внешняя)     
#define VALDES_INDEX_THERMO_TEMPSET             2   //  valdes[2]

#define VALDES_INDEX_PUMP_DELAY                 -1

// расписание вкл.выкл
#define VALDES_INDEX_SCHEDULE_ENABLED           3   //  valdes[3]

#define VALDES_TEMPSET_SCHEDULE_START_IDX       -1

// внешняя температура (вычисляется в УД по датчикам)
#define VALDES_INDEX_TEMP_EXTERNAL              4   //  valdes[4]

// внутренняя температура (вычисляется в Интерпретер по датчикам,  VSENS, D2D)
#define VALDES_INDEX_TEMP_INTERNAL              5   //  valdes[5]

#define CFG_TEMPSET_SCHEDULE_COUNT      5
#define TEMP_INTERNAL                    valdes[VALDES_INDEX_TEMP_INTERNAL]
#define TEMP_EXTERNAL                    valdes[VALDES_INDEX_TEMP_EXTERNAL]           


uint8_t work_mode = 0; // режим работы (cfg0)

#ifdef PUMP_DELAY
uint16_t pump_delay = 300;
uint16_t pump_active = 0;
os_timer_t pump_timer;
#endif

uint8_t kotel_gpio = 255;
uint16_t global_tempset = THERMOSTAT_TEMPSET;
uint8_t temp_source = 0;

typedef struct {
    int32_t value;
    uint8_t idx;
    uint8_t hour;
    uint8_t min;  
    uint16_t tempset;   // x10
} schedules_tempset_t;

schedules_tempset_t schedules_tempset[CFG_TEMPSET_SCHEDULE_COUNT];
uint8_t schedule_enabled = 0;

int16_t temperature = 0;

//************************ объект Термостат и функции ***********************************************************
typedef void (* func_therm)(void *args);  
typedef void *thermostat_handle_t;  

typedef struct thermostat {
    uint8_t enabled;
    uint8_t state;
    uint8_t period;
    int16_t tempset;// уставка х10
    uint8_t hysteresis;// гистерезис х10
    int16_t value; // текущая температура х10
    func_therm process;       // сама функция термостата, вызывает коллбеки load_off и load_on
    void *args_process;
    func_therm load_off;
    void *args_on;
    func_therm load_on;
    void *args_off;
    os_timer_t tmr;
} thermostat_t;


void ICACHE_FLASH_ATTR thermostat_enable(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    //thermo->state = 1;
    thermo->enabled = 1;
}

void ICACHE_FLASH_ATTR thermostat_disable(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    thermo->state = 0;
    thermo->enabled = 0;
}

void ICACHE_FLASH_ATTR thermostat_start(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    os_timer_disarm( &thermo->tmr );
	os_timer_setfn( &thermo->tmr, (os_timer_func_t *)thermo->process, NULL);
	os_timer_arm( &thermo->tmr, thermo->period * 1000, 1);
}

void ICACHE_FLASH_ATTR thermostat_stop(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    os_timer_disarm( &thermo->tmr );
}

void ICACHE_FLASH_ATTR thermostat_set_load_on_cb(thermostat_handle_t thermo_h, func_therm _cb, void *_args)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;    
    if ( _cb == NULL ) return;
    thermo->load_on = _cb;
    thermo->args_on = _args;
}

void ICACHE_FLASH_ATTR thermostat_set_load_off_cb(thermostat_handle_t thermo_h, func_therm _cb, void *_args)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;    
    if ( _cb == NULL ) return;
    thermo->load_off = _cb;
    thermo->args_off = _args;
}

void ICACHE_FLASH_ATTR thermostat_process(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    if ( thermo->enabled == 0 ) return; // термостат выключен

    if ( thermo->value >= thermo->tempset + thermo->hysteresis )
    {
        if ( thermo->load_off ) thermo->load_off( thermo->args_off );
        thermo->state = 0;
    } 
    else if ( thermo->value <= thermo->tempset - thermo->hysteresis )
    {
        if ( thermo->load_on ) thermo->load_on( thermo->args_on );
        thermo->state = 1;
    }
}

thermostat_handle_t ICACHE_FLASH_ATTR thermostat_create(uint8_t _period, int16_t _tempset, uint8_t _hysteresis)
{
    //thermostat_t *thermo;
    thermostat_t *_thermo = (thermostat_t *) os_malloc(sizeof(thermostat_t));
    _thermo->state = 0;
    _thermo->enabled = 0;
    _thermo->period = _period;
    _thermo->tempset = _tempset;
    _thermo->hysteresis = _hysteresis;
    //thermo->process = thermostat_process;
    //thermo->arg_process = thermo;
    _thermo->load_off = NULL;
    _thermo->args_off = NULL;
    _thermo->load_on = NULL;
    _thermo->args_on = NULL;
    _thermo->value = 0;
    return (thermostat_handle_t) _thermo;
}

void ICACHE_FLASH_ATTR thermostat_set_value(thermostat_handle_t thermo_h, int16_t _value)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    thermo->value = _value;
}

void ICACHE_FLASH_ATTR thermostat_set_tempset(thermostat_handle_t thermo_h, int16_t _value)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    thermo->tempset = _value;
}

void ICACHE_FLASH_ATTR thermostat_set_hysteresis(thermostat_handle_t thermo_h, uint8_t _value)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    thermo->hysteresis = _value;
}

void ICACHE_FLASH_ATTR thermostat_set_period(thermostat_handle_t thermo_h, uint8_t _value)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    thermo->period = _value;
}

//**************************************************************************************************************

thermostat_handle_t thermo_h;
thermostat_t *thermo;

int8_t ICACHE_FLASH_ATTR get_schedule_index() 
{
    int8_t idx = -1;
    if ( !schedule_enabled ) return idx;

    uint8_t i;

    uint16_t local_minutes = time_loc.hour*60 + time_loc.min;

    for ( i = 0; i < CFG_TEMPSET_SCHEDULE_COUNT; i++)
    {
        uint16_t schedule_minutes = schedules_tempset[i].hour*60 + schedules_tempset[i].min;
        uint16_t schedule_minutes_next = 23*60+59;
        uint16_t schedule_minutes_prev = 0;
        if ( i > 0 ) schedule_minutes_prev = schedules_tempset[i-1].hour*60 + schedules_tempset[i-1].min;
        if ( i < CFG_TEMPSET_SCHEDULE_COUNT-1 ) schedule_minutes_next = schedules_tempset[i+1].hour*60 + schedules_tempset[i+1].min;

        bool is_active = local_minutes >= schedule_minutes
                        && local_minutes > schedule_minutes_prev
                        && local_minutes < schedule_minutes_next
                        && schedule_minutes != 24*60;

        if ( is_active )
        {
            idx = i;
            break;
        }
        
    }
    return idx;
}

#ifdef PUMP_DELAY
void ICACHE_FLASH_ATTR reset_pump_cb(){
    if ( pump_active > 0 ) {
        pump_active--;
    } else {
        os_timer_disarm( &pump_timer );
    }

}

void ICACHE_FLASH_ATTR start_pump_timer(){
    pump_active = pump_delay; // активируем флаг, что насос котла 2 еще работает после выключения котла 2
    os_timer_disarm( &pump_timer );
    os_timer_setfn( &pump_timer, (os_timer_func_t *)reset_pump_cb, NULL);
    os_timer_arm( &pump_timer, 1000, 1);    
}
#endif

kotel_type_t ICACHE_FLASH_ATTR set_active_kotel()
{
    // функция срабатывает каждую секунду
    kotel_type_t _kotel = KOTEL_NONE;

    if ( work_mode == 0)  {
        // ручное управление котлами через gpio, термостат не используется 
        return _kotel;
    }

    if ( work_mode == 1) {
        // auto режим
        if ( time_loc.hour >= NIGHT_MODE_END_TIME && time_loc.hour < NIGHT_MODE_START_TIME ) {
            // день, Т1
            _kotel = KOTEL_1; 
            kotel_gpio = KOTEL1_GPIO;
            
            #ifdef KOTEL2_GPIO
                #ifdef KOTEL2_NIGHT_MODE_GPIO
                    GPIO_ALL(KOTEL2_NIGHT_MODE_GPIO, GPIO_OFF);
                #endif

                uint8_t kotel2_gpio_state = GPIO_ALL_GET(KOTEL2_GPIO);
                if ( kotel2_gpio_state )
                {
                    #ifdef PUMP_DELAY
                    start_pump_timer();
                    #endif
                }
                GPIO_ALL(KOTEL2_GPIO, GPIO_OFF);
            #endif
        }
        #ifdef KOTEL2_GPIO 
        else {
            // ночь, Т2
            _kotel = KOTEL_2; 
            kotel_gpio = KOTEL2_GPIO;

            #ifdef KOTEL2_NIGHT_MODE_GPIO
                GPIO_ALL(KOTEL2_NIGHT_MODE_GPIO, GPIO_ON);
            #endif
            
            GPIO_ALL(KOTEL1_GPIO, GPIO_OFF);
        }   
        #endif
    } else if ( work_mode == 2 ) {
        // принудительная работа котла 1 по термостату, котел2 не используется
        _kotel = KOTEL_1;
        kotel_gpio = KOTEL1_GPIO;
    } 
    #ifdef KOTEL2_GPIO
    else if ( work_mode == 3 ) {
        // принудительная работа котла 2 по термостату, котел1 не используется
        _kotel = KOTEL_2;
        kotel_gpio = KOTEL2_GPIO;
    }
    #endif

    return _kotel;
}

void ICACHE_FLASH_ATTR load_off(void *args)
{
    // функция отключения нагрузки
    //GPIO_ALL_M(kotel_gpio, GPIO_OFF);
    
    #ifdef KOTEL2_GPIO
    if ( kotel_gpio == KOTEL2_GPIO ) {
        #ifdef PUMP_DELAY
        uint8_t st = GPIO_ALL_GET( kotel_gpio );
        if ( st == GPIO_ON ) {
            start_pump_timer();
        }
        #endif
    }
    #endif

    GPIO_ALL(kotel_gpio, GPIO_OFF);
}

void ICACHE_FLASH_ATTR load_on(void *args)
{
    // функция включения нагрузки
    //GPIO_ALL_M(kotel_gpio, GPIO_ON);  
    #ifdef PUMP_DELAY
    if ( kotel_gpio == KOTEL1_GPIO && pump_active > 0 ) return; // следующий запуск по термостату должен включить, т.к. к тому времени флаг должен быть сброшен
    #endif
    GPIO_ALL(kotel_gpio, GPIO_ON);   
}



void ICACHE_FLASH_ATTR startfunc()
{
    // выполняется один раз при старте модуля.
    // прочитать настройки из конфига
    thermo_h = thermostat_create(THERMOSTAT_PERIOD, THERMOSTAT_TEMPSET, THERMOSTAT_HYSTERESIS);
    thermo = (thermostat_t *) thermo_h;
    thermostat_set_load_off_cb(thermo_h, load_off, NULL);
    thermostat_set_load_on_cb(thermo_h, load_on, NULL);
    //thermostat_start(thermo_h);
}


void ICACHE_FLASH_ATTR mqtt_send_valuedes(int8_t idx, int32_t value)
{
    if ( idx < 0 ) return;
    valdes[idx] = value;
    // чтобы все корректно работало и значене не поменялось обратно через mqtt, нужно отправить новое значение
    #ifdef MQTTD
    if ( sensors_param.mqtten && mqtt_client != NULL) {
        system_soft_wdt_feed();
        os_memset(payload, 0, 20);
        os_sprintf(payload,"%d", value);
        char topic[12];
        sprintf(topic, "valuesdes%d", idx)
        MQTT_Publish(mqtt_client, topic, payload, os_strlen(payload), 2, 0, 1);
        os_delay_us(20);
    }
    #endif  
}

bool ICACHE_FLASH_ATTR handle_config_param(uint8_t cfg_idx, int8_t valdes_idx, int32_t *param, int32_t def_val, int32_t min_val, int32_t max_val)
{
    int32_t val = 0; 
    bool need_save = false;

    // проверка valdes в первую очередь, могли прийти по http get или по mqtt с внешней системы
    // сравниваем с текущим значением  и меняем его, а так же меняем cfgdes
    // теперь смотрим valdes, поменялось ли там значение


    //  а теперь смотрим опцию, поменялось ли там значение
    val =  (sensors_param.cfgdes[cfg_idx] < min_val || sensors_param.cfgdes[cfg_idx] > max_val) ? def_val : sensors_param.cfgdes[cfg_idx];  
    if ( val != *param ) {  // значение изменилось, в опции новое значение
        //memcpy(param, &val, sizeof(int32_t));
        *param = val;
        mqtt_send_valuedes(valdes_idx, val);
    }

    if ( valdes_idx >= 0 ) // для переменной используется valdes
    {    
        val = ( valdes[valdes_idx] < min_val || valdes[valdes_idx] > max_val ) ? def_val : valdes[valdes_idx]; // различные проверки, можно убрать
        if ( val != *param ) {
            // прилетело новое значение, сохранить
            //memcpy(param, &val, sizeof(int32_t));
            *param = val;
            sensors_param.cfgdes[cfg_idx] = val;    // в опцию пропишем новое значени
            need_save = true;                          // флаг, что надо сохранить изменени опции во флеш
        }  
    }
    return need_save;  
}

void ICACHE_FLASH_ATTR handle_params() {
    // читаем параметры из настроек (cfgdes) или получаем их через valuedes
    bool need_save = false;
    int32_t val;
    val = work_mode;
    need_save |= handle_config_param(CFG_INDEX_WORK_MODE,               VALDES_INDEX_WORK_MODE,                 &val,                 0, 0, 3);
    work_mode = val;

    val = temp_source;
    need_save |= handle_config_param(CFG_INDEX_TEMP_SOURCE,           VALDES_INDEX_TEMP_SOURCE,             &val,          0, 0,1);
    temp_source = val;

    val = thermo->period;
    need_save |= handle_config_param(CFG_INDEX_THERMO_PERIOD,           VALDES_INDEX_THERMO_PERIOD,             &val,          THERMOSTAT_PERIOD, 5, 600);
    thermo->period = val;

    val = thermo->hysteresis;
    need_save |= handle_config_param(CFG_INDEX_THERMO_HYSTERESIS,       VALDES_INDEX_THERMO_HYSTERESIS,         &val,      THERMOSTAT_HYSTERESIS, 1, 100);
    thermo->hysteresis = val;

    val = global_tempset;
    need_save |= handle_config_param(CFG_INDEX_THERMO_TEMPSET,          VALDES_INDEX_THERMO_TEMPSET,            &val,         THERMOSTAT_TEMPSET, 50, 300);
    global_tempset = val;

    #ifdef PUMP_DELAY
    val = pump_delay;
    need_save |= handle_config_param(CFG_INDEX_PUMP_DELAY,  VALDES_INDEX_PUMP_DELAY,    &val,    PUMP_DELAY, 0, 1200);
    pump_delay = val;
    #endif

    val = schedule_enabled;
    need_save |= handle_config_param(CFG_INDEX_SCHEDULE_ENABLED,  VALDES_INDEX_SCHEDULE_ENABLED,    &val,    0, 0, 2);
    schedule_enabled = val;
    
    uint8_t i;
    for ( i = 0; i < CFG_TEMPSET_SCHEDULE_COUNT; i++) {
        val = schedules_tempset[i].value;
        need_save |= handle_config_param(CFG_TEMPSET_SCHEDULE_START_IDX + i, VALDES_TEMPSET_SCHEDULE_START_IDX, &val, 0, 0, 99999999);
        schedules_tempset[i].value = val;

        schedules_tempset[i].idx = val / DIV_INDEX;   

        val = val % DIV_INDEX;         
        schedules_tempset[i].hour = (uint8_t)( val / DIV_HOUR);

        val = val % DIV_HOUR;
        schedules_tempset[i].min =  (uint8_t)( val / DIV_MINTE );

        val = val % DIV_MINTE;
        schedules_tempset[i].tempset = val;
    }

    if ( need_save ) {
        system_soft_wdt_feed();
        SAVEOPT;
    }
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
    //if ( timersrc % 5 == 0 )
    //{
        handle_params();
    //}

    if ( timersrc > 60 ) {

        // управление состоянием термостата
        if ( work_mode == 0)  {
            // термостат недоступен
            thermostat_disable(thermo_h);
        } else {
            thermostat_enable(thermo_h);

            // вычисление уставки по расписанию, если не найдено расписание или отключено, то используется глобальная уставка
            int8_t schedule_idx = get_schedule_index();

            if ( schedule_idx >= 0)
                thermostat_set_tempset(thermo_h, schedules_tempset[schedule_idx].tempset);    
            else
                thermostat_set_tempset(thermo_h, global_tempset);        
        }   

        // определение активного 
        active_kotel = set_active_kotel();
        thermostat_set_value(thermo_h, temperature);

        if ( thermo->enabled && timersrc % thermo->period == 0 )
        {
            // сработка термостата с заданным интервалом
            
            thermostat_process(thermo_h);
        }

    }


    if ( timersrc % 60 == 0 )
    {
        // выполнение кода каждую минуту

        temperature = temp_source ? TEMP_EXTERNAL : TEMP_INTERNAL;  // vsens обновляется с периодичностью 1 раз в 30 сек

        // отправить новую уставку по mqtt

    }
}

void webfunc(char *pbuf) 
{
    int8_t schedule_idx = get_schedule_index();
    uint16_t local_minutes = time_loc.hour*60 + time_loc.min;

    //********************************************************************************************
    uint8_t gpio_st = GPIO_ALL_GET(KOTEL1_GPIO);
	os_sprintf(HTTPBUFF,"<br>");
    os_sprintf(HTTPBUFF,"Mode: <br>"); 

    os_sprintf(HTTPBUFF, "<a href='#' onclick='wm(0)'><div class='g_%d k kk fll wm' id='v0'>Manual</div></a>", work_mode == 0);
    #ifdef KOTEL2_GPIO
    os_sprintf(HTTPBUFF, "<a href='#' onclick='wm(1)'><div class='g_%d k kk fll wm' id='v1'>Auto</div></a>", work_mode == 1);
    #endif
    os_sprintf(HTTPBUFF, "<a href='#' onclick='wm(2)'><div class='g_%d k kk fll wm' id='v2'>%s</div></a>", work_mode == 2, KOTEL1_NAME);
    #ifdef KOTEL2_GPIO
    os_sprintf(HTTPBUFF, "<a href='#' onclick='wm(3)'><div class='g_%d k kk fll wm' id='v3'>%s</div></a>", work_mode == 3, KOTEL2_NAME);
    #endif

    #ifdef KOTEL2_NIGHT_MODE_GPIO
    gpio_st = GPIO_ALL_GET(KOTEL2_NIGHT_MODE_GPIO);
    os_sprintf(HTTPBUFF,"<a href='?gpio=%d'><div class='g_%d k kk fll'>%s</div></a>"
                            , KOTEL2_NIGHT_MODE_GPIO
                            , gpio_st
                            , "Night (X2)");
    #endif


		os_sprintf(HTTPBUFF,"<br>");
        if ( schedule_idx == -1 ) {
            //os_sprintf(HTTPBUFF,"<div>Уставка глобальная: <b>%d.%d °C</b></div>", (uint16_t)(global_tempset / 10), global_tempset % 10); 
            os_sprintf(HTTPBUFF,"Global tempset: <b>%d.%d °C</b>", (uint16_t)(global_tempset / 10), global_tempset % 10); 
        } else {
            os_sprintf(HTTPBUFF,"Schedule tempset: <b>%d.%d °C</b>", (uint16_t)(thermo->tempset / 10), thermo->tempset % 10); 
        }

        //os_sprintf(HTTPBUFF,"<div>Гистерезис: <b>%d.%d °C</b></div>", (int16_t)(thermo->hysteresis / 10), thermo->hysteresis % 10);
        
        os_sprintf(HTTPBUFF,"<br>Temperature%s: <b>%d.%d °C</b><br>"
                           , temp_source ? " (ext)" : ""
                           , (int16_t)(thermo->value / 10)
                           , thermo->value % 10
                           );

        #ifdef PUMP_DELAY
        os_sprintf(HTTPBUFF,"Pump overrun (%d sec): <b>%s</b><br>", pump_active > 0 ? pump_active : pump_delay, pump_active > 0 ? "Yes" : "No"); 
        #endif         

        // данные расписание
        os_sprintf(HTTPBUFF,"Schedule: <br>"); 
        os_sprintf(HTTPBUFF, "<a id='ushd' href='#' data-val='%d' onclick='schd(this.dataset.val)'><div class='g_%d k kk fll' id='sch' data-text='%s'>%s</div></a><br>"
                            , !schedule_enabled
                            , schedule_enabled
                            , schedule_enabled ? "Off" : "On" //обратное значение, подставится после нажатия
                            , schedule_enabled ? "On" : "Off"
                            );
    os_sprintf(HTTPBUFF,"<small>Firmware: %s</small>", FW_VER); 

    os_sprintf(HTTPBUFF, "<script type='text/javascript'>"

                        "window.onload=function()"
                        "{"
                            "let e=document.createElement('style');"
                            "e.innerText='.blk{float:none;display:flex;padding:6px 0px;}"
                                                ".flr{float:right;}"
//                                                "#tshd{display:table;}"
//                                                ".t1{width:60%%;}"
//                                                ".t2{width:40%%;}"
//                                                ".sr{font-weight:bold;color:red;}"
                                                ".kk{border-radius:4px;margin:-20px 2px 0px 4px;width:60px;float:right;}"
                                                "';"
                            "document.head.appendChild(e)"
                        "};"

                        "function wm(t)"
                        "{"
                            "ajax_request('/valdes?int=%d'+'&set='+t,"
                                "function(res)"
                                "{"
                                    "let v=document.getElementsByClassName('wm');"
                                    "for(let i=0;i<v.length;i++){v[i].classList.remove('g_1');v[i].classList.add('g_0')}"
                                    "document.getElementById('v'+t).classList.add('g_1')"
                                "}"
                            ")"
                        "};"

                        "function schd(t)"
                        "{"
                            "ajax_request("
                                "'/valdes?int=%d'+'&set='+t,"
                                "function(res)"
                                    "{"
                                        "var n=1-parseInt(t);"
                                        "var sc=document.getElementById('sch');"
                                        "sc.classList.remove('g_'+n);"
                                        "sc.classList.add('g_'+t);"
                                        "sc.innerHTML=sc.getAttribute('data-text');"
//                                        "document.getElementById('tshd').style.display=(t)?'table':'none';"
                                        "document.getElementById('ushd').setAttribute('data-val',n);"
                                    "}"
                            ")"
                        "}"
                        "</script>"
                        , VALDES_INDEX_WORK_MODE
                        , VALDES_INDEX_SCHEDULE_ENABLED
                        );

                         // class=c2
                         //<div class="h" style="background: #73c140">GPIO:</div>
}