/**
 * @file common.h
 * @author Derek Huang
 * @brief C/C++ header for common macros
 * @copyright MIT License
 */

#ifndef PDNNET_COMMON_H_
#define PDNNET_COMMON_H_

// extern "C" scoping macros
#ifdef __cplusplus
#define PDNNET_EXTERN_C_BEGIN extern "C" {
#define PDNNET_EXTERN_C_END }
#else
#define PDNNET_EXTERN_C_BEGIN
#define PDNNET_EXTERN_C_END
#endif  // __cplusplus

// C++ standard detection. MSVC sets __cplusplus incorrectly
#if defined(_MSVC_LANG)
#define PDNNET_CPLUSPLUS _MSVC_LANG
#else
#define PDNNET_CPLUSPLUS __cplusplus
#endif  // !defined(_MSVC_LANG)

// stringification macros
#define PDNNET_STRINGIFY_I(x) #x
#define PDNNET_STRINGIFY(x) PDNNET_STRINGIFY_I(x)

// concatenation macros
#define PDNNET_CONCAT_I(x, y) x ## y
#define PDNNET_CONCAT(x, y) PDNNET_CONCAT_I(x, y)

// allow C++-like use of inline in C code
#if defined(__cplusplus)
#define PDNNET_INLINE inline
#else
#define PDNNET_INLINE static inline
#endif  // !defined(__cplusplus)

// macro to indicate extern inline, i.e. never static inline
#define PDNNET_EXTERN_INLINE inline

// path separator as char and string
#if defined(_WIN32)
#define PDNNET_PATH_SEP_CHAR '\\'
#define PDNNET_PATH_SET_STRING "\\"
#else
#define PDNNET_PATH_SEP_CHAR '/'
#define PDNNET_PATH_SEP_STRING "/"
#endif  // !defined(_WIN32)

#endif  // PDNNET_COMMON_H_
