# esp_wifi

Thin **ESP-IDF Wi‑Fi** bindings for [Klin](https://github.com/klin-lang/klin)
(**STA** + **SoftAP** + **scan** + **link stats**).

The radio is in the **silicon**; this package does **not** belong in
[`machine_esp`](https://github.com/klin-lang/machine_esp) (MMIO Pin…Adc+Rmt MVP).
Same split as MicroPython: `machine` vs `network` — see Klin
[061](https://github.com/klin-lang/klin/blob/main/issues/061-micropython-machine-api.md),
[101](https://github.com/klin-lang/klin/blob/main/issues/101-esp-wifi-idf.md),
[104](https://github.com/klin-lang/klin/blob/main/issues/104-later-tracks-esp-network.md).

C engine = **ESP-IDF** (`esp_wifi`, netif, event loop, NVS). Klin is a thin
FFI client (`@[link("sta_idf.c")]` / `@[link("ap_idf.c")]` + `@[cimport]`). IDF
heap / NVS / the default event loop / DHCPS are **IDF contracts**, not hidden
Klin allocation.

**STA IP mode:** default = **DHCP (dynamic)**. `sta_set_static_ip` is optional.
**SoftAP IP:** default = IDF AP address (typically `192.168.4.1` + DHCPS);
`ap_set_ip` is optional. **Do not** call `sta_*` and `ap_*` in the same binary
on this tag (APSTA / dual → later under [104] N1).

## Status (`@v0.4.0`)

### STA

| API | Notes |
|---|---|
| `sta_init` | NVS + netif + default event loop + `esp_wifi_init` + STA mode |
| `sta_set_static_ip` / `ipv4` / `sta_set_hostname` | Optional; DHCP off when static set |
| `sta_connect(ssid, pass)` | `wifi_config_t` fill + start + connect |
| `sta_wait_connected` / `sta_connected` | Assoc (`WIFI_EVENT_STA_CONNECTED`) |
| `sta_wait_ip(timeout_ms)` | GOT_IP (`-1` = forever); DHCP or static |
| `sta_ip_u32` / `sta_gateway_u32` / `sta_netmask_u32` | After GOT_IP |
| `sta_disconnect` / `sta_stop` | Thin IDF calls |
| `sta_log_ip` / `sta_log_ip_info` | Debug `printf` |
| `sta_rssi` / `sta_channel` / `sta_authmode` / `sta_ap_ssid` / `sta_log_link` | After assoc (`esp_wifi_sta_get_ap_info`; each call → IDF) |

### SoftAP (W1)

| API | Notes |
|---|---|
| `ap_init` | NVS + netif + event loop + `esp_wifi_init` + SoftAP mode |
| `ap_set_ip` / `ipv4` | Optional AP IPv4 + DHCPS restart |
| `ap_start(ssid, pass, channel)` | channel `1`…`13`; empty pass = open; else WPA2 (≥8); max 4 STAs |
| `ap_wait_started` / `ap_started` | `WIFI_EVENT_AP_START` |
| `ap_ip_u32` / `ap_gateway_u32` / `ap_netmask_u32` | After start |
| `ap_station_num` | Associated STA count |
| `ap_stop` / `ap_log_ip` / `ap_log_ip_info` | Thin / debug |

### Scan (W2)

| API | Notes |
|---|---|
| `scan_start(timeout_ms)` | After `sta_init`; blocking SCAN_DONE; keeps up to **16** APs in C |
| `scan_max` / `scan_count` | Cap / stored count |
| `scan_ssid(i, buf, max)` | Copy SSID into **caller** buffer |
| `scan_rssi` / `scan_channel` / `scan_authmode` | Per-index fields |
| `scan_log` | Debug `printf` of all rows |

RSSI-after-assoc is under STA above (W3). BLE / sockets / HTTP / TLS / APSTA /
dual Wi‑Fi+ETH → **out of scope**
([104](https://github.com/klin-lang/klin/blob/main/issues/104-later-tracks-esp-network.md)).

`version()` → `5` (`@v0.4.0`).

## Requirements

- [Klin](https://github.com/klin-lang/klin) compiler
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) **v5.x** (`IDF_PATH`)
- App `sdkconfig` with Wi‑Fi + NVS (+ SoftAP for AP demos)

## Layout

```text
esp_wifi/
  version.kl
  sta.kl              # STA Klin API
  sta_idf.c / .h      # STA + scan C
  scan.kl             # Scan Klin API
  ap.kl               # SoftAP Klin API
  ap_idf.c / .h
examples/sta_connect/ # ESP32-S3 STA idf.py demo
examples/softap/      # ESP32-S3 SoftAP idf.py demo
examples/scan/        # ESP32-S3 scan idf.py demo
examples/smoke/       # emit-c (no IDF)
```

## Usage — STA (DHCP default)

```klin
import "github/klin-lang/esp_wifi" wifi

@[cexport, codename("klin_app_main")]
fn app() {
  let mut e = wifi.sta_init()
  if e != wifi.err_ok() {
    return
  }
  e = wifi.sta_connect("myssid", "mypass")
  if e != wifi.err_ok() {
    return
  }
  e = wifi.sta_wait_connected(15000)
  if e != wifi.err_ok() {
    return
  }
  e = wifi.sta_wait_ip(20000)
  if e != wifi.err_ok() {
    return
  }
  wifi.sta_log_ip_info()
  wifi.sta_log_link()
}
```

## Usage — SoftAP

```klin
import "github/klin-lang/esp_wifi" wifi

@[cexport, codename("klin_app_main")]
fn app() {
  let mut e = wifi.ap_init()
  if e != wifi.err_ok() {
    return
  }
  // channel 6; password ≥ 8 for WPA2 ("" = open)
  e = wifi.ap_start("klin-ap", "klinpass1", 6)
  if e != wifi.err_ok() {
    return
  }
  e = wifi.ap_wait_started(5000)
  if e != wifi.err_ok() {
    return
  }
  wifi.ap_log_ip_info()
}
```

## Usage — Scan

```klin
import "github/klin-lang/esp_wifi" wifi

@[cexport, codename("klin_app_main")]
fn app() {
  let mut e = wifi.sta_init()
  if e != wifi.err_ok() {
    return
  }
  e = wifi.scan_start(15000)
  if e != wifi.err_ok() {
    return
  }
  wifi.scan_log()
  let mut ssid: [33]u8
  let _n = wifi.scan_ssid(0, cast(*mut u8, &ssid[0]), 33)
}
```

```sh
klin get github/klin-lang/esp_wifi@v0.4.0
```

Local / in-repo:

```klin
import "../../esp_wifi" wifi
```

## Example (hardware)

```sh
cd examples/sta_connect   # or examples/softap / examples/scan
# edit credentials / SSID in the .kl file
. $IDF_PATH/export.sh
make emit KLIN=/path/to/klin/bin/klin.dart
make build
make flash
```

Target: **esp32s3**. Board pack
[`waveshare_esp32_s3_pico`](https://github.com/klin-lang/waveshare_esp32_s3_pico)
stays pin/WS2812-only — no radio API there.

Sibling: [`esp_eth`](https://github.com/klin-lang/esp_eth).

## Contract (prime rule)

- No Klin GC / hidden heap — SSID/pass are C strings you pass in; scan SSID
  goes into a **caller** buffer (`scan_ssid`).
- SoftAP `max_connection` = **4** (fixed in `ap_idf.c`, documented).
- Scan keeps at most **16** APs in a fixed C table (`scan_max`, documented).
- STA link stats (`sta_rssi` …) call IDF each time (no Klin cache).
- STA reconnect retry: max 5 in `sta_idf.c` (documented).
- Errors are `i32` (`esp_err_t`); check them.
- LwIP sockets, TLS, APSTA: later / other packages.

## Changelog

| Tag | Notes |
|---|---|
| `@v0.1.0` | STA + DHCP |
| `@v0.1.1` | STA static IP / hostname / gw+mask / assoc wait |
| `@v0.2.0` | SoftAP (`ap_*`) — [104] W1 |
| `@v0.3.0` | Scan (`scan_*`) — [104] W2 |
| `@v0.4.0` | STA link stats (`sta_rssi` …) — [104] W3 |

## Links

- Klin STA issue: https://github.com/klin-lang/klin/blob/main/issues/101-esp-wifi-idf.md
- Network later: https://github.com/klin-lang/klin/blob/main/issues/104-later-tracks-esp-network.md
- Ethernet sibling: https://github.com/klin-lang/esp_eth
- Chip MMIO: https://github.com/klin-lang/machine_esp
