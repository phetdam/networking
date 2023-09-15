/**
 * @file cliopt.h
 * @author Derek Huang
 * @brief C/C++ header for command-line option parsing
 * @copyright MIT License
 *
 * This header provides a simple interface for parsing a number of command-line
 * options that are common to the networking programs in this project and only
 * needs to be included in the translation unit that defines `main`.
 *
 * Configuring the available command-line options and their possible defaults
 * is done by defining macros before including this header. For example, for an
 * option `<ARG>`, define `PDNNET_ADD_CLIOPT_<ARG>` to enable option parsing
 * and to print the option as part of the usage output. Some options have
 * defaults that can be redefined by defining `PDNNET_CLIOPT_<ARG>_DEFAULT` to
 * the appropriate value, usually an integral value or string literal.
 *
 * Below are the available command-line options.
 *  HOST
 *  PORT
 *  MESSAGE_BYTES
 *  MAX_CONNECT
 */

#ifndef PDNNET_CLIOPT_H_
#define PDNNET_CLIOPT_H_

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdnnet/common.h"
#include "pdnnet/sa.h"

#define PDNNET_CLIOPT_I(name) pdnnet_cliopt_ ## name
#define PDNNET_CLIOPT(name) PDNNET_CLIOPT_I(name)

#define PDNNET_PROGRAM_NAME pdnnet_main_program_name
#define PDNNET_PROGRAM_USAGE pdnnet_main_program_usage

#define PDNNET_PROGRAM_USAGE_DEF(text) \
  static const char *PDNNET_PROGRAM_USAGE = text;

#ifndef PDNNET_HAS_PROGRAM_USAGE
PDNNET_PROGRAM_USAGE_DEF("")
#endif  // PDNNET_HAS_PROGRAM_USAGE

static const char *PDNNET_PROGRAM_NAME = NULL;
static bool PDNNET_CLIOPT(print_usage) = false;

// macros for main() argc and argv
#define PDNNET_ARGC argc
#define PDNNET_ARGV argv

/**
 * Variables and defaults for command-line arguments are defined below.
 *
 * Control the definitions via macro definition before including the header.
 * For an argument `<ARG>`, we list the user-definable macros below.
 *
 * `PDNNET_ADD_CLIOPT_<ARG>`
 *    Indicate that the program should accept this command-line option. If
 *    specified, the option will be parsed and show up in the help output.
 * `PDNNET_CLIOPT_<ARG>_DEFAULT`
 *    Default value for the command-line option. Not all command-line options
 *    have default values, so this may not be necessary for each `<ARG>`.
 *
 * Below are additional macros that are defined when `PDNNET_CLIOPT_<ARG>` are
 * defined, but which should not be redefined by the user. Any macros not
 * documented below are to be considered internal and should not be redefined.
 *
 * `PDNNET_CLIOPT_<ARG>_SHORT_OPTION`
 *    String literal for the short option flag for `<ARG>`, e.g. `"-p"`
 * `PDNNET_CLIOPT_<ARG>_OPTION`
 *    String literal for the long option flag for `<ARG>`, e.g. `"--port"`
 * `PDNNET_CLIOPT_<ARG>_ARG_NAME`
 *    String literal for the option's argument name for `<ARG>`, e.g. `"PORT"`.
 *    Not all command-line options will take arguments so this may not be
 *    defined for each particular `<ARG>` option.
 */

// verbosity level
#if defined(PDNNET_ADD_CLIOPT_VERBOSE)
#define PDNNET_CLIOPT_VERBOSE_SHORT_OPTION "-v"
#define PDNNET_CLIOPT_VERBOSE_OPTION "--verbose"
#define PDNNET_CLIOPT_VERBOSE_ARG_NAME "VERBOSE"
static unsigned short PDNNET_CLIOPT(verbose) = 0;
#define PDNNET_CLIOPT_VERBOSE_USAGE \
  "  " \
    PDNNET_CLIOPT_VERBOSE_SHORT_OPTION ", " \
    PDNNET_CLIOPT_VERBOSE_OPTION " [" \
    PDNNET_CLIOPT_VERBOSE_ARG_NAME "]\n" \
    "                        Run verbosely, with larger values for greater\n" \
    "                        verbosity. If specified without an argument,\n" \
    "                        the verbosity level is set to 1.\n"
