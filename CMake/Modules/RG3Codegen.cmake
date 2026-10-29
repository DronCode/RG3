# RG3Codegen.cmake - CMake integration for RG3-based code generation
#
# This module provides add_codegen() to integrate RG3 Python codegen
# with CMake build system. It extracts target properties and passes
# them to a Python script that uses rg3py to analyze C++ code and
# generate output files.
#
# Key feature: Python DISCOVERs output files — no need to pre-declare them.
#
# Two-pass workflow:
#   Pass 1 (configure): Python runs in "discover" mode, reports output file list
#   Pass 2 (build):     Python runs normally, actually generates files
#
# Usage:
#   include(RG3Codegen)
#   add_codegen(
#       TARGET my_codegen
#       TARGET_NAME my_lib
#       SOURCES src/main.cpp
#       HEADERS include/types.hpp
#       CODEGEN_SCRIPT codegen.py
#       [INCLUDE_DIRS dir1 dir2]
#       [DEFINES FLAG1 FLAG2]
#   )

cmake_minimum_required(VERSION 3.26)

# Prevent multiple inclusion
if(TARGET RG3::CodegenModule)
    return()
endif()

# ============================================================================
# JSON helpers for CMake (avoid dependency on python/jq)
# ============================================================================

function(_rg3_json_array var_name)
    # Converts a CMake list to a JSON array string
    # Usage: _rg3_json_array(my_list json_out)
    set(_result "[")
    list(LENGTH "${var_name}" _len)
    math(EXPR _len "${_len} - 1")
    if(_len < 0)
        set("${var_name}" "[]" PARENT_SCOPE)
        return()
    endif()
    foreach(_i RANGE ${_len})
        list(GET "${var_name}" ${_i} _item)
        if(_i GREATER 0)
            set(_result "${_result}, ")
        endif()
        # Escape backslashes and quotes for JSON
        string(REPLACE "\\" "\\\\" _escaped "${_item}")
        string(REPLACE "\"" "\\\"" _escaped "${_escaped}")
        set(_result "${_result}\"${_escaped}\"")
    endforeach()
    set(_result "${_result}]")
    set("${var_name}" "${_result}" PARENT_SCOPE)
endfunction()

function(_rg3_json_string var_name)
    # Escapes a single string for JSON
    string(REPLACE "\\" "\\\\" _escaped "${var_name}")
    string(REPLACE "\"" "\\\"" _escaped "${_escaped}")
    set("${var_name}" "\"${_escaped}\"" PARENT_SCOPE)
endfunction()

# ============================================================================
# Main function
# ============================================================================

