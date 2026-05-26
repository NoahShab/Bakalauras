#include "flash_funkcijos.h"
#include "global_settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "Flash";

char owner_number[20] = PHONE_NUMBER;
char trusted_password[32] = DEFAULT_PASSWORD;

void busena_restore(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    nvs_handle_t h;
    if (nvs_open("scooter_state", NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open nepavyko");
        return;
    }
    uint8_t vog = 0, uzr = 1;
    nvs_get_u8(h, "vogiamas",   &vog);
    nvs_get_u8(h, "uzrakintas", &uzr);
    nvs_close(h);

    nvs_handle_t th;
    if (nvs_open("trusted", NVS_READONLY, &th) == ESP_OK) {
        size_t len = sizeof(owner_number);
        if (nvs_get_str(th, "number", owner_number, &len) != ESP_OK)
            strncpy(owner_number, PHONE_NUMBER, sizeof(owner_number) - 1);
        len = sizeof(trusted_password);
        if (nvs_get_str(th, "password", trusted_password, &len) != ESP_OK)
            strncpy(trusted_password, DEFAULT_PASSWORD, sizeof(trusted_password) - 1);
        nvs_close(th);
    }

    vogiamas   = (bool)vog;
    uzrakintas = (bool)uzr;

    ESP_LOGI(TAG, "Atkurta: vogiamas=%d uzrakintas=%d", vogiamas, uzrakintas);
}

void busena_save(void)
{
    nvs_handle_t h;
    if (nvs_open("scooter_state", NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open nepavyko");
        return;
    }

    nvs_set_u8(h, "vogiamas",   (uint8_t)vogiamas);
    nvs_set_u8(h, "uzrakintas", (uint8_t)uzrakintas);
    nvs_commit(h);
    nvs_close(h);
    trusted_save();

    ESP_LOGI(TAG, "Issaugota: vogiamas=%d uzrakintas=%d", vogiamas, uzrakintas);
}

void trusted_save(void)
{
    nvs_handle_t h;
    if (nvs_open("trusted", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_str(h, "number", owner_number);
    nvs_set_str(h, "password", trusted_password);
    nvs_commit(h);
    nvs_close(h);
}