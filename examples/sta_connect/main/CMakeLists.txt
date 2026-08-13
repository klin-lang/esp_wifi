# Klin emit must run before configure (see ../Makefile `emit` target).
set(KLIN_C "${CMAKE_SOURCE_DIR}/out/sta.c")
set(WIFI_C "${CMAKE_SOURCE_DIR}/../../esp_wifi/sta_idf.c")

if(NOT EXISTS "${KLIN_C}")
    message(FATAL_ERROR
        "Missing ${KLIN_C}. From examples/sta_connect run: make emit KLIN=...")
endif()

idf_component_register(
    SRCS "app_main.c" "${KLIN_C}" "${WIFI_C}"
    INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/out" "${CMAKE_SOURCE_DIR}/../../esp_wifi"
    REQUIRES nvs_flash esp_wifi esp_netif esp_event
)
