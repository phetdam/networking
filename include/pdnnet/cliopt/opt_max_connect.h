/**
 * @file cliopt/opt_max_connect.h
 * @author Derek Huang
 * @brief C/C++ header for the cliopt max accepted connections option
 * @copyright MIT License
 */

#ifndef PDNNET_CLIOPT_OPT_MAX_CONNECT_H_
#define PDNNET_CLIOPT_OPT_MAX_CONNECT_H_

// maximum number of accepted connections
#if defined(PDNNET_ADD_CLIOPT_MAX_CONNECT)
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pdnnet/cliopt/common.h"
#include "pdnnet/common.h"

#define PDNNET_CLIOPT_MAX_CONNECT_SHORT_OPTION "-M"
#define PDNNET_CLIOPT_MAX_CONNECT_OPTION "--max-connect"
#define PDNNET_CLIOPT_MAX_CONNECT_ARG_NAME "MAX_CONNECT"
#ifndef PDNNET_CLIOPT_MAX_CONNECT_DEFAULT
#define PDNNET_CLIOPT_MAX_CONNECT_DEFAULT 10
#endif  // PDNNET_CLIOPT_MAX_CONNECT_DEFAULT
static unsigned int PDNNET_CLIOPT(max_connect) = PDNNET_CLIOPT_MAX_CONNECT_DEFAULT;
#define PDNNET_CLIOPT_MAX_CONNECT_USAGE \
  "  " \
    PDNNET_CLIOPT_MAX_CONNECT_SHORT_OPTION ", " \
    PDNNET_CLIOPT_MAX_CONNECT_OPTION " " \
    PDNNET_CLIOPT_MAX_CONNECT_ARG_NAME \
    "\n" \
  "                        Max number of connections to accept, default " \
    PDNNET_STRINGIFY(PDNNET_CLIOPT_MAX_CONNECT_DEFAULT) "\n"

/**
 * Parse max accepted connections value.
 *
 * @param arg String max accepted connections
 * @returns `true` on successful parse, `false` otherwise
 */
static bool
pdnnet_cliopt_parse_max_connect(const char *arg) PDNNET_NOEXCEPT
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

/**
 * Parsing logic for matching and handling the max accepted connections option.
 *
 * @param argc Argument count from `main`
 * @param argv Argument vector from `main`
 * @param i Index to current argument
 */
#define PDNNET_CLIOPT_MAX_CONNECT_PARSE_CASE(argc, argv, i) \
  PDNNET_CLIOPT_PARSE_MATCHES( \
    argv, \
    i, \
    PDNNET_CLIOPT_MAX_CONNECT_SHORT_OPTION, \
    PDNNET_CLIOPT_MAX_CONNECT_OPTION \
  ) { \
    /* not enough arguments */ \
    if (++i >= argc) { \
      fprintf( \
        stderr, \
        "Error: Missing argument for " \
        PDNNET_CLIOPT_MAX_CONNECT_SHORT_OPTION ", " \
        PDNNET_CLIOPT_MAX_CONNECT_OPTION "\n" \
      ); \
      return false; \
    } \
    /* parse max accepted connections */ \
    if (!pdnnet_cliopt_parse_max_connect(argv[i])) \
      return false; \
  }
#else
#define PDNNET_CLIOPT_MAX_CONNECT_USAGE ""
#define PDNNET_CLIOPT_MAX_CONNECT_PARSE_CASE(argc, argv, i)
#endif  // !defined(PDNNET_ADD_CLIOPT_MAX_CONNECT)

#endif  // PDNNET_CLIOPT_OPT_MAX_CONNECT_H_
