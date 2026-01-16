idf_component_register(SRCS "app_main.cpp"
                       REQUIRES "esp_driver_gpio esp_timer"
                       INCLUDE_DIRS ".")

target_sources(__idf_main
  PRIVATE
  no_debounce.hpp
  no_debounce.cpp
  time_based.hpp time_based.cpp
  state_based.cpp
  state_based.hpp
  common.h
  handler_base.hpp
  handler_base.cpp
)
