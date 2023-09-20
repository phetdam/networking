cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

add_library(pdnnet csocket.c)
set_target_properties(pdnnet PROPERTIES DEFINE_SYMBOL PDNNET_BUILD_DLL)
