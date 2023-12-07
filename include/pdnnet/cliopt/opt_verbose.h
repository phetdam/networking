/**
 * @file cliopt/opt_verbose.h
 * @author Derek Huang
 * @brief C/C++ header for the cliopt verbose option
 * @copyright MIT License
 */

#ifndef PDNNET_CLIOPT_OPT_VERBOSE_H_
#define PDNNET_CLIOPT_OPT_VERBOSE_H_

// verbosity level
#if !defined(PDNNET_ADD_CLIOPT_VERBOSE)
#define PDNNET_CLIOPT_VERBOSE_USAGE ""
#else
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdnnet/cliopt/common.h"
#include "pdnnet/common.h"

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
#endif  // defined(PDNNET_ADD_CLIOPT_VERBOSE)

#endif  // PDNNET_CLIOPT_OPT_VERBOSE_H_
