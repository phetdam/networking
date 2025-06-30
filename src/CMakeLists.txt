cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# C socket support library
add_subdirectory(pdnnet)

# not written portably (yet)
if(UNIX)
    # C toy acknowledgment client
    add_executable(ackclient ackclient.c)
    target_link_libraries(ackclient PRIVATE pdnnet)
    # C toy acknowledgment server
    add_executable(ackserver ackserver.c)
    target_link_libraries(ackserver PRIVATE pdnnet)
    # unnecessary, but demonstrate how we can redefine some command-line option
    # default during compilation. here we update max read size, i.e. number of
    # bytes every read/recv requests, and max number of allowed connections
    target_compile_definitions(
        ackserver
        PRIVATE
            PDNNET_CLIOPT_MESSAGE_BYTES_MAX=4096
            PDNNET_ADD_CLIOPT_MAX_CONNECT_DEFAULT=5
    )
endif()
# C++ toy acknowledgment client
add_executable(ackclient++ ackclient++.cc)
target_link_libraries(ackclient++ PRIVATE pdnnet++)
# C++ toy acknowledgment server
add_executable(ackserver++ ackserver++.cc)
target_link_libraries(ackserver++ PRIVATE pdnnet++)
# C++ toy echo client
add_executable(echoclient echoclient.cc)
target_link_libraries(echoclient PRIVATE pdnnet++)
# C++ toy echo server
add_executable(echoserver echoserver.cc)
target_link_libraries(echoserver PRIVATE pdnnet++)
# C++ toy HTTPS client
add_executable(httpsclient httpsclient.cc)
target_link_libraries(httpsclient PRIVATE pdnnet++)
# different platforms require different TLS libraries
if(WIN32)
    target_link_libraries(httpsclient PRIVATE secur32)
else()  # UNIX
    target_link_libraries(httpsclient PRIVATE crypto ssl)
endif()
# list of portable executable targets
set(
    PDNNET_PROGRAMS
        ackclient++
        ackserver++
        echoclient
        echoserver
        httpsclient
)
# note: ackclient, ackserver not available for Windows
if(UNIX)
    list(PREPEND PDNNET_PROGRAMS ackclient ackserver)
endif()
# install rule for all executable targets
install(TARGETS ${PDNNET_PROGRAMS})
