cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# not written portably
if(UNIX)
    add_executable(acksvr acksvr.c)
    target_compile_definitions(acksvr PRIVATE MAX_READ_SIZE=4096)
endif()
