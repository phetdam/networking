:: Build script for networking.
::
:: Originally copied from the daily-coding-problem build.bat build script.
::
:: Author: Derek Huang
:: Copyright: MIT License
::

@echo off
setlocal EnableDelayedExpansion

:: program name, as any called label uses label name as %0
set PROGNAME=%0
:: default architecture, default build output prefix, default build config
set BUILD_ARCH=x86
set BUILD_OUTPUT_PREFIX=build_windows
set BUILD_CONFIG=Debug
:: current action to take, argument parsing mode
set BUILD_ACTION=
set PARSE_ACTION=

call :Main %*
exit /b !ERRORLEVEL!

::::
:: Print build script usage.
::
:PrintUsage
    echo Usage: %PROGNAME% [-h] [-o OUTPUT_PREFIX] [-a ARCH] [-c CONFIG] ^
[-Ca CMAKE_ARGS] [-Cb CMAKE_BUILD_ARGS]
    echo.
    echo Build script for networking Windows builds.
    echo.
    echo Uses the default Visual Studio generator and toolset.
    echo.
    echo Arguments of the form KEY^=VALUE must be double quoted, otherwise the
    echo KEY and VALUE will be split into separate arguments.
    echo.
    echo Options:
    echo   -h,  --help               Print this usage
    echo.
    echo   -o,  --output-prefix OUTPUT_PREFIX
    echo                             Build output directory prefix, default
    echo                             %BUILD_OUTPUT_PREFIX%. The actual build
    echo                             output directory will have _ARCH appended
    echo                             to the specified directory prefix.
    echo.
    echo   -a,  --arch ARCH          Target machine architecture, default ^
%BUILD_ARCH%.
    echo                             Supported values: x86, x64, arm, arm64
    echo.
    echo   -c,  --config CONFIG      Build configuration, default %BUILD_CONFIG%
    echo.
    echo   -Ca, --cmake-args CMAKE_ARGS
    echo                             Args to pass to cmake config command
    echo.
    echo   -Cb, --cmake-build-args CMAKE_BUILD_ARGS
    echo                             Args to pass to cmake build command
exit /b !ERRORLEVEL!

::::
:: Set CMake Visual Studio generator target arch given the provided arch name.
::
:: This sets CMAKE_BUILD_ARCH accordingly based on the specified value.
::
:: Arguments:
::  Target arch name, one of x86, x64, arm, or arm64
::
:SetCMakeArch
:: hardcoded since doing any interesting transformations with CMD is painful
if %~1==x86 (
    set CMAKE_BUILD_ARCH=Win32
) else (
    if %~1==x64 (
        set CMAKE_BUILD_ARCH=!BUILD_ARCH!
    ) else (
        if %~1==arm (
            set CMAKE_BUILD_ARCH=ARM
        ) else (
            if %~1==arm64 (
                set CMAKE_BUILD_ARCH=ARM64
            ) else (
                echo Error: Unknown target arch %~1, must be one of x86, x64, ^
arm, arm64
                exit /b 1
            )
        )
    )
)
exit /b !ERRORLEVEL!

::::
:: Parse incoming command-line arguments.
::
:: Arguments:
::  List of command-line arguments
::
:ParseArgs
:: notes on massive if-else block (since there is no else if...):
::
:: + function exists early when -h, --help is seen
:: + when a flag like -o, -a -Ca that takes args is recognized, PARSE_ACTION is
::   first set appropriately. then, unrecognized flags are consumed based on
::   the value of PARSE_ACTION, i.e. when parse_arch BUILD_ARCH is set
:: + innermost else prints error message and exits with error if none of the
::   parse actions are correct. more commonly, it is hit because an invalid
::   value would have been specified for a particular argument.
::
for %%A in (%*) do (
    if %%A==-h (
        set BUILD_ACTION=print_usage
        exit /b 0
    ) else (
        if %%A==--help (
            set BUILD_ACTION=print_usage
            exit /b 0
        ) else (
            if %%A==-o (
                set PARSE_ACTION=parse_output_prefix
            ) else (
                if %%A==--output-prefix (
                    set PARSE_ACTION=parse_output_prefix
                ) else (
                    if %%A==-a (
                        set PARSE_ACTION=parse_arch
                    ) else (
                        if %%A==--arch (
                            set PARSE_ACTION=parse_arch
                        ) else (
                            if %%A==-c (
                                set PARSE_ACTION=parse_config
                            ) else (
                                if %%A==--config (
                                    set PARSE_ACTION=parse_config
                                    set BUILD_CONFIG=%%A
                                ) else (
                                    if %%A==-Ca (
                                        set PARSE_ACTION=parse_cmake_args
                                    ) else (
                                        if %%A==--cmake-args (
                                            set PARSE_ACTION=parse_cmake_args
                                        ) else (
                                            if %%A==-Cb (
                                        set PARSE_ACTION=parse_cmake_build_args
                                            ) else (
                                                if %%A==--cmake-build-args (
                                        set PARSE_ACTION=parse_cmake_build_args
                                                ) else (
    if !PARSE_ACTION!==parse_output_prefix (
        set "BUILD_OUTPUT_PREFIX=%%A"
    ) else (
        if !PARSE_ACTION!==parse_arch (
            set "BUILD_ARCH=%%A"
        ) else (
            if !PARSE_ACTION!==parse_config (
                set "BUILD_CONFIG=%%A"
            ) else (
                if !PARSE_ACTION!==parse_cmake_args (
                    if not defined CMAKE_ARGS (
                        set "CMAKE_ARGS=%%A"
                    ) else (
                        set "CMAKE_ARGS=!CMAKE_ARGS! %%A"
                    )
                ) else (
                    if !PARSE_ACTION!==parse_cmake_build_args (
                        if not defined CMAKE_BUILD_ARGS (
                            set "CMAKE_BUILD_ARGS=%%A"
                        ) else (
                            set "CMAKE_BUILD_ARGS=!CMAKE_BUILD_ARGS! %%A"
                        )
                    ) else (
                        echo Error: Unknown arg %%A, try %PROGNAME% --help for usage
                        exit /b 1
                    )
                )
            )
        )
    )
                                                )
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
)
exit /b !ERRORLEVEL!

::::
:: Main entry point.
::
:: Arguments:
::  List of command-line arguments
::
:Main
:: parse command-line args + handle exit
call :ParseArgs %*
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
:: print usage if specified and exit
if !BUILD_ACTION!==print_usage (
    call :PrintUsage
    exit /b 0
)
:: set actual CMake arch + output directory
call :SetCMakeArch !BUILD_ARCH!
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
set BUILD_OUTPUT_DIR=!BUILD_OUTPUT_PREFIX!_!BUILD_ARCH!
:: otherwise, proceed with CMake call
cmake -S . -B !BUILD_OUTPUT_DIR! -A !CMAKE_BUILD_ARCH! !CMAKE_ARGS! && ^
cmake --build !BUILD_OUTPUT_DIR! --config !BUILD_CONFIG! -j !CMAKE_BUILD_ARGS!
exit /b !ERRORLEVEL!
