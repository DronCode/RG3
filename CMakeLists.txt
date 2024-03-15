cmake_minimum_required(VERSION 3.26)
project(RG3)

set(CMAKE_CXX_STANDARD 20)

if (UNIX)
    message(STATUS "-- Enabled -fPIC for UNIX based OS")
    set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif()

add_subdirectory(ThirdParty/fmt)

add_subdirectory(Cpp)
add_subdirectory(LLVM)
add_subdirectory(PyBind)

# Unit tests
add_subdirectory(ThirdParty/googletest)
add_subdirectory(Tests/Unit)

enable_testing()
add_test(NAME rg3_unit COMMAND $<TARGET_FILE:RG3_Unit> WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Tests")