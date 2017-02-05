void ICACHE_FLASH_ATTR startfunc(){}
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
}
void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br><iframe src='http://monitor.flymon.net/console.html?macs=5CCF7F0F71E6&graphs=temp,gpio&period=1h' style='width:100%;height:320px;border:0;'></iframe>");
}