#else
#define PDNNET_CLIOPT_VERBOSE_USAGE ""
#endif  // !defined(PDNNET_ADD_CLIOPT_VERBOSE)
// host name
#if defined(PDNNET_ADD_CLIOPT_HOST)
#define PDNNET_CLIOPT_HOST_SHORT_OPTION "-H"
#define PDNNET_CLIOPT_HOST_OPTION "--host"
#define PDNNET_CLIOPT_HOST_ARG_NAME "HOST"
#ifndef PDNNET_CLIOPT_HOST_DEFAULT
#define PDNNET_CLIOPT_HOST_DEFAULT "localhost"
#endif  // PDNNET_CLIOPT_HOST_DEFAULT
static const char *PDNNET_CLIOPT(host) = PDNNET_CLIOPT_HOST_DEFAULT;
#define PDNNET_CLIOPT_HOST_USAGE \
  "  " \
    PDNNET_CLIOPT_HOST_SHORT_OPTION ", " \
    PDNNET_CLIOPT_HOST_OPTION " " \
    PDNNET_CLIOPT_HOST_ARG_NAME \
    "       IPv4 host name or address, default " \
    PDNNET_STRINGIFY(PDNNET_CLIOPT_HOST_DEFAULT) "\n"
#else
#define PDNNET_CLIOPT_HOST_USAGE ""
#endif  // !defined(PDNNET_ADD_CLIOPT_HOST)
// port number
#if defined(PDNNET_ADD_CLIOPT_PORT)
#define PDNNET_CLIOPT_PORT_SHORT_OPTION "-p"
#define PDNNET_CLIOPT_PORT_OPTION "--port"
#define PDNNET_CLIOPT_PORT_ARG_NAME "PORT"
// use value of zero to allow OS to select the next available port
#if !defined(PDNNET_CLIOPT_PORT_DEFAULT)
#define PDNNET_CLIOPT_PORT_DEFAULT 0
// extra clarification on the port default value
#define PDNNET_CLIOPT_PORT_DEFAULT_NOTE " (next free port)"
#else
#define PDNNET_CLIOPT_PORT_DEFAULT_NOTE
#endif  // defined(PDNNET_CLIOPT_PORT_DEFAULT)
static uint16_t PDNNET_CLIOPT(port) = PDNNET_CLIOPT_PORT_DEFAULT;
#define PDNNET_CLIOPT_PORT_USAGE \
  "  " \
    PDNNET_CLIOPT_PORT_SHORT_OPTION ", " \
    PDNNET_CLIOPT_PORT_OPTION " " \
    PDNNET_CLIOPT_PORT_ARG_NAME \
    "       Port number to bind to, default " \
    PDNNET_STRINGIFY(PDNNET_CLIOPT_PORT_DEFAULT) \
    PDNNET_CLIOPT_PORT_DEFAULT_NOTE "\n"
#else
#define PDNNET_CLIOPT_PORT_USAGE ""
#endif  // !defined(PDNNET_ADD_CLIOPT_PORT)
// bytes requested in a read, write, send, or recv call
#if defined(PDNNET_ADD_CLIOPT_MESSAGE_BYTES)
#define PDNNET_CLIOPT_MESSAGE_BYTES_SHORT_OPTION "-m"
#define PDNNET_CLIOPT_MESSAGE_BYTES_OPTION "--message-bytes"
#define PDNNET_CLIOPT_MESSAGE_BYTES_ARG_NAME "MESSAGE_BYTES"
#ifndef PDNNET_CLIOPT_MESSAGE_BYTES_DEFAULT
#define PDNNET_CLIOPT_MESSAGE_BYTES_DEFAULT 512
#endif  // PDNNET_CLIOPT_MESSAGE_BYTES_DEFAULT
#ifndef PDNNET_CLIOPT_MAX_MESSAGE_BYTES
#define PDNNET_CLIOPT_MAX_MESSAGE_BYTES BUFSIZ
#endif  // PDNNET_CLIOPT_MAX_MESSAGE_BYTES
static size_t PDNNET_CLIOPT(message_bytes) = PDNNET_CLIOPT_MESSAGE_BYTES_DEFAULT;
#define PDNNET_CLIOPT_MESSAGE_BYTES_USAGE \
  "  " \
    PDNNET_CLIOPT_MESSAGE_BYTES_SHORT_OPTION ", " \
    PDNNET_CLIOPT_MESSAGE_BYTES_OPTION " " \
    PDNNET_CLIOPT_MESSAGE_BYTES_ARG_NAME \
    "\n" \
  "                        Number of bytes requested per read/write to/from a\n" \
  "                        client, default " \
    PDNNET_STRINGIFY(PDNNET_CLIOPT_MESSAGE_BYTES_DEFAULT) " bytes, max " \
    PDNNET_STRINGIFY(PDNNET_CLIOPT_MAX_MESSAGE_BYTES) " bytes\n"
