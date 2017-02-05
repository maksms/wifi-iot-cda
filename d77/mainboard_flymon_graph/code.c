void ICACHE_FLASH_ATTR startfunc(){
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
}
void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br><iframe src='http://monitor.flymon.net/console.html?macs=ВПИСАТЬ_МАК_АДРЕС_МОДУЛЯ&graphs=temp,gpio&period=1h' style='width:100%;height:320px;border:0;'></iframe>");
}
