# Example CMakeLists.txt - demonstrates RG3 codegen integration
# 
# This shows how to use add_codegen() to integrate RG3-based
# code generation with your CMake project.
# 
# Key feature: Python DISCOVERs output files automatically.
# No need to pre-declare OUTPUTS!

cmake_minimum_required(VERSION 3.26)
project(RG3CodegenExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Add RG3 CMake modules ----
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../CMake/Modules")
include(RG3Codegen)

# ---- Find Python (required by RG3 codegen) ----
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# ---- Example: Generate serialization code from C++ types ----

# Headers to analyze
set(EXAMPLE_HEADERS
    "include/example_types.hpp"
)

# Codegen script
set(EXAMPLE_CODEGEN_SCRIPT
    "${CMAKE_CURRENT_LIST_DIR}/serialize_codegen.py"
)

# Create codegen target - OUTPUTS discovered automatically!
add_codegen(
    TARGET example_serialize_codegen
    HEADERS ${EXAMPLE_HEADERS}
    CODEGEN_SCRIPT ${EXAMPLE_CODEGEN_SCRIPT}
    INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# ---- Print summary ----
message(STATUS "")
message(STATUS "RG3 Codegen Example:")
message(STATUS "  Analyzing: ${EXAMPLE_HEADERS}")
message(STATUS "  Script: ${EXAMPLE_CODEGEN_SCRIPT}")
message(STATUS "  Note: Outputs are discovered automatically!")
