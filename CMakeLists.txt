if(ESP_PLATFORM)
    idf_component_register(
        SRCS
            "src/status_report.cpp"
            "src/setting.cpp"
            "src/response.cpp"
            "src/push_message.cpp"
        INCLUDE_DIRS
            "include"
    )
    target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
    return()
endif()

cmake_minimum_required(VERSION 3.20)
project(grbl_protocol
    VERSION 0.3.0
    DESCRIPTION "GRBL protocol parsing and framing library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_library(grbl_protocol
    src/status_report.cpp
    src/setting.cpp
    src/response.cpp
    src/push_message.cpp
)
add_library(grbl_protocol::grbl_protocol ALIAS grbl_protocol)

target_include_directories(grbl_protocol PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(grbl_protocol PUBLIC cxx_std_17)
target_compile_options(grbl_protocol PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
)

include(CTest)
if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(tools)
endif()
