# Smoke (stubs)

`make emit` runs Klin `--emit-c` against the real `esp_wifi` module. The
`@[link("sta_idf.c")]` unit is the package C file; for host compile without IDF
use `stubs/` headers only as a reference — full link needs ESP-IDF (see
`examples/sta_connect/`).
