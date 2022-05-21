cmake_minimum_required(VERSION 3.14.0)

subdirs(matching_engine)
add_executable(main main.cc)
target_link_libraries(main order_book matching_engine)
target_include_directories(main PUBLIC "${OrderMatchingSimulator_SOURCE_DIR}/include")
target_include_directories(main PUBLIC "${OrderMatchingSimulator_SOURCE_DIR}/lib/core")


install(TARGETS main DESTINATION "${OrderMatchingSimulator_SOURCE_DIR}/OrderMatchingSimulator/")
