idf_component_register(
  SRCS "main.c" "../LoRa/LoRa.c"
  INCLUDE_DIRS "." "../LoRa"
  REQUIRES log esp_timer esp_driver_spi esp_driver_gpio freertos
)