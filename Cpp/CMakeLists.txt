project(RG3_Cpp)

file(GLOB_RECURSE GENRY_CXX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
add_library(RG3_Cpp STATIC ${GENRY_CXX_SOURCES})
add_library(RG3::Cpp ALIAS RG3_Cpp)
target_include_directories(RG3_Cpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
