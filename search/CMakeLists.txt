
add_library(search)

target_include_directories(search
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(search
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/search.cpp)

target_link_libraries(search
  PUBLIC
    engine
  PRIVATE
    pthread)

set_target_properties(search
  PROPERTIES
    LANGUAGE CXX
    CXX_STANDARD 17)

if (UNIX)
  target_compile_options(search
    PRIVATE
      -Wall -Wextra -Wpedantic -g)
endif()
