/* Thin STA helpers for Klin — fills IDF wifi_config_t / netif / event wait.
 * Heap, NVS, and the default event loop are ESP-IDF contracts (not Klin magic).
 * DHCP (dynamic IP) is the default; static IP is an optional opt-in.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Optional static IPv4 (lwIP byte order). Call before/after `klin_wifi_sta_init`
 * (applied when STA netif exists). Disables DHCP client on the STA netif.
 * Pass 0,0,0 to clear pending static config (DHCP on next apply / connect).
 */
int klin_wifi_sta_set_static_ip(uint32_t ip, uint32_t gw, uint32_t netmask);

/**
 * Optional hostname for the STA netif. Before/after init; applied when netif exists.
 * Empty / NULL clears the pending hostname.
 */
int klin_wifi_sta_set_hostname(const char *name);

/** NVS + netif + default event loop + esp_wifi_init + STA netif + mode STA. */
int klin_wifi_sta_init(void);

/** Copy ssid/pass into wifi_config_t, start, connect. Returns esp_err_t as int. */
int klin_wifi_sta_connect(const char *ssid, const char *pass);

/** Block until WIFI_EVENT_STA_CONNECTED or timeout_ms (-1 = forever). */
int klin_wifi_sta_wait_connected(int timeout_ms);

/** 1 after assoc (STA_CONNECTED), cleared on disconnect. */
int klin_wifi_sta_connected(void);

/** Block until GOT_IP or timeout_ms. Returns 0 on IP, else esp_err_t / fail. */
int klin_wifi_sta_wait_ip(int timeout_ms);

/** IPv4 / gateway / netmask as u32 (lwIP order) after GOT_IP; else 0. */
uint32_t klin_wifi_sta_ip_u32(void);
uint32_t klin_wifi_sta_gateway_u32(void);
uint32_t klin_wifi_sta_netmask_u32(void);

int klin_wifi_sta_disconnect(void);

int klin_wifi_sta_stop(void);

void klin_wifi_sta_log_ip(void);

/** Print ip / gateway / netmask. */
void klin_wifi_sta_log_ip_info(void);

#ifdef __cplusplus
}
#endif
