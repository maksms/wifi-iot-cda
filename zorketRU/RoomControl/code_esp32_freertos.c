#define SPOT_1 16
#define SPOT_2 33
#define SPOT_3 4
#define SPOT_4 32
#define PODVES_1 26
#define PODVES_2 25
#define WLED 27
#define KNOPKA 34

#define EFFECT sensors_param.cfgdes[6]

// Настройки ключей в веб-интерфейсе
// В конструкторе кода в поле "Количество настроек" прописать
//"Код кнопки для верхних спотов, Код кнопки для подвесных светильников, Код кнопки для светодиодной ленты, Код кнопки для выключения света, Длительность длинного нажатия мс, Максимальная пауза между нажатиями мс, Скорость эффекта включения мс"
#define key433_spot sensors_param.cfgdes[0]
#define key433_pendant sensors_param.cfgdes[1]
#define key433_WLED sensors_param.cfgdes[2]
#define key433_light_off sensors_param.cfgdes[3]

bool all_lights_status;    // Устанавливает 1 если включен хотя бы один из светильников
bool spot_light_status;    // Устанавливает 1 если включен хотя бы один спот
bool pendant_light_status; // Устанавливает 1 если включен хотя бы один подвесной светильник
bool WLED_light_status;    // Устанавливает 1 если включена светодиодная лента

// КНОПКА ВКЛЮЧЕНИЯ-ВЫКЛЮЧЕНИЯ СВЕТА
uint8_t num_but = 0;     // инкрементируемый счетчик нажатий, сбрасывается в ButtonAction
uint8_t num_click = 0;   // количество кликов по кнопке для обработки события в ButtonAction
bool long_click = false; // длинное нажатие

#define KNOPKA_DREBEZG 50
#define KNOPKA_LONG_PRESS_DURATION sensors_param.cfgdes[4]
#define KNOPKA_PRESS_MAX_PAUSE sensors_param.cfgdes[5]

void ButtonAction();

void button_task(void *pvParameter)
{

    bool button_state;
    bool prev_button_state = 0;
    TickType_t last_click_time = 0;
    TickType_t button_down_time = 0;

    while (1)
    {
        button_state = GPIO_ALL_GET(KNOPKA);

        if (button_state != prev_button_state)
        {
            vTaskDelay(KNOPKA_DREBEZG / portTICK_PERIOD_MS);
            button_state = GPIO_ALL_GET(KNOPKA);
            if (button_state != prev_button_state)
            {
                if (button_state == 1)
                {
                    long_click = 0;
                    button_down_time = xTaskGetTickCount();
                }
                else
                {
                    if (xTaskGetTickCount() - button_down_time >= KNOPKA_LONG_PRESS_DURATION / portTICK_PERIOD_MS)
                    {
                        long_click = 1;
                    }
                    else
                    {
                        if (xTaskGetTickCount() - last_click_time <= KNOPKA_PRESS_MAX_PAUSE / portTICK_PERIOD_MS)
                        {
                            num_but++;
                        }
                        else
                        {
                            num_but = 1;
                        }
                        last_click_time = xTaskGetTickCount();
                    }
                }
            }
        }

        if (xTaskGetTickCount() - last_click_time > KNOPKA_PRESS_MAX_PAUSE / portTICK_PERIOD_MS)
        {
            num_click = num_but;
            if (num_click > 0 || long_click > 0)
                ButtonAction();
        }

        prev_button_state = button_state;
    }
}

void SpotsON()
{
    digitalWrite(SPOT_1, 1); // спот 1
    vTaskDelay(EFFECT / portTICK_PERIOD_MS);
    digitalWrite(SPOT_2, 1); // спот 2
    vTaskDelay(EFFECT / portTICK_PERIOD_MS);
    digitalWrite(SPOT_3, 1); // спот 3
    vTaskDelay(EFFECT / portTICK_PERIOD_MS);
    digitalWrite(SPOT_4, 1); // спот 4
}
void SpotsOFF()
{
    digitalWrite(SPOT_4, 0); // спот 1
    vTaskDelay(EFFECT / portTICK_PERIOD_MS);
    digitalWrite(SPOT_1, 0); // спот 2
    vTaskDelay(EFFECT / portTICK_PERIOD_MS);
    digitalWrite(SPOT_2, 0); // спот 3
    vTaskDelay(EFFECT / portTICK_PERIOD_MS);
    digitalWrite(SPOT_3, 0); // спот 4
}
void PendantON()
{
    digitalWrite(PODVES_1, 1); // подвес 1 (слева)
    digitalWrite(PODVES_2, 1); // подвес 2 (справа)
}
void PendantOFF()
{
    digitalWrite(PODVES_1, 0); // подвес 1 (слева)
    digitalWrite(PODVES_2, 0); // подвес 2 (справа)
}

