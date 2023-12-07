/**
 * @file cliopt/opt_path.h
 * @author Derek Huang
 * @brief C/C++ header for the cliopt host resource path option
 * @copyright MIT License
 */

#ifndef PDNNET_CLIOPT_OPT_PATH_H_
#define PDNNET_CLIOPT_OPT_PATH_H_

// path to host resource
#if !defined(PDNNET_ADD_CLIOPT_PATH)
#define PDNNET_CLIOPT_PATH_USAGE ""
#else
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pdnnet/cliopt/common.h"
#include "pdnnet/common.h"

#define PDNNET_CLIOPT_PATH_SHORT_OPTION "-P"
#define PDNNET_CLIOPT_PATH_OPTION "--path"
#define PDNNET_CLIOPT_PATH_ARG_NAME "PATH"
// default resource path on host is just /, the root
#ifndef PDNNET_CLIOPT_PATH_DEFAULT
#define PDNNET_CLIOPT_PATH_DEFAULT "/"
#endif  // PDNNET_CLIOPT_PATH_DEFAULT
static const char *PDNNET_CLIOPT(path) = PDNNET_CLIOPT_PATH_DEFAULT;
#define PDNNET_CLIOPT_PATH_USAGE \
  "  " \
    PDNNET_CLIOPT_PATH_SHORT_OPTION ", " \
    PDNNET_CLIOPT_PATH_OPTION " " \
    PDNNET_CLIOPT_PATH_ARG_NAME \
    "       Path to host resource, default " \
    PDNNET_STRINGIFY(PDNNET_CLIOPT_PATH_DEFAULT) "\n"

/**
 * Parse path to host resource value.
 *
 * Relatively permissive as the path must simply be nonempty, start with `/`,
 * and not contain adjacent forward slashes, e.g. `//`.
 *
 * @param arg String path to host resource
 * @returns `true` on successful parse, `false` otherwise
 */
static bool
pdnnet_cliopt_parse_path(const char *arg)
{
  // must have nonzero length
  size_t path_len = strlen(arg);
  if (!path_len) {
    fprintf(stderr, "Error: Path is empty. Use / for the root path\n");
    return false;
  }
  // must start with /
  if (arg[0] != '/') {
    fprintf(stderr, "Error: Path %s invalid; must start with /\n", arg);
    return false;
  }
  // can't have two front slashes together
  for (size_t i = 0; i < path_len - 1; i++) {
    if (arg[i] == '/' && arg[i] == arg[i + 1]) {
      fprintf(
        stderr,
        "Error: Path %ss invalid; cannot contain adjacent forward slashes\n",
        arg
      );
      return false;
    }
  }
  // update path to host resource + return
  PDNNET_CLIOPT(path) = arg;
  return true;
}
#endif  // defined(PDNNET_ADD_CLIOPT_PATH)

#endif  // PDNNET_CLIOPT_OPT_PATH_H_
