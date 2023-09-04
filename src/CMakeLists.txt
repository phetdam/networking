cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# not written portably (yet)
if(UNIX)
    add_executable(ackclient ackclient.c)
    add_executable(ackserver ackserver.c)
    # unnecessary but we explicitly set the max bytes to read() we can use
    target_compile_definitions(ackserver PRIVATE MAX_READ_SIZE=4096)
endif()
