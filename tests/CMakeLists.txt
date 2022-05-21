include(FetchContent)


FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    OrderMatchingSimulatorTest
    test_matching_engine.cpp
    test_order_book.cpp
)

target_link_libraries(OrderMatchingSimulatorTest matching_engine order_book gtest_main)
target_include_directories(OrderMatchingSimulatorTest PUBLIC "${OrderMatchingSimulator_SOURCE_DIR}/include")
target_include_directories(OrderMatchingSimulatorTest PUBLIC "${OrderMatchingSimulator_SOURCE_DIR}/lib/core")



include(GoogleTest)
gtest_discover_tests(
    OrderMatchingSimulatorTest)

install(TARGETS OrderMatchingSimulatorTest DESTINATION "${OrderMatchingSimulator_SOURCE_DIR}/OrderMatchingSimulator/tests/")
