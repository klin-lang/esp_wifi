#include "ap_idf.h"

int klin_wifi_ap_set_ip(uint32_t ip, uint32_t gw, uint32_t netmask)
{
    (void)ip;
    (void)gw;
    (void)netmask;
    return 0;
}
int klin_wifi_ap_init(void) { return 0; }
int klin_wifi_ap_start(const char *ssid, const char *pass, int channel)
{
    (void)ssid;
    (void)pass;
    (void)channel;
    return 0;
}
int klin_wifi_ap_wait_started(int timeout_ms)
{
    (void)timeout_ms;
    return 0;
}
int klin_wifi_ap_started(void) { return 1; }
uint32_t klin_wifi_ap_ip_u32(void) { return 0x0104a8c0u; /* 192.168.4.1 LE */ }
uint32_t klin_wifi_ap_gateway_u32(void) { return 0x0104a8c0u; }
uint32_t klin_wifi_ap_netmask_u32(void) { return 0x00ffffffu; }
int klin_wifi_ap_station_num(void) { return 0; }
int klin_wifi_ap_stop(void) { return 0; }
void klin_wifi_ap_log_ip(void) {}
void klin_wifi_ap_log_ip_info(void) {}
