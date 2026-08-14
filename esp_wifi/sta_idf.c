/* STA bring-up for Klin apps under ESP-IDF v5.x.
 * Explicit steps; caller sees every return code via Klin wrappers.
 * Default = DHCP (dynamic). Optional static IP disables DHCP on STA netif.
 */
#include "sta_idf.h"

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

#define KLIN_WIFI_GOT_IP_BIT BIT0
#define KLIN_WIFI_FAIL_BIT   BIT1
#define KLIN_WIFI_ASSOC_BIT  BIT2
#define KLIN_WIFI_SCAN_DONE_BIT BIT3
#define KLIN_WIFI_HOSTNAME_MAX 32
#define KLIN_WIFI_SCAN_MAX 16
#define KLIN_WIFI_SSID_MAX 33

typedef struct {
    char ssid[KLIN_WIFI_SSID_MAX];
    int8_t rssi;
    uint8_t channel;
    uint8_t authmode;
} klin_wifi_scan_row_t;

static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *s_sta_netif;
static int s_retry_num;
static uint32_t s_ip_u32;
static uint32_t s_gw_u32;
static uint32_t s_mask_u32;
static int s_inited;
static int s_associated;
static int s_want_connect;
static int s_use_static;
static uint32_t s_static_ip;
static uint32_t s_static_gw;
static uint32_t s_static_mask;
static char s_hostname[KLIN_WIFI_HOSTNAME_MAX];
static klin_wifi_scan_row_t s_scan[KLIN_WIFI_SCAN_MAX];
static int s_scan_count;

static void klin_wifi_fmt_ipv4(char *buf, size_t n, uint32_t a)
{
    snprintf(buf, n, "%u.%u.%u.%u", (unsigned)(a & 0xffu),
             (unsigned)((a >> 8) & 0xffu), (unsigned)((a >> 16) & 0xffu),
             (unsigned)((a >> 24) & 0xffu));
}

static esp_err_t klin_wifi_apply_static_ip(void)
{
    esp_err_t err;
    esp_netif_ip_info_t ip_info;

    if (s_sta_netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_use_static) {
        return ESP_OK;
    }

    err = esp_netif_dhcpc_stop(s_sta_netif);
    if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        return err;
    }

    memset(&ip_info, 0, sizeof(ip_info));
    ip_info.ip.addr = s_static_ip;
    ip_info.gw.addr = s_static_gw;
    ip_info.netmask.addr = s_static_mask;
    return esp_netif_set_ip_info(s_sta_netif, &ip_info);
}

static esp_err_t klin_wifi_apply_hostname(void)
{
    if (s_sta_netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_hostname[0] == '\0') {
        return ESP_OK;
    }
    return esp_netif_set_hostname(s_sta_netif, s_hostname);
}

static void klin_wifi_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        /* Only connect when sta_connect asked — scan_start must not assoc. */
        if (s_want_connect) {
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_CONNECTED) {
        s_associated = 1;
        xEventGroupSetBits(s_wifi_event_group, KLIN_WIFI_ASSOC_BIT);
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_associated = 0;
        xEventGroupClearBits(s_wifi_event_group, KLIN_WIFI_ASSOC_BIT);
        if (s_want_connect && s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
        } else if (s_want_connect) {
            xEventGroupSetBits(s_wifi_event_group, KLIN_WIFI_FAIL_BIT);
        }
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_SCAN_DONE) {
        (void)event_data;
        xEventGroupSetBits(s_wifi_event_group, KLIN_WIFI_SCAN_DONE_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        s_ip_u32 = (uint32_t)event->ip_info.ip.addr;
        s_gw_u32 = (uint32_t)event->ip_info.gw.addr;
        s_mask_u32 = (uint32_t)event->ip_info.netmask.addr;
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, KLIN_WIFI_GOT_IP_BIT);
    }
}

int klin_wifi_sta_set_static_ip(uint32_t ip, uint32_t gw, uint32_t netmask)
{
    if (ip == 0 && gw == 0 && netmask == 0) {
        s_use_static = 0;
        s_static_ip = 0;
        s_static_gw = 0;
        s_static_mask = 0;
        return (int)ESP_OK;
    }

    s_use_static = 1;
    s_static_ip = ip;
    s_static_gw = gw;
    s_static_mask = netmask;

    if (s_sta_netif != NULL) {
        return (int)klin_wifi_apply_static_ip();
    }
    return (int)ESP_OK;
}

