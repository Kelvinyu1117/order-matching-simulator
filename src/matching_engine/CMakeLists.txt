cmake_minimum_required(VERSION 3.14.0)


add_library(matching_engine matching_engine.cc)
target_include_directories(matching_engine PUBLIC "${OrderMatchingSimulator_SOURCE_DIR}/include")
target_include_directories(matching_engine PUBLIC "${OrderMatchingSimulator_SOURCE_DIR}/lib/core")


target_link_libraries(matching_engine order_book)

install(
    TARGETS matching_engine
)

