idf_component_register(SRCS "app_main.cpp"
                       REQUIRES "esp_driver_gpio esp_timer"
                       INCLUDE_DIRS ".")

target_sources(__idf_main
  PRIVATE
  common.h
)
