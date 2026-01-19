idf_component_register(SRCS "app_main.cpp"
                       REQUIRES "esp_driver_gpio esp_timer esp_driver_gptimer"
                       INCLUDE_DIRS ".")

target_sources(__idf_main
  PRIVATE
  common.h
  TrafficLight.hpp
  TrafficLight.cpp
  Button.hpp
  Button.cpp
  LED.hpp LED.cpp
)
