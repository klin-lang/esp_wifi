/* Thin SoftAP helpers for Klin — fills IDF wifi_config_t / AP netif / event wait.
 * Heap, NVS, DHCP server, and the default event loop are ESP-IDF contracts.
 * SoftAP-only for this surface; APSTA with STA is a later track (not here).
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Optional SoftAP IPv4 (lwIP byte order). Call before/after `klin_wifi_ap_init`
 * (applied when AP netif exists). Stops the AP DHCP server, sets IP, then
 * restarts DHCPS. Pass 0,0,0 to clear pending override (IDF default AP IP).
 */
int klin_wifi_ap_set_ip(uint32_t ip, uint32_t gw, uint32_t netmask);

/** NVS + netif + default event loop + esp_wifi_init + AP netif + mode AP. */
int klin_wifi_ap_init(void);

/**
 * Fill SoftAP wifi_config_t, start. channel = 1..13.
 * Empty / NULL pass → WIFI_AUTH_OPEN; otherwise WPA2_PSK (pass length ≥ 8).
 * max_connection fixed at 4 (documented). Returns esp_err_t as int.
 */
int klin_wifi_ap_start(const char *ssid, const char *pass, int channel);

/** Block until WIFI_EVENT_AP_START or timeout_ms (-1 = forever). */
int klin_wifi_ap_wait_started(int timeout_ms);

/** 1 after AP_START, cleared on AP_STOP. */
int klin_wifi_ap_started(void);

/** AP IPv4 / gw / netmask as u32 (lwIP order) after start; else 0. */
uint32_t klin_wifi_ap_ip_u32(void);
uint32_t klin_wifi_ap_gateway_u32(void);
uint32_t klin_wifi_ap_netmask_u32(void);

/** Current associated STA count (0..max). */
int klin_wifi_ap_station_num(void);

int klin_wifi_ap_stop(void);

void klin_wifi_ap_log_ip(void);
void klin_wifi_ap_log_ip_info(void);

#ifdef __cplusplus
}
#endif
