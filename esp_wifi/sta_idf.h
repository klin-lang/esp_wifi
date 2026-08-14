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

/**
 * Link stats for the associated AP (`esp_wifi_sta_get_ap_info`).
 * Call after `klin_wifi_sta_connected` / GOT_IP. Each call hits IDF (no cache).
 * On failure (not assoc): rssi/channel/authmode return 0; ssid clears out.
 */
int klin_wifi_sta_rssi(void);
int klin_wifi_sta_channel(void);
int klin_wifi_sta_authmode(void);

/**
 * Copy associated AP SSID into caller buffer (NUL-terminated).
 * Returns length written (excluding NUL), or esp_err_t on error.
 */
int klin_wifi_sta_ap_ssid(char *out, int max_len);

/** Debug printf: rssi / channel / auth / ssid of associated AP. */
void klin_wifi_sta_log_link(void);

/** Max APs kept after `klin_wifi_scan_start` (fixed; documented). */
int klin_wifi_scan_max(void);

/**
 * Blocking active scan (STA mode). Requires `klin_wifi_sta_init` first.
 * Copies up to `klin_wifi_scan_max` APs into a fixed C table (no Klin heap).
 * timeout_ms for SCAN_DONE (-1 = forever). Returns esp_err_t as int.
 */
int klin_wifi_scan_start(int timeout_ms);

/** Number of APs stored after last successful scan (0..scan_max). */
int klin_wifi_scan_count(void);

int klin_wifi_scan_rssi(int index);
int klin_wifi_scan_channel(int index);
int klin_wifi_scan_authmode(int index);

/**
 * Copy SSID at index into caller buffer (NUL-terminated).
 * Returns byte length written (excluding NUL), or esp_err_t on error.
 */
int klin_wifi_scan_ssid(int index, char *out, int max_len);

/** Debug printf of all stored scan rows. */
void klin_wifi_scan_log(void);

#ifdef __cplusplus
}
#endif
