project(RG3_LLVM)

# ------- Boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS filesystem REQUIRED HINTS $ENV{Boost_ROOT})

# ------- Protobuf
# Required to have Protobuf_DIR, absl_DIR and utf8_range_DIR

find_package(absl REQUIRED CONFIG HINTS $ENV{absl_DIR})
find_package(utf8_range REQUIRED CONFIG HINTS $ENV{utf8_range_DIR})
find_package(Protobuf REQUIRED CONFIG HINTS $ENV{Protobuf_DIR})

message(STATUS "Found Protobuf ${Protobuf_VERSION} (${Protobuf_DIR})")

# ------- RG3 Protobuf
file(GLOB_RECURSE RG3_PROTOBUF_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
add_library(RG3_Protobuf STATIC ${RG3_PROTOBUF_SOURCES})
add_library(RG3::Protobuf ALIAS RG3_Protobuf)
target_include_directories(RG3_Protobuf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(RG3_Protobuf PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(RG3_Protobuf PUBLIC ${absl_INCLUDE_DIRS})
target_include_directories(RG3_Protobuf PUBLIC ${utf8_range_INCLUDE_DIRS})

if (MSVC)
    # /bigobj support
    target_compile_options(RG3_LLVM PUBLIC /bigobj)
endif()

target_link_libraries(RG3_Protobuf PUBLIC
        # Protobuf
        protobuf::libprotobuf
        protobuf::libprotoc
        protobuf::libupb
        utf8_range::utf8_range
        absl::base
        absl::strings
        # Boost
        ${Boost_LIBRARIES}       # RG3
        RG3::Cpp
        # FMT
        fmt::fmt
)