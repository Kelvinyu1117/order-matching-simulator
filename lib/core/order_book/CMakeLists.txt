cmake_minimum_required(VERSION 3.14.0)
add_library(order_book order_book.cc)

target_include_directories(
    order_book
    PUBLIC
    "${OrderMatchingSimulator_SOURCE_DIR}/include"
)

target_include_directories(
    order_book
    PUBLIC
    "${OrderMatchingSimulator_SOURCE_DIR}/lib/core"
)

target_link_libraries(order_book order)

install(
    TARGETS order_book 
)
