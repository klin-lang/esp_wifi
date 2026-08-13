# esp_wifi

Thin **ESP-IDF Wi‑Fi STA** bindings for [Klin](https://github.com/klin-lang/klin).

The radio is in the **silicon**; this package does **not** belong in
[`machine_esp`](https://github.com/klin-lang/machine_esp) (MMIO Pin…Adc+Rmt MVP).
Same split as MicroPython: `machine` vs `network` — see Klin
[061](https://github.com/klin-lang/klin/blob/main/issues/061-micropython-machine-api.md),
[101](https://github.com/klin-lang/klin/blob/main/issues/101-esp-wifi-idf.md).

C engine = **ESP-IDF** (`esp_wifi`, netif, event loop, NVS). Klin is a thin
FFI client (`@[link("sta_idf.c")]` + `@[cimport]`). IDF heap / NVS / the default
event loop are **IDF contracts**, not hidden Klin allocation.

## Status (`@v0.1.0`)

| API | Notes |
|---|---|
| `sta_init` | NVS + netif + default event loop + `esp_wifi_init` + STA mode |
| `sta_connect(ssid, pass)` | `wifi_config_t` fill + start + connect |
| `sta_wait_ip(timeout_ms)` | Explicit poll/wait on GOT_IP (`-1` = forever) |
| `sta_ip_u32` | IPv4 as `u32` after wait success |
| `sta_disconnect` / `sta_stop` | Thin IDF calls |
| `sta_log_ip` | Debug `printf` of stored IPv4 |
| SoftAP / BLE / sockets / HTTP / TLS | **Out of scope** |

`version()` → `1`.

## Requirements

- [Klin](https://github.com/klin-lang/klin) compiler
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) **v5.x** (`IDF_PATH`)
- App `sdkconfig` with Wi‑Fi + NVS (defaults in `examples/sta_connect/`)

## Layout

```text
esp_wifi/
  version.kl
  sta.kl              # Klin API
  sta_idf.c / .h      # IDF wifi_config_t + event wait (linked via @[link])
examples/sta_connect/ # ESP32-S3 idf.py demo (edit SSID/pass)
examples/smoke/       # emit-c against C stubs (no IDF)
```

## Usage

```klin
import "github/klin-lang/esp_wifi" wifi

@[cexport, codename("klin_app_main")]
fn app() {
  let e = wifi.sta_init()
  if e != wifi.err_ok() {
    return
  }
  e = wifi.sta_connect("myssid", "mypass")
  if e != wifi.err_ok() {
    return
  }
  e = wifi.sta_wait_ip(20000)
  if e != wifi.err_ok() {
    return
  }
  let ip = wifi.sta_ip_u32()
  // use ip; sockets/HTTP are separate (not this package)
}
```

```sh
klin get github/klin-lang/esp_wifi@v0.1.0
```

Local / in-repo:

```klin
import "../../esp_wifi" wifi
```

## Example (hardware)

```sh
cd examples/sta_connect
# edit sta.kl — set SSID / password
. $IDF_PATH/export.sh
make emit KLIN=/path/to/klin/bin/klin.dart
make build
make flash
```

Target: **esp32s3** (any S3 board with antenna; Waveshare ESP32-S3-Pico works).
Board pack [`waveshare_esp32_s3_pico`](https://github.com/klin-lang/waveshare_esp32_s3_pico)
stays pin/WS2812-only — no radio API there.

## Contract (prime rule)

- No Klin GC / hidden heap — buffers for SSID/pass are C strings you pass in.
- No hidden reconnect policy beyond the small, documented retry in `sta_idf.c`
  (max 5 `esp_wifi_connect` attempts while waiting for IP).
- Errors are `i32` (`esp_err_t`); check them.
- BLE, SoftAP, LwIP sockets, TLS: later / other packages.

## Links

- Klin issue: https://github.com/klin-lang/klin/blob/main/issues/101-esp-wifi-idf.md
- Chip MMIO: https://github.com/klin-lang/machine_esp ([099](https://github.com/klin-lang/klin/blob/main/issues/099-machine-esp-esp32-s3.md))
- Pattern (RTOS FFI client): https://github.com/klin-lang/klin_freertos
