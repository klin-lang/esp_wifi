# STA connect (ESP32-S3)

Hardware demo for [`esp_wifi`](../../README.md) `@v0.1.0`.

1. Edit `sta.kl` — set SSID / password.
2. `. $IDF_PATH/export.sh`
3. `make emit KLIN=/path/to/klin/bin/klin.dart`
4. `make build` / `make flash` / `make monitor`

Needs ESP-IDF **v5.x** and an ESP32-S3 board with antenna.
