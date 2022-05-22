# Order Matching Simulator

## Brief Introduction

This project implemented a simple order matching engine which support the insertion and cancellation of both **Market Order** and **Limit Order** and **self-trade prevention**.

The following self-trade prevention policies are supported:

* CANCEL_PASSIVE
* CANCEL_ACTIVE
* CANCEL_BOTH

All the functionalities are tested through unit test.

## Requirements

* C++ 17 or above
* CMake 3.14.0
* GCC/G++ 9 or Clang/Clang++ 13
* GoogleTest

## Development Environment

* Ubuntu 20.04.2 LTS (Focal Fossa)

## Build and Install

```bash
mkdir build 
cd build && cmake ..
make install -j 8
```

## Run the test cases

```bash
cd OrderMatchingSimulator/tests/
./OrderMatchingSimulatorTest
```
