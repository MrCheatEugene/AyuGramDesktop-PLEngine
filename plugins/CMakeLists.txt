cmake_minimum_required(VERSION 3.10)

option(BUILD_PLUGINS "Build plugin shared libraries" ON)

if(BUILD_PLUGINS)
  add_subdirectory(loader)
  add_subdirectory(sample_plugin)
endif()
