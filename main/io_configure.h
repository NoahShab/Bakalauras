/*
Nojus Šabasevičius
Periferijos kanalų konfigūravimas
https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/gpio.html
*/

#ifndef __IO_CONFIGURE_H__
#define __IO_CONFIGURE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"
#include "esp_log.h"
#include "global_settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


void io_configure(void);
void pwrkey_pin(void);
void uart_pins(void);
void interrupt_pin(void);
void gpio_isr_handler(void *arg);

void gnss_antenna_pin(void);
#define GNSS_ANTENNA_ON 0
#define GNSS_ANTENNA_OFF 1

void scooter_pin(void);
#define SCOOTER_ON 0
#define SCOOTER_OFF 1


#ifdef __cplusplus
}
#endif

#endif  