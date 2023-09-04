/**
 * @file platform.h
 * @author Derek Huang
 * @brief C/C++ header for platform feature detection
 * @copyright MIT License
 */

#ifndef PDNNET_PLATFORM_H_
#define PDNNET_PLATFORM_H_

// *nix
#if defined(unix) || defined(__unix) || defined(__unix__)
#define PDNNET_UNIX
#endif  // !defined(unix) && !defined(__unix) && !defined(__unix__)
// Linux
#if defined(linux) || defined(__linux) || defined(__linux__)
#define PDNNET_LINUX
#endif  // !defined(linux) && !defined(__linux) && !defined(__linux__)
// Apple OS
#if defined(__APPLE__) || defined(__MACH__)
#define PDNNET_MACOS
#endif  // !defined(__APPLE__) && !defined(__MACH__)

#endif  // PDNNET_PLATFORM_H_
