cmake_minimum_required(VERSION 3.14.0)
add_library(order order.cc)

target_include_directories(
    order
    PUBLIC
    "${OrderMatchingSimulator_SOURCE_DIR}/include"
)

install(
    TARGETS order 
)
