cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# CMake script to set up preferred Doxygen configuration

set(DOXYGEN_HTML_COLORSTYLE "DARK")
# warning + message settings
set(DOXYGEN_QUIET YES)
set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
# predefined macros
set(DOXYGEN_PREDEFINED "PDNNET_DOXYGEN")
# ensure conditional compilation on platforms works correctly
if(WIN32)
    list(APPEND DOXYGEN_PREDEFINED "_WIN32")
else()
    list(APPEND DOXYGEN_PREDEFINED "PDNNET_UNIX")
endif()
# ensure the cliopt headers get documentation generated
list(
    APPEND DOXYGEN_PREDEFINED
        PDNNET_ADD_CLIOPT_VERBOSE
        PDNNET_ADD_CLIOPT_HOST
        PDNNET_ADD_CLIOPT_PORT
        PDNNET_ADD_CLIOPT_PATH
        PDNNET_ADD_CLIOPT_MESSAGE_BYTES
        PDNNET_ADD_CLIOPT_MAX_CONNECT
        PDNNET_ADD_CLIOPT_TIMEOUT
)
# use Markdown main page
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${PROJECT_SOURCE_DIR}/doc/README.md)
# use custom stylesheet
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${PROJECT_SOURCE_DIR}/doc/pdnnet.css)
# strip include path
set(DOXYGEN_STRIP_FROM_INC_PATH ${PROJECT_SOURCE_DIR}/include)
# use dynamic sections for cleaner page look (hide dot graphs by default)
set(DOXYGEN_HTML_DYNAMIC_SECTIONS YES)
