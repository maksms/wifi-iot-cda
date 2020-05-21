#define DEVICE_IP "192.168.2.218"

static uint8_t gpio_pin1, gpio_pin2, gpio_log;
static uint8_t gpio_state1, gpio_state2, state_log;

static void ICACHE_FLASH_ATTR tcpclient_recon_cb(void *arg, sint8 err);

struct Gp {
    uint8_t pin;
    uint8_t state;
};

static void ICACHE_FLASH_ATTR tcpclient_connect_cb(void *arg) {
    struct espconn *pespconn = (struct espconn *)arg;
    char payload[512];
    espconn_regist_sentcb(pespconn, tcpclient_sent_cb);
    espconn_regist_disconcb(pespconn, tcpclient_discon_cb);

    struct Gp *gp = (struct Gp *)pespconn->reverse;

    os_sprintf(payload, "GET /gpio?pin=%d&st=%d", gp->pin, gp->state);
    os_sprintf(payload + os_strlen(payload), " HTTP/1.1\r\nHost: testdomen\r\nConnection: keep-alive\r\nAccept: */*\r\n\r\n");
    espconn_sent(pespconn, payload, strlen(payload));

    os_free(gp);
}

static void ICACHE_FLASH_ATTR send_tcp_data(const char* addr1, uint8_t pin, uint8_t st){
    struct espconn *pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
    if (pCon == NULL) {
        // ошибка
        return;
    }
    pCon->type = ESPCONN_TCP;
    pCon->state = ESPCONN_NONE;
    pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    pCon->proto.tcp->local_port = espconn_port();
    pCon->proto.tcp->remote_port = 80; // порт
    char adrr[16] = "192.168.2.218";             //  ип

    struct Gp *gp = (struct Gp *)os_zalloc(sizeof(struct Gp));
    gp->pin = pin;
    gp->state = st;
	pCon->reverse = gp;

    uint32_t ip = ipaddr_addr(adrr);        // сервер
    os_memcpy(pCon->proto.tcp->remote_ip, &ip, 4);
    espconn_regist_connectcb(pCon, tcpclient_connect_cb); // функция отправки GET запроса
    espconn_regist_reconcb(pCon, tcpclient_recon_cb);
    espconn_connect(pCon);    
}

void ICACHE_FLASH_ATTR
startfunc(){
// выполняется один раз при старте модуля.
}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
// выполнение кода каждую 1 секунду

uint8_t new_state1 = GPIO_ALL_GET(15);
if (  new_state1 != gpio_state1 ) {
    gpio_pin1 = 12;
    gpio_state1 = new_state1;
    send_tcp_data(DEVICE_IP, gpio_pin1, gpio_state1);
}

uint8_t new_state2 = GPIO_ALL_GET(13);
if (  new_state2 != gpio_state2 ) {
    gpio_pin2 = 13;
    gpio_state2 = new_state2;
    send_tcp_data(DEVICE_IP, gpio_pin2, gpio_state2);
}

if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}
}

void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br>test"); // вывод данных на главной модуля
os_sprintf(HTTPBUFF,"<br>gpio_log: %d", gpio_log); // вывод данных на главной модуля
os_sprintf(HTTPBUFF,"<br>state_log: %d", state_log); // вывод данных на главной модуля
}