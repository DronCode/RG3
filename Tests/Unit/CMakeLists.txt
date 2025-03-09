cmake_minimum_required(VERSION 3.26)
project(RG3_Tests_Unit)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE RG3_UNIT_SOURCES "source/*.cpp")

add_executable(RG3_Unit ${RG3_UNIT_SOURCES})
target_link_libraries(RG3_Unit
        gtest
        RG3::LLVM
        RG3::Cpp
        RG3::Protobuf)