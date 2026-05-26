#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "sim7070g.h"
#include "esp_timer.h"

#include "io_configure.h"
#include "lis3dh.h"
#include "sleep_funkcijos.h"
#include "sim7070g.h"
#include "flash_funkcijos.h"
#include "ble_funkcijos.h"

static const char *TAG = "MAIN";

static float lat;
static float lon;


void app_main(void)
{
    io_configure();
    lis3dh_init();
    busena_restore();
    ble_funkcijos_init();
    // vogiamas=false;
    // uzrakintas=true;
    //gnss_power_off();
    if (sim_at("AT+CPOWD=1", "OK", SIM_AT_TIMEOUT_MS, NULL, 0)) ESP_LOGI(TAG, "Išjungtas SIM7070G");

    
    ESP_LOGI(TAG,"Testing mode");
    //vTaskDelay(pdMS_TO_TICKS(15000)); 

    if (sim_init() != ESP_OK) ESP_LOGE(TAG, "SIM7070G init failed");
    if (sim_sms_configure() != ESP_OK) ESP_LOGE(TAG, "SMS configure failed");
    catm1_send_sms(PHONE_NUMBER, "Ijungtas modulis");
    sim_at("AT+CPSMS=0", "OK", SIM_AT_TIMEOUT_MS, NULL, 0 );   
    process_stored_messages();

    ESP_LOGI(TAG,"Ciklas------------------");
    while(1){

        if(vogiamas==true){
            ESP_LOGI(TAG,"Vogiamas rezimas prasidejo");
            catm1_send_sms(PHONE_NUMBER, "Paspirtukas yra vagiamas!");
            gpio_set_level(SCOOTER, SCOOTER_OFF);

            while(vogiamas==true) ////////// Vagiamas LOOP
            {
                GNSS(PHONE_NUMBER);
                int64_t start = esp_timer_get_time();
                while((esp_timer_get_time() - start) < (int64_t)ACTIVE_TIME * 1000000/4 && vogiamas==true) //// tiek sekundziu palaukia zinuciu. 4 kartus maziau nei kitiem rezimam, nes neisjungiamas pilnai SIM modulis
                {
                int idx = poll_for_cmti();
                if (idx >= 0) {
                   handle_sms(idx);
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                process_stored_messages();
                vTaskDelay(pdMS_TO_TICKS(200));
                }

                if(vogiamas==true)sleep_vogiamas(SLEEP_VOGIAMAS_S);

            }
            ESP_LOGI(TAG,"Vogiamas rezimas pasibaigė");
            busena_save();


        }else if(uzrakintas==true){ //////////// Uzrakintas LOOP
            ESP_LOGI(TAG,"Užrakintas režimas prasidėjo");
            catm1_send_sms(PHONE_NUMBER, "Paspirtukas yra uzrakintas");
            gpio_set_level(SCOOTER, SCOOTER_OFF);

            while(vogiamas==false && uzrakintas==true)
            {
                int64_t start = esp_timer_get_time();
                while((esp_timer_get_time() - start) < (int64_t)ACTIVE_TIME * 1000000 && vogiamas==false && uzrakintas==true) //// tiek sekundziu palaukia zinuciu
                {
                int idx = poll_for_cmti();
                if (idx >= 0) {
                   handle_sms(idx);
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                process_stored_messages();
                vTaskDelay(pdMS_TO_TICKS(200));
                }
                if(vogiamas==false && uzrakintas==true)sleep_uzrakintas(SLEEP_UZRAKINTAS_S);
               // vTaskDelay(pdMS_TO_TICKS(500));


            }
            ESP_LOGI(TAG,"Užrakintas režimas pasibaigė");
            busena_save();


        }else if(uzrakintas==false){ ////// Atrakintas LOOP
            ESP_LOGI(TAG,"Atrakintas režimas prasidėjo");
            catm1_send_sms(PHONE_NUMBER, "Paspirtukas yra atrakintas");
            gpio_set_level(SCOOTER, SCOOTER_ON);

            while(vogiamas==false && uzrakintas==false)
            {
                process_stored_messages();
                vTaskDelay(pdMS_TO_TICKS(500)); /// pakeist i miega
            }
            ESP_LOGI(TAG,"Atrakintas režimas pasibaigė");
            busena_save();
        }
        



    }
    
    return;
   
}

