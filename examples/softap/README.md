# SoftAP example (ESP32-S3)

```sh
# edit ap.kl — SSID / password / channel
. $IDF_PATH/export.sh
make emit KLIN=/path/to/klin/bin/klin.dart
make build
make flash
make monitor
```

Join the SoftAP from a phone/laptop; UART should show the AP IP (default
`192.168.4.1` unless `ap_set_ip` was used).
