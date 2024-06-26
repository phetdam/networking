/**
 * @file process.hh
 * @author Derek Huang
 * @brief C++ header for process control
 * @copyright MIT License
 */

#ifndef PDNNET_PROCESS_HH_
#define PDNNET_PROCESS_HH_

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <processthreadsapi.h>
#endif  // _WIN32

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include "pdnnet/error.hh"
#include "pdnnet/features.h"
#include "pdnnet/platform.h"

#ifdef PDNNET_UNIX
#include <sys/types.h>
#include <unistd.h>
#endif  // PDNNET_UNIX

// currently only needed when PDNNET_BSD_DEFAULT_SOURCE not defined
#ifndef PDNNET_BSD_DEFAULT_SOURCE
#include <cstdlib>
#endif  // PDNNET_BSD_DEFAULT_SOURCE

namespace pdnnet {

#ifdef PDNNET_UNIX
/**
 * Make the current program run as a system daemon.
 *
 * Wraps the `daemon` system call if possible, otherwise uses `fork`. If `fork`
 * is being used, a compile-time warning is printed and parameters are ignored.
 *
 * @param nochdir `false` to change process working directory to `/`
 * @param noclose `false` to redirect `stdin`, `stdout`, `stderr` to `/dev/null`
 */
inline void daemonize(bool nochdir, bool noclose)
{
  // run in background as daemon
#if defined(PDNNET_BSD_DEFAULT_SOURCE)
  if (daemon(true, true) < 0)
    throw std::runtime_error{
      "daemon() failed: " + std::string{std::strerror(errno)}
    };
  // fallback using fork() call otherwise
#else
// #warning is not standard until C++23, but is supported by at least GCC/Clang
#if defined(PDNNET_HAS_CC_23) || defined(__GNUC__) || defined(__clang__)
#warning "pdnnet::daemonize using fork() instead of daemon()"
#endif  // !defined(PDNNET_HAS_CC_23) && !defined(__GNUC__) &&
        // !defined(__clang__)
  switch (fork()) {
    case -1:
      throw std::runtime_error{errno_error("fork() failed")};
    case 0: break;
    // parent exits immediately to orphan child
    default: _exit(EXIT_SUCCESS);
  }
#endif  // !defined(PDNNET_BSD_DEFAULT_SOURCE)
}

/**
 * Make the current program run as a system daemon.
 *
 * Does not change the process working directory or redirect streams.
 */
inline void daemonize() { daemonize(true, true); }
#endif  // PDNNET_UNIX

#if defined(PDNNET_UNIX) || defined(_WIN32)
/**
 * Return the process ID of the calling process.
 *
 * This function is available on POSIX and Windows.
 */
inline auto getpid() noexcept
{
#if defined(_WIN32)
  return GetCurrentProcessId();
#else
  return ::getpid();
#endif  // !defined(_WIN32)
}
#endif  // !defined(PDNNET_UNIX) && !defined(_WIN32)

}  // namespace pdnnet

#endif  // PDNNET_PROCESS_HH_
