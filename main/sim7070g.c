#include "sim7070g.h"

static const char *TAG = "SIM7070G";

static int XTRA_used = false;
int cold_start = 1;

static bool uart_wait_for(const char *needle, uint32_t timeout_ms, char *resp_buf, size_t resp_buf_len);

static void uart_tx(const char *send)
{
    uart_write_bytes(UART_PORT, send, strlen(send));
}

bool sim_at(const char *cmd, const char *expected, uint32_t timeout_ms, char *resp_buf, size_t resp_buf_len)
{
    uart_flush_input(UART_PORT);
    ESP_LOGD(TAG, ">> %s", cmd);
    uart_tx(cmd);
    uart_tx("\r\n");

    bool ok = uart_wait_for(expected, timeout_ms, resp_buf, resp_buf_len);
    if (!ok) ESP_LOGW(TAG, "No '%s' after: %s", expected ? expected : "?", cmd);
    return ok;
}

static bool uart_wait_for(const char *needle, uint32_t timeout_ms, char *resp_buf, size_t resp_buf_len)
{
    char buf[RX_BUF_LEN];
    int pos = 0;
    uint32_t elapsed = 0;
    memset(buf, 0, sizeof(buf));

    while (elapsed < timeout_ms) {
        int n = uart_read_bytes(UART_PORT, (uint8_t *)(buf + pos), sizeof(buf) - pos - 1, pdMS_TO_TICKS(UART_READ_TIMEOUT_MS));
        elapsed += UART_READ_TIMEOUT_MS;

        if (n > 0) {
            pos += n;
            buf[pos] = '\0';

            if (PRINT_UART) ESP_LOGI("UART_RX", ">>>>>>>>>> %s", buf);

            if (needle && strstr(buf, needle)) {
                if (resp_buf && resp_buf_len > 0) {
                    strncpy(resp_buf, buf, resp_buf_len - 1);
                    resp_buf[resp_buf_len - 1] = '\0';
                }
                return true;
            }

            if (pos >= (int)sizeof(buf) - 128) {
                memmove(buf, buf + pos - 256, 256);
                pos = 256;
                buf[pos] = '\0';
            }
        }
    }

    if (resp_buf && resp_buf_len > 0) {
        strncpy(resp_buf, buf, resp_buf_len - 1);
        resp_buf[resp_buf_len - 1] = '\0';
    }
    return false;
}

static bool sim_is_alive(void)
{
    uart_flush_input(UART_PORT);
    vTaskDelay(pdMS_TO_TICKS(70));  /// spaminam, kad pabustų
    uart_tx("AT\r\n");
    vTaskDelay(pdMS_TO_TICKS(70));
    uart_flush_input(UART_PORT);
    vTaskDelay(pdMS_TO_TICKS(70));
    uart_tx("AT\r\n");
    vTaskDelay(pdMS_TO_TICKS(70));
    return sim_at("AT", "OK", SIM_AT_TIMEOUT_SHORT, NULL, 0);
}

static void sim_pwrkey_pulse(void)
{
    ESP_LOGI(TAG, "Pulsing PWRKEY");
    gpio_set_level(PWRKEY, 1);
    vTaskDelay(pdMS_TO_TICKS(SIM_PWRKEY_PULSE_MS));
    gpio_set_level(PWRKEY, 0);
}

