/*
Nojus Šabasevičius
Miego rėžimų funkcijos

*/


#ifndef __SLEEP_FUNKCIJOS_H__
#define __SLEEP_FUNKCIJOS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sim7070g.h"
#include "global_settings.h"
#include "io_configure.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_sleep.h"
#include "lis3dh.h"

esp_err_t sim_vogiamas_sleep(const char* cycle_value);
esp_err_t sleep_uzrakintas(uint8_t seconds);
esp_err_t sleep_atrakintas(uint8_t seconds);
esp_err_t sleep_vogiamas(uint8_t seconds);

#ifdef __cplusplus
}
#endif

#endif