# main/CMakeLists.txt

# Define the list of source files
set(srcs "main.c"
         "st7796s.c"
         "fontx.c"
         "decode_png.c"
         "pngle.c"
         "decode_gif.c"
         "decode_rgb565ani.c"
         "gpio_led.c"
         )

# Conditionally include decode_jpeg.c for the ESP32-S3 target
if(IDF_TARGET STREQUAL "esp32s3")
    list(APPEND srcs "decode_jpeg.c")
endif()

# Register the component and its dependencies
idf_component_register(
    SRCS ${srcs}                  # Source files
    INCLUDE_DIRS "."              # Include current directory for headers
                 "${CMAKE_SOURCE_DIR}/managed_components/espressif__tinyusb/src"
                 "${CMAKE_SOURCE_DIR}/managed_components/espressif__tinyusb/src/class/msc"
    REQUIRES driver               # Public dependencies
             freertos
             esp_system
             esp_lcd
             spiffs
             fatfs
             espressif__tinyusb
             sdmmc
    PRIV_REQUIRES nsgif           # Private dependencies
                  esp_timer
                  esp_psram
                #   esp_hw_support  # Add this line to include the internal hw support APIs
)
