void ICACHE_FLASH_ATTR
startfunc()
{
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
    if ( timersrc!=0 && timersrc%300==0 && mtest != 15 && 1000 < wfrc ) system_restart() ;
}
void webfunc(char *pbuf) 
{
    os_sprintf(HTTPBUFF,"<br>mqtt state: %s , reconnect: %d ", mtest==15?"Ok":"!" , wfrc );
}
