cmake_minimum_required(VERSION 3.25)

project(abbey VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings applied to every target in this project.
if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Library: abbey (static)
#
# Headers and sources live together under src/abbey/. Consumers include them
# as "abbey/<name>.h". List new .cc files here as they are added.
# ---------------------------------------------------------------------------
add_library(abbey STATIC
  src/abbey/abbey.cc
  src/abbey/print.cc
)
target_include_directories(abbey PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---------------------------------------------------------------------------
# Tests: abbey_test (command-line driver)
# ---------------------------------------------------------------------------
add_executable(abbey_test
  test/main.cc
  test/print.cc
)
target_link_libraries(abbey_test PRIVATE abbey)

enable_testing()
# ---------------------------------------------------------------------------
# The same executable can be registered more than once with different
# command, supplying different arguments. If the command returns 0, it
# is considered passed.
# ---------------------------------------------------------------------------
add_test(NAME print_system0 COMMAND abbey_test print system0)
add_test(NAME print_system2 COMMAND abbey_test print system2)
add_test(NAME print_system_int COMMAND abbey_test print system_int)
add_test(NAME print_system_real COMMAND abbey_test print system_real)
add_test(NAME print_system_decimal COMMAND abbey_test print system_decimal)
add_test(NAME print_system_scientific COMMAND abbey_test print system_scientific)
add_test(NAME print_system_string COMMAND abbey_test print system_string)
add_test(NAME print_system_message COMMAND abbey_test print system_message)
add_test(NAME print_debug COMMAND abbey_test print debug)