#else
#define PDNNET_CLIOPT_MESSAGE_BYTES_USAGE ""
#endif  // !defined(PDNNET_ADD_CLIOPT_MESSAGE_BYTES)
// maximum number of accepted connections
#if defined(PDNNET_ADD_CLIOPT_MAX_CONNECT)
#define PDNNET_ADD_CLIOPT_MAX_CONNECT_SHORT_OPTION "-M"
#define PDNNET_ADD_CLIOPT_MAX_CONNECT_OPTION "--max-connect"
#define PDNNET_ADD_CLIOPT_MAX_CONNECT_ARG_NAME "MAX_CONNECT"
#ifndef PDNNET_ADD_CLIOPT_MAX_CONNECT_DEFAULT
#define PDNNET_ADD_CLIOPT_MAX_CONNECT_DEFAULT 10
#endif  // PDNNET_ADD_CLIOPT_MAX_CONNECT_DEFAULT
static unsigned int PDNNET_CLIOPT(max_connect) = PDNNET_ADD_CLIOPT_MAX_CONNECT_DEFAULT;
#define PDNNET_ADD_CLIOPT_MAX_CONNECT_USAGE \
  "  " \
    PDNNET_ADD_CLIOPT_MAX_CONNECT_SHORT_OPTION ", " \
    PDNNET_ADD_CLIOPT_MAX_CONNECT_OPTION " " \
    PDNNET_ADD_CLIOPT_MAX_CONNECT_ARG_NAME \
    "\n" \
  "                        Max number of connections to accept, default " \
    PDNNET_STRINGIFY(PDNNET_ADD_CLIOPT_MAX_CONNECT_DEFAULT) "\n"
#else
#define PDNNET_ADD_CLIOPT_MAX_CONNECT_USAGE ""
#endif  // !defined(PDNNET_ADD_CLIOPT_MAX_CONNECT)

/**
 * Internal function to set `PDNNET_PROGRAM_NAME` from `PDNNET_ARGV`.
 *
 * The body is not run again if `PDNNET_PROGRAM_NAME` has already been set.
 */
static void
pdnnet_internal_set_program_name(PDNNET_SA(In) char **argv)
{
  if (PDNNET_PROGRAM_NAME)
    return;
  PDNNET_PROGRAM_NAME = strrchr(argv[0], PDNNET_PATH_SEP_CHAR);
  if (!PDNNET_PROGRAM_NAME)
    PDNNET_PROGRAM_NAME = argv[0];
  else
    PDNNET_PROGRAM_NAME++;
}

#ifdef PDNNET_ADD_CLIOPT_VERBOSE
/**
 * Parse verbosity level.
 *
 * Zero verbosity level is the same as not specifying the verbosity level.
 *
 * @param arg String verbosity level
 * @returns `true` on successful parse, `false` otherwise
 */
static bool
pdnnet_cliopt_parse_verbose(const char *arg)
{
  // zero is allowed. reassign to override previous values (if any)
  if (!strcmp(arg, "0")) {
    PDNNET_CLIOPT(verbose) = 0;
    return true;
  }
  // get value + handle error
  int value = atoi(arg);
  if (!value) {
    fprintf(stderr, "Error: Unable to convert %s to a verbosity level\n", arg);
    return false;
  }
  // USHORT_MAX is max verbosity level
  if (value > USHRT_MAX) {
    fprintf(
      stderr, "Error: Verbosity level %s exceeds maximum %u\n", arg, USHRT_MAX
    );
    return false;
  }
  // otherwise, assign verbosity level + return
  PDNNET_CLIOPT(verbose) = (unsigned short) value;
  return true;
}
#endif  // PDNNET_ADD_CLIOPT_VERBOSE

