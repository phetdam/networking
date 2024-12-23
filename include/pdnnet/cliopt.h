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
 * and to print the option as part of the usage output.
 *
 * Some options have defaults that can be redefined by defining the
 * `PDNNET_CLIOPT_<ARG>_DEFAULT` macro to the appropriate value, usually an
 * integral value or string literal, but also another user-defined C type.
 *
 * Below are the available command-line options.
 * - `VERBOSE`
 * - `HOST`
 * - `PORT`
 * - `PATH`
 * - `MESSAGE_BYTES`
 * - `MAX_CONNECT`
 * - `TIMEOUT`
 */

#ifndef PDNNET_CLIOPT_H_
#define PDNNET_CLIOPT_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdnnet/cliopt/common.h"
#include "pdnnet/cliopt/opt_host.h"
#include "pdnnet/cliopt/opt_max_connect.h"
#include "pdnnet/cliopt/opt_message_bytes.h"
#include "pdnnet/cliopt/opt_path.h"
#include "pdnnet/cliopt/opt_port.h"
#include "pdnnet/cliopt/opt_verbose.h"
#include "pdnnet/cliopt/opt_timeout.h"
#include "pdnnet/common.h"
#include "pdnnet/sa.h"

/**
 * Name of the string variable holding the main program name.
 */
#define PDNNET_PROGRAM_NAME pdnnet_main_program_name

/**
 * Name of the string variable holding the main program usage.
 */
#define PDNNET_PROGRAM_USAGE pdnnet_main_program_usage

/**
 * Macro defining the variable holding the program usage text.
 *
 * @param text Program usage text
 */
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
 * @file cliopt.h
 *
 * Variables and defaults for command-line arguments should be done as follows.
 *
 * Control the definitions via macro definition before including the header.
 * For an argument `<ARG>`, we list the user-definable macros below.
 *
 * - `PDNNET_ADD_CLIOPT_<ARG>`:
 *    Indicate that the program should accept this command-line option. If
 *    specified, the option will be parsed and show up in the help output.
 * - `PDNNET_CLIOPT_<ARG>_DEFAULT`:
 *    Default value for the command-line option. Not all command-line options
 *    have default values, so this may not be necessary for each `<ARG>`.
 * - `PDNNET_CLIOPT_<ARG>_MAX`:
 *    Maximum value for an arithmetic-valued command-line option. Not all
 *    command-line options have maximum values and should be defined as needed.
 *
 * Below are additional macros that are defined when `PDNNET_CLIOPT_<ARG>` are
 * defined, but which should not be redefined by the user. Any macros not
 * documented below are to be considered internal and should not be redefined.
 *
 * These should all be defined in a `pdnnet/cliopt/opt_<arg>.h` header, where
 * `<arg>` is just the lowercase form of `<ARG>`.
 *
 * - `PDNNET_CLIOPT_<ARG>_SHORT_OPTION`:
 *    String literal for the short option flag for `<ARG>`, e.g. `"-p"`
 * - `PDNNET_CLIOPT_<ARG>_OPTION`:
 *    String literal for the long option flag for `<ARG>`, e.g. `"--port"`
 * - `PDNNET_CLIOPT_<ARG>_ARG_NAME`:
 *    String literal for the option's argument name for `<ARG>`, e.g. `"PORT"`.
 *    Not all command-line options will take arguments so this may not be
 *    defined for each particular `<ARG>` option.
 * - `PDNNET_CLIOPT_<ARG>_USAGE`:
 *    User-defined help text for the specified command-line option.
 *    - Must start with two spaces, the short and long options separated by
 *      ", ", another space, the respective `PDNNET_CLIOPT_<ARG>_ARG_NAME` if
 *      any, possibly encased in square brackets if optional.
 *    - If there is not enough room to put the usage on the same length
 *      starting from the 24th column, a newline should be added and then all
 *      usage justified from the 24th column. When `PDNNET_ADD_CLIOPT_<ARG>` is
 *      not defined, this should simply be defined as "".
 *
 * To parse the value (if any) from a command-line argument, one may also need
 * to define a static function with the following signature:
 *
 * @code{.cc}
 * static bool
 * pdnnet_cliopt_parse_<arg>(const char *arg);
 * @endcode
 *
 * `true` should be returned on success, `false` on failure, with the
 * respective `PDNNET_CLIOPT(<arg>)` top-level static global correctly updated
 * as necessary. The following macro should also be defined to handle the
 * short/long option and start parsing:
 *
 * `PDNNET_CLIOPT_<ARG>_PARSE_CASE(argc, argv, i)`
 *
 * This should start with a `PDNNET_CLIOPT_PARSE_MATCHES` invocation that opens
 * into a braced context. When `PDNNET_ADD_CLIOPT_<ARG>` is not defined, this
 * should simply be defined as an empty macro.
 */

/**
 * Internal function to set `PDNNET_PROGRAM_NAME` from `PDNNET_ARGV`.
 *
 * The body is not run again if `PDNNET_PROGRAM_NAME` has already been set.
 */
static void
pdnnet_internal_set_program_name(PDNNET_SA(In) char **argv) PDNNET_NOEXCEPT
{
  if (PDNNET_PROGRAM_NAME)
    return;
  PDNNET_PROGRAM_NAME = strrchr(argv[0], PDNNET_PATH_SEP_CHAR);
  if (!PDNNET_PROGRAM_NAME)
    PDNNET_PROGRAM_NAME = argv[0];
  else
    PDNNET_PROGRAM_NAME++;
}

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
pdnnet_cliopt_parse_args(int argc, PDNNET_SA(In) char **argv) PDNNET_NOEXCEPT
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
    PDNNET_CLIOPT_VERBOSE_PARSE_CASE(argc, argv, i)
    // host
    PDNNET_CLIOPT_HOST_PARSE_CASE(argc, argv, i)
    // port
    PDNNET_CLIOPT_PORT_PARSE_CASE(argc, argv, i)
    // path to host resource
    PDNNET_CLIOPT_PATH_PARSE_CASE(argc, argv, i)
    // read/write or recv/send message bytes
    PDNNET_CLIOPT_MESSAGE_BYTES_PARSE_CASE(argc, argv, i)
    // max number of accepted connections
    PDNNET_CLIOPT_MAX_CONNECT_PARSE_CASE(argc, argv, i)
    // operation timeout
    PDNNET_CLIOPT_TIMEOUT_PARSE_CASE(argc, argv, i)
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
pdnnet_cliopt_internal_print_usage(
  PDNNET_SA(In) char **argv, const char *desc) PDNNET_NOEXCEPT
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
      PDNNET_CLIOPT_PATH_USAGE
      PDNNET_CLIOPT_MESSAGE_BYTES_USAGE
      PDNNET_CLIOPT_MAX_CONNECT_USAGE
      PDNNET_CLIOPT_TIMEOUT_USAGE,
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
#undef PDNNET_CLOPT_PATH_USAGE
#undef PDNNET_CLIOPT_MESSAGE_BYTES_USAGE
#undef PDNNET_CLIOPT_MAX_CONNECT_USAGE
#undef PDNNET_CLIOPT_TIMEOUT_USAGE

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
