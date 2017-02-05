//заранее вбить имена роутеров и пороли в 19,20,26,27 строке,после 500реконектов поднимается точка доступа с хостнейм модуля и паролем 87654321
//при сборке ввести переменных 0, а в поле настройки 0
uint8_t len_ssi;//длина названия wi-fi
soft_ap_mode(){//функция в которой спрятан вызов точки доступа
    struct softap_config apConfig;
    wifi_softap_get_config(&apConfig);
    os_sprintf(apConfig.ssid,"%s",sensors_param.hostname);//имя точки доступа берем хостнэйм
    apConfig.authmode=4;//0 - без шифрования,4 с шифрованием 
    os_sprintf(apConfig.password,"87654321");//пароль точки 
    len_ssi=apConfig.ssid_len =strlen(apConfig.ssid);//длина имени точки доступа
    wifi_softap_set_config(&apConfig);//применить параметры точки
    wifi_set_opmode(3);//включить точку доступа
}
alt_ssid_mode1(){    
    struct station_config stationConf;  
    wifi_station_get_config(&stationConf);
    os_sprintf(stationConf.ssid,"ROUTER1");//имя роутера 1  
    os_sprintf(stationConf.password,"PASSWORD");//пароль роутера 1 
    wifi_station_set_config(&stationConf);;//применить параметры роутера 1
}   
alt_ssid_mode2(){    
    struct station_config stationConf;  
    wifi_station_get_config(&stationConf);
    os_sprintf(stationConf.ssid,"PASSWORD");//имя роутера 2
    os_sprintf(stationConf.password,"petrovich");//пароль роутера 2 
    wifi_station_set_config(&stationConf);;//применить параметры роутера 2
}
void ICACHE_FLASH_ATTR startfunc(){}
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    if(!(wfrc%50))if(!(wfrc%100))alt_ssid_mode1();else alt_ssid_mode2();
    if(wfrc==500){soft_ap_mode();}//если реконектов больше 500 вызываем функцию с точкой
}
void webfunc(char *pbuf) {
    os_sprintf(HTTPBUFF,"<br>обрывы связи : %d ",wfrc);//вывод на главную реконнектов
}