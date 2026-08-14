/* SoftAP bring-up for Klin apps under ESP-IDF v5.x.
 * Explicit steps; caller sees every return code via Klin wrappers.
 * SoftAP-only (WIFI_MODE_AP). Do not mix with STA init in the same binary
 * for this tag — APSTA / dual is a later network track.
 */
#include "ap_idf.h"

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

#define KLIN_WIFI_AP_START_BIT BIT0
#define KLIN_WIFI_AP_MAX_CONN  4

static EventGroupHandle_t s_ap_event_group;
static esp_netif_t *s_ap_netif;
static uint32_t s_ip_u32;
static uint32_t s_gw_u32;
static uint32_t s_mask_u32;
static int s_inited;
static int s_started;
static int s_use_custom_ip;
static uint32_t s_custom_ip;
static uint32_t s_custom_gw;
static uint32_t s_custom_mask;

static void klin_wifi_fmt_ipv4(char *buf, size_t n, uint32_t a)
{
    snprintf(buf, n, "%u.%u.%u.%u", (unsigned)(a & 0xffu),
             (unsigned)((a >> 8) & 0xffu), (unsigned)((a >> 16) & 0xffu),
             (unsigned)((a >> 24) & 0xffu));
}

static void klin_wifi_ap_read_ip_info(void)
{
    esp_netif_ip_info_t ip_info;

    if (s_ap_netif == NULL) {
        return;
    }
    if (esp_netif_get_ip_info(s_ap_netif, &ip_info) != ESP_OK) {
        return;
    }
    s_ip_u32 = (uint32_t)ip_info.ip.addr;
    s_gw_u32 = (uint32_t)ip_info.gw.addr;
    s_mask_u32 = (uint32_t)ip_info.netmask.addr;
}

static esp_err_t klin_wifi_ap_apply_ip(void)
{
    esp_err_t err;
    esp_netif_ip_info_t ip_info;

    if (s_ap_netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_use_custom_ip) {
        return ESP_OK;
    }

    err = esp_netif_dhcps_stop(s_ap_netif);
    if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        return err;
    }

    memset(&ip_info, 0, sizeof(ip_info));
    ip_info.ip.addr = s_custom_ip;
    ip_info.gw.addr = s_custom_gw;
    ip_info.netmask.addr = s_custom_mask;
    err = esp_netif_set_ip_info(s_ap_netif, &ip_info);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_netif_dhcps_start(s_ap_netif);
    if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
        return err;
    }
    klin_wifi_ap_read_ip_info();
    return ESP_OK;
}

static void klin_wifi_ap_event_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        s_started = 1;
        klin_wifi_ap_read_ip_info();
        xEventGroupSetBits(s_ap_event_group, KLIN_WIFI_AP_START_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP) {
        s_started = 0;
        xEventGroupClearBits(s_ap_event_group, KLIN_WIFI_AP_START_BIT);
    }
}

int klin_wifi_ap_set_ip(uint32_t ip, uint32_t gw, uint32_t netmask)
{
    if (ip == 0 && gw == 0 && netmask == 0) {
        s_use_custom_ip = 0;
        s_custom_ip = 0;
        s_custom_gw = 0;
        s_custom_mask = 0;
        return (int)ESP_OK;
    }

    s_use_custom_ip = 1;
    s_custom_ip = ip;
    s_custom_gw = gw;
    s_custom_mask = netmask;

    if (s_ap_netif != NULL) {
        return (int)klin_wifi_ap_apply_ip();
    }
    return (int)ESP_OK;
}

int klin_wifi_ap_init(void)
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

    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_ap_netif == NULL) {
        return (int)ESP_FAIL;
    }

    err = klin_wifi_ap_apply_ip();
    if (err != ESP_OK) {
        return (int)err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        return (int)err;
    }

    s_ap_event_group = xEventGroupCreate();
    if (s_ap_event_group == NULL) {
        return (int)ESP_ERR_NO_MEM;
    }

    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                     &klin_wifi_ap_event_handler, NULL);
    if (err != ESP_OK) {
        return (int)err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        return (int)err;
    }

    s_inited = 1;
    s_started = 0;
    s_ip_u32 = 0;
    s_gw_u32 = 0;
    s_mask_u32 = 0;
    return (int)ESP_OK;
}

