cmake_minimum_required (VERSION 2.6)
project (clavata CXX)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall")
endif()

add_library(clvt clavata.cc)
add_executable(clavata test.cc)
target_link_libraries(clavata clvt)