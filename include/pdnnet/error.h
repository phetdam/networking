/**
 * @file error.h
 * @author Derek Huang
 * @brief C error handling helpers
 * @copyright MIT License
 */

#ifndef PDNNET_ERROR_H_
#define PDNNET_ERROR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Print an error to `stderr` and exit with `EXIT_FAILURE`.
 *
 * @param fmt `fprintf` format string literal
 * @param ... Varargs for `fprintf`
 */
#define PDNNET_ERROR_EXIT_EX(fmt, ...) \
  do { \
    fprintf(stderr, "Error: " fmt, __VA_ARGS__) \
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

#endif  // PDNNET_ERROR_H_