int klin_wifi_sta_set_hostname(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        s_hostname[0] = '\0';
        return (int)ESP_OK;
    }

    strncpy(s_hostname, name, sizeof(s_hostname) - 1);
    s_hostname[sizeof(s_hostname) - 1] = '\0';

    if (s_sta_netif != NULL) {
        return (int)klin_wifi_apply_hostname();
    }
    return (int)ESP_OK;
}

int klin_wifi_sta_init(void)
{
    esp_err_t err;

    if (s_inited) {
        return (int)ESP_OK;
    }

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            return (int)err;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return (int)err;
    }

    err = esp_netif_init();
    if (err != ESP_OK) {
        return (int)err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return (int)err;
    }

    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (s_sta_netif == NULL) {
        return (int)ESP_FAIL;
    }

    err = klin_wifi_apply_hostname();
    if (err != ESP_OK) {
        return (int)err;
    }
    err = klin_wifi_apply_static_ip();
    if (err != ESP_OK) {
        return (int)err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        return (int)err;
    }

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        return (int)ESP_ERR_NO_MEM;
    }

    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                     &klin_wifi_event_handler, NULL);
    if (err != ESP_OK) {
        return (int)err;
    }
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                     &klin_wifi_event_handler, NULL);
    if (err != ESP_OK) {
        return (int)err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        return (int)err;
    }

    s_inited = 1;
    s_ip_u32 = 0;
    s_gw_u32 = 0;
    s_mask_u32 = 0;
    s_associated = 0;
    s_want_connect = 0;
    s_retry_num = 0;
    s_scan_count = 0;
    return (int)ESP_OK;
}

int klin_wifi_sta_connect(const char *ssid, const char *pass)
{
    wifi_config_t wifi_config;
    esp_err_t err;

    if (!s_inited || ssid == NULL) {
        return (int)ESP_ERR_INVALID_STATE;
    }

    /* Re-apply in case netif DHCP was restarted by IDF between calls. */
    err = klin_wifi_apply_hostname();
    if (err != ESP_OK) {
        return (int)err;
    }
    err = klin_wifi_apply_static_ip();
    if (err != ESP_OK) {
        return (int)err;
    }

    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (pass != NULL) {
        strncpy((char *)wifi_config.sta.password, pass,
                sizeof(wifi_config.sta.password) - 1);
    }

    s_retry_num = 0;
    s_ip_u32 = 0;
    s_gw_u32 = 0;
    s_mask_u32 = 0;
    s_associated = 0;
    s_want_connect = 1;
    xEventGroupClearBits(s_wifi_event_group, KLIN_WIFI_GOT_IP_BIT |
                                                 KLIN_WIFI_FAIL_BIT |
                                                 KLIN_WIFI_ASSOC_BIT);

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        return (int)err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        return (int)err;
    }

    /* STA_START handler connects when s_want_connect; also connect if already up. */
    err = esp_wifi_connect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        return (int)err;
    }
    return (int)ESP_OK;
}

int klin_wifi_sta_wait_connected(int timeout_ms)
{
    TickType_t ticks;
    EventBits_t bits;

    if (!s_inited || s_wifi_event_group == NULL) {
        return (int)ESP_ERR_INVALID_STATE;
    }

    if (timeout_ms < 0) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS((uint32_t)timeout_ms);
    }

    bits = xEventGroupWaitBits(s_wifi_event_group,
                               KLIN_WIFI_ASSOC_BIT | KLIN_WIFI_FAIL_BIT,
                               pdFALSE, pdFALSE, ticks);

    if (bits & KLIN_WIFI_ASSOC_BIT) {
        return (int)ESP_OK;
    }
    if (bits & KLIN_WIFI_FAIL_BIT) {
        return (int)ESP_FAIL;
    }
    return (int)ESP_ERR_TIMEOUT;
}

int klin_wifi_sta_connected(void)
{
    return s_associated ? 1 : 0;
}

