#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#define FW_VER "2.0"
/*
0    1                    2          3              4           5                   6          7      8        9      10       11     12       13     14       15    
Авто,Источник температуры,Период/сек,Гистерезис/x10,Уставка/x10,Задержка насоса/сек,Расписание,ЧЧММ-1,Уст1/x10,ЧЧММ-2,Уст2/x10,ЧЧММ-3,Уст3/x10,ЧЧММ-4,Уст4/x10,ЧЧММ-5,Уст5/x10
используется 2 термостата
cfg0 (valdes0)   режим работы управления котлами:
    0 - ручной, термостаты не включены, можно кнопками вкл/выкл gpio котлов
    1 - авто, используются термостаты в автоматическом режиме, можно как с глобальной уставкой, так и по расписанию
    2 - активный котел 1, используется термостат
    3 - активный котел 2, используется термостат

cfg1    источник температуры
        0 - внутренние расчеты через функцию get_temp
        1 - внешние расчеты и прием через valdes4 из внешней системы

cfg2    период срабатывания термостата в секундах
cfg3    гистерезис х10 (valdes1)
cfg4 (valdes2)   глобальная устав, принимаем из внешней системы

cfg5    задержка выключения насоса котла2, сек,      котел 1 не должен включаться столько времени, после выключения котла 2
cfg6    расписание (0 - используется, 1 - не используется) valdes3

!!!!! в функции get_temp() можно описать свою логику вычисления температуры.
      можно указать просто значение датчика data1wire[X], или dht_tX, или другой датчик
      или вообще брать среднюю температуру с нескольких датчиков
      или брать минимальную температуру с нескольких датчиков

!!!!! для расписания доступно 5 временных интервалов, их можно увеличить (до 7) или уменьшить через CFG_TEMPSET_SCHEDULE_COUNT
!!!!! если используете расписание, то необходимо заполнять все временные интервалы и уставки
!!!!! заполнение всех временных интервалов должно идти последовательно по времени от меньшего к большему
!!!!! температура уставки так же должна быть заполнена
!!!!! если вам интервалов много, сократите их
cfg7	часы-минуты Т1
cfg8	уставка для Т1
	
cfg9	часы минуты Т2
cfg10	уставка для Т2
	
cfg11	часы минуты Т3
cfg12	уставка для Т3
	
cfg13	часы минуты Т4
cfg14	уставка для Т4
	
cfg15	часы минуты Т5
cfg16	уставка для Т5


---------------------
valdes0 - режим работы
valdes1 - гистерезис
valdes2 - уставка
valdes3 - расписание
valdes4 - внешняя температура

*/


#define GPIO_ON 1
#define GPIO_OFF 0

#define KOTEL_NONE_NAME "--------"
#define KOTEL1_GPIO 12
//#define KOTEL1_NAME "дизельный"
#define KOTEL1_NAME "Котел1"

// !!!! если не нужен котел 2, просто закоментируйте строки
#define KOTEL2_GPIO 14 
//#define KOTEL2_NAME "электрический"
#define KOTEL2_NAME "Котел2"
#define KOTEL2_NIGHT_MODE_GPIO 16  // ESC режим для Протерма

#define NIGHT_MODE_START_TIME 23
#define NIGHT_MODE_END_TIME 7

#define THERMOSTAT_PERIOD 40
#define THERMOSTAT_TEMPSET 250
#define THERMOSTAT_HYSTERESIS 5

#define PUMP_DELAY 300
#define KOTEL_2_MIN_TEMP -100 // -10  x10
#define KOTEL_1_MAX_TEMP  50 // 5  x10

#define HIDE_GPIO_BLOCK 

typedef enum {
    KOTEL_NONE,
    KOTEL_1,
    KOTEL_2
} kotel_type_t;

kotel_type_t active_kotel = 0; // 0 - нет активного котла, 1 - дизельный, 2 - электрический

#define CFG_INDEX_WORK_MODE 0
#define CFG_INDEX_TEMP_SOURCE 1
#define CFG_INDEX_THERMO_PERIOD 2
#define CFG_INDEX_THERMO_HYSTERESIS 3
#define CFG_INDEX_THERMO_TEMPSET 4
#define CFG_INDEX_PUMP_DELAY 5
#define CFG_INDEX_SCHEDULE_ENABLED 6
#define CFG_TEMPSET_SCHEDULE_START_IDX 7

