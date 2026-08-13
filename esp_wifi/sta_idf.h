/* Thin STA helpers for Klin — fills IDF wifi_config_t / netif / event wait.
 * Heap, NVS, and the default event loop are ESP-IDF contracts (not Klin magic).
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** NVS + netif + default event loop + esp_wifi_init + STA netif + mode STA. */
int klin_wifi_sta_init(void);

/** Copy ssid/pass into wifi_config_t, start, connect. Returns esp_err_t as int. */
int klin_wifi_sta_connect(const char *ssid, const char *pass);

/** Block until GOT_IP or timeout_ms. Returns 0 on IP, else esp_err_t / -1. */
int klin_wifi_sta_wait_ip(int timeout_ms);

/** IPv4 host-order u32 after wait_ip success; 0 otherwise. */
uint32_t klin_wifi_sta_ip_u32(void);

int klin_wifi_sta_disconnect(void);

int klin_wifi_sta_stop(void);

/** Print stored IPv4 via IDF `esp_rom_printf` / `printf` (debug aid). */
void klin_wifi_sta_log_ip(void);

#ifdef __cplusplus
}
#endif
