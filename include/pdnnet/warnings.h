/**
 * @file warnings.h
 * @author Derek Huang
 * @brief C/C++ header for warnings control
 * @copyright MIT License
 */

#ifndef PDNNET_WARNINGS_H_
#define PDNNET_WARNINGS_H_

#include "pdnnet/common.h"

// macros for disabling and reenabling warnings for MSVC
#if defined(_MSC_VER)
/**
 * Push MSVC warning state.
 */
#define PDNNET_MSVC_WARNING_PUSH() __pragma(warning(push))

/**
 * Disable specified MSVC warnings.
 *
 * @param wnos MSVC warning number(s), e.g. 5045, 5105
 */
#define PDNNET_MSVC_WARNING_DISABLE(wnos) __pragma(warning(disable: wnos))

/**
 * Pop MSVC warning state.
 */
#define PDNNET_MSVC_WARNING_POP() __pragma(warning(pop))
#else
/**
 * Push MSVC warning state onto stack.
 */
#define PDNNET_MSVC_WARNING_PUSH()

/**
 * Disable specified MSVC warnings.
 *
 * @param wnos MSVC warning number(s), e.g. 5045, 5105
 */
#define PDNNET_MSVC_WARNING_DISABLE(wnos)

/**
 * Pop MSVC warning state.
 */
#define PDNNET_MSVC_WARNING_POP()
#endif  // !defined(_MSC_VER)

// macros for disabling and reenabling warnings for GCC/Clang
#if defined(__GNUC__)
/**
 * Push GNU/Clang warning state.
 */
#define PDNNET_GNU_WARNING_PUSH() _Pragma("GCC diagnostic push")

/**
 * Disable specifed GNU/Clang warning option.
 *
 * @param option Warning option, e.g. `-Wuninitialized`
 */
#define PDNNET_GNU_WARNING_DISABLE(option) \
  _Pragma(PDNNET_STRINGIFY(GCC diagnostic ignored option))

/**
 * Pop GNU/Clang warning state.
 */
#define PDNNET_GNU_WARNING_POP() _Pragma("GCC diagnostic pop")
#else
/**
 * Disable specified GNU/Clang warning option.
 *
 * @param option Warning option, e.g. `-Wuninitialized`
 */
#define PDNNET_GNU_WARNING_DISABLE(option)

/**
 * Pop GNU/Clang warning state.
 */
#define PDNNET_GNU_WARNING_POP()
#endif  // !defined(__GNUC__)

#endif  // PDNNET_WARNINGS_H_