#define VALDES_INDEX_WORK_MODE 0  
#define VALDES_INDEX_TEMP_SOURCE -1
#define VALDES_INDEX_THERMO_PERIOD -1 
#define VALDES_INDEX_THERMO_HYSTERESIS 1 
#define VALDES_INDEX_THERMO_TEMPSET 2
#define VALDES_INDEX_PUMP_DELAY -1
#define VALDES_INDEX_SCHEDULE_ENABLED 3
#define VALDES_TEMPSET_SCHEDULE_START_IDX -1

#define VALDES_INDEX_TEMP_EXTERNAL 4

#define CFG_TEMPSET_SCHEDULE_COUNT 5

uint8_t work_mode = 0; // режим работы (cfg0)

#ifdef PUMP_DELAY
uint16_t pump_delay = 300;
uint16_t pump_active = 0;
os_timer_t pump_timer;
#endif

uint8_t kotel_gpio = 255;
uint16_t global_tempset = THERMOSTAT_TEMPSET;
uint8_t temp_source = 0;

int16_t kotel_2_min_temp;
int16_t kotel_1_max_temp;

typedef struct schedules_tempset {
    int32_t hhmm;
    uint8_t hour;
    uint8_t min;  
    uint16_t tempset;   // x10
} schedules_tempset_t;

schedules_tempset_t schedules_tempset[CFG_TEMPSET_SCHEDULE_COUNT];
uint8_t schedule_enabled = 0;

#define TEMP_DELTA 10
#define TEMP_DELTA_ATTEMPT 180

int16_t temperature = 0;
int16_t temp_prev = 2550;

#define TEMP_FRAME_SIZE 5
int16_t temp1_arr[TEMP_FRAME_SIZE] = { 850, 850, 850, 850, 850 };
int16_t temp2_arr[TEMP_FRAME_SIZE] = { 850, 850, 850, 850, 850 };
int16_t temp3_arr[TEMP_FRAME_SIZE] = { 850, 850, 850, 850, 850 };

void register_temp(int16_t *temp_arr, int16_t temp)
{
    uint8_t i;
    for (i=1;i<TEMP_FRAME_SIZE;i++)
    {
        temp_arr[i-1] = temp_arr[i];
    }
    temp_arr[TEMP_FRAME_SIZE-1] = temp;
}


int16_t get_median_temp(int16_t *temp_arr, int16_t temp)
{
    register_temp(temp_arr, temp);

    // sort
    int16_t arr[TEMP_FRAME_SIZE];
    memcpy(&arr, temp_arr, sizeof(int16_t) * TEMP_FRAME_SIZE);

	uint8_t i, j = 0;
	for ( i = 0; i < TEMP_FRAME_SIZE; i++) {
		for ( j = i + 1; j < TEMP_FRAME_SIZE; j++) {
			if (arr[i] < arr[j]) {
				int16_t t = arr[i];
				arr[i] = arr[j];
				arr[j] = t;
			}
		}
	}
    return arr[ (uint8_t)(TEMP_FRAME_SIZE / 2)];
}

