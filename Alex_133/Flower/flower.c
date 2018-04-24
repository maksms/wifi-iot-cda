// Проект "Цветок" для конструктора кода ESP32
// Настройки: [0]:Сухая земля, [1]:Яркость низкая, [2]:Яркость высокая,
// [3]:Время полива, [4]:Зелёный вкл., [5]:Насос вкл., [6]:PWM вкл.,
// [7]:Инверт. PWM, [8]:Время вкл.часы, [9]:Время вкл. мин.,
// [10]:Время выкл. часы, [11]:Время выкл. мин., [12]:Задержка после полива мин.
// [13]:Ошибка влажности
// Глобальные переменные: 7 шт.
// valdes[0] - предыдущее задание ШИМ
// valdes[1] - задание ШИМ в %
// valdes[2] - разрешение работать по времени
// valdes[3] - текущее задание ШИМ
// valdes[4] - минут после последнего полива
// valdes[5] - комманда от кнопок с web-страницы
// valdes[6] - принудительное включение подсветки по времени

void startfunc(){  // выполняется один раз при старте модуля.
  //sensors_param.cfgdes[12]=sensors_param.cfgdes[12]+1;SAVEOPT
  valdes[2] = 0; // разрешение работать подсветке по времени
  valdes[4] = 0; // минут после последнего полива
  valdes[5] = 0; // переменная нажатой кнопки
  // гасим подсветку
  if(sensors_param.cfgdes[7]){
    for(int8_t i=0; i<3; i++){
      analogWrite(i,4095);
      delay(2);
    }
  }
  else{
    for(int8_t i=0; i<3; i++){
      analogWrite(i,0);
      delay(2);
    }
  }
  valdes[3]=0;   // задание для подсветки
  valdes[0]=0;   // текущая яркость позсветки
  valdes[1]=0;   // яркость подсветки в процентах
  valdes[6]=0;   // принудительное включение подсветки по времени
}

