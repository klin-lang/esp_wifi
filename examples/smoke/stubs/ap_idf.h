#pragma once
#include <stdint.h>
int klin_wifi_ap_set_ip(uint32_t ip, uint32_t gw, uint32_t netmask);
int klin_wifi_ap_init(void);
int klin_wifi_ap_start(const char *ssid, const char *pass, int channel);
int klin_wifi_ap_wait_started(int timeout_ms);
int klin_wifi_ap_started(void);
uint32_t klin_wifi_ap_ip_u32(void);
uint32_t klin_wifi_ap_gateway_u32(void);
uint32_t klin_wifi_ap_netmask_u32(void);
int klin_wifi_ap_station_num(void);
int klin_wifi_ap_stop(void);
void klin_wifi_ap_log_ip(void);
void klin_wifi_ap_log_ip_info(void);