function(add_codegen)
    set(options "")
    set(one_value_args TARGET TARGET_NAME CODEGEN_SCRIPT)
    set(multi_value_args SOURCES HEADERS INCLUDE_DIRS DEFINES)
    cmake_parse_arguments(CODEGEN "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Validate required arguments
    if(NOT CODEGEN_TARGET)
        message(FATAL_ERROR "add_codegen(): TARGET is required")
    endif()
    if(NOT CODEGEN_CODEGEN_SCRIPT)
        message(FATAL_ERROR "add_codegen(): CODEGEN_SCRIPT is required")
    endif()
    if(NOT CODEGEN_HEADERS AND NOT CODEGEN_SOURCES)
        message(FATAL_ERROR "add_codegen(): at least one of HEADERS or SOURCES is required")
    endif()

    # Generate unique codegen name for intermediate files
    string(MD5 CODEGEN_HASH "${CODEGEN_TARGET}")
    set(CODEGEN_NAME "rg3_codegen_${CODEGEN_HASH}")

    # ---- Extract target properties if TARGET_NAME is specified ----
    set(_cmake_standard 11)
    set(_cmake_includes "")
    set(_cmake_defines "")

    if(CODEGEN_TARGET_NAME AND TARGET ${CODEGEN_TARGET_NAME})
        # Extract C++ standard
        get_target_property(_target_type ${CODEGEN_TARGET_NAME} TYPE)
        if(_target_type STREQUAL "INTERFACE_LIBRARY")
            get_target_property(_std ${CODEGEN_TARGET_NAME} INTERFACE_CXX_STANDARD)
        else()
            get_target_property(_std ${CODEGEN_TARGET_NAME} CXX_STANDARD)
        endif()
        if(_std)
            set(_cmake_standard "${_std}")
        endif()

        # Extract include directories
        get_target_property(_incs ${CODEGEN_TARGET_NAME} INCLUDE_DIRECTORIES)
        if(_incs)
            list(APPEND _cmake_includes ${_incs})
        endif()
        get_target_property(_inc_dirs ${CODEGEN_TARGET_NAME} INTERFACE_INCLUDE_DIRECTORIES)
        if(_inc_dirs)
            list(APPEND _cmake_includes ${_inc_dirs})
        endif()

        # Extract compile definitions
        get_target_property(_defs ${CODEGEN_TARGET_NAME} COMPILE_DEFINITIONS)
        if(_defs)
            list(APPEND _cmake_defines ${_defs})
        endif()
        get_target_property(_i_defs ${CODEGEN_TARGET_NAME} INTERFACE_COMPILE_DEFINITIONS)
        if(_i_defs)
            list(APPEND _cmake_defines ${_i_defs})
        endif()
    endif()

    # Merge with user-provided values
    if(CODEGEN_INCLUDE_DIRS)
        list(APPEND _cmake_includes ${CODEGEN_INCLUDE_DIRS})
    endif()
    if(CODEGEN_DEFINES)
        list(APPEND _cmake_defines ${CODEGEN_DEFINES})
    endif()

    # Deduplicate
    list(REMOVE_DUPLICATES _cmake_includes)
    list(REMOVE_DUPLICATES _cmake_defines)

    # ---- Set up paths ----
    if(NOT RG3_CODEGEN_ENTRY_PATH)
        if(DEFINED ENV{RG3_SOURCE_DIR})
            set(RG3_CODEGEN_ENTRY_PATH "$ENV{RG3_SOURCE_DIR}/PyBind/source/rg3_codegen_entry.py")
        else()
            set(RG3_CODEGEN_ENTRY_PATH "${CMAKE_CURRENT_LIST_DIR}/../../PyBind/source/rg3_codegen_entry.py")
        endif()
    endif()

    # Resolve Python interpreter
    if(NOT RG3_PYTHON)
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        set(RG3_PYTHON Python3::Interpreter)
    endif()
    # Get the actual python executable path for execute_process
    if(TARGET Python3::Interpreter)
        get_target_property(_python_exe ${RG3_PYTHON} IMPORTED_LOCATION)
        if(NOT _python_exe)
            # Fallback: get from properties
            get_target_property(_python_exe ${RG3_PYTHON} LOCATION)
        endif()
        if(NOT _python_exe)
            execute_process(
                COMMAND ${Python3_EXECUTABLE} -c "import sys; print(sys.executable)"
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE _python_exe
            )
        endif()
    else()
        set(_python_exe ${RG3_PYTHON})
    endif()

    # ---- Output directory ----
    set(CODEGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/${CODEGEN_NAME}")

    # ---- Manifest paths ----
    set(CODEGEN_INPUT_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/${CODEGEN_NAME}_input.json")
    set(CODEGEN_DISCOVER_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/${CODEGEN_NAME}_discovered.json")
    set(CODEGEN_OUTPUT_MANIFEST "${CODEGEN_OUTPUT_DIR}/${CODEGEN_NAME}_output.json")

    # ---- Build input JSON ----
    _rg3_json_array(CODEGEN_SOURCES)
    set(_sources_json "${CODEGEN_SOURCES}")

    _rg3_json_array(CODEGEN_HEADERS)
    set(_headers_json "${CODEGEN_HEADERS}")

    _rg3_json_array(_cmake_includes)
    set(_includes_json "${_cmake_includes}")

    _rg3_json_array(_cmake_defines)
    set(_defines_json "${_cmake_defines}")

    # Write input manifest (same for both discover and generate passes)
    file(WRITE "${CODEGEN_INPUT_MANIFEST}"
        "{\n"
        "  \"sources\": ${_sources_json},\n"
        "  \"headers\": ${_headers_json},\n"
        "  \"include_dirs\": ${_includes_json},\n"
        "  \"compile_definitions\": ${_defines_json},\n"
        "  \"cpp_standard\": ${_cmake_standard},\n"
        "  \"output_dir\": \"${CODEGEN_OUTPUT_DIR}\",\n"
        "  \"codegen_script\": \"${CODEGEN_CODEGEN_SCRIPT}\"\n"
        "}\n"
    )

    # ========================================================================
    # PASS 1: Discover outputs (at configure time)
    # ========================================================================
    message(STATUS "[RG3 Codegen] Discovering outputs for: ${CODEGEN_TARGET}")
    
    execute_process(
        COMMAND ${_python_exe}
            "${RG3_CODEGEN_ENTRY_PATH}"
            "${CODEGEN_INPUT_MANIFEST}"
            "${CODEGEN_DISCOVER_MANIFEST}"
            "--discover"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        RESULT_VARIABLE _discover_result
        OUTPUT_VARIABLE _discover_output
        ERROR_VARIABLE _discover_error
        TIMEOUT 120
    )

    if(NOT _discover_result EQUAL 0)
        message(WARNING "[RG3 Codegen] Discovery failed for '${CODEGEN_TARGET}':")
        if(_discover_error)
            message(WARNING "  ${_discover_error}")
        endif()
        if(_discover_output)
            message(WARNING "  ${_discover_output}")
        endif()
        message(WARNING "  Make sure rg3py is available and the codegen script is correct.")
        message(WARNING "  Skipping codegen target '${CODEGEN_TARGET}'.")
        return()
    endif()

    # Read discovered outputs from JSON
    file(READ "${CODEGEN_DISCOVER_MANIFEST}" _discovered_json)
    
    # Parse the discovered outputs using Python (reliable JSON parsing)
    execute_process(
        COMMAND ${_python_exe} -c "
import json, sys
with open('${CODEGEN_DISCOVER_MANIFEST}') as f:
    data = json.load(f)
for f in data.get('outputs', []):
    print(f)
"
        OUTPUT_VARIABLE _discovered_raw
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _parse_result
    )

    if(NOT _parse_result EQUAL 0)
        message(FATAL_ERROR "[RG3 Codegen] Failed to parse discovered outputs for '${CODEGEN_TARGET}'")
    endif()

    # Convert discovered outputs to CMake list
    string(REPLACE "\n" ";" _discovered_list "${_discovered_raw}")
    list(FILTER _discovered_list EXCLUDE REGEX "^$")
    
    set(CODEGEN_DISCOVERED_OUTPUTS "${_discovered_list}")

    if(CODEGEN_DISCOVERED_OUTPUTS STREQUAL "")
        message(WARNING "[RG3 Codegen] No outputs discovered for '${CODEGEN_TARGET}'. "
                        "The codegen script may not be generating any files.")
    else()
        message(STATUS "[RG3 Codegen] Discovered ${CODEGEN_DISCOVERED_OUTPUTS} output(s):")
        foreach(_out ${CODEGEN_DISCOVERED_OUTPUTS})
            message(STATUS "    - ${_out}")
        endforeach()
    endif()

    # ========================================================================
    # PASS 2: Build command (at build time)
    # ========================================================================
    
    # Build absolute output paths for CMake custom command
    set(_cmake_outputs)
    foreach(_out ${CODEGEN_DISCOVERED_OUTPUTS})
        if(IS_ABSOLUTE "${_out}")
            list(APPEND _cmake_outputs "${_out}")
        else()
            list(APPEND _cmake_outputs "${CODEGEN_OUTPUT_DIR}/${_out}")
        endif()
    endforeach()
    # Also include the output manifest as an output
    list(APPEND _cmake_outputs "${CODEGEN_OUTPUT_MANIFEST}")

    if(_cmake_outputs)
        add_custom_command(
            OUTPUT ${_cmake_outputs}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CODEGEN_OUTPUT_DIR}"
            COMMAND ${_python_exe}
                "${RG3_CODEGEN_ENTRY_PATH}"
                "${CODEGEN_INPUT_MANIFEST}"
                "${CODEGEN_OUTPUT_MANIFEST}"
            DEPENDS
                "${CODEGEN_INPUT_MANIFEST}"
                "${CODEGEN_CODEGEN_SCRIPT}"
                "${RG3_CODEGEN_ENTRY_PATH}"
            COMMENT "[RG3] Running codegen: ${CODEGEN_TARGET}"
            VERBATIM
        )
    endif()

    # ---- Add target to track codegen ----
    if(_cmake_outputs)
        add_custom_target(${CODEGEN_TARGET}
            DEPENDS ${_cmake_outputs}
        )
    else()
        add_custom_target(${CODEGEN_TARGET})
    endif()

    # ---- Link generated files to target ----
    if(CODEGEN_TARGET_NAME AND TARGET ${CODEGEN_TARGET_NAME} AND _cmake_outputs)
        # Add generated files as sources
        target_sources(${CODEGEN_TARGET_NAME} PRIVATE ${_cmake_outputs})
        add_dependencies(${CODEGEN_TARGET_NAME} ${CODEGEN_TARGET})

        # Add output dir to include path
        target_include_directories(${CODEGEN_TARGET_NAME} PRIVATE "${CODEGEN_OUTPUT_DIR}")
    endif()

    # ---- Print summary ----
    message(STATUS "")
    message(STATUS "[RG3 Codegen] ${CODEGEN_TARGET}:")
    message(STATUS "  Target: ${CODEGEN_TARGET_NAME}")
    if(CODEGEN_SOURCES)
        message(STATUS "  Sources: ${CODEGEN_SOURCES}")
    endif()
    if(CODEGEN_HEADERS)
        message(STATUS "  Headers: ${CODEGEN_HEADERS}")
    endif()
    if(CODEGEN_DISCOVERED_OUTPUTS)
        message(STATUS "  Discovered outputs (${CODEGEN_DISCOVERED_OUTPUTS} files):")
        foreach(_out ${CODEGEN_DISCOVERED_OUTPUTS})
            message(STATUS "    - ${_out}")
        endforeach()
    endif()
    message(STATUS "  Output dir: ${CODEGEN_OUTPUT_DIR}")
    message(STATUS "  C++ Standard: ${_cmake_standard}")
    message(STATUS "")
endfunction()
