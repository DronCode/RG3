project(RG3_PyBind)

set(Boost_USE_STATIC_LIBS ON)

# ------- LLVM BACKEND
find_package(PythonLibs 3.10...3.99 REQUIRED)   # 3.99 to support all 3.x Python versions (dev based on 3.10)
find_package(Boost COMPONENTS python filesystem REQUIRED)

# ------- RG3 PyBind frontend
file(GLOB_RECURSE RG3_PYBIND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
add_library(RG3_PyBind SHARED ${RG3_PYBIND_SOURCES})
add_library(RG3::PyBind ALIAS RG3_PyBind)
target_include_directories(RG3_PyBind PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(RG3_PyBind PUBLIC RG3::Cpp RG3::LLVM)

target_link_libraries(RG3_PyBind PUBLIC ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
target_include_directories(RG3_PyBind PUBLIC ${PYTHON_INCLUDE_DIRS})

set_target_properties(RG3_PyBind PROPERTIES OUTPUT_NAME "rg3py" SUFFIX ".pyd")