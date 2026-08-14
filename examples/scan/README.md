# Scan example (ESP32-S3)

```sh
. $IDF_PATH/export.sh
make emit KLIN=/path/to/klin/bin/klin.dart
make build
make flash
make monitor
```

Uses `sta_init` + `scan_start` (no association). UART prints up to 16 APs
(`scan_log`). SoftAP mode cannot scan — use STA init.
