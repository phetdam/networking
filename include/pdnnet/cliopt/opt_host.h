/**
 * @file cliopt/opt_host.h
 * @author Derek Huang
 * @brief C/C++ header for the cliopt host name option
 * @copyright MIT License
 */

#ifndef PDNNET_CLIOPT_OPT_HOST_H_
#define PDNNET_CLIOPT_OPT_HOST_H_

// host name
#if defined(PDNNET_ADD_CLIOPT_HOST)
#include <stdbool.h>
#include <stdio.h>

#include "pdnnet/cliopt/common.h"
#include "pdnnet/common.h"

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
    "       Host name, default " \
    PDNNET_STRINGIFY(PDNNET_CLIOPT_HOST_DEFAULT) "\n"

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

/**
 * Parsing logic for matching and handling the host name option.
 *
 * @param argc Argument count from `main`
 * @param argv Argument vector from `main`
 * @param i Index to current argument
 */
#define PDNNET_CLIOPT_HOST_PARSE_CASE(argc, argv, i) \
  PDNNET_CLIOPT_PARSE_MATCHES( \
    argv, i, PDNNET_CLIOPT_HOST_SHORT_OPTION, PDNNET_CLIOPT_HOST_OPTION \
  ) { \
    /* not enough arguments */ \
    if (++i >= argc) { \
      fprintf( \
        stderr, \
        "Error: Missing argument for " PDNNET_CLIOPT_HOST_SHORT_OPTION ", " \
          PDNNET_CLIOPT_HOST_OPTION "\n" \
      ); \
      return false; \
    } \
    /* parse port value */ \
    if (!pdnnet_cliopt_parse_host(argv[i])) \
      return false; \
  }
#else
#define PDNNET_CLIOPT_HOST_USAGE ""
#define PDNNET_CLIOPT_HOST_PARSE_CASE(argc, argv, i)
#endif  // !defined(PDNNET_ADD_CLIOPT_HOST)

#endif  // PDNNET_CLIOPT_OPT_HOST_H_
