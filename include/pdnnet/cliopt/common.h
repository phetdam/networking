/**
 * @file cliopt/common.h
 * @author Derek Huang
 * @brief C/C++ header for common cliopt definitions
 * @copyright MIT License
 */

#ifndef PDNNET_CLIOPT_COMMON_H_
#define PDNNET_CLIOPT_COMMON_H_

// namespacing macros for a variable corresponding to a command-line option
#define PDNNET_CLIOPT_I(name) pdnnet_cliopt_ ## name
#define PDNNET_CLIOPT(name) PDNNET_CLIOPT_I(name)

/**
 * Macro for an else-if statement that matches the short or long option name.
 *
 * @param argv Argument vector from `main`
 * @param i Index to current argument
 * @param short_name Short option name
 * @param long_name Long option name
 */
#define PDNNET_CLIOPT_PARSE_MATCHES(argv, i, short_name, long_name) \
  else if (!strcmp(argv[i], short_name) || !strcmp(argv[i], long_name))

#endif  // PDNNET_CLIOPT_COMMON_H_