#ifdef PDNNET_ADD_CLIOPT_HOST
/**
 * Parse host name.
 *
 * Does not actually check whether or not the arg is an actual IPv4 host name.
 * This would be done by the relevant `gethostbyname` or `getaddrinfo` call.
 *
 * @param arg String host name
 * @returns `true` on successful parse, `false` otherwise
 */
static bool
pdnnet_cliopt_parse_host(const char *arg)
{
  // no checking for now
  PDNNET_CLIOPT(host) = arg;
  return true;
}
#endif  // PDNNET_ADD_CLIOPT_HOST

#ifdef PDNNET_ADD_CLIOPT_PORT
/**
 * Parse port value.
 *
 * @param arg String port
 * @returns `true` on successful parse, `false` otherwise
 */
static bool
pdnnet_cliopt_parse_port(const char *arg)
{
  // don't allow zero port value
  if (!strcmp(arg, "0")) {
    fprintf(stderr, "Error: Cannot specify 0 as a port value\n");
    return false;
  }
  // get value + handle error
  int value = atoi(arg);
  if (!value) {
    fprintf(stderr, "Error: Unable to convert %s to a port number\n", arg);
    return false;
  }
  // can't be greater than UINT16_MAX
  if (value > UINT16_MAX) {
    fprintf(
      stderr,
      "Error: Port number %d exceeds max port number %d\n",
      value,
      UINT16_MAX
    );
    return false;
  }
  // update port value + return
  PDNNET_CLIOPT(port) = (uint16_t) value;
  return true;
}
#endif  // PDNNET_ADD_CLIOPT_PORT

#ifdef PDNNET_ADD_CLIOPT_MESSAGE_BYTES
/**
 * Parse number of bytes to request for each socket read/write call.
 *
 * @param arg String request size in bytes
 * @returns `true` on successful parse, `false` otherwise
 */
static bool
pdnnet_cliopt_parse_message_bytes(const char *arg)
{
  // don't allow zero message size
  if (!strcmp(arg, "0")) {
    fprintf(stderr, "Error: Cannot specify 0 as message size\n");
    return false;
  }
  // get value + handle error
  long long value = atoll(arg);
  if (!value) {
    fprintf(stderr, "Error: Unable to convert %s to message size\n", arg);
    return false;
  }
  // must be positive
  if (value < 1) {
    fprintf(stderr, "Error: Message size value must be positive\n");
    return false;
  }
  // must not exceed MAX_READ_SIZE
  if (value > PDNNET_CLIOPT_MAX_MESSAGE_BYTES) {
    fprintf(
      stderr,
      "Error: Message size value %lld exceeds allowed max %zu\n",
      value,
      (size_t) PDNNET_CLIOPT_MAX_MESSAGE_BYTES
    );
    return false;
  }
  // update message size value + return
  PDNNET_CLIOPT(message_bytes) = (size_t) value;
  return true;
}
#endif  // PDNNET_ADD_CLIOPT_MESSAGE_BYTES

#ifdef PDNNET_ADD_CLIOPT_MAX_CONNECT
/**
 * Parse max accepted connections value.
 *
 * @param arg String max accepted connections
 * @returns `true` on successful parse, `false` otherwise
 */
static bool
pdnnet_cliopt_parse_max_connect(const char *arg)
{
  // don't allow zero max connections
  if (!strcmp(arg, "0")) {
    fprintf(stderr, "Error: Cannot specify 0 as number of max connects\n");
    return false;
  }
  // get value + handle error
  int value = atoi(arg);
  if (!value) {
    fprintf(stderr, "Error: Can't convert %s to number of max connects\n", arg);
    return false;
  }
  // must be positive
  if (value < 1) {
    fprintf(stderr, "Error: Max connection value must be positive\n");
    return false;
  }
  // must be within integer range
  if (value > INT_MAX) {
    fprintf(
      stderr,
      "Error: Max connect value %u exceeds allowed max %d\n",
      value,
      INT_MAX
    );
    return false;
  }
  // update max connections value + return
  PDNNET_CLIOPT(max_connect) = (unsigned int) value;
  return true;
}
#endif  // PDNNET_ADD_CLIOPT_MAX_CONNECT

