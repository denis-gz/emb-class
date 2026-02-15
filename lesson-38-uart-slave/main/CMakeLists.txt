idf_component_register(SRCS "app_main.cpp"
                       REQUIRES "esp_driver_gpio esp_driver_uart esp_driver_gptimer esp_timer"
                       INCLUDE_DIRS ".")

target_sources(__idf_main
  PRIVATE
  common.h

  UartReceiver.h UartReceiver.cpp
  S7_Display.h S7_Display.cpp
  S7_Digit.h S7_Digit.cpp
  Calculator.h Calculator.cpp
)

target_compile_options(__idf_main
  PRIVATE
  -Wno-missing-field-initializers
  -Wno-unused-variable
  -Wno-switch
)
