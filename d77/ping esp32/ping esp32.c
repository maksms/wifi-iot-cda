#include "ping.h"
#include "esp_ping.h"
bool ping_status_esp32=0;
uint8_t ping_count = 20;
esp_err_t pingResults(ping_target_id_t msgType, esp_ping_found * pf)
{
	printf("AvgTime:%.1fmS Sent:%d Rec:%d min(mS):%d max(mS):%d Resp(mS):%d Timeouts:%d Total Time:%d\n", (float)pf->total_time/pf->recv_count, pf->send_count, pf->recv_count,  pf->min_time, pf->max_time ,pf->resp_time, pf->timeout_count, pf->total_time);
	ping_status_esp32=(pf->total_time && (pf->send_count/2) < pf->recv_count )?1:0;
	return ESP_OK;
}
void ping_test_esp32()
{
    	uint32_t ip_pinG = ipaddr_addr("8.8.8.8");//ip adress ping
    	uint32_t ping_timeout = 1; //Sek till we consider it timed out
    	uint32_t ping_delay = 1; //Sek between pings
	ping_deinit();
	esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &ping_count, sizeof(uint32_t));
	esp_ping_set_target(PING_TARGET_RCV_TIMEO, &ping_timeout, sizeof(uint32_t)); 
	esp_ping_set_target(PING_TARGET_DELAY_TIME, &ping_delay, sizeof(uint32_t));
	esp_ping_set_target(PING_TARGET_IP_ADDRESS, &ip_pinG, sizeof(uint32_t));
	esp_ping_set_target(PING_TARGET_RES_FN, &pingResults, sizeof(pingResults));
	ping_init();
}
void startfunc()
{
}
void timerfunc(uint32_t  timersrc) 
{ 
        if(timersrc%(ping_count*3)==0 && timersrc )
	{
		ping_test_esp32();
	}
        vTaskDelay(1000 / portTICK_PERIOD_MS);
}
void webfunc(char *pbuf) 
{
	os_sprintf(HTTPBUFF,"<hr>ping %d <meta http-equiv='refresh' content='20'>",ping_status_esp32);// refresh main 20 sec
}
