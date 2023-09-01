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

#define PDNNET_ERROR_EXIT_EX(fmt, ...) \
  do { \
    fprintf(stderr, "Error: " fmt, __VA_ARGS__) \
    exit(EXIT_FAILURE); \
  } \
  while (0)

#define PDNNET_ERROR_EXIT(msg) \
  do { \
    fprintf(stderr, "Error: %s\n", msg); \
    exit(EXIT_FAILURE); \
  } \
  while (0)

#define PDNNET_ERRNO_EXIT_EX(err, fmt, ...) \
  do { \
    fprintf(stderr, "Error: " fmt ": %s\n", __VA_ARGS__, strerror(err)); \
    exit(EXIT_FAILURE); \
  } \
  while (0)

#define PDNNET_ERRNO_EXIT(err, fmt) \
  do { \
    fprintf(stderr, "Error: %s\n", strerror(err)); \
    exit(EXIT_FAILURE); \
  } \
  while (0)

#endif  // PDNNET_ERROR_H_
