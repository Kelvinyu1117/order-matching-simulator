include(FetchContent)


FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    OrderMatchingSimulatorTest
    test_matching_engine.cpp
)

target_link_libraries(OrderMatchingSimulatorTest matching_engine order_book gtest_main)

include(GoogleTest)
gtest_discover_tests(
    OrderMatchingSimulatorTest)

install(TARGETS OrderMatchingSimulatorTest DESTINATION "${OrderMatchingSimulator_SOURCE_DIR}/OrderMatchingSimulator/tests/")
