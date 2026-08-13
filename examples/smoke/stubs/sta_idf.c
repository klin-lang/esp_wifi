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
