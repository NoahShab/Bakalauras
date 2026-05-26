/*
Nojus Šabasevičius
Periferijos kanalų konfigūravimas
https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/gpio.html
https://www.st.com/resource/en/datasheet/lis3dh.pdf
*/



#ifndef __LIS3DH_H__
#define __LIS3DH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "global_settings.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "rom/ets_sys.h"
#include "io_configure.h"
#include "esp_timer.h"

#define LIS3DH_ADDR     0x19

// Naudojami registrai
#define REG_WHO_AM_I    0x0F
#define REG_CTRL1       0x20
#define REG_CTRL2       0x21
#define REG_CTRL3       0x22
#define REG_CTRL4       0x23
#define REG_CTRL5       0x24
#define REG_INT1_CFG    0x30
#define REG_INT1_SRC    0x31
#define REG_INT1_THS    0x32
#define REG_INT1_DUR    0x33
#define REG_OUT_X_L     0x28

#define TILT_THRESHOLD_MG   250u // 250 mG jautrumas
#define INT_THS_BITS         0x0F // Threshold_mG / 16 (nes 2G sensitivity) = 15.625; 15 = 0x0F 
extern TaskHandle_t g_sim_handle;

esp_err_t write_reg(uint8_t reg, uint8_t val);
esp_err_t read_reg(uint8_t reg, uint8_t *out, size_t len);
bool wreg(uint8_t reg, uint8_t val); 
void I2C_init(void);
esp_err_t lis3dh_init(void);
void lis3dh_int_received(void);  
void interrupt_task(void *pvParameters);
esp_err_t enable_interrupts(void);
esp_err_t disable_interrupts(void);

#ifdef __cplusplus
}
#endif

#endif  

