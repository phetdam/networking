/**
 * @file features.h
 * @author Derek Huang
 * @brief C/C++ header for platform and langauge feature detection
 * @copyright MIT License
 */

#ifndef PDNNET_FEATURES_H_
#define PDNNET_FEATURES_H_

#include "pdnnet/common.h"

// test for POSIX unistd.h
#if defined(__has_include)
#if __has_include(<unistd.h>)
#define PDNNET_HAS_UNISTD
#endif  // !__has_include(<unistd.h>)
#endif

// test for BSD/default source definitions
#ifdef _BSD_SOURCE
#define PDNNET_BSD_SOURCE
#endif  // _BSD_SOURCE
#ifdef _DEFAULT_SOURCE
#define PDNNET_DEFAULT_SOURCE
#endif  // _DEFAULT_SOURCE
// convenient helper so we don't need to use _BSD_SOURCE || _DEFAULT_SOURCE
#if defined(PDNNET_BSD_SOURCE) || defined(PDNNET_DEFAULT_SOURCE)
#define PDNNET_BSD_DEFAULT_SOURCE
#endif  // !defined(PDNNET_BSD_SOURCE) && !defined(PDNNET_DEFAULT_SOURCE)

// test for POSIX features
#ifdef _POSIX_C_SOURCE
// we have some subset of POSIX features
#define PDNNET_POSIX
// if we have POSIX.1-1990 and ISO C (1990) features
#if _POSIX_C_SOURCE >= 1
#define PDNNET_POSIX_1
#endif  // !_POSIX_C_SOURCE < 1
// if we have POSIX.2-1992 features
#if _POSIX_C_SOURCE >= 2
#define PDNNET_POSIX_2
#endif  // !_POSIX_C_SOURCE < 2
#if _POSIX_C_SOURCE >= 199309L
// if we have POSIX.1b (real-time extensions) features
#define PDNNET_POSIX_1_B
#endif  // !_POSIX_C_SOURCE < 199309L
// if we have POSIX.1c (threads) features
#if _POSIX_C_SOURCE >= 199506L
#define PDNNET_POSIX_1_C
#endif  // !_POSIX_C_SOURCE < 199506L
// if we have POSIX.1-2001 features (no XSI extension)
#if _POSIX_C_SOURCE >= 200112L
#define PDNNET_POSIX_1_2001
#endif  // !_POSIX_C_SOURCE < 200112L
// if wehave POSIX.1-2008 features (no XSI extension)
#if _POSIX_C_SOURCE >= 200809L
#define PDNNET_POSIX_1_2008
#endif  // !_POSIX_C_SOURCE < 200809L
#endif  // _POSIX_C_SOURCE

// test for GNU features
#ifdef _GNU_SOURCE
#define PDNNET_GNU_SOURCE
#endif  // _GNU_SOURCE

// test for C standard features
#ifdef __STDC_VERSION__
// C89
#if __STDC_VERSION__ >= 199409L
#define PDNNET_HAS_C89
#endif  // __STDC_VERSION__ < 199409L
// C99
#if __STDC_VERSION__ >= 199901L
#define PDNNET_HAS_C99
#endif  // __STDC_VERSION__ < 199001L
// C11
#if __STDC_VERSION__ >= 201112L
#define PDNNET_HAS_C11
#endif  // __STDC_VERSION__ < 201112L
// C17
#if __STDC_VERSION__ >= 201710L
#define PDNNET_HAS_C17
#endif  // __STDC_VERSION__ < 201710L
#endif  // __STDC_VERSION__

// test for C++ standard features
#ifdef PDNNET_CPLUSPLUS
// C++98
#if PDNNET_CPLUSPLUS >= 199711L
#define PDNNET_HAS_CC_98
#endif  // PDNNET_CPLUSPLUS < 199711L
// C++11
#if PDNNET_CPLUSPLUS >= 201103L
#define PDNNET_HAS_CC_11
#endif  // PDNNET_CPLUSPLUS < 201103L
// C++14
#if PDNNET_CPLUSPLUS >= 201402L
#define PDNNET_HAS_CC_14
#endif  // PDNNET_CPLUSPLUS < 201402L
// C++17
#if PDNNET_CPLUSPLUS >= 201703L
#define PDNNET_HAS_CC_17
#endif  // PDNNET_CPLUSPLUS < 201703L
// C++20
#if PDNNET_CPLUSPLUS >= 202002L
#define PDNNET_HAS_CC_20
#endif  // PDNNET_CPLUSPLUS < 202002L
// C++23
#if PDNNET_CPLUSPLUS >= 202302L
#define PDNNET_HAS_CC_23
#endif  // PDNNET_CPLUSPLUS < 202302L
#endif  // PDNNET_CPLUSPLUS

#endif  // PDNNET_FEATURES_H_
