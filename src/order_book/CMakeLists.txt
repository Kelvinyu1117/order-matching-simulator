cmake_minimum_required(VERSION 3.14.0)
add_library(order_book order_book.cc)

include_directories(
    "${OrderMatchingSimulator_SOURCE_DIR}/include"
)
