cmake_minimum_required(VERSION 3.10)
project(sample_plugin)

add_library(sample_plugin SHARED sample_plugin.cpp)
# Убрать префикс "lib" на некоторых платформах (по желанию)
set_target_properties(sample_plugin PROPERTIES PREFIX "")

# Выводить собранные артефакты в build/plugins
set_target_properties(sample_plugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)

# Установочные правила (опционально)
install(TARGETS sample_plugin
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
    ARCHIVE DESTINATION plugins
)
