// системные определения КК:
typedef enum {
    LS_MODE_NA =0, // неизвестный датчик, только положительные (uint16_t, uint32_t)
    LS_MODE_TEMP=3, // температура
    LS_MODE_HUM=4, // влажность
    LS_MODE_PRESSURE=5, // давление
    LS_MODE_CNT=6, // счетчики
    LS_MODE_LIGHT=7, // light LUX
    LS_MODE_PPMCO2=8, // со2
    LS_MODE_VOLT=9, // напряжение
    LS_MODE_CURRENT=10, // ток
    LS_MODE_WATT=11, // ватт
    LS_MODE_WATTH=12, //квт*ч
    LS_MODE_ERROR=13, // ошибка датчика
    LS_MODE_NANEG=14, // неизвестный датчик c отрицательными значениями (int16_t, int32_t)
    LS_MODE_HIDDEN=15, //пустая метрика, скрываем её везде.
    LS_MODE_RSSI=16, // уровень сигнала
    LS_MODE_BAT=17, // заряд батареи %
    LS_MODE_WEIGHT=18, // weight вес
    LS_MODE_ANGLE=19, //angle угол
    LS_MODE_FERT=20, // Fertility Плодородие
    LS_MODE_DIST=21,// distance расстояние
    LS_MODE_RADIATION=22,// радиация
    LS_MODE_FR=24,//  frequency частота
    LS_MODE_PM10=25,//  мкг/м3 PMS1.0
    LS_MODE_PM25=26,//  мкг/м3 PMS2.5
    LS_MODE_PM100=27,//  мкг/м3 PMS10
    // 28-31 = 4 свободны
  
    LSENSFUNS=256, // значит вызываем функцию !!!
    LSENS32BIT=32 // наша переменная 32 бита !!!
} lsensmode_t;

typedef enum {
    LSENSFL0= 0,
    LSENSFL1= 64,
    LSENSFL2= 128,
    LSENSFL3= 192
} lsensflmode;


/*
Пример 32 битного счетчика:
{200,LS_MODE_CNT|LSENS32BIT,"MYCOUNTER","counter",&mycounter,NULL},

Пример вывода температуры с 2 знаками:
{201,LS_MODE_TEMP|LSENSFL2,"MYTEMP","temp",&mytemp,NULL},
*/
