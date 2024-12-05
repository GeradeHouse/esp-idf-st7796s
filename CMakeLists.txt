# CMakeLists.txt in the root of your project

# Minimum required CMake version
cmake_minimum_required(VERSION 3.5)

# Add this line to specify where to find extra components
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/components")

# Include the necessary ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define your project name
project(st7789)

# Define the SPIFFS image directory within the build directory
set(SPIFFS_IMAGE_DIR "${CMAKE_BINARY_DIR}/spiffs_image_combined")

# Custom target to prepare the SPIFFS image directory by copying files
add_custom_target(
    prepare_spiffs_image_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SPIFFS_IMAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/font/." "${SPIFFS_IMAGE_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/esp32_images/." "${SPIFFS_IMAGE_DIR}/"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/font" "${CMAKE_CURRENT_SOURCE_DIR}/esp32_images"
)

# Create the SPIFFS partition image from the combined directory, specifying the dependency
spiffs_create_partition_image(storage ${SPIFFS_IMAGE_DIR} FLASH_IN_PROJECT
    DEPENDS prepare_spiffs_image_dir)