int klin_wifi_sta_wait_ip(int timeout_ms)
{
    TickType_t ticks;
    EventBits_t bits;

    if (!s_inited || s_wifi_event_group == NULL) {
        return (int)ESP_ERR_INVALID_STATE;
    }

    if (timeout_ms < 0) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS((uint32_t)timeout_ms);
    }

    bits = xEventGroupWaitBits(s_wifi_event_group,
                               KLIN_WIFI_GOT_IP_BIT | KLIN_WIFI_FAIL_BIT,
                               pdFALSE, pdFALSE, ticks);

    if (bits & KLIN_WIFI_GOT_IP_BIT) {
        return (int)ESP_OK;
    }
    if (bits & KLIN_WIFI_FAIL_BIT) {
        return (int)ESP_FAIL;
    }
    return (int)ESP_ERR_TIMEOUT;
}

uint32_t klin_wifi_sta_ip_u32(void)
{
    return s_ip_u32;
}

uint32_t klin_wifi_sta_gateway_u32(void)
{
    return s_gw_u32;
}

uint32_t klin_wifi_sta_netmask_u32(void)
{
    return s_mask_u32;
}

int klin_wifi_sta_disconnect(void)
{
    if (!s_inited) {
        return (int)ESP_ERR_INVALID_STATE;
    }
    s_want_connect = 0;
    return (int)esp_wifi_disconnect();
}

int klin_wifi_sta_stop(void)
{
    if (!s_inited) {
        return (int)ESP_ERR_INVALID_STATE;
    }
    s_want_connect = 0;
    return (int)esp_wifi_stop();
}

void klin_wifi_sta_log_ip(void)
{
    char ip[16];
    klin_wifi_fmt_ipv4(ip, sizeof(ip), s_ip_u32);
    printf("klin_wifi: ip %s\n", ip);
}

void klin_wifi_sta_log_ip_info(void)
{
    char ip[16];
    char gw[16];
    char mask[16];
    klin_wifi_fmt_ipv4(ip, sizeof(ip), s_ip_u32);
    klin_wifi_fmt_ipv4(gw, sizeof(gw), s_gw_u32);
    klin_wifi_fmt_ipv4(mask, sizeof(mask), s_mask_u32);
    printf("klin_wifi: ip %s gw %s mask %s\n", ip, gw, mask);
}

static int klin_wifi_sta_fill_ap_info(wifi_ap_record_t *ap)
{
    if (!s_inited || !s_associated || ap == NULL) {
        return (int)ESP_ERR_INVALID_STATE;
    }
    memset(ap, 0, sizeof(*ap));
    return (int)esp_wifi_sta_get_ap_info(ap);
}

int klin_wifi_sta_rssi(void)
{
    wifi_ap_record_t ap;
    if (klin_wifi_sta_fill_ap_info(&ap) != (int)ESP_OK) {
        return 0;
    }
    return (int)ap.rssi;
}

int klin_wifi_sta_channel(void)
{
    wifi_ap_record_t ap;
    if (klin_wifi_sta_fill_ap_info(&ap) != (int)ESP_OK) {
        return 0;
    }
    return (int)ap.primary;
}

int klin_wifi_sta_authmode(void)
{
    wifi_ap_record_t ap;
    if (klin_wifi_sta_fill_ap_info(&ap) != (int)ESP_OK) {
        return 0;
    }
    return (int)ap.authmode;
}

int klin_wifi_sta_ap_ssid(char *out, int max_len)
{
    wifi_ap_record_t ap;
    int n;
    int err;

    if (out == NULL || max_len <= 0) {
        return (int)ESP_ERR_INVALID_ARG;
    }
    err = klin_wifi_sta_fill_ap_info(&ap);
    if (err != (int)ESP_OK) {
        out[0] = '\0';
        return err;
    }
    n = (int)strlen((const char *)ap.ssid);
    if (n >= max_len) {
        n = max_len - 1;
    }
    memcpy(out, ap.ssid, (size_t)n);
    out[n] = '\0';
    return n;
}

void klin_wifi_sta_log_link(void)
{
    wifi_ap_record_t ap;
    if (klin_wifi_sta_fill_ap_info(&ap) != (int)ESP_OK) {
        printf("klin_wifi: link (not associated)\n");
        return;
    }
    printf("klin_wifi: link rssi=%d ch=%u auth=%u ssid=%s\n", (int)ap.rssi,
           (unsigned)ap.primary, (unsigned)ap.authmode, (const char *)ap.ssid);
}

