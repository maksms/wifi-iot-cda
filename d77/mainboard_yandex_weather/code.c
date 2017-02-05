void ICACHE_FLASH_ATTR startfunc(){
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
}
void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF,"<br><img src='https://info.weather.yandex.net/saint-petersburg/4.png' style='width:100%;height:320px;border:0;'></iframe>");
}