/**
 * Parse incoming command-line arguments.
 *
 * Conditional compilation is used to control which options are understood.
 *
 * @param argc Argument count from `main`
 * @param argv Argument vector from `main`
 * @return `true` on success, `false` otherwise
 */
static bool
pdnnet_cliopt_parse_args(int argc, PDNNET_SA(In) char **argv)
{
  // set program name if necessary
  pdnnet_internal_set_program_name(argv);
  // loop through args
  for (int i = 1; i < argc; i++) {
    // help flag
    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      PDNNET_CLIOPT(print_usage) = true;
      return true;
    }
    // verbosity level
#ifdef PDNNET_ADD_CLIOPT_VERBOSE
    else if (
      !strcmp(argv[i], PDNNET_CLIOPT_VERBOSE_SHORT_OPTION) ||
      !strcmp(argv[i], PDNNET_CLIOPT_VERBOSE_OPTION)
    ) {
      // not enough arguments, so treat as 1
      if (++i >= argc)
        PDNNET_CLIOPT(verbose) = (unsigned short) 1;
      // if argument starts with a dash, assume it is an option
      else if (argv[i][0] == '-')
        ;
      // otherwise, try to parse the value
      else if (!pdnnet_cliopt_parse_verbose(argv[i]))
        return false;
    }
#endif  // PDNNET_ADD_CLIOPT_VERBOSE
    // host
#ifdef PDNNET_ADD_CLIOPT_HOST
    else if (
      !strcmp(argv[i], PDNNET_CLIOPT_HOST_SHORT_OPTION) ||
      !strcmp(argv[i], PDNNET_CLIOPT_HOST_OPTION)
    ) {
      // not enough arguments
      if (++i >= argc) {
        fprintf(
          stderr,
          "Error: Missing argument for " PDNNET_CLIOPT_HOST_SHORT_OPTION ", "
            PDNNET_CLIOPT_HOST_OPTION "\n"
        );
        return false;
      }
      // parse port value
      if (!pdnnet_cliopt_parse_host(argv[i]))
        return false;
    }
#endif  // PDNNET_ADD_CLIOPT_HOST
    // port
#ifdef PDNNET_ADD_CLIOPT_PORT
    else if (
      !strcmp(argv[i], PDNNET_CLIOPT_PORT_SHORT_OPTION) ||
      !strcmp(argv[i], PDNNET_CLIOPT_PORT_OPTION)
    ) {
      // not enough arguments
      if (++i >= argc) {
        fprintf(
          stderr,
          "Error: Missing argument for " PDNNET_CLIOPT_PORT_SHORT_OPTION ", "
            PDNNET_CLIOPT_PORT_OPTION "\n"
        );
        return false;
      }
      // parse port value
      if (!pdnnet_cliopt_parse_port(argv[i]))
        return false;
    }
#endif  // PDNNET_ADD_CLIOPT_PORT
    // read/write or recv/send message bytes
#ifdef PDNNET_ADD_CLIOPT_MESSAGE_BYTES
    else if (
      !strcmp(argv[i], PDNNET_CLIOPT_MESSAGE_BYTES_SHORT_OPTION) ||
      !strcmp(argv[i], PDNNET_CLIOPT_MESSAGE_BYTES_OPTION)
    ) {
      // not enough arguments
      if (++i >= argc) {
        fprintf(
          stderr,
          "Error: Missing argument for "
            PDNNET_CLIOPT_MESSAGE_BYTES_SHORT_OPTION ", "
            PDNNET_CLIOPT_MESSAGE_BYTES_OPTION "\n"
        );
        return false;
      }
      // parse message byte count
      if (!pdnnet_cliopt_parse_message_bytes(argv[i]))
        return false;
    }
#endif  // PDNNET_ADD_CLIOPT_MESSAGE_BYTES
    // max number of accepted connections
#ifdef PDNNET_ADD_CLIOPT_MAX_CONNECT
    else if (
      !strcmp(argv[i], PDNNET_ADD_CLIOPT_MAX_CONNECT_SHORT_OPTION) ||
      !strcmp(argv[i], PDNNET_ADD_CLIOPT_MAX_CONNECT_OPTION)
    ) {
      // not enough arguments
      if (++i >= argc) {
        fprintf(
          stderr,
          "Error: Missing argument for "
            PDNNET_ADD_CLIOPT_MAX_CONNECT_SHORT_OPTION ", "
            PDNNET_ADD_CLIOPT_MAX_CONNECT_OPTION "\n"
        );
        return false;
      }
      // parse max accepted connections
      if (!pdnnet_cliopt_parse_max_connect(argv[i]))
        return false;
    }
#endif  // PDNNET_ADD_CLIOPT_MAX_CONNECT
    else {
      fprintf(stderr, "Error: Unknown option %s\n", argv[i]);
      return false;
    }
  }
  return true;
}

