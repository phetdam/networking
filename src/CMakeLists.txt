cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# not written portably (yet)
if(UNIX)
    add_subdirectory(pdnnet)
    # toy acknowledgment client
    add_executable(ackclient ackclient.c)
    target_link_libraries(ackclient PRIVATE pdnnet)
    # toy acknowledgment server
    add_executable(ackserver ackserver.c)
    target_link_libraries(ackserver PRIVATE pdnnet)
    # unnecessary, but demonstrate how we can redefine some command-line option
    # default during compilation. here we update max read size, i.e. number of
    # bytes every read/recv requests, and max number of allowed connections
    target_compile_definitions(
        ackserver
        PRIVATE
            PDNNET_CLIOPT_MAX_MESSAGE_BYTES=4096
            PDNNET_ADD_CLIOPT_MAX_CONNECT_DEFAULT=5
    )
    # toy echo server
    add_executable(echoserver echoserver.cc)
endif()
