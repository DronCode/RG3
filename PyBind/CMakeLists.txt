project(RG3_PyBind)

set(Boost_USE_STATIC_LIBS ON)

# ------- LLVM BACKEND
find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
#./bootstrap.sh --with-libraries=python,system,filesystem --with-python=/Users/USERNAME/.pyenv/versions/3.10-dev/bin/python3 --with-python-version=3.10 --with-python-root=/Users/USERNAME/.pyenv/versions/3.10-dev/bin/python3
find_package(Boost COMPONENTS python filesystem REQUIRED HINTS $ENV{Boost_ROOT})
message("Boost found at $ENV{Boost_ROOT}")

# ------- RG3 PyBind frontend
file(GLOB_RECURSE RG3_PYBIND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
add_library(RG3_PyBind SHARED ${RG3_PYBIND_SOURCES})
add_library(RG3::PyBind ALIAS RG3_PyBind)
target_include_directories(RG3_PyBind PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(RG3_PyBind PUBLIC RG3::Cpp RG3::LLVM)

target_link_libraries(RG3_PyBind PUBLIC ${Boost_LIBRARIES} ${Python3_LIBRARIES} Python3::Module)
target_link_libraries(RG3_PyBind PRIVATE fmt::fmt)
target_include_directories(RG3_PyBind PUBLIC ${Python3_INCLUDE_DIRS})

if (WIN32)
    set_target_properties(RG3_PyBind PROPERTIES
            OUTPUT_NAME "rg3py"
            SUFFIX ".pyd"
            PREFIX ""
    )
else()
    set_target_properties(RG3_PyBind PROPERTIES
            OUTPUT_NAME "rg3py"
            SUFFIX ".so"
            PREFIX ""
    )
endif()

# After build
message(STATUS "[CI|POST_BUILD] Copy native extension to ${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_command(
        TARGET RG3_PyBind
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RG3_PyBind> ${CMAKE_CURRENT_SOURCE_DIR}/$<TARGET_FILE_NAME:RG3_PyBind>)