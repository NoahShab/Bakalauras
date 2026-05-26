/*
Nojus Šabasevičius
SIM7070G modulio valdymas, GNSS, LTE-M funkcijos

https://edworks.co.kr/wp-content/uploads/2022/04/SIM7070_SIM7080_SIM7090-Series_AT-Command-Manual_V1.05.pdf
https://www.simcom.com/product/SIM7070G.html - SIM7070_SIM7080_SIM7090 Series_Low Power Mode_Application Note_V1.02, SIM7070_SIM7080_SIM7090 Series_GNSS_Application Note_V1.02

*/

#ifndef __SIM7070G_H__
#define __SIM7070G_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sleep_funkcijos.h"
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

#include "flash_funkcijos.h"

extern int start_state;

#define CATM1_BAND       20  
#define CATM1_APN        "iot.1nce.net"   

#define SIM_POWER_RETRIES       3
#define SIM_PWRKEY_PULSE_MS     600   // SIM7070G iš duomenų lapo: Ton
#define SIM_BOOT_WAIT_MS        1900   // wait after pulse before first AT
#define SIM_BOOT_WAIT_SLOWER_MS 2500   // wait after pulse before first AT
#define SIM_AT_TIMEOUT_SHORT    500
#define SIM_AT_TIMEOUT_MS       3000
#define AT_LONG_MS              15000
#define SIM_SMS_SEND_TIMEOUT_MS 60000  // spec: 60 s max
#define SIM_GNSS_POLL_MS        2000   // interval between AT+CGNSINF polls
#define UART_READ_TIMEOUT_MS    100
#define RX_BUF_LEN              1024

#define TIMEOUT_HOT_START_S  60
#define TIMEOUT_WARM_START_S 15
#define TIMEOUT_COLD_START_S 60



esp_err_t sim_power_on(void);

esp_err_t sim_init(void);

esp_err_t sim_sms_configure(void);

esp_err_t sim_sms_send(const char *number, const char *text);

void sim_sms_task(void *pvParameters);
void gnss_mode_init(void);
void process_stored_messages(void);
void clear_all_sms(void);
void GNSS_XTRA(void);

esp_err_t gnss_power_on(void);

esp_err_t gnss_power_off(void);

esp_err_t sim_gnss_get_fix(float *lat, float *lon);

esp_err_t catm1_mode_init(void);
esp_err_t catm1_send_sms(const char *number, const char *message);

bool sim_at(const char *cmd, const char *expected, uint32_t timeout_ms, char *resp_buf, size_t resp_buf_len);

void sim_set_owner(const char *number);


bool is_trusted_number(const char *number);
bool trusted_password_check(const char *candidate);
esp_err_t trusted_number_set(const char *number);
esp_err_t trusted_password_set(const char *password);
void help(char *sender);
void echo(char *sender);
void AT(char *sender, char *body);
void GNSS(char *sender);
void uzrakinimas();
void atrakinimas();
void vagiamas();
void rastas();
int poll_for_cmti(void);
void handle_sms(int index);


#ifdef __cplusplus
}
#endif

#endif