cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# pdnnet: C support library
add_library(pdnnet socket.c)
add_library(pdnnet::pdnnet ALIAS pdnnet)
target_include_directories(
    pdnnet PUBLIC
    # TODO: use PDNNET_INCLUDE_DIR here?
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
# need to link Windows Sockets 2 libraries publicly (part of interface)
if(WIN32)
    target_link_libraries(pdnnet PUBLIC ws2_32)
endif()
# define macro during build for symbol import/export + debug suffix
set_target_properties(
    pdnnet PROPERTIES
    DEFINE_SYMBOL PDNNET_BUILD_DLL
    DEBUG_POSTFIX d
)
# install rule for exported target
install(TARGETS pdnnet EXPORT pdnnet_exports)
