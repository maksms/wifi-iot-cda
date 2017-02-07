//при сборке добавить 9 настроек ,переменных 0 
//при отрыве  и других проблем с датчиками,работает по сохраненной температуре,ограничил время такой работы одной минутой.кнопкой принять сохраняются настройки,кнопка режим.режим работы гпио- постоянно выкл,постоянно вкл,,охладитель.термостат.настройки на главной модуля
//отличается от первого только более компактной записью кода
int32_t tempSave=1;
int32_t tempVhod;
uint8_t vremia_oshibki=0;
bool time=0;
uint8_t i;
uint8_t gpio=16; //номер гпио выход
int16_t cfg[]= {-600,600,230,-600,600,240,0,23,8,00,59,0,-600,600,260,-600,600,275,0,23,20,0,59,30,0,3,2};
void ICACHE_FLASH_ATTR startfunc() {
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t timersrc) { 
    tempVhod = data1wire[0] ;//датчик температуры dsw1
    for(i=0;i<9;i++){
        if ( sensors_param.cfgdes[i] < cfg[i*3] || cfg[(i*3)+1] < sensors_param.cfgdes[i] ) sensors_param.cfgdes[i]	= cfg[(i*3)+2];   
    }
    if ( ( tempVhod != 0 && tempVhod != 850 && tempVhod != 2550 ) || ( 60 <= vremia_oshibki ) ) { //если датчик оборвало работаем минуту по сохраненному значению,если он не восстановится ,тогда уже станем в ошибкy
        tempSave = tempVhod ; 
        vremia_oshibki = 0 ;
    } else {
	vremia_oshibki++ ;
    }   
    time = ( sensors_param.cfgdes[2] < sensors_param.cfgdes[6] )?( ( time_loc.hour == sensors_param.cfgdes[2] && sensors_param.cfgdes[3] <= time_loc.min ) || ( sensors_param.cfgdes[2] < time_loc.hour && time_loc.hour <sensors_param.cfgdes[6] ) || ( time_loc.hour == sensors_param.cfgdes[6 ] && time_loc.min < sensors_param.cfgdes[7] ) ) ?1:0:( ( time_loc.hour == sensors_param.cfgdes[2] && sensors_param.cfgdes[3] <= time_loc.min ) || ( sensors_param.cfgdes[2] < time_loc.hour && time_loc.hour <24 ) || ( 0 <= time_loc.hour && time_loc.hour < sensors_param.cfgdes[6] ) || ( time_loc.hour == sensors_param.cfgdes[6] && time_loc.min < sensors_param.cfgdes[7] ) )?1:0;
    if ( sensors_param.cfgdes[8] == 1 || sensors_param.cfgdes[8] == 2){   
        if ( sensors_param.cfgdes[5-time*4] < tempSave ) 
	    digitalWrite(gpio,(((sensors_param.cfgdes[8])-2)*(-1)));
        if ( tempSave < sensors_param.cfgdes[4-time*4] ) 
	    digitalWrite(gpio,(sensors_param.cfgdes[8])-1);
    } else {
	digitalWrite(gpio,(sensors_param.cfgdes[8])/3); 
    }
}	
void webfunc(char *pbuf) {
    os_sprintf(HTTPBUFF,"</div><br><div class='h' style='background: #73c140 '> ТЕРМОСТАТ : <font color=%s",( sensors_param.cfgdes[8] ==1 && GPIO_ALL_GET(gpio) )?"'blue'>включен</font> . &nbsp;&nbsp;":( sensors_param.cfgdes[8] ==2 && GPIO_ALL_GET(gpio) )?"'yellow'>включен</font> . &nbsp;&nbsp;":( sensors_param.cfgdes[8] ==3 )?"'red'>включен</font> . &nbsp;&nbsp;":"'black'>выключен</font> . &nbsp;");
    os_sprintf(HTTPBUFF," Режим :<b><font color=%s",(sensors_param.cfgdes[8] ==1)?"'blue'> охлаждение</font></b>":(sensors_param.cfgdes[8] ==2)?"'yellow'> нагреватель</font></b>":(sensors_param.cfgdes[8] ==3)?"'red'> включен постоянно</font></b>":"'black'> выключен постоянно</font></b>");
    os_sprintf(HTTPBUFF,"</div><div class='c'><div class='main'><form action=configdes><pre><table border='0'class='catalogue'><tr style='background-color: yellow'><td> часы </td><td> минуты </td>%s</tr><tr><td>c <INPUT size=2 NAME='cfg3'value='%02d'></td><td> <INPUT size=2 NAME='cfg4'value='%02d'></td><td> ниже <INPUT size=2 NAME='cfg1' value='%s'> °C</td>",(sensors_param.cfgdes[8] ==1)?"<td> темп выкл</td><td> темп вкл </td>":"<td> темп вкл </td><td> темп выкл</td>",sensors_param.cfgdes[2],sensors_param.cfgdes[3], fltostr(sensors_param.cfgdes[0]));
    os_sprintf(HTTPBUFF+os_strlen(HTTPBUFF),"<td> выше <INPUT size=2 NAME='cfg2'value='%s'> °C</td></tr>",fltostr(sensors_param.cfgdes[1]));
    os_sprintf(HTTPBUFF+os_strlen(HTTPBUFF),"<tr><td>c <INPUT size=2 NAME='cfg7'value='%02d'></td><td> <INPUT size=2 NAME='cfg8'value='%02d'></td><td> ниже <INPUT size=2 NAME='cfg5' value='%s'> °C</td>",sensors_param.cfgdes[6],sensors_param.cfgdes[7], fltostr(sensors_param.cfgdes[4]));
    os_sprintf(HTTPBUFF+os_strlen(HTTPBUFF),"<td> выше <INPUT size=2 NAME='cfg6'value='%s'> °C</td></tr></table><br><input type='hidden'name='cfg9' value='%d'><input type='hidden'name='st'value=3><input type=submit value='принять'onclick='pb(cfg1);pb(cfg2);pb(cfg5);pb(cfg6)'> <input type=submit value='режим'onclick='pb(cfg1);pb(cfg2);pb(cfg5);pb(cfg6);eb(cfg9)'></pre></form></div><script>function pb(x){x.value = x.value*10};function eb(x){x.value =Number(x.value)+ 1}</script>",fltostr(sensors_param.cfgdes[5]),sensors_param.cfgdes[8]);
}
