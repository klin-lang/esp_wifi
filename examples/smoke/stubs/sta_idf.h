#pragma once
#include <stdint.h>
int klin_wifi_sta_set_static_ip(uint32_t ip, uint32_t gw, uint32_t netmask);
int klin_wifi_sta_set_hostname(const char *name);
int klin_wifi_sta_init(void);
int klin_wifi_sta_connect(const char *ssid, const char *pass);
int klin_wifi_sta_wait_connected(int timeout_ms);
int klin_wifi_sta_connected(void);
int klin_wifi_sta_wait_ip(int timeout_ms);
uint32_t klin_wifi_sta_ip_u32(void);
uint32_t klin_wifi_sta_gateway_u32(void);
uint32_t klin_wifi_sta_netmask_u32(void);
int klin_wifi_sta_disconnect(void);
int klin_wifi_sta_stop(void);
void klin_wifi_sta_log_ip(void);
void klin_wifi_sta_log_ip_info(void);
int klin_wifi_sta_rssi(void);
int klin_wifi_sta_channel(void);
int klin_wifi_sta_authmode(void);
int klin_wifi_sta_ap_ssid(char *out, int max_len);
void klin_wifi_sta_log_link(void);
int klin_wifi_scan_max(void);
int klin_wifi_scan_start(int timeout_ms);
int klin_wifi_scan_count(void);
int klin_wifi_scan_rssi(int index);
int klin_wifi_scan_channel(int index);
int klin_wifi_scan_authmode(int index);
int klin_wifi_scan_ssid(int index, char *out, int max_len);
void klin_wifi_scan_log(void);
