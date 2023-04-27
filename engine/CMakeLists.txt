
add_library(engine)

target_include_directories(engine
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(engine
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/board.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/eval.h)

set_target_properties(engine
  PROPERTIES
    LANGUAGE CXX
    CXX_STANDARD 17)

target_compile_options(engine
  PRIVATE
    -g)