void timerfunc(uint32_t  timersrc) {// раз 1 секунду
  if(valdes[5]){                    // обработка нажатия кнопок на web-странице
    int32_t buf = valdes[5]/100000000;
    switch (buf) {
      case 1:
        sensors_param.cfgdes[6]=valdes[5]%100000000;SAVEOPT
      break;
      case 2:
        sensors_param.cfgdes[5]=valdes[5]%100000000;SAVEOPT
      break;
      case 3:
        sensors_param.cfgdes[4]=valdes[5]%100000000;SAVEOPT
      break;
      case 4:
        sensors_param.cfgdes[8]=(valdes[5]%100000000)/1000000;
        sensors_param.cfgdes[9]=(valdes[5]%1000000)/10000;
        sensors_param.cfgdes[10]=(valdes[5]%10000)/100;
        sensors_param.cfgdes[11]=valdes[5]%100;
        SAVEOPT
      break;
      case 5:
        if(digitalRead(22)){ // при воде в бачке
          valdes[4]=0;
          digitalWrite(19,1);
          delay(sensors_param.cfgdes[3]);
          digitalWrite(19,0);
        }
      break;
      case 6:
        valdes[6]=valdes[5]%100000000;
      break;
    }
    valdes[5]=0;
  }
  if(timersrc%60==10){   // раз в минуту
    static int32_t sum_en=0;
    valdes[4]++;
    int32_t time_start = 0;
    int32_t time_real = 0;
    int32_t time_stop = 0;
    time_start = sensors_param.cfgdes[8] * 60 + sensors_param.cfgdes[9];
    time_real = time_loc.hour * 60 + time_loc.min;
    time_stop = sensors_param.cfgdes[10] * 60 + sensors_param.cfgdes[11];
    if(time_start < time_real && time_real < time_stop){
      valdes[2] = 1;     // разрешаем работать подсветке по времени
      valdes[6] = 0;     // сбрасываем принудительное включение по времени
    }
    else{  
      if(valdes[6]){valdes[2]=1;}  // принудительная работа подсветки по времени
      else{valdes[2]=0;}           // автоматическая работа подсветки по времени
    }
    if(sensors_param.cfgdes[6] && valdes[2]){   // если включена работа PWM или работа по времени
      int32_t bridges = 0;
      bridges = analogRead(3);
      if(bridges<sensors_param.cfgdes[2] || bridges>sensors_param.cfgdes[1]){ // яркость в пределах диапазона регулирования
        valdes[3]=convertRange(bridges, sensors_param.cfgdes[1], sensors_param.cfgdes[2], 4095, 0);
        valdes[3]=minRangeMax(valdes[3], 0, 4095);
        valdes[1]=convertRange(valdes[3], 0, 4095, 0, 100);
      }
      if(bridges>=sensors_param.cfgdes[2]){ // яркость больше максимума
        valdes[3]=0;
        valdes[1]=0;
      }
      if(bridges<=sensors_param.cfgdes[1]){ // яркость меньше минимума
        valdes[3]=4095;
        valdes[1]=100;
      }
    }
    else{ // нет разрешения работы подсветки
      valdes[3] = 0;
      valdes[1]=0;
    }
    if(sensors_param.cfgdes[5] && sensors_param.cfgdes[12]<valdes[4]){
         // если включена работа насоса и вышло время задержки после последнего полива
      if(digitalRead(22)){ // при воде в бачке
        int32_t buf = analogRead(0);
        if(buf>sensors_param.cfgdes[0]&&buf<sensors_param.cfgdes[13]){sum_en++;} // при сухой земле и достоверных показаниях
        else{sum_en=0;}
        if(sum_en >= 2 && valdes[2]){ // в период времени  работы подсветки и срабатывани датчика влажности 2 раза подряд
          valdes[4]=0;
          digitalWrite(19,1);
          delay(sensors_param.cfgdes[3]);
          digitalWrite(19,0);
          sum_en=0;
        }
      }
    }
    if(!sensors_param.cfgdes[5]){valdes[4]=sensors_param.cfgdes[12];}
  }
  change_pwm();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void change_pwm() { // изменение яркости подсветки
  int32_t bufpwm = 0;
  int32_t green = 0;
  int32_t rgb_pwm[3] = {0,0,0};
  if(sensors_param.cfgdes[7]){green = 4095;}
  if(valdes[0] > valdes[3]){ // если задание меньше яркости
    if(valdes[0]-valdes[3]>20){valdes[0]=valdes[0]-20;}
    else{valdes[0]=valdes[3];}
      if(sensors_param.cfgdes[7]){bufpwm = 4095 - valdes[0];}
      else{bufpwm = valdes[0];}
      rgb_pwm[0] = bufpwm;
      if(!sensors_param.cfgdes[4]){rgb_pwm[1] = green;} // если отключен зелёный
      else{rgb_pwm[1] = bufpwm;}
      rgb_pwm[2] = bufpwm;
      analogWrite(0,rgb_pwm[0]);
      delay(2);
      analogWrite(1,rgb_pwm[1]);
      delay(2);
      analogWrite(2,rgb_pwm[2]);
      delay(2);
  }
  if(valdes[0]<valdes[3]){ // если задание больше яркости
    if(valdes[3]-valdes[0]>20){valdes[0]=valdes[0]+20;}
    else{valdes[0]=valdes[3];}
      if(sensors_param.cfgdes[7]){bufpwm = 4095 - valdes[0];}
      else{bufpwm = valdes[0];}
      rgb_pwm[0] = bufpwm;
      if(!sensors_param.cfgdes[4]){rgb_pwm[1] = green;}
      else{rgb_pwm[1] = bufpwm;}
      rgb_pwm[2] = bufpwm;
      analogWrite(0,rgb_pwm[0]);
      delay(2);
      analogWrite(1,rgb_pwm[1]);
      delay(2);
      analogWrite(2,rgb_pwm[2]);
      delay(2);
  }
  if(valdes[0]==valdes[3]){ // если задание равно яркости
      if(!sensors_param.cfgdes[4]){rgb_pwm[1] = green;}
      else{
        if(sensors_param.cfgdes[7]){bufpwm = 4095 - valdes[0];}
        else{bufpwm = valdes[0];}
        rgb_pwm[1] = bufpwm;
      }
      analogWrite(1,rgb_pwm[1]);
      delay(2);
  }
}

int32_t convertRange(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max){ // изменение диапазона
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int32_t minRangeMax(int32_t x, int32_t in_min, int32_t in_max){ // ограничение диапазона
	if (x > in_max) {x = in_max;}
	if (x < in_min) {x = in_min;}
	return x;
}

void webfunc(char *pbuf) { // вывод данных на главной модуля
  os_sprintf(HTTPBUFF,"Освещение: %d (0-4095)<br>",adc1_get_raw(3));
  os_sprintf(HTTPBUFF,"Подсветка задание: <progress value='%d' max='100'></progress> %d%%<br>",valdes[1],valdes[1]);
  int32_t buf = analogRead(0);
  if(buf<sensors_param.cfgdes[0]){os_sprintf(HTTPBUFF,"Влажность почвы <b><font color='green'>в норме</font></b> (%d)<br>",buf);}
  if(buf>=sensors_param.cfgdes[0]&&buf<sensors_param.cfgdes[13]){os_sprintf(HTTPBUFF,"Влажность почвы <b><font color='red'><blink>низкая</blink></font></b> (%d)<br>",buf);}
  if(buf>sensors_param.cfgdes[13]){os_sprintf(HTTPBUFF,"Влажность почвы <b><font color='red'><blink>недостоверная</blink></font></b> (%d)<br>",buf);}
  if(!digitalRead(22)){os_sprintf(HTTPBUFF,"<b><font color='red'>Вода в баке <blink>закончилась!</blink></font></b><br>");}
  os_sprintf(HTTPBUFF,"После полива : %d д. %d ч. %d мин.<br>",valdes[4]/1440,valdes[4]/60%24,valdes[4]%60);
  
  if(valdes[6]){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(6, 0);repage()' style='width:195px;height:20px;color:#FFF;background:#00FF00'><b>Выкл освещение раньше</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(6, 1);repage()' style='width:195px;height:20px'><b>Вкл освещение раньше</b></button>");
  }
  
  os_sprintf(HTTPBUFF,"<button type='button' onclick='func(5, 0);repage()' style='width:195px;height:20px'><b>Полить один раз</b></button>");
    os_sprintf(HTTPBUFF,"<hr>");
    os_sprintf(HTTPBUFF,"<b>Контроль функций:</b><br>");
  if(sensors_param.cfgdes[6]){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(1, 0);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Освещение авто</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(1, 1);repage()' style='width:130px;height:20px'><b>Освещение выкл</b></button>");
  }
  if(sensors_param.cfgdes[5]){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(2, 0);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Полив авто</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(2, 1);repage()' style='width:130px;height:20px'><b>Полив выкл</b></button>");
  }
  if(sensors_param.cfgdes[4]){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(3, 0);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Зелёный вкл</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(3, 1);repage()' style='width:130px;height:20px'><b>Зелёный выкл</b></button>");
  }
  os_sprintf(HTTPBUFF,"<div class='c'><div class='main'><pre><table name='table1'border='0'class='catalogue'><b>Время работы освещения:</b><tr style='background-color: yellow'><td> часы </td><td> минуты </td><td> часы </td><td> минуты</td></tr>");
  os_sprintf(HTTPBUFF,"<tr><td>c <INPUT size=2 NAME='cfg8'id='cfg8'value='%02d'></td><td> <INPUT size=2 NAME='cfg9'id='cfg9'value='%02d'></td><td> по <INPUT size=2 NAME='cfg10' id='cfg10' value='%02d'></td><td><INPUT size=2 NAME='cfg11'id='cfg11'value='%02d'></td></tr></table><br>",sensors_param.cfgdes[8],sensors_param.cfgdes[9],sensors_param.cfgdes[10],sensors_param.cfgdes[11]);
  os_sprintf(HTTPBUFF,"<button type='button' onclick='func2()'style='width:100px;height:20px'><b>Принять</b></button></pre></div>");


  os_sprintf(HTTPBUFF,"<script>var request = new XMLHttpRequest();");
  os_sprintf(HTTPBUFF,"function reqReadyStateChange(){if(request.readyState == 4){var status = request.status;if (status == 200) {document.getElementById('output').innerHTML=request.responseText+ ' wait data save...';}}}");
  os_sprintf(HTTPBUFF,"function func(confset, valset){valset=confset*100000000+valset;request.open('GET', 'valdes?int=5&set='+valset, true);request.onreadystatechange = reqReadyStateChange;request.send();}");
  //os_sprintf(HTTPBUFF,"function func2(){func(4, cfg8.value);setTimeout('func(5, cfg9.value);', 2000);setTimeout('func(6, cfg10.value);', 4000);setTimeout('func(7, cfg11.value);', 6000);setTimeout('repage();', 6000);}");
  os_sprintf(HTTPBUFF,"function func2(){var buf = parseInt(cfg8.value)*1000000+parseInt(cfg9.value)*10000+parseInt(cfg10.value)*100+parseInt(cfg11.value);func(4, buf);repage();}");
  os_sprintf(HTTPBUFF,"function repage(){setTimeout('location.reload(true);', 2000);}</script>");

  os_sprintf(HTTPBUFF,"<hr>");
    os_sprintf(HTTPBUFF,"<div id='output'></div>");
//  os_sprintf(HTTPBUFF,"Непрерывная работа: %d дней, %d:%d:%d <hr>",timer_uptime.day,timer_uptime.hour,timer_uptime.min,timer_uptime.sec);
}