#include "sleep_funkcijos.h"

static const char *TAG = "SLEEP";
 TaskHandle_t g_sim_handle;

bool vogiamas = false;
bool uzrakintas = true;

esp_err_t sim_vogiamas_sleep(const char* cycle_value) ///eDRX sleep
{
    if(cycle_value == NULL) {
        cycle_value = VogiamasSleepCycle;
    }
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CEDRXS=0");
   // snprintf(cmd, sizeof(cmd), "AT+CEDRXS=1,4,\"%s\"", cycle_value);

    if (sim_at(cmd, "OK", SIM_AT_TIMEOUT_MS, NULL, 0)) {
        ESP_LOGI(TAG, "eDRX configured");
    }

    if (sim_at("AT+CSCLK=1", "OK", SIM_AT_TIMEOUT_MS, NULL, 0)) {
        ESP_LOGI(TAG, "Sulėtintas laikrodis (neveiks GNSS)");
    }else {
        ESP_LOGW(TAG, "Nepavyko sulėtinti laikrodžio, tęsiama be to");
    }

    return ESP_OK;
}


esp_err_t sleep_uzrakintas(uint8_t seconds)
{
    ESP_LOGI(TAG,"Sleep užrakintas");
    enable_interrupts();
    //if(!sim_at("AT+CPSMS=1,,,\"10010001\",\"00000001\"", "OK", 3000, NULL, 0))ESP_LOGI(TAG,"PSM SIM nepavayko");
    if (sim_at("AT+CPOWD=1", "OK", SIM_AT_TIMEOUT_MS, NULL, 0)) ESP_LOGI(TAG, "Išjungtas SIM7070G");

    ESP_ERROR_CHECK(gpio_set_level(SCOOTER, SCOOTER_OFF));
    

    if(TESTING)
    {       
        g_sim_handle = xTaskGetCurrentTaskHandle();
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(seconds*1000));
    }else{

        gpio_wakeup_enable(INTERRUPT_PIN, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();

        esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
        esp_light_sleep_start();

        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        if (cause == ESP_SLEEP_WAKEUP_GPIO) {
            ///// interrupt funkcija
            ESP_LOGI(TAG, "Pabudo dėl interrupt");
        }

    }
    ESP_LOGI(TAG, "Pulsing PWRKEY...");
    gpio_set_level(PWRKEY, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(PWRKEY, 0);
    sim_at("AT+CPSMS=0", "OK", 3000, NULL, 0);
    if (sim_init() != ESP_OK) ESP_LOGE(TAG, "SIM7070G init failed");
    //if (sim_sms_configure() != ESP_OK) ESP_LOGE(TAG, "SMS configure failed");
    ESP_LOGI(TAG, "Pabudo iš sleep_uzrakintas");
    return ESP_OK;
}

esp_err_t sleep_atrakintas(uint8_t seconds)
{
    ESP_LOGI(TAG,"Sleep atrakintas");
    disable_interrupts();

    if (sim_at("AT+CPOWD=1", "OK", SIM_AT_TIMEOUT_MS, NULL, 0)) ESP_LOGI(TAG, "Išjungtas SIM7070G");

    gpio_hold_en(SCOOTER); // reikia tik Sleep atrakintui

    if(TESTING)
    {
            g_sim_handle = xTaskGetCurrentTaskHandle();
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(seconds*1000));
    }else{
        esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
        esp_light_sleep_start();
    }
    gpio_hold_dis(SCOOTER); // reikia tik Sleep atrakintui

    ESP_LOGI(TAG, "Pabudo iš sleep_atrakintas");
    return ESP_OK;
}

esp_err_t sleep_vogiamas(uint8_t seconds)
{
    ESP_LOGI(TAG,"Sleep vogiamas");
    disable_interrupts();

    sim_vogiamas_sleep(VogiamasSleepCycle);

    if(TESTING)
    {
                ESP_LOGI(TAG,"Test sleep");
                vTaskDelay(pdMS_TO_TICKS(15000));
            // g_sim_handle = xTaskGetCurrentTaskHandle();
            // ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(seconds*1000));
    }else{
        esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
        esp_light_sleep_start();
    }

    
    if (sim_at("AT+CSCLK=0", "OK", SIM_AT_TIMEOUT_MS, NULL, 0)) {
        ESP_LOGI(TAG, "Pagreitintas laikrodis");
    }else {
        ESP_LOGW(TAG, "Nepavyko pagreitinti laikrodžio, neveiks GNSS");
    }

    ESP_LOGI(TAG, "Pabudo iš sleep_vogiamas");
    return ESP_OK;
    

}