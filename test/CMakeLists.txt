include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(grbl_protocol_tests
    test_status_report.cpp
    test_setting.cpp
    test_response.cpp
    test_push_message.cpp
    test_modal_state.cpp
    test_realtime.cpp
    test_commands.cpp
)
target_link_libraries(grbl_protocol_tests PRIVATE
    grbl_protocol::grbl_protocol
    Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(grbl_protocol_tests)
