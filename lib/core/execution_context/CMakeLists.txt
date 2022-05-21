cmake_minimum_required(VERSION 3.14.0)
add_library(execution_context execution_context.cc)


target_include_directories(
    execution_context
    PUBLIC
    "${OrderMatchingSimulator_SOURCE_DIR}/include"
)

INSTALL(
    TARGETS execution_context 
)
