cmake_minimum_required(VERSION 3.14.0)
add_library(self_trade_handler self_trade_handler.cc)

target_include_directories(
    self_trade_handler
    PUBLIC
    "${OrderMatchingSimulator_SOURCE_DIR}/include"
)

install(
    TARGETS self_trade_handler 
)
