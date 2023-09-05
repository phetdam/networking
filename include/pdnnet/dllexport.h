/**
 * @file dllexport.h
 * @author Derek Huang
 * @brief C/C++ header for DLL macro export defs
 * @copyright MIT License
 */

#ifndef PDNNET_DLLEXPORT_H_
#define PDNNET_DLLEXPORT_H_

// defined when building as shared. implies PDNNET_DLL
#ifdef PDNNET_BUILD_DLL
#define PDNNET_DLL
#endif  // PDNNET_DLL
// using shared library
#if defined(PDNNET_DLL)
// Windows
#if defined(_WIN32)
#if defined(PDNNET_BUILD_DLL)
#define PDNNET_PUBLIC __declspec(dllexport)
#else
#define PDNNET_PUBLIC __declspec(dllimport)
#endif  // !defined(PDNNET_BUILD_DLL)
// non-Windows
#else
#define PDNNET_PUBLIC
#endif  // !defined(_WIN32)
// using static library
#else
#define PDNNET_PUBLIC
#endif  // !defined(PDNNET_DLL)

#endif  // PDNNET_DLLEXPORT_H_
