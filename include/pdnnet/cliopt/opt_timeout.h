/**
 * @file cliopt/opt_timeout.h
 * @author Derek Huang
 * @brief C/C++ header for the cliopt timeout option
 * @copyright MIT License
 */

#ifndef PDNNET_CLIOPT_OPT_TIMEOUT_H_
#define PDNNET_CLIOPT_OPT_TIMEOUT_H_

// operation timeout
#if defined(PDNNET_ADD_CLIOPT_TIMEOUT)
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdnnet/cliopt/common.h"
#include "pdnnet/common.h"

#define PDNNET_CLIOPT_TIMEOUT_SHORT_OPTION "-t"
#define PDNNET_CLIOPT_TIMEOUT_OPTION "--timeout"
#define PDNNET_CLIOPT_TIMEOUT_ARG_NAME "TIMEOUT"
#ifndef PDNNET_CLIOPT_TIMEOUT_DEFAULT
#define PDNNET_CLIOPT_TIMEOUT_DEFAULT 1
#endif  // PDNNET_CLIOPT_TIMEOUT_DEFAULT
static unsigned int PDNNET_CLIOPT(timeout) = PDNNET_CLIOPT_TIMEOUT_DEFAULT;
#define PDNNET_CLIOPT_TIMEOUT_USAGE \
  "  " \
    PDNNET_CLIOPT_TIMEOUT_SHORT_OPTION ", " \
    PDNNET_CLIOPT_TIMEOUT_OPTION " " \
    PDNNET_CLIOPT_TIMEOUT_ARG_NAME \
    "\n" \
  "                        Operation timeout in ms, default " \
  PDNNET_STRINGIFY(PDNNET_CLIOPT_TIMEOUT_DEFAULT) "\n"

/**
 * Parse operation timeout.
 *
 * @todo Consider allowing user-specified suffixes that are understood and
 *  parseable into milliseconds. This is convenient, e.g. specifying `1s`
 *  instead of `1000`, or `1m` instead of `60000`.
 *
 * @param arg String timeout duration
 * @returns `true` on successful parse, `false` otherwise
 */
static bool
pdnnet_cliopt_parse_timeout(const char *arg) PDNNET_NOEXCEPT
{
  // don't allow zero timeout
  if (!strcmp(arg, "0")) {
    fprintf(stderr, "Error: Cannot specify a timeout of 0\n");
    return false;
  }
  // get value + handle error
  long value = atol(arg);
  if (!value) {
    fprintf(stderr, "Error: Unable to convert %s to a timeout value\n", arg);
    return false;
  }
  // must be positive
  if (value < 1) {
    fprintf(stderr, "Error: Timeout value must be positive\n");
    return false;
  }
  // cannot exceed UINT_MAX
  if (value > UINT_MAX) {
    fprintf(
      stderr,
      "Error: Timeout value %ld exceeds allowed maximum %u\n",
      value,
      UINT_MAX
    );
    return false;
  }
  // update timeout value + return
  PDNNET_CLIOPT(timeout) = (unsigned int) value;
  return true;
}

/**
 * Parsing logic for matching and handling the operation timeout option.
 *
 * @param argc Argument count from `main`
 * @param argv Argument vector from `main`
 * @param i Index to current argument
 */
#define PDNNET_CLIOPT_TIMEOUT_PARSE_CASE(argc, argv, i) \
  PDNNET_CLIOPT_PARSE_MATCHES( \
    argv, \
    i, \
    PDNNET_CLIOPT_TIMEOUT_SHORT_OPTION, \
    PDNNET_CLIOPT_TIMEOUT_OPTION \
  ) { \
    /* not enough arguments */ \
    if (++i >= argc) { \
      fprintf( \
        stderr, \
        "Error: Missing argument for " \
        PDNNET_CLIOPT_TIMEOUT_SHORT_OPTION ", " \
        PDNNET_CLIOPT_TIMEOUT_OPTION "\n" \
      ); \
      return false; \
    } \
    /* parse timeout value */ \
    if (!pdnnet_cliopt_parse_timeout(argv[i])) \
      return false; \
  }
#else
#define PDNNET_CLIOPT_TIMEOUT_USAGE ""
#define PDNNET_CLIOPT_TIMEOUT_PARSE_CASE(argc, argv, i)
#endif  // !defined(PDNNET_ADD_CLIOPT_TIMEOUT)

#endif  // PDNNET_CLIOPT_OPT_TIMEOUT_H_
