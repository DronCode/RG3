cmake_minimum_required(VERSION 3.26)
project(RG3)


set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Cpp)
add_subdirectory(LLVM)
add_subdirectory(PyBind)

add_subdirectory(ThirdParty/googletest)

add_subdirectory(Tests/Unit)