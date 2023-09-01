#!/usr/bin/bash
#
# Build script for networking.
#
# Originally copied from the daily-coding-problem build.sh build script.
#
# Author: Derek Huang
# Copyright: MIT License
#

# program name ($0 refers to current function)
PROGNAME=$0
# current action to take, argument parsing mode
BUILD_ACTION=
PARSE_ACTION=
# CMake configure and build arguments
CMAKE_ARGS=
CMAKE_BUILD_ARGS=
# default build output directory and build configuration
BUILD_OUTPUT_DIR=build
BUILD_CONFIG=Debug

##
# Print build script usage.
#
print_usage() {
    echo "Usage: $PROGNAME [-h] [-o OUTPUT_DIR] [-c CONFIG] [-Ca CMAKE_ARGS]" \
        "[-Cb CMAKE_BUILD_ARGS]"
    echo
    echo "Build driver script for networking *nix builds."
    echo
    echo "Only supports single-configuration CMake generators, e.g. Makefile"
    echo "generators or Ninja, with \"Unix Makefiles\" as the default."
    echo
    echo "Options:"
    echo "  -h,  --help                     Print this usage"
    echo "  -o,  --output-dir OUTPUT_DIR    Build output directory, default" \
        "$BUILD_OUTPUT_DIR"
    echo "  -c,  --config CONFIG            Build configuration, default" \
        "$BUILD_CONFIG"
    echo "  -Ca, --cmake-args CMAKE_ARGS    Args to pass to cmake config command"
    echo
    echo "  -Cb, --cmake-build-args CMAKE_BUILD_ARGS"
    echo "                                  Args to pass to cmake build command"
}

##
# Parse incoming arguments and populate CMake config and build args.
#
# Arguments:
#   List of command-line arguments
#
parse_args() {
    for ARG in $@
    do
        case $ARG in
            # break early to print usage
            -h | --help)
                BUILD_ACTION=print_usage
                return 0
                ;;
            # set build output directory
            -o | --output-dir)
                PARSE_ACTION=output_dir
                ;;
            # set build configuration
            -c | --config)
                PARSE_ACTION=build_config
                ;;
            # collect CMake configure args
            -Ca | --cmake-args)
                PARSE_ACTION=cmake_args
                ;;
            # collect CMake build args
            -Cb | --cmake-build-args)
                PARSE_ACTION=cmake_build_args
                ;;
            # operate according to PARSE_ACTION
            *)
                # set build output dir
                if [ "$PARSE_ACTION" = output_dir ]
                then
                    BUILD_OUTPUT_DIR=$ARG
                # set build configuration
                elif [ "$PARSE_ACTION" = build_config ]
                then
                    BUILD_CONFIG=$ARG
                # update CMake configure args
                elif [ "$PARSE_ACTION" = cmake_args ]
                then
                    # assign directly if empty to prevent adding extra space
                    if [ -z "$CMAKE_ARGS" ]
                    then
                        CMAKE_ARGS=$ARG
                    else
                        CMAKE_ARGS="$CMAKE_ARGS $ARG"
                    fi
                # update CMake build args
                elif [ "$PARSE_ACTION" = cmake_build_args ]
                then
                    # assign directly if empty to prevent adding extra space
                    if [ -z "$CMAKE_BUILD_ARGS" ]
                    then
                        CMAKE_BUILD_ARGS=$ARG
                    else
                        CMAKE_BUILD_ARGS="$CMAKE_BUILD_ARGS $ARG"
                    fi
                # error otherwise
                else
                    echo "Error: Unknown option '$ARG'." \
                        "Try $PROGNAME --help for usage."
                    return 1
                fi
                ;;
        esac
    done
    return 0
}

##
# Main entry point.
#
# Arguments:
#   List of command-line arguments
#
main () {
    # parse args and exit if error
    parse_args "$@"
    if [ $? -ne 0 ]; then return $?; fi
    # handle actions. either print usage or build
    if [ "$BUILD_ACTION" = "print_usage" ]
    then
        print_usage
    else
        cmake -S . -B $BUILD_OUTPUT_DIR -DCMAKE_BUILD_TYPE=$BUILD_CONFIG \
            $CMAKE_ARGS && cmake --build $BUILD_OUTPUT_DIR -j $CMAKE_BUILD_ARGS
    fi
    return 0
}

main "$@"
