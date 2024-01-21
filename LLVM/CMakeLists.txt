project(RG3_LLVM)

# ------- Boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS system filesystem REQUIRED)

# ------- LLVM BACKEND
find_package(LLVM REQUIRED CONFIG)
find_package(CLANG REQUIRED CONFIG)
message(STATUS "Found CLANG ${LLVM_VERSION} (${LLVM_DIR})")

# ------- RG3 LLVM
file(GLOB_RECURSE RG3_LLVM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
add_library(RG3_LLVM STATIC ${RG3_LLVM_SOURCES})
add_library(RG3::LLVM ALIAS RG3_LLVM)
target_include_directories(RG3_LLVM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(RG3_LLVM PUBLIC ${CLANG_INCLUDE_DIRS})
target_include_directories(RG3_LLVM PUBLIC ${LLVM_INCLUDE_DIRS})

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
