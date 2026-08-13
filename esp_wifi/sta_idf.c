/* STA bring-up for Klin apps under ESP-IDF v5.x.
 * Explicit steps; caller sees every return code via Klin wrappers.
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

#define KLIN_WIFI_CONNECTED_BIT BIT0
#define KLIN_WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *s_sta_netif;
static int s_retry_num;
static uint32_t s_ip_u32;
static int s_inited;

static void klin_wifi_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
        } else {
            xEventGroupSetBits(s_wifi_event_group, KLIN_WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        s_ip_u32 = (uint32_t)event->ip_info.ip.addr;
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, KLIN_WIFI_CONNECTED_BIT);
    }
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
    s_retry_num = 0;
    return (int)ESP_OK;
}

int klin_wifi_sta_connect(const char *ssid, const char *pass)
{
    wifi_config_t wifi_config;
    esp_err_t err;

    if (!s_inited || ssid == NULL) {
        return (int)ESP_ERR_INVALID_STATE;
    }

    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (pass != NULL) {
        strncpy((char *)wifi_config.sta.password, pass,
                sizeof(wifi_config.sta.password) - 1);
    }

    s_retry_num = 0;
    s_ip_u32 = 0;
    xEventGroupClearBits(s_wifi_event_group,
                         KLIN_WIFI_CONNECTED_BIT | KLIN_WIFI_FAIL_BIT);

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        return (int)err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        return (int)err;
    }

    /* STA_START handler calls esp_wifi_connect(); also connect here if already started. */
    err = esp_wifi_connect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        return (int)err;
    }
    return (int)ESP_OK;
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
                               KLIN_WIFI_CONNECTED_BIT | KLIN_WIFI_FAIL_BIT,
                               pdFALSE, pdFALSE, ticks);

    if (bits & KLIN_WIFI_CONNECTED_BIT) {
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

int klin_wifi_sta_disconnect(void)
{
    if (!s_inited) {
        return (int)ESP_ERR_INVALID_STATE;
    }
    return (int)esp_wifi_disconnect();
}

int klin_wifi_sta_stop(void)
{
    if (!s_inited) {
        return (int)ESP_ERR_INVALID_STATE;
    }
    return (int)esp_wifi_stop();
}

void klin_wifi_sta_log_ip(void)
{
    uint32_t a = s_ip_u32;
    printf("klin_wifi: ip %u.%u.%u.%u\n",
           (unsigned)(a & 0xffu),
           (unsigned)((a >> 8) & 0xffu),
           (unsigned)((a >> 16) & 0xffu),
           (unsigned)((a >> 24) & 0xffu));
}
