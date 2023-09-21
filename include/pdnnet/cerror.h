/**
 * @file cerror.h
 * @author Derek Huang
 * @brief C error handling helpers
 * @copyright MIT License
 */

#ifndef PDNNET_CERROR_H_
#define PDNNET_CERROR_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdnnet/features.h"

#ifdef PDNNET_BSD_DEFAULT_SOURCE
#include <netdb.h>
#endif  // !PDNNET_BSD_DEFAULT_SOURCE

/**
 * Return `-errno` if `expr` is less than zero.
 *
 * Most C library functions return `-1` on error and set `errno`. These
 * functions are suitable for use with this convenience macro.
 *
 * @param expr Expression evaluating to a signed integer
 */
#define PDNNET_ERRNO_RETURN(expr) if ((expr) < 0) return -errno

/**
 * Print an error to `stderr` and exit with `EXIT_FAILURE`.
 *
 * @param fmt `fprintf` format string literal
 * @param ... Varargs for `fprintf`
 */
#define PDNNET_ERROR_EXIT_EX(fmt, ...) \
  do { \
    fprintf(stderr, "Error: " fmt, __VA_ARGS__); \
    exit(EXIT_FAILURE); \
  } \
  while (0)

/**
 * Print an error to `stderr` and exit with `EXIT_FAILURE`.
 *
 * @param msg Error message to print
 */
#define PDNNET_ERROR_EXIT(msg) \
  do { \
    fprintf(stderr, "Error: %s\n", msg); \
    exit(EXIT_FAILURE); \
  } \
  while (0)

/**
 * Print an `errno` error value to `stderr` and exit with `EXIT_FAILURE`.
 *
 * @param err `errno` value
 * @param fmt `fprintf` format string literal
 * @param ... Varargs for `fprintf`
 */
#define PDNNET_ERRNO_EXIT_EX(err, fmt, ...) \
  do { \
    fprintf(stderr, "Error: " fmt ": %s\n", __VA_ARGS__, strerror(err)); \
    exit(EXIT_FAILURE); \
  } \
  while (0)

/**
 * Print an `errno` error value to `stderr` and exit with `EXIT_FAILURE`.
 *
 * @param err `errno` value
 * @param msg Error message to print
 */
#define PDNNET_ERRNO_EXIT(err, msg) \
  do { \
    fprintf(stderr, "Error: %s: %s\n", msg, strerror(err)); \
    exit(EXIT_FAILURE); \
  } \
  while (0)

#ifdef PDNNET_BSD_DEFAULT_SOURCE
/**
 * Print a `h_errno` error value to `stderr` and exit with `EXIT_FAILURE`.
 *
 * @param err `h_errno` value
 * @param fmt `fprintf` format string literal
 * @param ... Varargs for `fprintf`
 */
#define PDNNET_H_ERRNO_EXIT_EX(err, fmt, ...) \
  do { \
    fprintf(stderr, "Error: " fmt ": %s\n", __VA_ARGS__, hstrerror(err)); \
    exit(EXIT_FAILURE); \
  } \
  while (0)

/**
 * Print a `h_errno` error value to `stderr` and exit with `EXIT_FAILURE`.
 *
 * @param err `h_errno` value
 * @param msg Error message to print
 */
#define PDNNET_H_ERRNO_EXIT(err, msg) \
  do { \
    fprintf(stderr, "Error: %s: %s\n", msg, hstrerror(err)); \
    exit(EXIT_FAILURE); \
  } \
  while (0)
#endif  // PDNNET_BSD_DEFAULT_SOURCE


#endif  // PDNNET_CERROR_H_
