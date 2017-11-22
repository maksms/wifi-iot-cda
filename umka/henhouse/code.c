Код КК с двумя временными интервалами и управлением кнопкой
Первые 4 строчки скрывают пункты меню.

#define hwvisible
#define gpiovisible
#define otavisible
#define kettlemode
bool flag=0;
uint32_t timer_srcSave=0;
void ICACHE_FLASH_ATTR startfunc()
{
}
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc)
{
    if(digitalRead(0)==0)
    {  
        if(flag)       
        {
            flag=0;
        }  
        else
        {
            flag=1;
            digitalWrite(12,1);
            timer_srcSave=timersrc;
        }
    }
    if((timer_srcSave+600)<timersrc&&flag)
        flag=0;
    if(((time_loc.hour>=sensors_param.cfgdes[0] &&time_loc.hour<sensors_param.cfgdes[1]) || (time_loc.hour>=sensors_param.cfgdes[2]&& time_loc.hour<sensors_param.cfgdes[3])) && bh_l<sensors_param.cfgdes[4] )
    {
        digitalWrite(12,1);
        flag=0;
    }
    else
    {
        if(flag==0)
            digitalWrite(12,0);
    }
}
void webfunc(char *pbuf)
{
}
