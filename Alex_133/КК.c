// Проект "Рекуператор" для конструктора кода ESP8266
// Настройки: [0]:Ночь от часы, [1]:Ночь от мин., [2]:Ночь до часы,
// [3]:Ночь до мин., [4]:Уров. огран. ночью, [5]:СО2 макс., [6]:СО2 срд., [7]:СО2 мин.,
// [8]:СО2 откл., [9]:Гист. СО2 [10]:Режим уст., [11]:Мин. Т притока,
// [12]:Т вкл. нагр., [13]:Т выкл. нагр., [14]:Таймер продувки
// Режимы установки:
// 0 - отключена
// 1 - работа по СО
// 2 - минимальная производительность
// 3 - средняя производительность
// 4 - максимальная производительность
// Глобальные переменные: 8 шт.
// valdes[0] - Режим установки
// valdes[1] - Задание GPIO вентиляторов
// 0 - отключены
// 1 - минимальная производительность
// 2 - средняя производительность
// 3 - максимальная производительность
// valdes[2] - КПД рекуператора по температуре в %
// valdes[3] - аварийный счётчик
// valdes[4] - режим нагревателя
// valdes[5] - для обработки нажатия кнопок на web-странице
// valdes[6] - сохранять ли новый режим установки после перезагрузки
// valdes[7] - режим задержки отключения вентиляторов

void temperature_kpd() 
{                                                                                           //расчет КПД рекуператора
    int32_t x = 0;
    x = (data1wire[3] - data1wire[0]) / (data1wire[1] - data1wire[0]);
    valdes[2] = x * 100;
}

void push_web()
{                                                                                           // обработка нажатия кнопок на web-странице
        int32_t buf = valdes[5]/100000000;
    switch (buf) {
      case 1:
        if(valdes[6]){                                                                      // если нужно сохранять режим установки
            valdes[0] = valdes[5]%100000000;                                                // записываем новый режим установки в оперативку
            sensors_param.cfgdes[10] = valdes[5]%100000000;SAVEOPT                          // записываем новый режим установки на флеш
        }
        else{                                                                               // иначе
            valdes[0] = valdes[5]%100000000;                                                // записываем новый режим установки в оперативку
        }
      break;
      case 2:                                                                               // сохранять ли настройки в память
        valdes[6]=valdes[5]%100000000;
      break;
      case 4:                                                                               // сохранение периода ночьного времени
        sensors_param.cfgdes[0]=(valdes[5]%100000000)/1000000;
        sensors_param.cfgdes[1]=(valdes[5]%1000000)/10000;
        sensors_param.cfgdes[2]=(valdes[5]%10000)/100;
        sensors_param.cfgdes[3]=valdes[5]%100;
        SAVEOPT
      break;

    }
    valdes[5]=0;
}

void reg_CO()                                                                               // регулировка производительности по СО
{ 		
    if(co2data <= sensors_param.cfgdes[8]){                                                 // если СО2 меньше уставки отключения
        if(valdes[1] != 0){                                                                 // и вентиляторы работают
            valdes[1] = 0;                                                                  // отключаем вентиляторы
            vent_gpio();                                                                    // применение нового режима вентиляторов
        }                 							
    }
    else if(co2data > sensors_param.cfgdes[8] && co2data <= sensors_param.cfgdes[7]){       // если СО2 между уставкой отключения и минимальной уставкой
        if(valdes[1] > 1){                                                                  // и вентиляторы работают больше минимальной производительности
            valdes[1] = 1;                                                                  // переключаем вентиляторы на минимальную производительность
            vent_gpio();                                                                    // применение нового режима вентиляторов
        }
    }
    else if(co2data > sensors_param.cfgdes[7] && co2data <= sensors_param.cfgdes[6]){       // если СО2 между минимальной уставкой и средней уставкой
        if(valdes[1] > 2){                                                                  // и вентиляторы работают больше средней производительности
            valdes[1] = 2;                                                                  // переключаем вентиляторы на среднюю производительность
            vent_gpio();                                                                    // применение нового режима вентиляторов
        }
        if(valdes[1] < 1){                                                                  // и вентиляторы работают меньше минимальной производительности
            valdes[1] = 1;                                                                  // переключаем вентиляторы на минимальную производительность
            vent_gpio();                                                                    // применение нового режима вентиляторов
        }
    }
    else if(co2data > sensors_param.cfgdes[6] && co2data <= sensors_param.cfgdes[5]){       // если СО2 между средней уставкой и максимальной уставкой
        if(valdes[1] < 2){                                                                  // и вентиляторы работают меньше средней производительности
            valdes[1] = 2;                                                                  // переключаем вентиляторы на среднюю производительность
            vent_gpio();                                                                    // применение нового режима вентиляторов
        }
    }
    else if(co2data > sensors_param.cfgdes[5]){                                             // если СО2 больше максимальной уставки
        if(valdes[1] < 3){                                                                  // и вентиляторы работают меньше максимальной производительности
            valdes[1] = 3;                                                                  // переключаем вентиляторы на максимальную производительность
            vent_gpio();                                                                    // применение нового режима вентиляторов
        }
    }
}

