#include "io_configure.h"

static const char *TAG = "IO_CONFIGURE";


void io_configure(void)
{
    pwrkey_pin();         
    gnss_antenna_pin();   
    scooter_pin();       
    uart_pins();    

    interrupt_pin();

    ESP_LOGI(TAG, "IO PINS CONFIGURED");
}

void pwrkey_pin(void)
{
    gpio_config_t pwrkey_config = {
        .pin_bit_mask = (1ULL << PWRKEY),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    
    ESP_ERROR_CHECK(gpio_config(&pwrkey_config));
    
    // Start with PWRKEY high (inactive)
    ESP_ERROR_CHECK(gpio_set_level(PWRKEY, 0));
}


void gnss_antenna_pin(void)
{
    gpio_config_t gnss_config = {
        .pin_bit_mask = (1ULL << GNSS_ANT),
        .mode         = GPIO_MODE_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    
    ESP_ERROR_CHECK(gpio_config(&gnss_config));
    
    ESP_ERROR_CHECK(gpio_set_level(GNSS_ANT, GNSS_ANTENNA_OFF));
}

void scooter_pin(void)
{
    gpio_config_t scooter_config = {
        .pin_bit_mask = (1ULL << SCOOTER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    
    ESP_ERROR_CHECK(gpio_config(&scooter_config));

    ESP_ERROR_CHECK(gpio_set_level(SCOOTER, SCOOTER_OFF));

}

void uart_pins(void)
{
    const uart_config_t cfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX, UART_RX,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE * 2,
                                        0, 0, NULL, 0));
}


void interrupt_pin(void)
{    

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << INTERRUPT_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_POSEDGE,
    };

    ESP_ERROR_CHECK(gpio_config(&io));
    
}

