#include "sta_idf.h"

int klin_wifi_sta_set_static_ip(uint32_t ip, uint32_t gw, uint32_t netmask)
{
    (void)ip;
    (void)gw;
    (void)netmask;
    return 0;
}
int klin_wifi_sta_set_hostname(const char *name)
{
    (void)name;
    return 0;
}
int klin_wifi_sta_init(void) { return 0; }
int klin_wifi_sta_connect(const char *ssid, const char *pass)
{
    (void)ssid;
    (void)pass;
    return 0;
}
int klin_wifi_sta_wait_connected(int timeout_ms)
{
    (void)timeout_ms;
    return 0;
}
int klin_wifi_sta_connected(void) { return 1; }
int klin_wifi_sta_wait_ip(int timeout_ms)
{
    (void)timeout_ms;
    return 0;
}
uint32_t klin_wifi_sta_ip_u32(void) { return 0x0101a8c0u; /* 192.168.1.1 LE */ }
uint32_t klin_wifi_sta_gateway_u32(void) { return 0x0100a8c0u; }
uint32_t klin_wifi_sta_netmask_u32(void) { return 0x00ffffffu; }
int klin_wifi_sta_disconnect(void) { return 0; }
int klin_wifi_sta_stop(void) { return 0; }
void klin_wifi_sta_log_ip(void) {}
void klin_wifi_sta_log_ip_info(void) {}
int klin_wifi_sta_rssi(void) { return -42; }
int klin_wifi_sta_channel(void) { return 6; }
int klin_wifi_sta_authmode(void) { return 3; }
int klin_wifi_sta_ap_ssid(char *out, int max_len)
{
    const char *s = "stub-assoc";
    int n = 10;
    if (out == NULL || max_len <= 0) {
        return -1;
    }
    if (n >= max_len) {
        n = max_len - 1;
    }
    for (int i = 0; i < n; i++) {
        out[i] = (char)s[i];
    }
    out[n] = '\0';
    return n;
}
void klin_wifi_sta_log_link(void) {}
int klin_wifi_scan_max(void) { return 16; }
int klin_wifi_scan_start(int timeout_ms)
{
    (void)timeout_ms;
    return 0;
}
int klin_wifi_scan_count(void) { return 1; }
int klin_wifi_scan_rssi(int index)
{
    (void)index;
    return -40;
}
int klin_wifi_scan_channel(int index)
{
    (void)index;
    return 6;
}
int klin_wifi_scan_authmode(int index)
{
    (void)index;
    return 3;
}
int klin_wifi_scan_ssid(int index, char *out, int max_len)
{
    const char *s = "stub-ap";
    int n = 7;
    (void)index;
    if (out == NULL || max_len <= 0) {
        return -1;
    }
    if (n >= max_len) {
        n = max_len - 1;
    }
    for (int i = 0; i < n; i++) {
        out[i] = (char)s[i];
    }
    out[n] = '\0';
    return n;
}
void klin_wifi_scan_log(void) {}