esp_err_t sim_power_on(void)
{
    if (sim_is_alive()) {
        ESP_LOGI(TAG, "SIM7070G jau ijungtas");
        return ESP_OK;
    }

    for (int i = 1; i <= SIM_POWER_RETRIES; i++) {
        ESP_LOGI(TAG, "Power-on bandymas %d/%d", i, SIM_POWER_RETRIES);
        sim_pwrkey_pulse();
        vTaskDelay(pdMS_TO_TICKS(SIM_BOOT_WAIT_MS));
        if (sim_is_alive()) return ESP_OK;
    }

    ESP_LOGW(TAG, "Bandoma su ilgesniu boot laiku");
    for (int i = 1; i <= 2; i++) {
        sim_pwrkey_pulse();
        vTaskDelay(pdMS_TO_TICKS(SIM_BOOT_WAIT_SLOWER_MS));
        uart_flush_input(UART_PORT);
        vTaskDelay(pdMS_TO_TICKS(500));
        uart_tx("AT\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
        uart_flush_input(UART_PORT);
        vTaskDelay(pdMS_TO_TICKS(500));
        uart_tx("AT\r\n");
        if (sim_is_alive()) return ESP_OK;
    }

    ESP_LOGE(TAG, "SIM7070G neatsako");
    return ESP_ERR_TIMEOUT;
}

esp_err_t sim_sms_configure(void)
{
    bool ok = true;
    ok &= sim_at("AT+CMGF=1", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    ok &= sim_at("AT+CSCS=\"GSM\"","OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    ok &= sim_at("AT+CSMS=1", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    ok &= sim_at("AT+CNMI=2,1,0,0,0", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    ok &= sim_at("AT+CPMS=\"SM\",\"SM\",\"SM\"", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    if (!ok) ESP_LOGE(TAG, "SMS konfiguracija nepavyko");
    return ok ? ESP_OK : ESP_FAIL;
}

esp_err_t catm1_mode_init(void)
{
    ESP_LOGI(TAG, "=== CAT-M1 MODE INIT ===");

    sim_at("AT+CMNB=1",  "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    sim_at("AT+CNMP=38", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CBANDCFG=\"CAT-M\",%d", CATM1_BAND);
    sim_at(cmd, "OK", SIM_AT_TIMEOUT_MS, NULL, 0);

    char resp[256];
    if (sim_at("AT+CGDCONT?", "OK", SIM_AT_TIMEOUT_MS, resp, sizeof(resp))) {
        if (!strstr(resp, CATM1_APN)) {
            snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", CATM1_APN);
            sim_at(cmd, "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
        }
    }

    if (sim_at("AT+CNACT?", "OK", SIM_AT_TIMEOUT_MS, resp, sizeof(resp))) {
        if (!strstr(resp, "0,1")) {
            sim_at("AT+CNACT=0,1", "OK", AT_LONG_MS, NULL, 0);
        }
    }

    sim_at("AT+CPSI?", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    sim_at("AT+CSQ",   "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    sim_at("AT+CSCA?", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);

    return ESP_OK;
}

esp_err_t catm1_send_sms(const char *number, const char *message)
{
    const char *dest = (number != NULL) ? number : owner_number;
    if (strlen(dest) == 0) {
        ESP_LOGE(TAG, "Nera numerio");
        return ESP_ERR_INVALID_ARG;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", dest);
    uart_flush_input(UART_PORT);
    uart_tx(cmd);
    uart_tx("\r");

    if (!uart_wait_for(">", 5000, NULL, 0)) {
        ESP_LOGE(TAG, "Nera '>' prompt");
        uart_tx("\x1B");
        return ESP_FAIL;
    }

    uart_tx(message);
    uart_tx("\x1A");

    char resp[128];
    bool ok = uart_wait_for("OK", SIM_SMS_SEND_TIMEOUT_MS, resp, sizeof(resp));
    if (ok) ESP_LOGI(TAG, "SMS issiusta -> %s", dest);
    else    ESP_LOGE(TAG, "SMS nepavyko: %s", resp);
    return ok ? ESP_OK : ESP_FAIL;
}

esp_err_t gnss_power_off(void)
{
    bool ok = sim_at("AT+CGNSPWR=0", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    if (ok) ESP_LOGI(TAG, "GNSS isjungtas");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(gpio_set_level(GNSS_ANT, GNSS_ANTENNA_OFF));
    return ok ? ESP_OK : ESP_FAIL;
}

static bool parse_cgnsinf(const char *resp, float *lat, float *lon)
{
    const char *p = strstr(resp, "+CGNSINF: ");
    if (!p) return false;
    p += strlen("+CGNSINF:");

    int run = 0, fix = 0;
    if (sscanf(p, "%d,%d", &run, &fix) < 2) return false;
    if (run == 0 || fix == 0) return false;

    for (int commas = 0; commas < 3 && *p; p++)
        if (*p == ',') commas++;

    float la = 0, lo = 0;
    if (sscanf(p, "%f,%f", &la, &lo) < 2) return false;
    *lat = la;
    *lon = lo;
    return true;
}

esp_err_t sim_gnss_get_fix(float *lat, float *lon)
{
    gnss_mode_init();
    uint32_t elapsed = 0;
    char resp[256];
    uint32_t timeout = cold_start ? TIMEOUT_COLD_START_S : TIMEOUT_WARM_START_S;

    while (elapsed < timeout) {
        if (sim_at("AT+CGNSINF", "OK", SIM_AT_TIMEOUT_MS, resp, sizeof(resp))) {
            if (parse_cgnsinf(resp, lat, lon)) {
                cold_start = 0;
                ESP_LOGI(TAG, "GNSS fix: %.6f, %.6f", *lat, *lon);
                gnss_power_off();
                return ESP_OK;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SIM_GNSS_POLL_MS));
        elapsed += SIM_GNSS_POLL_MS / 1000;
    }

    ESP_LOGW(TAG, "GNSS timeout");
    cold_start = 1;
    gnss_power_off();
    return ESP_ERR_TIMEOUT;
}

static void trim(char *s)
{
    int len = strlen(s);
    while (len > 0 && (s[len-1] == '\r' || s[len-1] == '\n' ||
                       s[len-1] == ' '  || s[len-1] == '\t'))
        s[--len] = '\0';
}

static bool parse_cmgr(const char *resp, char *sender, size_t sender_len, char *body, size_t body_len)
{
    const char *p = strstr(resp, "+CMGR:");
    if (!p) return false;

    p = strchr(p, '"');
    if (!p) return false;
    p = strchr(p + 1, '"');
    if (!p) return false;
    p++;
    if (*p == ',') p++;
    if (*p != '"') return false;
    p++;

    int i = 0;
    while (*p && *p != '"' && i < (int)sender_len - 1) sender[i++] = *p++;
    sender[i] = '\0';

    const char *nl = strchr(p, '\n');
    if (!nl) return false;
    nl++;

    i = 0;
    while (*nl && *nl != '\r' && *nl != '\n' && i < (int)body_len - 1) body[i++] = *nl++;
    body[i] = '\0';
    trim(body);

    return (strlen(body) > 0);
}

void handle_sms(int index)
{
    char cmd[32], resp[512], sender[32] = "", body[161] = "";

    snprintf(cmd, sizeof(cmd), "AT+CMGR=%d", index);
    if (!sim_at(cmd, "OK", 5000, resp, sizeof(resp))) {
        ESP_LOGE(TAG, "Nepavyko skaityti SMS %d", index);
        return;
    }

    if (!parse_cmgr(resp, sender, sizeof(sender), body, sizeof(body))) {
        ESP_LOGW(TAG, "Nepavyko parse SMS %d", index);
        snprintf(cmd, sizeof(cmd), "AT+CMGD=%d", index);
        sim_at(cmd, "OK", 3000, NULL, 0);
        return;
    }

     ///// Skirtingų SMS komandų paskirstymas

     if (!is_trusted_number(sender)) {/// Jei slaptazodis buvo teisingas arba numeris patikimas, atliekamos sms funkcijos
        if (trusted_password_check(body)) {
            trusted_number_set(sender);
            catm1_send_sms(sender, "Teisingas slaptazodis");
        } else {
            ESP_LOGW(TAG, "Untrusted sender: %s", sender);
        }
        snprintf(cmd, sizeof(cmd), "AT+CMGD=%d", index);
        sim_at(cmd, "OK", 3000, NULL, 0);
        return;
    }

    if (strcmp(body, "ECHO") == 0 || strcmp(body, "Echo") == 0 || strcmp(body, "echo") == 0) {
        echo(sender);
    } else if (strcmp(body, "HELP") == 0 || strcmp(body, "Help") == 0 || strcmp(body, "help") == 0) {
        help(sender);
    } else if (strcmp(body, "GNSS") == 0 || strcmp(body, "Gnss") == 0 || strcmp(body, "gnss") == 0) {
        GNSS(sender);
    } else if (strncmp(body, "AT", 2) == 0 || strncmp(body, "at", 2) == 0) {
        AT(sender,body);
    } else if(strcmp(body, "VOGIAMAS") == 0 || strcmp(body, "Vogiamas") == 0 || strcmp(body, "vogiamas") == 0 || strcmp(body, "VAGIAMAS") == 0 || strcmp(body, "Vagiamas") == 0 || strcmp(body, "vagiamas") == 0) {
        vagiamas();
    } else if (strcmp(body, "RASTAS") == 0 || strcmp(body, "Rastas") == 0 || strcmp(body, "rastas") == 0) {
        rastas();
    } else if (strcmp(body, "ATRAKINTI") == 0 || strcmp(body, "Atrakinti") == 0 || strcmp(body, "atrakinti") == 0) {
        atrakinimas();
    } else if (strcmp(body, "UZRAKINTI") == 0 || strcmp(body, "Uzrakinti") == 0 || strcmp(body, "uzrakinti") == 0) {
        uzrakinimas();
    } else if (strncmp(body, "NAUJAS_NUMERIS: ", 16) == 0) {
        trusted_number_set(body + 16);
        catm1_send_sms(sender, "Patikimas numeris atnaujintas");
    } else if (strncmp(body, "NAUJAS_SLAPTAZODIS: ", 20) == 0) {
        trusted_password_set(body + 20);
        catm1_send_sms(sender, "Slaptazodis atnaujintas");
    }else {
        ESP_LOGW(TAG, "Nezinoma SMS komanda: \"%s\"", body);
        char reply[200];
        snprintf(reply, sizeof(reply), "Nezinoma komanda: \"%.60s\"\nSend \"HELP\"", body);
        catm1_send_sms(sender, reply);
    }

    snprintf(cmd, sizeof(cmd), "AT+CMGD=%d", index);
    sim_at(cmd, "OK", 3000, NULL, 0);
}

void gnss_mode_init(void)
{
    ESP_LOGI(TAG, "=== GNSS MODE INIT ===");
    ESP_ERROR_CHECK(gpio_set_level(GNSS_ANT, GNSS_ANTENNA_ON));

    sim_at("AT+CSCLK=0", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    sim_at("AT+CEDRXS=0", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    sim_at("AT+CGNSMOD=1,0,0,1,0","OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    sim_at("AT+CLTS=1","OK", SIM_AT_TIMEOUT_MS, NULL, 0);

    GNSS_XTRA();

    const char *start_cmd = cold_start ? "AT+CGNSCOLD" : "AT+CGNSHOT";
    if (!sim_at(start_cmd, "OK", SIM_AT_TIMEOUT_MS, NULL, 0)) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        sim_at(start_cmd, "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    }
    ESP_LOGI(TAG, "GNSS %s", cold_start ? "cold start" : "hot start");

    sim_at("AT+CGNSPWR?", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
}

void GNSS_XTRA(void)
{
    XTRA_used = false;
    char resp[256];

    bool time_ok = false;
    for (int i = 0; i < 15; i++) {
        if (sim_at("AT+CCLK?", "OK", 2000, resp, sizeof(resp))) {
            if (!strstr(resp, "\"80/") && !strstr(resp, "\"00/")) {
                ESP_LOGI(TAG, "Laikas sinchronizuotas: %s", resp);
                time_ok = true;
                break;
            }
        }
        ESP_LOGI(TAG, "Laukiama laiko sinchronizacijos(%d/15)", i + 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    if (!time_ok) {
        ESP_LOGW(TAG, "Laikas nesinchronizuotas, XTRA praleistas");
        return;
    }

    if (sim_at("AT+CNACT?", "OK", 3000, resp, sizeof(resp))) {
        if (!strstr(resp, "+CNACT: 0,1")) {
            if (!sim_at("AT+CNACT=0,1", "ACTIVE", 30000, NULL, 0)) {
                ESP_LOGW(TAG, "CNACT nepavyko, XTRA praleistas");
                return;
            }
        }
    }

    ESP_LOGI(TAG, "Atsiunčiamas XTRA");
    if (!sim_at("AT+HTTPTOFS=\"http://iot1.xtracloud.net/xtra3ge_72h.bin\","
                "\"/customer/Xtra3.bin\"",
                "+HTTPTOFS:", 60000, resp, sizeof(resp)) || !strstr(resp, "200")) {
        ESP_LOGW(TAG, "XTRA atsisiuntimas nepavyko: %s", resp);
        return;
    }

    if (!sim_at("AT+CGNSCPY", "OK", 10000, resp, sizeof(resp))) {
        ESP_LOGW(TAG, "CGNSCPY nepavyko");
        return;
    }

    if (sim_at("AT+CGNSXTRA", "OK", 5000, resp, sizeof(resp))) {
        if (strstr(resp, "-")) {
            ESP_LOGW(TAG, "XTRA failas netinkamas");
            return;
        }
    }

    if (!sim_at("AT+CGNSXTRA=1", "OK", 8000, resp, sizeof(resp))) {
        ESP_LOGW(TAG, "CGNSXTRA=1 nepavyko");
        return;
    }

    ESP_LOGI(TAG, "XTRA ijungtas");
    XTRA_used = true;
}

int poll_for_cmti(void)
{
    char buf[128];
    memset(buf, 0, sizeof(buf));

    int n = uart_read_bytes(UART_PORT, (uint8_t *)buf, sizeof(buf) - 1, pdMS_TO_TICKS(200));
    if (n <= 0) return -1;
    buf[n] = '\0';

    const char *p = strstr(buf, "+CMTI:");
    if (!p) return -1;

    p = strchr(p, ',');
    if (!p) return -1;

    int idx = atoi(p + 1);
    ESP_LOGI(TAG, "+CMTI, index=%d", idx);
    return idx;
}

void process_stored_messages(void)
{
    ESP_LOGI(TAG, "Tikrinam neskaitytas SMS");
    char *resp_buf = malloc(2048);
    if (!resp_buf) return;

    bool buvo = false;
    if (sim_at("AT+CMGL=\"ALL\"", "OK", 5000, resp_buf, 2048)) {
        char *ptr = resp_buf;
        while ((ptr = strstr(ptr, "+CMGL: ")) != NULL) {
            vTaskDelay(pdMS_TO_TICKS(100));
            int idx = -1;
            if (sscanf(ptr, "+CMGL: %d", &idx) == 1) {
                handle_sms(idx);
                buvo = true;
            }
            ptr++;
        }
        if (buvo) clear_all_sms();
    }

    free(resp_buf);
}

void clear_all_sms(void)
{
    if (sim_at("AT+CMGD=1,4", "OK", 5000, NULL, 0))
        ESP_LOGI(TAG, "SMS atmintis išvalyta");
    else
        ESP_LOGW(TAG, "SMS atminties valymas nepavyko");
}

esp_err_t sim_init(void)
{
    ESP_LOGI(TAG, "SIM7070G init");
    esp_err_t ret = sim_power_on();
    if (ret != ESP_OK) return ret;
    sim_at("AT+IPR=115200", "OK", SIM_AT_TIMEOUT_MS, NULL, 0);
    catm1_mode_init();
    ESP_LOGI(TAG, "SIM7070G paruostas");
    return ESP_OK;
}

bool is_trusted_number(const char *number)
{
    return (strcmp(number, owner_number) == 0);
}

bool trusted_password_check(const char *candidate)
{
    return (strcmp(candidate, trusted_password) == 0);
}

/////------------
/////------------
/////------------ SMS funkcijos
/////------------
/////------------


esp_err_t trusted_number_set(const char *number)
{
    strncpy(owner_number, number, sizeof(owner_number) - 1);
    owner_number[sizeof(owner_number) - 1] = '\0';
    trusted_save();
    return ESP_OK;
}

esp_err_t trusted_password_set(const char *password)
{
    strncpy(trusted_password, password, sizeof(trusted_password) - 1);
    trusted_password[sizeof(trusted_password) - 1] = '\0';
    trusted_save();
    return ESP_OK;
}


void help(char *sender)
{
    catm1_send_sms(sender,
        "Komandos:\n"
        "ECHO\n"
        "HELP\n"
        "GNSS\n"
        "ATRAKINTI\n"
        "UZRAKINTI\n"
        "VAGIAMAS\n"
        "NAUJAS_NUMERIS: +xxx\n"
        "NAUJAS_SLAPTAZODIS: xxx\n"
        "AT...");
}

void echo(char *sender)
{
    catm1_send_sms(sender, "ECHO");
}

void AT(char *sender, char *body)
{
    char at_resp[256] = "";
    sim_at(body, "OK", 10000, at_resp, sizeof(at_resp));
    at_resp[160] = '\0';
    trim(at_resp);
    if (strlen(at_resp) == 0) strncpy(at_resp, "No response", sizeof(at_resp) - 1);
    catm1_send_sms(sender, at_resp);
}

void GNSS(char *sender)
{
    float lat = 0, lon = 0;
    char reply[160];
    if (sim_gnss_get_fix(&lat, &lon) == ESP_OK)
        snprintf(reply, sizeof(reply),
                 "Koordinates:\n%.6f,%.6f\nhttps://maps.google.com/?q=%.6f,%.6f",
                 lat, lon, lat, lon);
    else
        snprintf(reply, sizeof(reply), "GNSS: no fix (timeout)");
    catm1_send_sms(sender, reply);
}

void uzrakinimas()
{
     uzrakintas = true;
     vogiamas = false;
}
void atrakinimas()
{
    uzrakintas = false;
    vogiamas = false;
}
void vagiamas()
{
     uzrakintas = true;
     vogiamas = true;
}
void rastas()
{
     uzrakintas = true; 
     vogiamas = false;
}