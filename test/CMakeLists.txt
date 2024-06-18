cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# integration test for the echo server and client
add_executable(echoserver_test echoserver_test.cc)
target_link_libraries(echoserver_test PRIVATE GTest::gtest_main)
if(WIN32)
    target_link_libraries(echoserver_test PRIVATE ws2_32)
endif()

add_test(NAME echoserver_test COMMAND echoserver_test)
