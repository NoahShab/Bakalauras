/*
ESP IDF oficialių pavyzdžių kodas
https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/nimble/bleprph/main
https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/ble/gatt_server


*/
#ifndef __BLE_FUNKCIJOS_H__
#define __BLE_FUNKCIJOS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "nimble/ble.h"
#include "host/ble_gatt.h"
#include "modlog/modlog.h"

#include "global_settings.h"

#define BLE_RX_BUF_LEN 128
extern char ble_rx_buf[BLE_RX_BUF_LEN];

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int  gatt_svr_init(void);
void ble_advertise(void);
void ble_dispatch(char *body);
void ble_funkcijos_init(void);


#ifdef __cplusplus
}
#endif

#endif