void vent_gpio()                                                                            // применение нового режима вентиляторов
{
	if(valdes[4] && valdes[1] == 0){                                                        // если нагреватель включен и вентиляторы отключены
        valdes[7] = 1;                                                                      // включаем режим задержки отключения вентиляторов
        // выключаем нагреватель         
        valdes[4] = 0;                                                                      // отмечаем новый статус нагревателя
    }
    if(valdes[7] == 0){
        // применяем задание вентиляторам
    }
}

void activ_heater()                                                                         // управление нагревателем уличного воздуха
{
    if(data1wire[0] < sensors_param.cfgdes[12] && valdes[0] != 0){                          // если Т меньше уставки включения нагревателя и установка в работе
        if(valdes[4] != 0){                                                                 // если нагреватель выключен
            // включаем нагреватель
        }
    }
    if(data1wire[0] > sensors_param.cfgdes[13]){                                            // если Т больше уставки отключения нагревателя
        if(valdes[4]){                                                                      // если нагреватель включен
            // выключаем нагреватель
        }
    }
}

void ICACHE_FLASH_ATTR
startfunc(){                                                                                // выполняется один раз при старте модуля.
    valdes[0] = 0;                                                                          // обнуление переменных
    valdes[1] = 0;
    valdes[2] = 0;
    valdes[3] = 0;
    valdes[4] = 0;
    valdes[5] = 0;
    valdes[6] = 0;
    valdes[7] = 0;
    valdes[0] = sensors_param.cfgdes[10];                                                   // чтение режима установки
}

void ICACHE_FLASH_ATTR
timerfunc(uint32_t  timersrc) {                                                             // выполнение кода каждую 1 секунду
    if(valdes[7]) {                                                                         // если режим задержки отключения вентиляторов активен
        valdes[7]++;                                                                        // инкрементируем переменную задержки отключения вентиляторов
        if(valdes[7] >= sensors_param.cfgdes[14]){                                          // если время задержки отключения вентиляторов вышло
            valdes[7] = 0;                                                                  // сбрасываем режим задержки отключения вентиляторов
            vent_gpio();                                                                    // применение нового режима вентиляторов
        }
    }
    if(valdes[5]){                                                                          // обработка нажатия кнопок на web-странице
        push_web();
    }
    if(valdes[0] == 1){                                                                     // если режим работы по СО
        reg_CO();
    }
    if(valdes[0] == 0){                                                                     // если режим работы "Установка отключена"
        if(valdes[1] != 0){                                                                 // и вентиляторы работают
            valdes[1] = 0;                                                                  // отключаем вентиляторы
            vent_gpio();                                                                    // применение нового режима вентиляторов
        } 
    }
    if(valdes[0] == 2){                                                                     // если режим работы "Минимальная производительность"
        if(valdes[1] != 1){                                                                 // и вентиляторы работают не на минимальной производительности
            valdes[1] = 1;                                                                  // переключаем вентиляторы на минимальную производительность
            vent_gpio();                                                                    // применение нового режима вентиляторов
        } 
    }
    if(valdes[0] == 3){                                                                     // если режим работы "Средняя производительность"
        if(valdes[1] != 2){                                                                 // и вентиляторы работают не на средней производительности
            valdes[1] = 2;                                                                  // переключаем вентиляторы на среднюю производительность
            vent_gpio();                                                                    // применение нового режима вентиляторов
        } 
    }
    if(valdes[0] == 4){                                                                     // если режим работы "Максимальная производительность"
        if(valdes[1] != 3){                                                                 // и вентиляторы работают не на максимальной производительности
            valdes[1] = 3;                                                                  // переключаем вентиляторы на максимальную производительность
            vent_gpio();                                                                    // применение нового режима вентиляторов
        } 
    }
    if(timersrc%10==0){                                                                     // выполнение кода каждые 10 секунд
        activ_heater();                                                                     // управление нагревателем уличного воздуха
        temperature_kpd();                                                                  // расчёт КПД
        if(data1wire[0] >= sensors_param.cfgdes[11] && valdes[0] != 0) {                    // если температура притока аварийно низкая и установка не отключена
            valdes[3]++;                                                                    // +1 к переменной аварийного счётчика
        }
        else {
            if(valdes[3] > 0) {                                                             // если аварийный счётчик больше нуля
               valdes[3]--;                                                                 // -1 от переменной аварийного счётчика
            }
        }
        if(valdes[3] >= 3) {                                                                // если аварийный счётчик досчитал до трёх
            valdes[0] = 0;                                                                  // устанавливаем режим работы "Установка отключена"
        }
    }
}

