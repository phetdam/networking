/**
 * @file cliopt/opt_port.h
 * @author Derek Huang
 * @brief C/C++ header for the cliopt port number option
 * @copyright MIT License
 */

#ifndef PDNNET_CLIOPT_OPT_PORT_H_
#define PDNNET_CLIOPT_OPT_PORT_H_

// port number
#if defined(PDNNET_ADD_CLIOPT_PORT)
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pdnnet/cliopt/common.h"
#include "pdnnet/common.h"

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
// typedef'd to in_port_t on *nix, USHORT on Windows
static uint16_t PDNNET_CLIOPT(port) = PDNNET_CLIOPT_PORT_DEFAULT;
#define PDNNET_CLIOPT_PORT_USAGE \
  "  " \
    PDNNET_CLIOPT_PORT_SHORT_OPTION ", " \
    PDNNET_CLIOPT_PORT_OPTION " " \
    PDNNET_CLIOPT_PORT_ARG_NAME \
    "       Port number to bind to, default " \
    PDNNET_STRINGIFY(PDNNET_CLIOPT_PORT_DEFAULT) \
    PDNNET_CLIOPT_PORT_DEFAULT_NOTE "\n"

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

/**
 * Parsing logic for matching and handling the port number option.
 *
 * @param argc Argument count from `main`
 * @param argv Argument vector from `main`
 * @param i Index to current argument
 */
#define PDNNET_CLIOPT_PORT_PARSE_CASE(argc, argv, i) \
  PDNNET_CLIOPT_PARSE_MATCHES( \
    argv, i, PDNNET_CLIOPT_PORT_SHORT_OPTION, PDNNET_CLIOPT_PORT_OPTION \
  ) { \
    /* not enough arguments */ \
    if (++i >= argc) { \
      fprintf( \
        stderr, \
        "Error: Missing argument for " PDNNET_CLIOPT_PORT_SHORT_OPTION ", " \
        PDNNET_CLIOPT_PORT_OPTION "\n" \
      ); \
      return false; \
    } \
    /* parse port value */ \
    if (!pdnnet_cliopt_parse_port(argv[i])) \
      return false; \
  }
#else
#define PDNNET_CLIOPT_PORT_USAGE ""
#define PDNNET_CLIOPT_PORT_PARSE_CASE(argc, argv, i)
#endif  // !defined(PDNNET_ADD_CLIOPT_PORT)

#endif  // PDNNET_CLIOPT_OPT_PORT_H_
