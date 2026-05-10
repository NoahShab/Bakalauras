#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "sim7070g.h"

#include "io_configure.h"
#include "lis3dh.h"
#include "sleep_funkcijos.h"


void app_main(void)
{
    io_configure();
    lis3dh_init();
    vogiamas=false;
    uzrakintas=true;

    ESP_LOGI("TAG","Testing mode");
    vTaskDelay(pdMS_TO_TICKS(15000)); 

    //// Užrakinto modulio testas
    gpio_set_level(GNSS_ANT, GNSS_ANTENNA_ON);vTaskDelay(pdMS_TO_TICKS(1000));gpio_set_level(GNSS_ANT, GNSS_ANTENNA_OFF);
    gpio_set_level(SCOOTER, SCOOTER_OFF);
    sleep_uzrakintas(20);

    //// Atrakinto modulio testas
    gpio_set_level(GNSS_ANT, GNSS_ANTENNA_ON);vTaskDelay(pdMS_TO_TICKS(1000));gpio_set_level(GNSS_ANT, GNSS_ANTENNA_OFF);
    gpio_set_level(SCOOTER,SCOOTER_ON);
    sleep_atrakintas(8);

    //// Vogiamo modulio testas
    if (sim_init(SIM_MODE_LTE_M) != ESP_OK) ESP_LOGE("TAG", "SIM7070G init failed");
    if (sim_sms_configure() != ESP_OK) ESP_LOGE("TAG", "SMS configure failed");
    catm1_send_sms(PHONE_NUMBER, "ESP32 System Online. Send GNSS to request location");

    gpio_set_level(GNSS_ANT, GNSS_ANTENNA_ON);vTaskDelay(pdMS_TO_TICKS(1000));gpio_set_level(GNSS_ANT, GNSS_ANTENNA_OFF);
    gpio_set_level(SCOOTER, SCOOTER_OFF);
    sleep_vogiamas(20);

    gpio_set_level(GNSS_ANT, GNSS_ANTENNA_ON);vTaskDelay(pdMS_TO_TICKS(1000));gpio_set_level(GNSS_ANT, GNSS_ANTENNA_OFF);

    vTaskDelay(pdMS_TO_TICKS(30000));
}



