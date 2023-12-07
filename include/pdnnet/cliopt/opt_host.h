/**
 * @file cliopt/opt_host.h
 * @author Derek Huang
 * @brief C/C++ for the cliopt host name option
 * @copyright MIT License
 */

#ifndef PDNNET_CLIOPT_OPT_HOST_H_
#define PDNNET_CLIOPT_OPT_HOST_H_

// host name
#if !defined(PDNNET_ADD_CLIOPT_HOST)
#define PDNNET_CLIOPT_HOST_USAGE ""
#else
#include <stdbool.h>

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
#endif  // defined(PDNNET_ADD_CLIOPT_HOST)

#endif  // PDNNET_CLIOPT_OPT_HOST_H_
