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

#endif  // PDNNET_CLIOPT_COMMON_H_
