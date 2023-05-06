add_executable(perft perft.cpp)

target_link_libraries(perft
  PRIVATE
    engine)

set_target_properties(perft
  PROPERTIES
    LANGUAGE CXX
    CXX_STANDARD 17)

if (UNIX)
  target_compile_options(perft
    PRIVATE
      -Wall -Wextra -Wpedantic -g)
endif()

add_executable(search_test search_test.cpp)

target_link_libraries(search_test
  PRIVATE
    search)

set_target_properties(search_test
  PROPERTIES
    LANGUAGE CXX
    CXX_STANDARD 17)

if (UNIX)
  target_compile_options(search_test
    PRIVATE
      -Wall -Wextra -Wpedantic -g)
endif()
