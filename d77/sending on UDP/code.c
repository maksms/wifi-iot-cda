char rx_buf_plus_tsp[512];               // буфер отправки
struct espconn *udp_clientdata;
 
senduserudp()
{
    char payload[512];
    os_sprintf(payload,"%s",rx_buf_plus_tsp);
    espconn_sent(udp_clientdata, payload, strlen(payload));
}  
 
void ICACHE_FLASH_ATTR startfunc()
{
    #if sdkver > 109
        wifi_set_broadcast_if(3);
    #endif
    udp_clientdata = (struct espconn *)os_zalloc(sizeof(struct espconn));
    espconn_delete(udp_clientdata);
    udp_clientdata->type = ESPCONN_UDP;
    udp_clientdata->state = ESPCONN_NONE;
    udp_clientdata->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    udp_clientdata->proto.udp->remote_port = 23456; // порт
    uint32_t ip_udp = ipaddr_addr("192.168.1.10"); // сервер
    os_memcpy(udp_clientdata->proto.udp->remote_ip, &ip_udp, 4);
    udp_clientdata->proto.udp->local_port = espconn_port();
    espconn_create(udp_clientdata);//это иницилазция
}
 
void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc)
{
    os_sprintf(rx_buf_plus_tsp,"");
    os_sprintf(rx_buf_plus_tsp,"helloy world!");
    senduserudp();
}
void webfunc(char *pbuf)
{
 
}