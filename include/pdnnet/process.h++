/**
 * @file process.h++
 * @author Derek Huang
 * @brief C++ header for process control
 * @copyright MIT License
 */

#ifndef PDNNET_PROCESS_H_PP_
#define PDNNET_PROCESS_H_PP_

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include "pdnnet/error.h++"
#include "pdnnet/features.h"
#include "pdnnet/platform.h"

#ifdef PDNNET_UNIX
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
inline void
daemonize(bool nochdir, bool noclose)
{
  // run in background as daemon
#if defined(PDNNET_BSD_DEFAULT_SOURCE)
  if (daemon(true, true) < 0)
    throw std::runtime_error{
      "daemon() failed: " + std::string{std::strerror(errno)}
    };
  // fallback using fork() call otherwise
#else
#warning "pdnnet::daemonize using fork() instead of daemon()"
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
inline void
daemonize() { daemonize(true, true); }
#endif  // PDNNET_UNIX

}  // namespace pdnnet

#endif  // PDNNET_PROCESS_H_PP_
