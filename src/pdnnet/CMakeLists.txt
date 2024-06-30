cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# support library, including compiled C and C++ headers
add_library(pdnnet socket.c)
# need to link Windows Sockets 2 libraries publicly (part of interface)
if(WIN32)
    target_link_libraries(pdnnet PUBLIC ws2_32)
endif()
# define macro during library compilation for symbol import/export
set_target_properties(pdnnet PROPERTIES DEFINE_SYMBOL PDNNET_BUILD_DLL)
# public headers to install. cliopt.h and cliopt headers not public
set(
    PDNNET_PDNNET_PUBLIC_HEADERS
    ${PDNNET_INCLUDE_DIR}/pdnnet/common.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/dllexport.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/error.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/features.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/inet.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/platform.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/sa.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/socket.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/warnings.h
)
# note: must be quoted
set_target_properties(
    pdnnet PROPERTIES
    PUBLIC_HEADER "${PDNNET_PDNNET_PUBLIC_HEADERS}"
)
