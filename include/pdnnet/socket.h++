/**
 * @file socket.h++
 * @author Derek Huang
 * @brief C++ header for sockets helpers
 * @copyright MIT License
 */

#ifndef PDNNET_SOCKET_H_PP_
#define PDNNET_SOCKET_H_PP_

// on Windows, we use Windows Sockets 2
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
// don't pollute translation units
#undef WIN32_LEAN_AND_MEAN
// for *nix systems, use standard socket API
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <cerrno>
#include <cstring>

#include "pdnnet/platform.h"

// not ready for Windows use yet
#ifndef PDNNET_UNIX
#error "socket.h++ cannot be used on non-Unix platforms yet"
#endif  // PDNNET_UNIX

namespace pdnnet {

/**
 * Socket handle type.
 *
 * Conditional compliation to handle platform-specific differences.
 */
#if defined(_WIN32)
using socket_handle = SOCKET;
#else
using socket_handle = int;
#endif  // !defined(_WIN32)

/**
 * Macro with value of an invalid socket handle.
 */
#if defined(_WIN32)
#define PDNNET_BAD_SOCKET_HANDLE INVALID_SOCKET
#else
#define PDNNET_BAD_SOCKET_HANDLE -1
#endif  // !defined(_WIN32)

/**
 * Close the socket handle.
 *
 * Prefer using `unique_socket` instead of using raw socket handles directly.
 *
 * @param handle Socket handle to close
 * @returns 0 on success, -1 (*nix) or SOCKET_ERROR (Win32) on failure
 */
inline int
close_handle(socket_handle handle) noexcept
{
#if defined(_WIN32)
  return closesocket(handle);
#else
  return close(handle);
#endif  // !defined(_WIN32)
}

#ifdef PDNNET_UNIX
/**
 * Return a new `sockaddr_in`.
 *
 * Inputs should be in host byte order.
 *
 * @param family Address family, e.g. `AF_INET`
 * @param address Internet address, e.g. `INADDR_ANY`
 * @param port Port number, e.g. `8888`
 */
inline auto
create_sockaddr_in(sa_family_t family, in_addr_t address, in_port_t port)
{
  sockaddr_in addr{};
  addr.sin_family = family;
  addr.sin_addr.s_addr = htons(address);
  addr.sin_port = htons(port);
  return addr;
}
#endif  // PDNNET_UNIX

/**
 * Socket class maintaining unique ownership of a socket handle.
 */
class unique_socket {
public:
  /**
   * Ctor.
   *
   * Construct from a raw socket handle. The handle's validity is not checked.
   *
   * @param handle Socket handle
   */
  explicit unique_socket(socket_handle handle) : handle_{handle} {}

  /**
   * Ctor.
   *
   * Construct directly using the `socket` function.
   *
   * @param af_domain Socket address family/domain, e.g. `AF_INET`, `AF_UNIX`
   * @param type Socket type, e.g. `SOCK_STREAM`, `SOCK_RAW`
   * @param protocol Socket protocol, e.g. `0`
   */
  unique_socket(int af_domain, int type, int protocol)
  {
    // attempt to create socket handle
    handle_ = ::socket(af_domain, type, protocol);
#if defined(_WIN32)
    // TODO: add error handling for Windows Sockets using WSAGetLastError later
    if (handle_ == INVALID_SOCKET)
      throw std::runtime_error{"Could not open socket"};
#else
    if (handle_ < 0)
      throw std::runtime_error{
        "Could not open socket: " + std::string{std::strerror(errno)}
      };
#endif  // !defined(_WIN32)
  }

  /**
   * Deleted copy ctor.
   */
  unique_socket(const unique_socket& socket) = delete;

  /**
   * Move ctor.
   */
  unique_socket(unique_socket&& socket) { handle_ = socket.release(); }

  /**
   * Dtor.
   *
   * Close the raw socket handle if not released.
   */
  ~unique_socket()
  {
    if (handle_ != PDNNET_BAD_SOCKET_HANDLE)
      close_handle(handle_);
  }

  /**
   * Return underlying socket handle.
   *
   * Provided to mimic the STL `unique_ptr` interface. If `release` has been
   * called or if default-constructed, this returns `PDNNET_BAD_SOCKET_HANDLE`.
   */
  auto
  get() const noexcept { return handle_; }

  /**
   * Return underlying socket handle.
   */
  auto
  handle() const noexcept { return handle_; }

  /**
   * Release ownership of the underlying socket handle.
   *
   * Once released, destroying the `unique_socket` will not close the handle.
   */
  auto
  release() noexcept
  {
    auto old_handle = handle_;
    handle_ = PDNNET_BAD_SOCKET_HANDLE;
    return old_handle;
  }

  /**
   * Return `true` if a valid socket handle is owned.
   *
   * `false` if `release` has been called or after default construction.
   */
  auto
  valid() const noexcept { return handle_ == PDNNET_BAD_SOCKET_HANDLE; }

private:
  socket_handle handle_;
};

}  // namespace pdnnet

#endif  // PDNNET_SOCKET_H_PP_