int16_t ICACHE_FLASH_ATTR get_temp() 
{
    // срабатывает раз в секунду
    int16_t _temp, _temp2, _temp3 = 240; // взять с датчика, или из valdes, или расчитать среднее или минимальное по датчикам
    static int16_t prev = 0;
    static uint8_t cnt = 0;
    if ( temp_source == 0) {
        _temp =  get_median_temp( &temp1_arr[0], vsens[0][0]);
        _temp2 =  get_median_temp( &temp2_arr[0], vsens[1][0]);
        _temp3 =  get_median_temp( &temp3_arr[0], vsens[2][0]);

        //  _temp =  vsens[0][0];
        //  _temp2 =  vsens[1][0];
        //  _temp3 =  vsens[2][0];

        //******* работаем по минимальной  температуре
        _temp = ( _temp < _temp2 ) ? _temp : _temp2;
        _temp = ( _temp < _temp3 ) ? _temp : _temp3;
        //****************************************
    } else {
        _temp = valdes[VALDES_INDEX_TEMP_EXTERNAL];
    }

    // if ( prev == 0 ) prev = _temp;

    // if ( _temp == 0 || _temp == 850 || _temp == 2550 // глюки и отвалы датчик ds18b20
    //      || _temp - prev > TEMP_DELTA  // текущая больше предыдущей на дельту
    //      || prev - _temp > TEMP_DELTA  // предыдущая  больше текущей на дельту
    //     )
    // {
    //     cnt++;          // счетчик отклонений температуры, если отклонение было более 3 мин (180 сек), значит было открыто окно или дверь
    //                     // и это не глюк датчика, при глюке датчика температура обычно скачет вверх/вниз и возвращается обратно
    //     if ( cnt > TEMP_DELTA_ATTEMPT ) {
    //         prev = _temp;
    //         cnt = 0;
    //     } else {
    //         _temp = prev;  // оставляем предыдущую
    //     }
    // } else {
    //     prev = _temp;
    //     cnt = 0;
    // }
    return _temp;
}

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
        val = schedules_tempset[i].hhmm;
        need_save |= handle_config_param(CFG_TEMPSET_SCHEDULE_START_IDX + i*2,     VALDES_TEMPSET_SCHEDULE_START_IDX, &val, 0, 0, 2400);
        schedules_tempset[i].hhmm = val;

        val = schedules_tempset[i].tempset;
        need_save |= handle_config_param(CFG_TEMPSET_SCHEDULE_START_IDX + 1 + i*2, VALDES_TEMPSET_SCHEDULE_START_IDX, &val, 0, 50, 300);
        schedules_tempset[i].tempset = val;

        schedules_tempset[i].hour = (uint8_t)( schedules_tempset[i].hhmm / 100);
        schedules_tempset[i].min =  (uint8_t)( schedules_tempset[i].hhmm % 100);
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
        //temperature = get_temp(); // каждую секунду слишком часто, не срабатывает фильтр, т.к. vsens каждые 60 сек приходит
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

        temperature = get_temp();  // vsens обновляется с периодичностью 1 раз в минуту, если верить документации

        // отправить новую уставку по mqtt

    }
}

