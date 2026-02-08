add_executable(ezcodec_tests
    test_main.cpp
)

target_link_libraries(ezcodec_tests PRIVATE ezcodec_lib)

add_test(NAME EzCodecTests COMMAND ezcodec_tests)