int klin_wifi_scan_max(void)
{
    return KLIN_WIFI_SCAN_MAX;
}

int klin_wifi_scan_start(int timeout_ms)
{
    esp_err_t err;
    TickType_t ticks;
    EventBits_t bits;
    uint16_t ap_num;
    uint16_t number;
    wifi_ap_record_t records[KLIN_WIFI_SCAN_MAX];
    int i;

    if (!s_inited || s_wifi_event_group == NULL) {
        return (int)ESP_ERR_INVALID_STATE;
    }

    s_scan_count = 0;
    xEventGroupClearBits(s_wifi_event_group, KLIN_WIFI_SCAN_DONE_BIT);

    /* Scan needs the STA interface started; do not request association. */
    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        return (int)err;
    }

    err = esp_wifi_scan_start(NULL, false);
    if (err != ESP_OK) {
        return (int)err;
    }

    if (timeout_ms < 0) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS((uint32_t)timeout_ms);
    }

    bits = xEventGroupWaitBits(s_wifi_event_group, KLIN_WIFI_SCAN_DONE_BIT,
                               pdTRUE, pdFALSE, ticks);
    if (!(bits & KLIN_WIFI_SCAN_DONE_BIT)) {
        (void)esp_wifi_scan_stop();
        (void)esp_wifi_clear_ap_list();
        return (int)ESP_ERR_TIMEOUT;
    }

    err = esp_wifi_scan_get_ap_num(&ap_num);
    if (err != ESP_OK) {
        (void)esp_wifi_clear_ap_list();
        return (int)err;
    }

    number = ap_num;
    if (number > KLIN_WIFI_SCAN_MAX) {
        number = KLIN_WIFI_SCAN_MAX;
    }
    if (number == 0) {
        (void)esp_wifi_clear_ap_list();
        return (int)ESP_OK;
    }

    memset(records, 0, sizeof(records));
    err = esp_wifi_scan_get_ap_records(&number, records);
    if (err != ESP_OK) {
        (void)esp_wifi_clear_ap_list();
        return (int)err;
    }

    for (i = 0; i < (int)number; i++) {
        memset(&s_scan[i], 0, sizeof(s_scan[i]));
        strncpy(s_scan[i].ssid, (const char *)records[i].ssid,
                KLIN_WIFI_SSID_MAX - 1);
        s_scan[i].rssi = records[i].rssi;
        s_scan[i].channel = records[i].primary;
        s_scan[i].authmode = (uint8_t)records[i].authmode;
    }
    s_scan_count = (int)number;
    return (int)ESP_OK;
}

int klin_wifi_scan_count(void)
{
    return s_scan_count;
}

int klin_wifi_scan_rssi(int index)
{
    if (index < 0 || index >= s_scan_count) {
        return 0;
    }
    return (int)s_scan[index].rssi;
}

int klin_wifi_scan_channel(int index)
{
    if (index < 0 || index >= s_scan_count) {
        return 0;
    }
    return (int)s_scan[index].channel;
}

int klin_wifi_scan_authmode(int index)
{
    if (index < 0 || index >= s_scan_count) {
        return 0;
    }
    return (int)s_scan[index].authmode;
}

int klin_wifi_scan_ssid(int index, char *out, int max_len)
{
    int n;

    if (out == NULL || max_len <= 0) {
        return (int)ESP_ERR_INVALID_ARG;
    }
    if (index < 0 || index >= s_scan_count) {
        out[0] = '\0';
        return (int)ESP_ERR_INVALID_ARG;
    }

    n = (int)strlen(s_scan[index].ssid);
    if (n >= max_len) {
        n = max_len - 1;
    }
    memcpy(out, s_scan[index].ssid, (size_t)n);
    out[n] = '\0';
    return n;
}

void klin_wifi_scan_log(void)
{
    int i;
    for (i = 0; i < s_scan_count; i++) {
        printf("klin_wifi_scan: [%d] ch=%u rssi=%d auth=%u ssid=%s\n", i,
               (unsigned)s_scan[i].channel, (int)s_scan[i].rssi,
               (unsigned)s_scan[i].authmode, s_scan[i].ssid);
    }
}
