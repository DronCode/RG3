project(RG3_LLVM)

# ------- Boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS python filesystem REQUIRED HINTS $ENV{Boost_ROOT})

# ------- LLVM BACKEND
find_package(LLVM REQUIRED CONFIG HINTS $ENV{LLVM_DIR})
find_package(Clang REQUIRED CONFIG HINTS $ENV{CLANG_DIR})
message(STATUS "Found Clang ${LLVM_VERSION} (${LLVM_DIR})")

# ------- RG3 Git Hash
find_package(Git REQUIRED)

execute_process(
        COMMAND git log -1 --pretty=format:%h
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
)

message(STATUS "RG3 Git hash: ${GIT_HASH}")

configure_file(${CMAKE_SOURCE_DIR}/CMake/RG3_Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/RG3_Config.h @ONLY)

# ------- RG3 LLVM
file(GLOB_RECURSE RG3_LLVM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
add_library(RG3_LLVM STATIC ${RG3_LLVM_SOURCES})
add_library(RG3::LLVM ALIAS RG3_LLVM)
target_include_directories(RG3_LLVM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(RG3_LLVM PUBLIC ${CLANG_INCLUDE_DIRS})
target_include_directories(RG3_LLVM PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(RG3_LLVM PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(RG3_LLVM PUBLIC
        # LLVM
        LLVMCore
        LLVMSupport
        clangAST
        clangIndex
        clangBasic
        clangSupport
        clangFrontend
        clangToolingCore
        clangSerialization
        clangASTMatchers
        # Boost
        ${Boost_LIBRARIES}
        # RG3
        RG3::Cpp
)