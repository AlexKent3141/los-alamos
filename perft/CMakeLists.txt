add_executable(perft perft.cpp)

target_link_libraries(perft
  PRIVATE
    engine)

set_target_properties(perft
  PROPERTIES
    LANGUAGE CXX
    CXX_STANDARD 17)

target_compile_options(perft
  PRIVATE
    -g)
