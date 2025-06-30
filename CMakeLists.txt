cmake_minimum_required(VERSION 3.21)

# project version values and full version
set(PDNNET_VERSION_MAJOR 0)
set(PDNNET_VERSION_MINOR 1)
set(PDNNET_VERSION_PATCH 0)
set(
    PDNNET_VERSION
    ${PDNNET_VERSION_MAJOR}.${PDNNET_VERSION_MINOR}.${PDNNET_VERSION_PATCH}
)

project(
    networking
    VERSION ${PDNNET_VERSION}
    DESCRIPTION "C/C++ networking examples."
    HOMEPAGE_URL https://github.com/phetdam/networking
    LANGUAGES C CXX
)

option(BUILD_SHARED_LIBS "Build libraries as shared" ON)
option(PDNNET_ENABLE_ASAN "Enable AddressSanitizer instrumentation" OFF)
# provide an extra $<CONFIG> subdirectory for lib and bin on Windows. this is
# on by default; we don't support flat install with "d" suffix on Windows yet
# TODO: remove this option since we use "d" suffix universally now
option(
    PDNNET_INSTALL_MULTI_CONFIG
    "Add per-config lib + bin subdirectories for Windows installations" ON
)
option(PDNNET_FETCH_GTEST "Use FetchContent to build local Google Test copy" OFF)

# update CMake module path for our CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# include our CMake modules
include(pdnnet_config_compile)

# find Git
find_package(Git 2.30)
if(Git_FOUND)
    message(STATUS "Git version: ${GIT_VERSION_STRING}")
else()
    message(STATUS "Git version: None")
endif()

# TODO: if Git found, get build info unless a release build is being done

