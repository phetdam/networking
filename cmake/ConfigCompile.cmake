cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

if(MSVC)
    add_compile_definitions(
        # stop MSVC from warning about unsafe functions, C is unsafe by nature
        _CRT_SECURE_NO_WARNINGS
        # disable warnings about deprecated Windows Sockets API functions
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
    add_compile_options(
        /Wall
        # enum value not explicitly handled in by case label in switch
        /wd4061
        # implicit definition of copy, move ctor/operator= as deleted. MSVC
        # tends to emit this whenenever one uses Google Test.
        /wd4625 /wd4626 /wd5026 /wd5027
        # unreferenced inline function removed
        /wd4514
        # assignment within conditional expression
        /wd4706
        # /Wall enables excessive warnings about automatic inline expansion
        /wd4710 /wd4711
        # silence warnings on padding emitted when compiling x64 binaries
        /wd4820
        # compiler may not enforce left-to-right eval in braced init list
        /wd4868
        # pointer/reference to potentially throwing function passed to extern
        # "C" function with /EHsc exception handling model. originally emitted
        # by the winbase.h and WS2tcpip.h Win32 headers
        /wd5039
        # Spectre mitigation, winbase.h macro expansion issue
        /wd5045 /wd5105
        # /Od applied by default when using Debug config, /O2 for Release
    )
    # no ENABLE_ASAN for Windows yet. have had problems before getting it to
    # work nicely with Google Test and the Windows implementation is missing
    # LeakSanitizer that is available for *nix.
# Clang/GCC can accept most of the same options
else()
    add_compile_options(
        -Wall
        # -O0 is default optimization level anyways
        $<IF:$<CONFIG:Release>,-O3,-O0> $<$<NOT:$<CONFIG:Release>>:-g>
    )
    # enable AddressSanitizer use
    if(ENABLE_ASAN)
        message(STATUS "AddressSanitizer (-fsanitize=address) enabled")
        # must specifiy for both compile and link
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()
