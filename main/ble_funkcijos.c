#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "esp_log.h"

#include "ble_funkcijos.h"
#include "flash_funkcijos.h"

#define TAG "BLE"

char ble_rx_buf[BLE_RX_BUF_LEN];
bool ble_new_data = false;

static bool ble_trusted = false;   

static const ble_uuid128_t gatt_svr_svc_uuid =
    BLE_UUID128_INIT(0x2d, 0x71, 0xa2, 0x59, 0xb4, 0x58, 0xc8, 0x12,
                     0x99, 0x99, 0x43, 0x95, 0x12, 0x2f, 0x46, 0x59);

static const ble_uuid128_t gatt_svr_chr_uuid =
    BLE_UUID128_INIT(0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11,
                     0x22, 0x22, 0x22, 0x22, 0x33, 0x33, 0x33, 0x33);

static uint16_t gatt_svr_chr_val_handle;
static uint8_t  ble_addr_type;

static int gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int ble_gap_event(struct ble_gap_event *event, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type            = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid            = &gatt_svr_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid       = &gatt_svr_chr_uuid.u,
                .access_cb  = gatt_svc_access,
                .val_handle = &gatt_svr_chr_val_handle,
                .flags      = BLE_GATT_CHR_F_WRITE |
                              BLE_GATT_CHR_F_WRITE_NO_RSP |
                              BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 }, 
        },
    },
    { 0 }, 
};

static int gatt_svr_write(struct os_mbuf *om, uint16_t min_len,
                          uint16_t max_len, void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    return 0;
}

static int gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;

    switch (ctxt->op) {

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            MODLOG_DFLT(INFO, "Characteristic write; conn_handle=%d attr_handle=%d\n",
                        conn_handle, attr_handle);
        } else {
            MODLOG_DFLT(INFO, "Characteristic write by NimBLE stack; attr_handle=%d\n",
                        attr_handle);
        }

        if (attr_handle == gatt_svr_chr_val_handle) {
            uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
            if (om_len >= BLE_RX_BUF_LEN) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            rc = ble_hs_mbuf_to_flat(ctxt->om, ble_rx_buf, om_len, NULL);
            if (rc != 0) {
                return rc;
            }
            ble_rx_buf[om_len] = '\0';
            ESP_LOGI(TAG, "Gauta: %s", ble_rx_buf);

            ble_dispatch(ble_rx_buf);
            return 0;
        }
        goto unknown;

    case BLE_GATT_ACCESS_OP_READ_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            MODLOG_DFLT(INFO, "Characteristic read; conn_handle=%d attr_handle=%d\n",
                        conn_handle, attr_handle);
        } else {
            MODLOG_DFLT(INFO, "Characteristic read by NimBLE stack; attr_handle=%d\n",
                        attr_handle);
        }
        goto unknown;

    default:
        goto unknown;
    }

unknown:
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void ble_dispatch(char *body)
{
    if (!ble_trusted) {
        if (strcmp(body, DEFAULT_PASSWORD) == 0) {
            ble_trusted = true;
            ESP_LOGI(TAG, "Slaptazodis teisingas, prieiga suteikta");
        } else {
            ESP_LOGW(TAG, "Neteisingas slaptazodis: \"%s\"", body);
        }
        return;
    }

    if (strcmp(body, "VOGIAMAS") == 0 || strcmp(body, "Vogiamas") == 0 || strcmp(body, "vogiamas") == 0 ||
               strcmp(body, "VAGIAMAS") == 0 || strcmp(body, "Vagiamas") == 0 || strcmp(body, "vagiamas") == 0) {
        vagiamas();
    } else if (strcmp(body, "RASTAS") == 0 || strcmp(body, "Rastas") == 0 || strcmp(body, "rastas") == 0) {
        rastas();
    } else if (strcmp(body, "ATRAKINTI") == 0 || strcmp(body, "Atrakinti") == 0 || strcmp(body, "atrakinti") == 0) {
        atrakinimas();
    } else if (strcmp(body, "UZRAKINTI") == 0 || strcmp(body, "Uzrakinti") == 0 || strcmp(body, "uzrakinti") == 0) {
        uzrakinimas();
    } else if (strcmp(body, "LOGOUT") == 0 || strcmp(body, "Logout") == 0 || strcmp(body, "logout") == 0) {
        ble_trusted = false;
        ESP_LOGI(TAG, "Atsijungta");
    } else {
        ESP_LOGW(TAG, "Nezinoma BLE komanda: \"%s\"", body);
    }
}
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(DEBUG, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

void ble_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    memset(&fields, 0, sizeof(fields));
    fields.flags                 = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl            = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    name                = ble_svc_gap_device_name();
    fields.name         = (uint8_t *)name;
    fields.name_len     = strlen(name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields failed; rc=%d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
adv_params.itvl_min  = 160;   // 100ms (units of 0.625ms)
adv_params.itvl_max  = 160;

    rc = ble_gap_adv_start(ble_addr_type, NULL,
                           pdMS_TO_TICKS(BLE_ADV_DURATION_MS),
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_start failed; rc=%d", rc);
    }
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {

    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "Connection %s; status=%d",
                 event->connect.status == 0 ? "established" : "failed",
                 event->connect.status);
        if (event->connect.status != 0) {
            // connection failed, advertise again
            ble_advertise();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Disconnect; reason=%d", event->disconnect.reason);
        // advertise again after disconnect
        ble_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "Advertising complete");
        ble_advertise();   // restart advertising when it times out
        break;

    case BLE_GAP_EVENT_CONN_UPDATE:
        ESP_LOGI(TAG, "Connection updated; status=%d",
                 event->conn_update.status);
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "Subscribe; conn_handle=%d attr_handle=%d "
                 "reason=%d prevn=%d curn=%d previ=%d curi=%d",
                 event->subscribe.conn_handle,
                 event->subscribe.attr_handle,
                 event->subscribe.reason,
                 event->subscribe.prev_notify,
                 event->subscribe.cur_notify,
                 event->subscribe.prev_indicate,
                 event->subscribe.cur_indicate);
        break;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU update; conn_handle=%d cid=%d mtu=%d",
                 event->mtu.conn_handle,
                 event->mtu.channel_id,
                 event->mtu.value);
        break;

    default:
        break;
    }

    return 0;
}
static void ble_on_sync(void)
{
    int rc;

    rc = ble_hs_id_infer_auto(0, &ble_addr_type);
    assert(rc == 0);

    ble_advertise();
}

static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "BLE host reset; reason=%d", reason);
}

static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE host task started");
    nimble_port_run(); // blocks until nimble_port_stop() is called
    nimble_port_freertos_deinit();
}

void ble_funkcijos_init(void)
{
    int rc;

    nimble_port_init();

    ble_hs_cfg.reset_cb          = ble_on_reset;
    ble_hs_cfg.sync_cb           = ble_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb   = ble_store_util_status_rr;

    rc = gatt_svr_init();
    assert(rc == 0);

    rc = ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
    assert(rc == 0);


    nimble_port_freertos_init(ble_host_task);
}