void LightInfo()
{
    if (GPIO_ALL_GET(SPOT_1) == 1 || GPIO_ALL_GET(SPOT_2) == 1 || GPIO_ALL_GET(SPOT_3) == 1 || GPIO_ALL_GET(SPOT_4) == 1 || GPIO_ALL_GET(PODVES_1) == 1 || GPIO_ALL_GET(PODVES_2) == 1 || GPIO_ALL_GET(WLED) == 1)
        all_lights_status = 1;
    else
        all_lights_status = 0;

    if (GPIO_ALL_GET(SPOT_1) == 1 || GPIO_ALL_GET(SPOT_2) == 1 || GPIO_ALL_GET(SPOT_3) == 1 || GPIO_ALL_GET(SPOT_4) == 1)
        spot_light_status = 1;
    else
        spot_light_status = 0;

    if (GPIO_ALL_GET(PODVES_1) == 1 || GPIO_ALL_GET(PODVES_2) == 1)
        pendant_light_status = 1;
    else
        pendant_light_status = 0;

    if (GPIO_ALL_GET(WLED) == 1)
        WLED_light_status = 1;
    else
        WLED_light_status = 0;
}

void ButtonAction()
{
    if (long_click == 1)
    {
        long_click = 0; // обнуляем переменные
        if (sensors_param.mqtten == 1)
        {
            MQTT_Publish("KEY_ALARM", "KEY_ALARM", 9, 2, 0, 1); // шлем KEY_ALARM
        }
    }

    if (num_click > 0)
    {
        if (num_click == 1) // Одинарный щелчок
        {
            if (all_lights_status == 1) // выключает Gpio200+Gpio201+Gpio202+Gpio203, Gpio204+Gpio205, Gpio206, если хотя бы один Gpio200 Gpio201 Gpio202 Gpio203 Gpio204 Gpio205 или Gpio206 включен
            {
                SpotsOFF();
                PendantOFF();
                digitalWrite(WLED, 0);
            }
            else
            {
                SpotsON();
            }
        }

        if (num_click == 2) // Двойной щелчок
        {
            if (pendant_light_status == 1) // если Gpio204 или Gpio205  включен
            {
                PendantOFF();
            }
            else
            {
                PendantON();
            }
        }

        if (num_click == 3) // Тройной щелчок
        {
            if (GPIO_ALL_GET(WLED) == 1) // если Gpio206  включен
            {
                digitalWrite(WLED, 0); // Выключает
            }
            else
            {
                digitalWrite(WLED, 1); // Включает Gpio206, если он выключен
            }
        }
        num_click = 0;
        num_but = 0;
    }
}

void recvrcfunc(uint32_t key) // управление брелками 433
{
    if (key == key433_spot)
    {
        if (spot_light_status == 0)
        {
            SpotsON();
        }
        else
            SpotsOFF();
    }

    if (key == key433_pendant)
    {
        if (pendant_light_status == 0)
            PendantON();
        else
            PendantOFF();
    }

    if (key == key433_WLED)
    {
        if (WLED_light_status == 0)
            digitalWrite(WLED, 1);
        else
            digitalWrite(WLED, 0);
    }
    if (key == key433_light_off)
    {
        SpotsOFF();
        PendantOFF();
        digitalWrite(WLED, 0);
    }
}

void startfunc()
{ // выполняется один раз при старте модуля.

    cb_rcswitch_funs = recvrcfunc; // активация функции управления брелками 433МГц

    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
}

void timerfunc(uint32_t timersrc)
{ // выполнение кода 1 раз в секунду
    LightInfo();
    if (timersrc % 5 == 0)
    { // выполнение кода каждые 5 секунд
    }
    delay(1000); // обязательная строка, минимальное значение для RTOS систем- 10мс
}

