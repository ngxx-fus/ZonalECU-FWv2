idf_component_register(
    SRCS "ZonalECU-FWv2.c"
    INCLUDE_DIRS "."
    REQUIRES Common ESP-Logger SOME-IP esp_driver_gpio esp_timer freertos
)
