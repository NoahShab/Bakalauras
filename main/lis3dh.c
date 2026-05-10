#include "lis3dh.h"

static const char *TAG = "LIS3DH";
static i2c_master_dev_handle_t dev;

static uint8_t src = 0;

static TaskHandle_t interrupt_task_handle = NULL;

esp_err_t write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    esp_err_t ret = i2c_master_transmit(dev, buf, sizeof(buf), -1);
    if (ret != ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(5));
        ret = i2c_master_transmit(dev, buf, sizeof(buf), -1);
    }
    return ret;
}

esp_err_t read_reg(uint8_t reg, uint8_t *out, size_t len)
{
    reg |= 0x80;
    return i2c_master_transmit_receive(dev, &reg, 1, out, len, -1);
}

bool wreg(uint8_t reg, uint8_t val)
{
    if (write_reg(reg, val) != ESP_OK) {
        ESP_LOGE(TAG, "write_reg(0x%02X, 0x%02X) fail", reg, val);
        return false;
    }
    return true;
}

void I2C_init(void)
{
    i2c_master_bus_handle_t bus;
    i2c_master_bus_config_t bcfg = {
        .i2c_port              = -1,
        .sda_io_num            = PIN_SDA,
        .scl_io_num            = PIN_SCL,
        .clk_source            = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt     = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bcfg, &bus));

    i2c_device_config_t dcfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = LIS3DH_ADDR,
        .scl_speed_hz    = 10000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dcfg, &dev));
}

esp_err_t enable_interrupts(void)
{
    read_reg(REG_INT1_SRC, &src, 1);

    if (!wreg(REG_CTRL3, 0b01000000)) return ESP_FAIL;

    return ESP_OK;
}

esp_err_t disable_interrupts(void)
{

    if (!wreg(REG_CTRL3, 0b00000000)) return ESP_FAIL;

    return ESP_OK;
}

esp_err_t lis3dh_init(void)
{
    ESP_LOGI(TAG, "LIS3DH init start");

    I2C_init();

    uint8_t who = 0; /// pagal duomenų lapą slave adresas turi but 0x33

    for (int i = 0; i < 5; i++) {
        if( read_reg(REG_WHO_AM_I, &who, 1) ==ESP_OK && who == 0x33) break; ;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (who==0) {
        ESP_LOGE(TAG, "LIS3DH nerastas");
        return ESP_FAIL; 
    }else if(who != 0x33) {
        ESP_LOGE(TAG, "LIS3DH nerastas, rasta 0x%02X", who);
        return ESP_FAIL; 
    }
    ESP_LOGI(TAG, "LIS3DH rastas. WHO_AM_I=0x%02X", who);

    if (!wreg(REG_CTRL1,    0b00100111)) return ESP_FAIL;
    if (!wreg(REG_CTRL2,    0b11000001)) return ESP_FAIL;
    if (!wreg(REG_CTRL3,    0b01000000)) return ESP_FAIL;
    if (!wreg(REG_CTRL4,    0b10000000)) return ESP_FAIL;
    if (!wreg(REG_CTRL5,    0b00001000)) return ESP_FAIL;

    // 0x7F nurodo, kad paskutinis bit'as yra 0, taip nurodau jautrumą 16mG / bitą
    if (!wreg(REG_INT1_THS, INT_THS_BITS & 0x7F)) return ESP_FAIL;
    // ant pirmojo mėginio generuojama pertrauktis
    if (!wreg(REG_INT1_DUR, 0 & 0x7F)) return ESP_FAIL;
    if (!wreg(REG_INT1_CFG, 0b00101010)) return ESP_FAIL;

    read_reg(REG_INT1_SRC, &src, 1); // išvalo interrupt registrą
  
    xTaskCreate(interrupt_task, "interrupt_task", 2048, NULL,2,
                &interrupt_task_handle);

    return ESP_OK;
}

void IRAM_ATTR gpio_isr_handler(void *arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    vTaskNotifyGiveFromISR(
        interrupt_task_handle,
        &higher_priority_task_woken
    );

    if (higher_priority_task_woken)
    {
        portYIELD_FROM_ISR();
    }
}


void interrupt_task(void *arg)
{   

    while (1)
    {
        while(!gpio_get_level(INTERRUPT_PIN))vTaskDelay(pdMS_TO_TICKS(500));
        ESP_LOGI(TAG,"LIS3DH interrupt");

        vogiamas=true;/// NOTE reikia funkcijos, kuri atsižvelgia, ar buvo kelių s intervalai

        if (g_sim_handle) { /// Įspėja testavimo atveju sistemas, kad gautas interrupt
             xTaskNotifyGive(g_sim_handle);
        }
        
        read_reg(REG_INT1_SRC, &src, 1);
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

extern TaskHandle_t g_sim_task_handle; 



