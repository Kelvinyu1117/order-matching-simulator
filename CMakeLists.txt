cmake_minimum_required(VERSION 3.14.0)

project(OrderMatchingSimulator LANGUAGES CXX C)

# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OrderMatchingSimulator_SOURCE_DIR}/OrderMatchingSimulator/lib")
set(CMAKE_INSTALL_PREFIX "${OrderMatchingSimulator_SOURCE_DIR}/OrderMatchingSimulator")
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra -Wpedantic -Werror")

subdirs(src tests lib)
