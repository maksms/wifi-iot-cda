# esp_wifiiot/uart1_log

Esp8266 имеет 2 аппаратных UART:
* UART0 - полноценный TX (GPIO1) и RX (GPIO3)
* UART1 - только получение данныых, RX (GPIO2)

Для Wemod D1 mini GPIO2 - это пин D4.

Если на UART0 висит какое-то устройтсво, взаимодействие с которым требует отладки, будет удобным использовать UART1 для вывода своих логов, чтобы не мешать работе UART0.

Для вывода своих логов в UART1 используется функция userlog() с переменным числом параметров, полный аналог printf.

Примеры использования:

    userlog("\n put string to uart1 \n");
    userlog("\n put string value to uart1: %s \n", str);
    userlog("\n put int value to uart1: %d \n", i);
    userlog("\n put 2 int value to uart1: %d %d \n", k, i);
	
	