void webfunc(char *pbuf) {                                                                  // вывод данных на главной модуля
  os_sprintf(HTTPBUFF,"КПД рекуператора:         %d %<br>",valdes[2]);
  if(valdes[4]){
    os_sprintf(HTTPBUFF,"<b>Нагреватель включен</b><br>");
  }
  else if{
    os_sprintf(HTTPBUFF,"<b>Нагреватель выключен</b><br>");
  }
  // кнопки
    os_sprintf(HTTPBUFF,"<hr>");
    os_sprintf(HTTPBUFF,"<b>Режимы рекуператора:</b><br>");
  if(valdes[0] == 2){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Минимум</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(1, 2);repage()' style='width:130px;height:20px'><b>Минимум</b></button>");
  }
  if(valdes[0] == 3){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Средняя</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(1, 3);repage()' style='width:130px;height:20px'><b>Средняя</b></button>");
  }
  if(valdes[0] == 4){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Максимальная</b></button><br>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(1, 4);repage()' style='width:130px;height:20px'><b>Максимальная</b></button><br>");
  }

  if(vvaldes[0] == 0){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='' style='width:195px;height:20px;color:#FFF;background:#00FF00'><b>Отключено</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(1, 0);repage()' style='width:195px;height:20px'><b>Отключить</b></button>");
  }
  if(valdes[0] == 1){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='' style='width:195px;height:20px;color:#FFF;background:#00FF00'><b>Управление по СО2</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(1, 1);repage()' style='width:195px;height:20px'><b>Управлять по СО2</b></button>");
  }
  
    os_sprintf(HTTPBUFF,"<hr>");
  if(valdes[6]){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(2, 0);repage()' style='width:195px;height:20px;color:#FFF;background:#00FF00'><b>Сохранять новый режим</b></button><br>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(2, 1);repage()' style='width:195px;height:20px'><b>Не сохранять новый режим</b></button><br>");
  }
  // отображение режима сохранённого во флеш
    os_sprintf(HTTPBUFF,"<b>Сохранённый режим рекуператора:</b><br>");
  if(sensors_param.cfgdes[10] == 0){
    os_sprintf(HTTPBUFF,"<b>Отключено</b><br>");
  }
  else if(sensors_param.cfgdes[10] == 1){
    os_sprintf(HTTPBUFF,"<b>Работа по СО2</b><br>");
  }
  else if(sensors_param.cfgdes[10] == 2){
    os_sprintf(HTTPBUFF,"<b>Минимальная производительность</b><br>");
  }
  else if(sensors_param.cfgdes[10] == 3){
    os_sprintf(HTTPBUFF,"<b>Средняя производительность</b><br>");
  }
  else if(sensors_param.cfgdes[10] == 4){
    os_sprintf(HTTPBUFF,"<b>Максимальная производительность</b><br>");
  }
    os_sprintf(HTTPBUFF,"<hr>");
  // таблица с вводом ночного времени
  os_sprintf(HTTPBUFF,"<div class='c'><div class='main'><pre><table name='table1'border='0'class='catalogue'><b>Ночьное время:</b><tr style='background-color: yellow'><td> часы </td><td> минуты </td><td> часы </td><td> минуты</td></tr>");
  os_sprintf(HTTPBUFF,"<tr><td>c <INPUT size=2 NAME='cfg8'id='cfg8'value='%02d'></td><td> <INPUT size=2 NAME='cfg9'id='cfg9'value='%02d'></td><td> по <INPUT size=2 NAME='cfg10' id='cfg10' value='%02d'></td><td><INPUT size=2 NAME='cfg11'id='cfg11'value='%02d'></td></tr></table><br>",sensors_param.cfgdes[0],sensors_param.cfgdes[1],sensors_param.cfgdes[2],sensors_param.cfgdes[3]);
  os_sprintf(HTTPBUFF,"<button type='button' onclick='func2()'style='width:100px;height:20px'><b>Принять</b></button></pre></div>");

  // общие скрипты
  os_sprintf(HTTPBUFF,"<script>var request = new XMLHttpRequest();");
  os_sprintf(HTTPBUFF,"function reqReadyStateChange(){if(request.readyState == 4){var status = request.status;if (status == 200) {document.getElementById('output').innerHTML=request.responseText+ ' wait data save...';}}}");
  os_sprintf(HTTPBUFF,"function func(confset, valset){valset=confset*100000000+valset;request.open('GET', 'valdes?int=5&set='+valset, true);request.onreadystatechange = reqReadyStateChange;request.send();}");
  // скрипты для таблицы
  os_sprintf(HTTPBUFF,"function func2(){var buf = parseInt(cfg8.value)*1000000+parseInt(cfg9.value)*10000+parseInt(cfg10.value)*100+parseInt(cfg11.value);func(4, buf);repage();}");
  os_sprintf(HTTPBUFF,"function repage(){setTimeout('location.reload(true);', 2000);}</script>");
  // вывод ответа сервера
  os_sprintf(HTTPBUFF,"<hr>");
    os_sprintf(HTTPBUFF,"<div id='output'></div>");
}
