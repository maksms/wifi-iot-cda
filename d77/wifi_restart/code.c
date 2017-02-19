int32_t  zadergka_reboot=0;
void ICACHE_FLASH_ATTR
startfunc()
{
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
    if ( !timersrc && !timersrc%60 )  // выполнение кода каждые 60 секунд
    {
       if ( mtest == 15 && !mtest2  && !mtest3 && wfrc < 1000 ) zadergka_reboot = 0 ;
       if ( mtest != 15 || mtest2 != 0 || mtest3 != 0 || 1000 < wfrc ) zadergka_reboot ++ ;
       if ( 5 <= zadergka_reboot ) system_restart() ;
    }
}
void webfunc(char *pbuf) 
{
    os_sprintf(HTTPBUFF,"<br>mqtt: ( State: %d &frasl; Errors: %d &frasl; %d ) , reconnect: %d ", mtest , mtest2 , mtest3 , wfrc );
}