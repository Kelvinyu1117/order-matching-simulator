cmake_minimum_required(VERSION 3.14.0)

include_directories(
    "${OrderMatchingSimulator_SOURCE_DIR}/include"
    "${OrderMatchingSimulator_SOURCE_DIR}/include"
)

subdirs(order_book matching_engine)
add_executable(main main.cc)
target_link_libraries(main order_book matching_engine)

install(TARGETS main DESTINATION "${OrderMatchingSimulator_SOURCE_DIR}/OrderMatchingSimulator/")
