cmake_minimum_required(VERSION 3.14.0)

add_library(trader INTERFACE)

target_include_directories(trader INTERFACE .)

target_include_directories(
    trader
    INTERFACE
    "${OrderMatchingSimulator_SOURCE_DIR}/include"
)
target_include_directories(
    trader
    INTERFACE
    "${OrderMatchingSimulator_SOURCE_DIR}/lib/core"
)

target_link_libraries(trader INTERFACE order)