void webfunc(char *pbuf) // вывод данных на главной модуля
{
    if (all_lights_status == 1)
    {
        os_sprintf(HTTPBUFF, "Включено что-то из освещения:<br>");
        if (spot_light_status == 1)
            os_sprintf(HTTPBUFF, "Включены споты (или один из них)<br>");
        if (pendant_light_status == 1)
            os_sprintf(HTTPBUFF, "Включены подвесные светильники (или один из них)<br>");
        if (WLED_light_status == 1)
            os_sprintf(HTTPBUFF, "Включена лента<br>");
    }
    else
    {
        os_sprintf(HTTPBUFF, "Всё освещение выключено<br>");
    }
    os_sprintf(HTTPBUFF, "<div style='padding: 20px;'>"); // Открываем div блок со стилем

    os_sprintf(HTTPBUFF, "<table style='border-collapse: collapse;'>"); // Открываем таблицу со стилем

    os_sprintf(HTTPBUFF, "<tr style='background-color: #f0f0f0;'>");                            // Открываем строку с фоновым цветом
    os_sprintf(HTTPBUFF, "<th style='padding: 10px; border: 1px solid #ddd;'>Название</th>");   // Заголовок столбца с стилем
    os_sprintf(HTTPBUFF, "<th style='padding: 10px; border: 1px solid #ddd;'>Состояние</th>");  // Заголовок столбца с стилем
    os_sprintf(HTTPBUFF, "<th style='padding: 10px; border: 1px solid #ddd;'>Номер GPIO</th>"); // Заголовок столбца с стилем
    os_sprintf(HTTPBUFF, "</tr>");                                                              // Закрываем строку

    // Выводим данные для каждого устройства освещения
    os_sprintf(HTTPBUFF, "<tr><td>SPOT_1 &#128161;</td><td style='padding: 10px; border: 1px solid #ddd;'>%s</td><td style='padding: 10px; border: 1px solid #ddd;'>%d</td></tr>",
               GPIO_ALL_GET(16) == 1 ? "&#9989;" : "&#10060;", 16);
    os_sprintf(HTTPBUFF, "<tr><td>SPOT_2 &#128161;</td><td style='padding: 10px; border: 1px solid #ddd;'>%s</td><td style='padding: 10px; border: 1px solid #ddd;'>%d</td></tr>",
               GPIO_ALL_GET(33) == 1 ? "&#9989;" : "&#10060;", 33);
    os_sprintf(HTTPBUFF, "<tr><td>SPOT_3 &#128161;</td><td style='padding: 10px; border: 1px solid #ddd;'>%s</td><td style='padding: 10px; border: 1px solid #ddd;'>%d</td></tr>",
               GPIO_ALL_GET(4) == 1 ? "&#9989;" : "&#10060;", 4);
    os_sprintf(HTTPBUFF, "<tr><td>SPOT_4 &#128161;</td><td style='padding: 10px; border: 1px solid #ddd;'>%s</td><td style='padding: 10px; border: 1px solid #ddd;'>%d</td></tr>",
               GPIO_ALL_GET(32) == 1 ? "&#9989;" : "&#10060;", 32);
    os_sprintf(HTTPBUFF, "<tr><td>PODVES_1 &#128161;</td><td style='padding: 10px; border: 1px solid #ddd;'>%s</td><td style='padding: 10px; border: 1px solid #ddd;'>%d</td></tr>",
               GPIO_ALL_GET(26) == 1 ? "&#9989;" : "&#10060;", 26);
    os_sprintf(HTTPBUFF, "<tr><td>PODVES_2 &#128161;</td><td style='padding: 10px; border: 1px solid #ddd;'>%s</td><td style='padding: 10px; border: 1px solid #ddd;'>%d</td></tr>",
               GPIO_ALL_GET(25) == 1 ? "&#9989;" : "&#10060;", 25);
    os_sprintf(HTTPBUFF, "<tr><td>WLED &#128161;</td><td style='padding: 10px; border: 1px solid #ddd;'>%s</td><td style='padding: 10px; border: 1px solid #ddd;'>%d</td></tr>",
               GPIO_ALL_GET(27) == 1 ? "&#9989;" : "&#10060;", 27);
    os_sprintf(HTTPBUFF, "<tr><td>KNOPKA &#128161;</td><td style='padding: 10px; border: 1px solid #ddd;'>%s</td><td style='padding: 10px; border: 1px solid #ddd;'>%d</td></tr>",
               GPIO_ALL_GET(34) == 1 ? "&#9989;" : "&#10060;", 34);

    os_sprintf(HTTPBUFF, "</table>"); // Закрываем таблицу

    os_sprintf(HTTPBUFF, "</div>"); // Закрываем div блок
}
