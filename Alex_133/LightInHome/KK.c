soft_ap_mode() 
{ //вызов точки доступа
    struct softap_config apConfig;
    wifi_softap_get_config(&apConfig);
    os_sprintf(apConfig.ssid, "LightInHome");                       //имя точки доступа берем хостнэйм
    apConfig.authmode = 4;                                          //0 - без шифрования,4 с шифрованием
    os_sprintf(apConfig.password, "********");                      //пароль точки
    apConfig.ssid_len = strlen(apConfig.ssid);                      //длина имени точки доступа
    wifi_softap_set_config(&apConfig);                              //применить параметры точки
    wifi_set_opmode(3);                                             //включить точку доступа
}

void ICACHE_FLASH_ATTR
startfunc(){// выполняется один раз при старте модуля.
    valdes[0] = 0; // переменная для управления светом
    valdes[1] = 0; // переменная для управления WiFi
}

void ICACHE_FLASH_ATTR
timerfunc(uint32_t  timersrc) {// выполнение кода каждую 1 секунду
    if (timersrc > 300 && !pingprint) {  //если время более 300с после старта и нет пинга роутера
        if (!valdes[1]) {
            soft_ap_mode(); // поднимем AP
            valdes[1] = 1;  // метим что подняли AP
        }
    }
    else {
        if (valdes[1]) {
            wifi_set_opmode(1); // гасим AP
            valdes[1] = 0;  // метим что AP выключна
        }
    }
    if (valdes[0]) {
        if (valdes[0] == 2) { // выключаем всё освещение
            GPIO_ALL(200,0); GPIO_ALL(201,0); GPIO_ALL(202,0); GPIO_ALL(203,0); GPIO_ALL(204,0); GPIO_ALL(205,0); GPIO_ALL(206,0); GPIO_ALL(207,0);
            GPIO_ALL(208,0); GPIO_ALL(209,0); GPIO_ALL(210,0); GPIO_ALL(211,0); GPIO_ALL(212,0); GPIO_ALL(213,0); GPIO_ALL(214,0); GPIO_ALL(215,0);
            valdes[0] = 0;
        }
        else if (valdes[0] == 1) { // включаем всё освещение
            GPIO_ALL(200,1); GPIO_ALL(201,1); GPIO_ALL(202,1); GPIO_ALL(203,1); GPIO_ALL(204,1); GPIO_ALL(205,1); GPIO_ALL(206,1); GPIO_ALL(207,1);
            GPIO_ALL(208,1); GPIO_ALL(209,1); GPIO_ALL(210,1); GPIO_ALL(211,1); GPIO_ALL(212,1); GPIO_ALL(213,1); GPIO_ALL(214,1); GPIO_ALL(215,1);
            valdes[0] = 0;
        }
    }
}

void webfunc(char *pbuf) { // вывод данных на главной модуля
    os_sprintf(HTTPBUFF,"<b>Контроль освещения:</b><br>");
  if(GPIO_ALL_GET(200)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(200);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Спальня</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(200);repage()' style='width:130px;height:20px'><b>Спальня</b></button>");
  }
  if(GPIO_ALL_GET(201)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(201);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Детская А</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(201);repage()' style='width:130px;height:20px'><b>Детская А</b></button>");
  }
  if(GPIO_ALL_GET(202)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(202);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Детская М</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(202);repage()' style='width:130px;height:20px'><b>Детская М</b></button>");
  }
  os_sprintf(HTTPBUFF,"<br>");
  if(GPIO_ALL_GET(203)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(203);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Кухня</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(203);repage()' style='width:130px;height:20px'><b>Кухня</b></button>");
  }
  if(GPIO_ALL_GET(204)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(204);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Гостинная</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(204);repage()' style='width:130px;height:20px'><b>Гостинная</b></button>");
  }
  if(GPIO_ALL_GET(205)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(205);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Коридор</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(205);repage()' style='width:130px;height:20px'><b>Коридор</b></button>");
  }
  os_sprintf(HTTPBUFF,"<br>");
  if(GPIO_ALL_GET(206)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(206);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Ванная</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(206);repage()' style='width:130px;height:20px'><b>Ванная</b></button>");
  }
  if(GPIO_ALL_GET(207)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(207);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Коридор 2</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(207);repage()' style='width:130px;height:20px'><b>Коридор 2</b></button>");
  }
  if(GPIO_ALL_GET(208)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(208);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Туалет</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(208);repage()' style='width:130px;height:20px'><b>Туалет</b></button>");
  }
  os_sprintf(HTTPBUFF,"<br>");
  if(GPIO_ALL_GET(209)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(209);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Балкон</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(209);repage()' style='width:130px;height:20px'><b>Балкон</b></button>");
  }
  if(GPIO_ALL_GET(210)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(210);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Гардеробная</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(210);repage()' style='width:130px;height:20px'><b>Гардеробная</b></button>");
  }
  if(GPIO_ALL_GET(211)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(211);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Подсветка 1</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(211);repage()' style='width:130px;height:20px'><b>Подсветка 1</b></button>");
  }
  os_sprintf(HTTPBUFF,"<br>");
  if(GPIO_ALL_GET(212)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(212);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Подсветка 2</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(212);repage()' style='width:130px;height:20px'><b>Подсветка 2</b></button>");
  }
  if(GPIO_ALL_GET(213)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(213);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Подсветка 3</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(213);repage()' style='width:130px;height:20px'><b>Подсветка 3</b></button>");
  }
  if(GPIO_ALL_GET(214)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(214);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Подсветка 4</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(214);repage()' style='width:130px;height:20px'><b>Подсветка 4</b></button>");
  }
  os_sprintf(HTTPBUFF,"<br>");
  if(GPIO_ALL_GET(215)){
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(215);repage()' style='width:130px;height:20px;color:#FFF;background:#00FF00'><b>Подсветка 5</b></button>");
  }
  else{
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func(215);repage()' style='width:130px;height:20px'><b>Подсветка 5</b></button>");
  }
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func2(2);repage()' style='width:130px;height:20px'><b>Выключить всё</b></button>");
    os_sprintf(HTTPBUFF,"<button type='button' onclick='func2(1);repage()' style='width:130px;height:20px'><b>Включить всё</b></button>");
    
    os_sprintf(HTTPBUFF,"<script>var request = new XMLHttpRequest();");
  os_sprintf(HTTPBUFF,"function reqReadyStateChange(){if(request.readyState == 4){var status = request.status;if (status == 200) {document.getElementById('output').innerHTML=request.responseText+ ' wait data save...';}}}");
  os_sprintf(HTTPBUFF,"function func(pinSet){request.open('GET', 'gpio?st=2&pin='+pinSet, true);request.onreadystatechange = reqReadyStateChange;request.send();}");
  os_sprintf(HTTPBUFF,"function func2(valset){request.open('GET', 'valdes?int=0&set='+valset, true);request.onreadystatechange = reqReadyStateChange;request.send();}");
  os_sprintf(HTTPBUFF,"function repage(){setTimeout('location.reload(true);', 2000);}</script>");

  os_sprintf(HTTPBUFF,"<hr>");
    os_sprintf(HTTPBUFF,"<div id='output'></div>");
}
