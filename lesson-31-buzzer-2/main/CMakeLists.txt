idf_component_register(SRCS "app_main.cpp"
                       REQUIRES "esp_driver_ledc esp_driver_gptimer"
                       INCLUDE_DIRS ".")

target_sources(__idf_main
  PRIVATE
  common.h
  MelodyPlayer.h MelodyPlayer.cpp
)

target_compile_options(__idf_main
  PRIVATE
  "-Wno-missing-field-initializers"
)