/**
 * Internal function for printing program usage.
 *
 * Prints a user-specified description before printing help for each option.
 *
 * @param argv `main()` argument vector
 * @param desc Description to print before option help text
 */
static void
pdnnet_cliopt_internal_print_usage(PDNNET_SA(In) char **argv, const char *desc)
{
  pdnnet_internal_set_program_name(argv);
  // description padding. if no description, then this can be empty string
  const char *desc_pad = (strlen(desc)) ? "\n\n" : "";
  printf(
    "Usage: %s [OPTIONS...]\n"
    "\n"
    "%s%s"
    "Options:\n"
    "\n"
    "  -h, --help            Print this usage\n"
      PDNNET_CLIOPT_VERBOSE_USAGE
      PDNNET_CLIOPT_HOST_USAGE
      PDNNET_CLIOPT_PORT_USAGE
      PDNNET_CLIOPT_MESSAGE_BYTES_USAGE
      PDNNET_ADD_CLIOPT_MAX_CONNECT_USAGE,
    PDNNET_PROGRAM_NAME,
    desc,
    desc_pad
  );
}

// don't pollute namespace with help text macros. these are usually not helpful
// for the user anyways, as they are intended to be printed with -h, --help
#undef PDNNET_CLIOPT_VERBOSE_USAGE
#undef PDNNET_CLIOPT_HOST_USAGE
#undef PDNNET_CLIOPT_PORT_USAGE
#undef PDNNET_CLIOPT_MESSAGE_BYTES_USAGE
#undef PDNNET_ADD_CLIOPT_MAX_CONNECT_USAGE

/**
 * Print program usage.
 *
 * Intended to be called from `main` and masks the internal usage print call.
 * Usually there is no need to call this directly.
 */
#define PDNNET_CLIOPT_PRINT_USAGE() \
  pdnnet_cliopt_internal_print_usage(PDNNET_ARGV, PDNNET_PROGRAM_USAGE)

/**
 * Macro for `main` that takes argument count and argument vector.
 *
 * Although seemingly redundant, the use of the `PDNNET_ARGC` and `PDNNET_ARGV`
 * macros means that in the future, if the macros' definitions change, the rest
 * of the corresponding `cliopt.h` API need not be refactored.
 */
#define PDNNET_ARG_MAIN \
  int \
  main(int PDNNET_ARGC, char **PDNNET_ARGV)

/**
 * Parse command-line options and update the values of the target variables.
 *
 * Intended to be the first statement in `main` that is evaluated.
 */
#define PDNNET_CLIOPT_PARSE_OPTIONS() \
  do { \
    if (!pdnnet_cliopt_parse_args(PDNNET_ARGC, PDNNET_ARGV)) \
      return EXIT_FAILURE; \
    if (PDNNET_CLIOPT(print_usage)) { \
      PDNNET_CLIOPT_PRINT_USAGE(); \
      return EXIT_SUCCESS; \
    } \
  } \
  while (0)

#endif  // PDNNET_CLIOPT_H_
