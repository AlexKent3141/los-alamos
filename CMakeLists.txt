cmake_minimum_required(VERSION 3.19)
project(los_alamos)

include(cmake/CPM.cmake)

CPMAddPackage(
  NAME punk
  GIT_REPOSITORY https://github.com/AlexKent3141/sdl-punk
  GIT_TAG master
  OPTIONS "BUILD_DEMO OFF")

add_executable(los_alamos
  main.cpp)

target_link_libraries(los_alamos
  PRIVATE
    punk)
