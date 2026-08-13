#pragma once
#include <stdint.h>
int klin_wifi_sta_init(void);
int klin_wifi_sta_connect(const char *ssid, const char *pass);
int klin_wifi_sta_wait_ip(int timeout_ms);
uint32_t klin_wifi_sta_ip_u32(void);
int klin_wifi_sta_disconnect(void);
int klin_wifi_sta_stop(void);
void klin_wifi_sta_log_ip(void);
