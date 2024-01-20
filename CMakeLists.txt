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
option(ENABLE_ASAN "Enable AddressSanitizer instrumentation" OFF)
option(ENABLE_DOCS "Enable building HTML docs target with Doxygen" ON)

# check if multi-config generator
get_property(PDNNET_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# set some system information variables used for the version info. note the
# relevant CMake variables used will be empty if used before project()
set(PDNNET_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
set(PDNNET_SYSTEM_VERSION ${CMAKE_SYSTEM_VERSION})
set(PDNNET_SYSTEM_ARCH ${CMAKE_SYSTEM_PROCESSOR})

# only Debug and Release builds available
set(PDNNET_BUILD_CONFIGS Debug Release)
# use Debug as the default build
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
    message(STATUS "Config: ${CMAKE_BUILD_TYPE}")
endif()
# print library config
if(BUILD_SHARED_LIBS)
    message(STATUS "Libraries: Shared")
else()
    message(STATUS "Libraries: Static")
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

# eliminate per-configuration subdirs from being appended to build artifact
# output dirs for multi-config generators, e.g. Visual Studio
set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$<$<CONFIG:Debug>:>
)
set(
    CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}$<$<CONFIG:Debug>:>
)
set(
    CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}$<$<CONFIG:Debug>:>
)

# update CMake module path for our CMake scripts
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# add compile options
include(ConfigCompile)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

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
if(FETCH_GTEST)
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
    find_package(GTest ${PDNNET_GTEST_VERSION})
    # for consistency when using FetchContent
    set(PDNNET_GTEST_FOUND GTest_FOUND)
endif()

# find Doxygen to enable documentation building. if not found, can't build
find_package(Doxygen 1.9)
if(NOT DOXYGEN_FOUND)
    set(ENABLE_DOCS OFF)
endif()
# warn that dark mode docs will not be built if we don't have 1.9.5
if(DOXYGEN_FOUND AND DOXYGEN_VERSION VERSION_LESS 1.9.5)
    message(
        WARNING
        "Doxygen ${DOXYGEN_VERSION} does not support dark mode theme \
(require 1.9.5)"
    )
endif()
# print status
if(ENABLE_DOCS)
    message(STATUS "docs target: Yes")
else()
    message(STATUS "docs target: No")
endif()

add_subdirectory(src)

# only add tests if Google Test was found
if(PDNNET_GTEST_FOUND)
    enable_testing()
    # no tests yet, uncomment later
    # add_subdirectory(test)
else()
    message(
        WARNING
        "Google Test >=${PDNNET_GTEST_VERSION} not found. No tests will be built."
    )
endif()

# Doxygen config options
set(DOXYGEN_HTML_COLORSTYLE "DARK")
set(DOXYGEN_PREDEFINED "PDNNET_DOXYGEN")
if(WIN32)
    set(DOXYGEN_PREDEFINED ${DOXYGEN_PREDEFINED} "_WIN32")
else()
    set(DOXYGEN_PREDEFINED ${DOXYGEN_PREDEFINED} "PDNNET_UNIX")
endif()

# build docs if able
if(ENABLE_DOCS)
    doxygen_add_docs(
        docs
        # headers
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        # specifying ALL will make this part of the default build target
        COMMENT "Building documentation"
    )
endif()
