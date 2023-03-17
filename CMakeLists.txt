cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
#cmake_minimum_required(VERSION 2.8)
project(fastree CXX)
# Require C++17 in all targets
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -pedantic)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

find_package(ROOT 6 REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(
  Boost 1.72 REQUIRED
  COMPONENTS log program_options stacktrace_basic
)


add_library(fastree SHARED
  src/Logger.cc
  src/Options.cc
)
target_include_directories(fastree PRIVATE include)
target_link_libraries(fastree
  PUBLIC yaml-cpp
  PUBLIC Boost::boost Boost::log Boost::program_options
  PUBLIC Boost::stacktrace_basic -rdynamic
  ${ROOT_LIBRARIES}
)
add_executable(test src/test.cc)
target_include_directories(test PRIVATE include)

target_link_libraries(test  PRIVATE fastree)
