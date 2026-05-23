add_executable(grbl_probe grbl_probe.cpp)
target_link_libraries(grbl_probe PRIVATE grbl_protocol::grbl_protocol)
target_compile_options(grbl_probe PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
)