# check if multi-config generator
get_property(PDNNET_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(PDNNET_IS_MULTI_CONFIG)
    message(STATUS "Generator config: Multi")
else()
    message(STATUS "Generator config: Single")
endif()

# set some system information variables used for the version info. note the
# relevant CMake variables used will be empty if used before project()
set(PDNNET_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
set(PDNNET_SYSTEM_VERSION ${CMAKE_SYSTEM_VERSION})
set(PDNNET_SYSTEM_ARCH ${CMAKE_SYSTEM_PROCESSOR})

# only Debug and Release builds available
set(PDNNET_BUILD_CONFIGS Debug Release)
# use Debug as the default build (ignored for multi-config)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
# check that user didn't specify something unrecognized
if(CMAKE_BUILD_TYPE IN_LIST PDNNET_BUILD_CONFIGS)
else()
    message(
        FATAL_ERROR
        "Error: CMAKE_BUILD_TYPE must be one of ${PDNNET_BUILD_CONFIGS}"
    )
endif()
# if single-config generator, print config
if(NOT PDNNET_IS_MULTI_CONFIG)
    message(STATUS "Build config: ${CMAKE_BUILD_TYPE}")
endif()
# print library config
if(BUILD_SHARED_LIBS)
    message(STATUS "Build libraries: Shared")
else()
    message(STATUS "Build libraries: Static")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add version definitions as macro definitions
add_compile_definitions(
    PDNNET_VERSION_MAJOR=${PDNNET_VERSION_MAJOR}
    PDNNET_VERSION_MINOR=${PDNNET_VERSION_MINOR}
    PDNNET_VERSION_PATCH=${PDNNET_VERSION_PATCH}
    # need double quotes for CMake to pass as a string
    PDNNET_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    PDNNET_SYSTEM_NAME="${PDNNET_SYSTEM_NAME}"
    PDNNET_SYSTEM_VERSION="${PDNNET_SYSTEM_VERSION}"
    PDNNET_SYSTEM_ARCH="${PDNNET_SYSTEM_ARCH}"
)

# put all build artifacts in top-level build dir. for multi-config generators
# like Visual Studio, each config's artifacts go into a subdirectory. it's not
# a good idea for multi-config generators to have different configs putting
# artifacts into the same directory since it no longer becomes possible to
# separately install debug and release artifacts.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# configure compile options
# note: may want to do this after FetchContent for Google Test as otherwise the
# compiler options can bleed in and cause Google Test compilation to fail since
# they started building treating warnings as errors
pdnnet_config_compile()

# top-level include directory
set(PDNNET_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Google Test [minimum] version to use. allow user override
if(NOT PDNNET_GTEST_VERSION)
    set(PDNNET_GTEST_VERSION 1.10.0)
endif()
# require major.minor.patch semantic version
if(NOT PDNNET_GTEST_VERSION MATCHES ^[0-9]+\.[0-9]+\.[0-9]+$)
    message(
        FATAL_ERROR
        "Google Test version ${PDNNET_GTEST_VERSION} invalid; must be of the \
format [major].[minor].[patch]"
    )
endif()
# find Google Test to enable testing. also allow use of FetchContent, which is
# convenient for CI/CD workflows or when on a system like Windows where there's
# no real way to "install" Google Test anywhere and GTEST_ROOT needs to be set.
if(PDNNET_FETCH_GTEST)
    message(STATUS "Google Test fetch: Yes")
    # URL prefix to the release archive
    set(
        GTEST_RELEASE_PREFIX
        https://github.com/google/googletest/archive/refs/tags
    )
    # Google changed release format from "release-x.y.z" to "vx.y.z" for 1.13.0+
    if(PDNNET_GTEST_VERSION VERSION_GREATER_EQUAL 1.13.0)
        set(GTEST_ARCHIVE_PREFIX "v")
    else()
        set(GTEST_ARCHIVE_PREFIX "release-")
    endif()
    # full path for convenience
    set(
        GTEST_ARCHIVE_PATH
        ${GTEST_RELEASE_PREFIX}/${GTEST_ARCHIVE_PREFIX}${PDNNET_GTEST_VERSION}.zip
    )
    # build Google Test from source as a subdirectory
    include(FetchContent)
    message(
        STATUS
        "Fetching Google Test ${PDNNET_GTEST_VERSION} from ${GTEST_ARCHIVE_PATH}"
    )
    FetchContent_Declare(googletest URL ${GTEST_ARCHIVE_PATH})
    # required on Windows to force linking against shared VC++ C runtime
    if(WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailable(googletest)
    # indicate that Google Test was found
    set(PDNNET_GTEST_FOUND TRUE)
else()
    message(STATUS "Google Test fetch: No")
    find_package(GTest ${PDNNET_GTEST_VERSION})
    # for consistency when using FetchContent
    set(PDNNET_GTEST_FOUND GTest_FOUND)
endif()
# report whether we found Google Test or not
if(PDNNET_GTEST_FOUND)
    message(STATUS "Google Test version: ${GTest_VERSION}")
else()
    message(STATUS "Google Test version: None")
endif()

# find Doxygen to enable documentation building
find_package(Doxygen 1.9)
if(DOXYGEN_FOUND)
    message(STATUS "Doxygen version: ${DOXYGEN_VERSION}")
else()
    message(STATUS "Doxygen version: None")
endif()
# warn that dark mode docs will not be built if we don't have 1.9.5
if(DOXYGEN_FOUND AND DOXYGEN_VERSION VERSION_LESS 1.9.5)
    message(
        WARNING
        "Doxygen ${DOXYGEN_VERSION} does not support dark mode theme \
(require 1.9.5)"
    )
endif()

add_subdirectory(include)
add_subdirectory(src)

# only add tests if Google Test was found
if(PDNNET_GTEST_FOUND)
    enable_testing()
    add_subdirectory(test)
else()
    message(
        WARNING
        "Google Test >=${PDNNET_GTEST_VERSION} not found. No tests will be built."
    )
endif()

# Doxygen config options
include(pdnnet_doxygen_config)

# create documentation target
if(DOXYGEN_FOUND)
    doxygen_add_docs(
        docs
        # headers
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        # main page
        # TODO: rewrite the top-level README? we can only use Markdown
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/README.md
        # specifying ALL will make this part of the default build target
        COMMENT "Building Doxygen documentation"
    )
endif()
