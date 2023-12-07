/**
 * @file cliopt/opt_message_bytes.h
 * @author Derek Huang
 * @brief C/C++ header for the cliopt bytes per message option
 * @copyright MIT License
 */

#ifndef PDNNET_CLIOPT_OPT_MESSAGE_BYTES_H_
#define PDNNET_CLIOPT_OPT_MESSAGE_BYTES_H_

// bytes requested in a read, write, send, or recv call
#if defined(PDNNET_ADD_CLIOPT_MESSAGE_BYTES)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pdnnet/cliopt/common.h"
#include "pdnnet/common.h"

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

/**
 * Parsing logic for matching and handling the bytes per message option.
 *
 * @param argc Argument count from `main`
 * @param argv Argument vector from `main`
 * @param i Index to current argument
 */
#define PDNNET_CLIOPT_MESSAGE_BYTES_PARSE_CASE(argc, argv, i) \
  PDNNET_CLIOPT_PARSE_MATCHES( \
    argv, \
    i, \
    PDNNET_CLIOPT_MESSAGE_BYTES_SHORT_OPTION, \
    PDNNET_CLIOPT_MESSAGE_BYTES_OPTION \
  ) { \
    /* not enough arguments */ \
    if (++i >= argc) { \
      fprintf( \
        stderr, \
        "Error: Missing argument for " \
        PDNNET_CLIOPT_MESSAGE_BYTES_SHORT_OPTION ", " \
        PDNNET_CLIOPT_MESSAGE_BYTES_OPTION "\n" \
      ); \
      return false; \
    } \
    /* parse message byte count */ \
    if (!pdnnet_cliopt_parse_message_bytes(argv[i])) \
      return false; \
  }
#else
#define PDNNET_CLIOPT_MESSAGE_BYTES_USAGE ""
#define PDNNET_CLIOPT_MESSAGE_BYTES_PARSE_CASE(argc, argv, i)
#endif  // !defined(PDNNET_ADD_CLIOPT_MESSAGE_BYTES)

#endif  // PDNNET_CLIOPT_OPT_MESSAGE_BYTES_H_
