cmake_minimum_required(VERSION 3.14.0)


add_library(matching_engine matching_engine.cc self_trade_handler.cc)
target_include_directories(matching_engine PUBLIC "${OrderMatchingSimulator_SOURCE_DIR}/include")
target_include_directories(matching_engine PUBLIC "${OrderMatchingSimulator_SOURCE_DIR}/lib/")


target_link_libraries(matching_engine order_book execution_context)

install(
    TARGETS matching_engine
)

