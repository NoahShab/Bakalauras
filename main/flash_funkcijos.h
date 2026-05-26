/*
Nojus Šabasevičius
Įrenginio būsenos išsaugojimas ir atstatymas iš flash atminties
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html
https://github.com/espressif/esp-idf/blob/master/examples/storage/nvs/nvs_rw_value/main/nvs_value_example_main.c

*/

#ifndef __FLASH_FUNKCIJOS_H__
#define __FLASH_FUNKCIJOS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "esp_err.h"

#include "sim7070g.h"

extern char owner_number[20];
extern char trusted_password[32];

void busena_restore(void);
void busena_save(void);
void trusted_save(void);

#ifdef __cplusplus
}
#endif

#endif