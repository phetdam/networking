cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# pdnnet++: C++ header-only library
add_library(pdnnet++ INTERFACE)
add_library(pdnnet::pdnnet++ ALIAS pdnnet++)
target_include_directories(
    pdnnet++ INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
# on Windows, Windows Sockets 2 is part of the link interface
if(WIN32)
    target_link_libraries(pdnnet++ INTERFACE ws2_32)
endif()
# install rule for target
install(TARGETS pdnnet++ EXPORT pdnnet_exports)
# install rule for headers
install(
    DIRECTORY pdnnet
    TYPE INCLUDE
    # C and C++ headers
    PATTERN "*.h"
    PATTERN "*.hh"
    # cliopt.h and cliopt headers are not public
    PATTERN "cliopt.h" EXCLUDE
    PATTERN "cliopt/*" EXCLUDE
)
