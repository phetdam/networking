cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# echoserver_test: integration tests for the echo server and client
add_executable(echoserver_test echoserver_test.cc)
target_link_libraries(echoserver_test PRIVATE pdnnet++ GTest::gtest_main)
add_test(NAME echoserver_test COMMAND echoserver_test)

# executor_test: integration tests for the thread-based executor
# add_executable(executor_test executor_test.cc)
