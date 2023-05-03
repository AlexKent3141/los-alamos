cmake_minimum_required(VERSION 3.19)
project(los_alamos)

include(cmake/CPM.cmake)

if (UNIX)
  add_compile_options(-Werror)
endif()

add_subdirectory(engine)
add_subdirectory(search)
add_subdirectory(tools)
add_subdirectory(ui)
