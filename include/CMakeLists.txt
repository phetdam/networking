cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# C++ header-only library
add_library(pdnnet++ INTERFACE)
# set public headers to install. cliopt.h and cliopt headers not public
set(
    PDNNET_PDNNETXX_PUBLIC_HEADERS
    ${PDNNET_INCLUDE_DIR}/pdnnet/common.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/echoserver.hh
    ${PDNNET_INCLUDE_DIR}/pdnnet/error.hh
    ${PDNNET_INCLUDE_DIR}/pdnnet/features.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/memory.hh
    ${PDNNET_INCLUDE_DIR}/pdnnet/platform.h
    ${PDNNET_INCLUDE_DIR}/pdnnet/process.hh
    ${PDNNET_INCLUDE_DIR}/pdnnet/server.hh
    ${PDNNET_INCLUDE_DIR}/pdnnet/socket.hh
    ${PDNNET_INCLUDE_DIR}/pdnnet/tls.hh
    ${PDNNET_INCLUDE_DIR}/pdnnet/warnings.h
)
# note: must be quoted
set_target_properties(
    pdnnet++ PROPERTIES
    PUBLIC_HEADER "${PDNNET_PDNNETXX_PUBLIC_HEADERS}"
)