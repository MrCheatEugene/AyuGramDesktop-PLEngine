cmake_minimum_required(VERSION 3.10)
project(plugin_loader)

add_library(plugin_loader STATIC plugin_loader.cpp)
# Public include directory for the header in the same folder
target_include_directories(plugin_loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Optionally install the loader library
install(TARGETS plugin_loader
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
