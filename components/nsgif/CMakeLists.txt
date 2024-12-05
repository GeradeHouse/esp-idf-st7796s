# components/libnsgif/CMakeLists.txt

# Minimum required CMake version
cmake_minimum_required(VERSION 3.5)

# Set the component name to 'nsgif'
set(COMPONENT_NAME "nsgif")

# Register the component with ESP-IDF
idf_component_register(
    SRCS "src/gif.c"
         "src/lzw.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
)
