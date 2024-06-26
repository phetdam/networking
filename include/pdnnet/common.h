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
#elif defined(__cplusplus)
#define PDNNET_CPLUSPLUS __cplusplus
#endif  // !defined(_MSVC_LANG) && !defined(__cplusplus)

// stringification macros
#define PDNNET_STRINGIFY_I(x) #x
#define PDNNET_STRINGIFY(x) PDNNET_STRINGIFY_I(x)

// concatenation macros
#define PDNNET_CONCAT_I(x, y) x ## y
#define PDNNET_CONCAT(x, y) PDNNET_CONCAT_I(x, y)

// identity macro. this is useful for a type template instance with multiple
// types as a single argument to a macro instead of as multiple arguments
#define PDNNET_IDENTITY(...) __VA_ARGS__

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

// C-compatible noexcept specifier
#if defined(__cplusplus)
#define PDNNET_NOEXCEPT noexcept
#else
#define PDNNET_NOEXCEPT
#endif  // !defined(__cplusplus)

#endif  // PDNNET_COMMON_H_
