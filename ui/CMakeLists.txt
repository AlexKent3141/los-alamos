CPMAddPackage(
  NAME punk
  GIT_REPOSITORY https://github.com/AlexKent3141/sdl-punk
  GIT_TAG master
  OPTIONS "BUILD_DEMO OFF")

add_executable(los_alamos
  main.cpp)

target_link_libraries(los_alamos
  PRIVATE
    engine
    punk
    search)

set_target_properties(los_alamos
  PROPERTIES
    LANGUAGE CXX
    CXX_STANDARD 17)

if (UNIX)
  target_compile_options(los_alamos
    PRIVATE
      -Wall -Wextra -Wpedantic -g)
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
