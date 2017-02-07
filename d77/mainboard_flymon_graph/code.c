void ICACHE_FLASH_ATTR startfunc(){
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
}
void webfunc(char *pbuf) {
    char hwaddr[6];
    char data[15];
    wifi_get_macaddr(0, hwaddr);
    os_sprintf(data,MACSTRT,MAC2STRT(hwaddr));
    os_sprintf(HTTPBUFF,"<br><iframe src='http://monitor.flymon.net/console.html?macs=%s&period=1h' style='width:100%;height:320px;border:0;'></iframe>",data);
}
