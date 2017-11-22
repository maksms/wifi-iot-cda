
Команды через GET запросы.
/configtermo&full=1?rmin0=16&rmax0=1&gpio0=14&ln0=31&full=1&st=1 // термостат на 14 пине, уставка 10, гистерезис 1.
/configall?sh30t=on&st=1&intv=2            // включаем sht30, интервал чтения 2 сек.
/configall?bhen=on&sh30t=on&st=1&intv=2    // то же, но с BH1750
/configall?sda=1&scl=3&st=5                // включаем i2c на 1-3 пинах.
/configpio?gpio=12&md=3&st=1               // gpio12 - output
/configpio?gpio=13&md=131&st=1             // gpio13 - output invert
/configpio?gpio=14&md=3&st=1               // gpio14 - output
/configdes?cfg1=16&cfg2=21&cfg3=1000&st=3  // des code option для КК. Начало периода включения света, выключения, уровень освещенности по BH1750, при котором включается свет.
 
 
*****************************************************************************************
 Прямые ссылки на страницы конфигурации:
 /configall               // настройка Hardware
 /configpio               // gpio
 /configupd?st=5          // normal OTA
 /configtermo&full=1       // полная версия термостата
 

*****************************************************************************************
команды mqtt через api.
{"gpiomode":{"0":"","1":"","2":"","3":"","4":"","5":"","12":"out","13":"outinv","14":"out","15":"","16":""}}
{"hardware":{"interval":2,"i2c":{"scl":3,"sda":1},"bh1750":{"en":0}}}
{"servers":{"narodmon":{"en":0,"interval":5},"flymon":{"en":1},"mqtt":{"en":1,"interval":10,"server":"mqtt.wifi-iot.com","port":1883,"login":"mailumka2@gmail.com","passw":"8vfhnf"}}}
{"thermostat":{"termo1":{"en":1,"sensor":31,"min":10,"max":11,"gpio":14}}}


******************************************************************************************
Команды термостата для Народмона
settermo1=20
entermo1=1

*******************************************************************************************