void webfunc(char *pbuf) 
{
    int8_t schedule_idx = get_schedule_index();
    uint16_t local_minutes = time_loc.hour*60 + time_loc.min;

    //********************************************************************************************
    os_sprintf(HTTPBUFF,"<div class='blk'>");
    os_sprintf(HTTPBUFF,"<div class='fll'>Активный котел:</div>");
    /*
    if ( active_kotel == KOTEL_1 )
        os_sprintf(HTTPBUFF, KOTEL1_NAME);
    else if ( active_kotel == KOTEL_2 )
        os_sprintf(HTTPBUFF, KOTEL2_NAME);
    else 
        os_sprintf(HTTPBUFF, KOTEL_NONE_NAME);
    */
    os_sprintf(HTTPBUFF, "<div class='flr'>");
    uint8_t gpio_st = GPIO_ALL_GET(KOTEL1_GPIO);
    os_sprintf(HTTPBUFF,"<a href='?gpio=%d'><div class='g_%d k kk fll'>%s</div></a>"
                            , KOTEL1_GPIO
                            , gpio_st
                            , KOTEL1_NAME);

    #ifdef KOTEL2_GPIO
    gpio_st = GPIO_ALL_GET(KOTEL2_GPIO);
    os_sprintf(HTTPBUFF,"<a href='?gpio=%d'><div class='g_%d k kk fll'>%s</div></a>"
                            , KOTEL2_GPIO
                            , gpio_st
                            , KOTEL2_NAME);
    #endif

    os_sprintf(HTTPBUFF, "</div>");  // <div class='flr'>
    os_sprintf(HTTPBUFF, "</div>");  // <div class='blk'>
    //********************************************************************************************
    os_sprintf(HTTPBUFF,"<div class='blk'>");
    os_sprintf(HTTPBUFF,"<div class='fll'>Режим: </div>"); 
    /*
    if (work_mode == 0)
        os_sprintf(HTTPBUFF,"Ручной"); 
    else if ( work_mode == 1)
        os_sprintf(HTTPBUFF,"Авто"); 
    else if ( work_mode == 2)
        os_sprintf(HTTPBUFF,"всегда %s", KOTEL1_NAME); 
    else if ( work_mode == 3)
        os_sprintf(HTTPBUFF,"всегда %s", KOTEL2_NAME); 
    else
        os_sprintf(HTTPBUFF,"<br>Режим: -------"); 
    */
    os_sprintf(HTTPBUFF, "<div class='flr'>");
    os_sprintf(HTTPBUFF, "<a href='#' onclick='wmode(0,0)'><div class='g_%d k kk fll v00' id='v00'>Ручной</div></a>", work_mode == 0);
    #ifdef KOTEL2_GPIO
    os_sprintf(HTTPBUFF, "<a href='#' onclick='wmode(0,1)'><div class='g_%d k kk fll v00' id='v01'>Авто</div></a>", work_mode == 1);
    #endif
    os_sprintf(HTTPBUFF, "<a href='#' onclick='wmode(0,2)'><div class='g_%d k kk fll v00' id='v02'>%s</div></a>", work_mode == 2, KOTEL1_NAME);
    #ifdef KOTEL2_GPIO
    os_sprintf(HTTPBUFF, "<a href='#' onclick='wmode(0,3)'><div class='g_%d k kk fll v00' id='v03'>%s</div></a>", work_mode == 3, KOTEL2_NAME);
    #endif

    #ifdef KOTEL2_NIGHT_MODE_GPIO
    gpio_st = GPIO_ALL_GET(KOTEL2_NIGHT_MODE_GPIO);
    os_sprintf(HTTPBUFF,"<a href='?gpio=%d'><div class='g_%d k kk fll'>%s</div></a>"
                            , KOTEL2_NIGHT_MODE_GPIO
                            , gpio_st
                            , "Ночной режим");
    #endif

    os_sprintf(HTTPBUFF, "</div>");  // <div class='flr'>
    os_sprintf(HTTPBUFF, "</div>");  // <div class='blk'>
    //********************************************************************************************
    os_sprintf(HTTPBUFF,"<div class='blk'>");
    os_sprintf(HTTPBUFF,"<div class='fll t1'>");
        // данные термостата
        os_sprintf(HTTPBUFF,"<div>Термостат: <b>%s</b></div>", thermo->enabled ? "Вкл" : "Выкл"); 
        os_sprintf(HTTPBUFF,"<div>Состояние: <b>%s</b></div>", thermo->state ? "Нагрев" : "Ожидание"); 

        if ( schedule_idx == -1 ) {
            os_sprintf(HTTPBUFF,"<div>Уставка глобальная: <b>%d.%d °C</b></div>", (uint16_t)(global_tempset / 10), global_tempset % 10); 
        } else {
            os_sprintf(HTTPBUFF,"<div>Уставка расписание: <b>%d.%d °C</b></div>", (uint16_t)(thermo->tempset / 10), thermo->tempset % 10); 
        }

        os_sprintf(HTTPBUFF,"<div>Гистерезис: <b>%d.%d °C</b></div>", (int16_t)(thermo->hysteresis / 10), thermo->hysteresis % 10);
        
        os_sprintf(HTTPBUFF,"<div>Температура%s: <b>%d.%d °C</b></div>"
                           , temp_source ? " (ext)" : ""
                           , (int16_t)(thermo->value / 10)
                           , thermo->value % 10
                           );

        #ifdef PUMP_DELAY
        os_sprintf(HTTPBUFF,"<div>");
        os_sprintf(HTTPBUFF,"<span>Выбег насоса (%d сек): <b>%s</b></span>", pump_active > 0 ? pump_active : pump_delay, pump_active > 0 ? "Да" : "Нет"); 
        os_sprintf(HTTPBUFF, "</div>");  // <div class='blk'>
        #endif         

    os_sprintf(HTTPBUFF, "</div>");  // <div class='fll t1'> 

    os_sprintf(HTTPBUFF,"<div class='flr t2'>");
        // данные расписание

        os_sprintf(HTTPBUFF,"<div class='blk'>");
        os_sprintf(HTTPBUFF,"<div class='fll'>Расписание: "); 
        //os_sprintf(HTTPBUFF,"%s</p>", schedule_enabled ? "Вкл" : "Выкл"); 
        os_sprintf(HTTPBUFF,"</div>"); 
        os_sprintf(HTTPBUFF,"<div class='fll'>"); 
        os_sprintf(HTTPBUFF, "<a id='ushd' href='#' data-val='%d' onclick='schd(%d,this.dataset.val)'><div class='g_%d k kk fll' id='sched' data-text='%s'>%s</div></a>"
                            , !schedule_enabled
                            , VALDES_INDEX_SCHEDULE_ENABLED
                            , schedule_enabled
                            , schedule_enabled ? "Выкл" : "Вкл" //обратное значение, подставится после нажатия
                            , schedule_enabled ? "Вкл" : "Выкл"
                            );
        os_sprintf(HTTPBUFF,"</div>");  // <div class='fll'>
        os_sprintf(HTTPBUFF,"</div>");  // <div class='blk'>
        


        //if ( schedule_enabled ) {
            os_sprintf(HTTPBUFF,"<table id='tshd'>"); 
            os_sprintf(HTTPBUFF,"<tr><th>#</th><th>Время</th><th>Уставка</th></tr>"); 
            uint8_t i;
            for (i=0; i < CFG_TEMPSET_SCHEDULE_COUNT; i++)
            {
            
                uint16_t schedule_minutes = schedules_tempset[i].hour*60 + schedules_tempset[i].min;
                uint16_t schedule_minutes_next = 23*60+59;
                uint16_t schedule_minutes_prev = 0;
                if ( i > 0 ) schedule_minutes_prev = schedules_tempset[i-1].hour*60 + schedules_tempset[i-1].min;
                if ( i <= CFG_TEMPSET_SCHEDULE_COUNT-2 ) schedule_minutes_next = schedules_tempset[i+1].hour*60 + schedules_tempset[i+1].min;


                os_sprintf(HTTPBUFF,"<tr %s><td>%d.%s</td><td>&nbsp;с %02d:%02d</td><td>&nbsp;%d.%d °C</td></tr>" //<td>%d -> %d <- %d</td></tr>"
                        , schedule_idx == i  ? "class='sr'" : ""
                        , i+1
                        , schedule_idx == i  ? "&nbsp;&#10148;" : ""
                        , schedules_tempset[i].hour
                        , schedules_tempset[i].min
                        , (uint16_t)(schedules_tempset[i].tempset / 10)
                        , schedules_tempset[i].tempset % 10 
                        //, schedule_idx == i ? "активно" : ""
                        //, schedule_minutes
                        //, local_minutes
                        //, schedule_minutes_next
                        ); 
            }

            os_sprintf(HTTPBUFF,"</table>"); 
        //}

    os_sprintf(HTTPBUFF, "</div>");  // <div class='flr t2'>
    os_sprintf(HTTPBUFF, "</div>");  // <div class='blk'>
    //********************************************************************************************   
    

    //********************************************************************************************
    os_sprintf(HTTPBUFF,"<div class='flr'><small>Прошивка: %s</small></div>", FW_VER); 

    os_sprintf(HTTPBUFF, "<script type='text/javascript'>"

                        "window.onload=function()"
                        "{"
                            "let stl=document.createElement('style');"
                            "stl.innerText='.blk{float:none;display:flex;padding:6px 0px;}"
                                                ".flr{float:right;}"
                                                "#tshd{display:table;}"
                                                ".t1{width:60%%;}"
                                                ".t2{width:40%%;}"
                                                ".sr{font-weight:bold;color:red;}"
                                                ".kk{border-radius:4px;margin-left:4px;width:60px;}"
                                                "';"
                            "document.head.appendChild(stl)"
                        "};"

                        "function wmode(idx,value)"
                        "{"
                            "ajax_request('/valdes?int='+idx+'&set='+value,"
                                "function(res)"
                                "{"
                                    "let vals=document.getElementsByClassName('v0'+idx);"
                                    "for(let i=0;i<vals.length;i++){vals[i].classList.remove('g_1');vals[i].classList.add('g_0')}"
                                    "document.getElementById('v0'+value).classList.add('g_1')"
                                "}"
                            ")"
                        "};"

                        "function schd(idx,value)"
                        "{"
                            "ajax_request("
                                "'/valdes?int='+idx+'&set='+value,"
                                "function(res)"
                                    "{"
                                        "var vnew=1-parseInt(value);"
                                        "var sc=document.getElementById('sched');"
                                        "sc.classList.remove('g_'+vnew);"
                                        "sc.classList.add('g_'+value);"
                                        "sc.innerHTML=sc.getAttribute('data-text');"
                                        "document.getElementById('tshd').style.display=(value)?'table':'none';"
                                        "document.getElementById('ushd').setAttribute('data-val',vnew);"
                                    "}"
                            ")"
                        "}"
                        "</script>"
                        );

                         // class=c2
                         //<div class="h" style="background: #73c140">GPIO:</div>
}