int klin_wifi_ap_start(const char *ssid, const char *pass, int channel)
{
    wifi_config_t wifi_config;
    esp_err_t err;
    size_t pass_len;

    if (!s_inited || ssid == NULL || ssid[0] == '\0') {
        return (int)ESP_ERR_INVALID_ARG;
    }
    if (channel < 1 || channel > 13) {
        return (int)ESP_ERR_INVALID_ARG;
    }

    err = klin_wifi_ap_apply_ip();
    if (err != ESP_OK) {
        return (int)err;
    }

    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = (uint8_t)strlen((char *)wifi_config.ap.ssid);
    wifi_config.ap.channel = (uint8_t)channel;
    wifi_config.ap.max_connection = KLIN_WIFI_AP_MAX_CONN;
    wifi_config.ap.beacon_interval = 100;

    pass_len = (pass == NULL) ? 0 : strlen(pass);
    if (pass_len == 0) {
        wifi_config.ap.password[0] = '\0';
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        if (pass_len < 8) {
            return (int)ESP_ERR_INVALID_ARG;
        }
        strncpy((char *)wifi_config.ap.password, pass,
                sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    s_started = 0;
    s_ip_u32 = 0;
    s_gw_u32 = 0;
    s_mask_u32 = 0;
    xEventGroupClearBits(s_ap_event_group, KLIN_WIFI_AP_START_BIT);

    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        return (int)err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        return (int)err;
    }
    return (int)ESP_OK;
}

int klin_wifi_ap_wait_started(int timeout_ms)
{
    TickType_t ticks;
    EventBits_t bits;

    if (!s_inited || s_ap_event_group == NULL) {
        return (int)ESP_ERR_INVALID_STATE;
    }

    if (timeout_ms < 0) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS((uint32_t)timeout_ms);
    }

    bits = xEventGroupWaitBits(s_ap_event_group, KLIN_WIFI_AP_START_BIT, pdFALSE,
                               pdFALSE, ticks);

    if (bits & KLIN_WIFI_AP_START_BIT) {
        klin_wifi_ap_read_ip_info();
        return (int)ESP_OK;
    }
    return (int)ESP_ERR_TIMEOUT;
}

int klin_wifi_ap_started(void)
{
    return s_started ? 1 : 0;
}

uint32_t klin_wifi_ap_ip_u32(void)
{
    return s_ip_u32;
}

uint32_t klin_wifi_ap_gateway_u32(void)
{
    return s_gw_u32;
}

uint32_t klin_wifi_ap_netmask_u32(void)
{
    return s_mask_u32;
}

int klin_wifi_ap_station_num(void)
{
    wifi_sta_list_t list;

    if (!s_inited || !s_started) {
        return 0;
    }
    if (esp_wifi_ap_get_sta_list(&list) != ESP_OK) {
        return 0;
    }
    return (int)list.num;
}

int klin_wifi_ap_stop(void)
{
    if (!s_inited) {
        return (int)ESP_ERR_INVALID_STATE;
    }
    return (int)esp_wifi_stop();
}

void klin_wifi_ap_log_ip(void)
{
    char ip[16];
    klin_wifi_fmt_ipv4(ip, sizeof(ip), s_ip_u32);
    printf("klin_wifi_ap: ip %s\n", ip);
}

void klin_wifi_ap_log_ip_info(void)
{
    char ip[16];
    char gw[16];
    char mask[16];
    klin_wifi_fmt_ipv4(ip, sizeof(ip), s_ip_u32);
    klin_wifi_fmt_ipv4(gw, sizeof(gw), s_gw_u32);
    klin_wifi_fmt_ipv4(mask, sizeof(mask), s_mask_u32);
    printf("klin_wifi_ap: ip %s gw %s mask %s\n", ip, gw, mask);
}
