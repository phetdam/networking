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
#include <sys/types.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

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
 * Invalid socket handle constexpr global.
 */
#if defined(_WIN32)
inline constexpr socket_handle bad_socket_handle = INVALID_SOCKET;
#else
inline constexpr socket_handle bad_socket_handle = -1;
#endif  // !defined(_WIN32)

/**
 * Macro for the default socket read/recv buffer size.
 *
 * This can be redefined at compile time. However, prefer to use the
 * `socket_read_size` constexpr global in actual application code.
 */
#ifndef PDNNET_SOCKET_READ_SIZE
#define PDNNET_SOCKET_READ_SIZE 512U
#endif  // PDNNET_SOCKET_READ_SIZE

/**
 * Default socket read/recv buffer size.
 */
inline constexpr std::size_t socket_read_size = PDNNET_SOCKET_READ_SIZE;

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

/**
 * Enum class indicating how to shut down a socket handle.
 *
 * Members ordered the same way *nix and Windows Sockets macros are ordered.
 */
enum class shutdown_type { read, write, read_write };

/**
 * Return underlying `shutdown_type` value type.
 *
 * @param how `shutdown_type` member
 */
inline auto
shutdown_value(shutdown_type how)
{
  return static_cast<std::underlying_type_t<decltype(how)>>(how);
}

/**
 * Shut down a socket handle.
 *
 * @param handle Socket handle
 * @param how Shutdown method
 */
inline void
shutdown(socket_handle handle, shutdown_type how)
{
  if (::shutdown(handle, shutdown_value(how)) < 0)
    throw std::runtime_error{
      "shutdown() with how=" + std::to_string(shutdown_value(how)) +
      " failed: " +
#if defined(_WIN32)
      std:to_string(WSAGetLastError())
#else
      std::string{std::strerror(errno)}
#endif  // !defined(_WIN32)
    };
}

/**
 * Shut down both receives and sends for a socket handle.
 *
 * @param handle Socket handle
 */
inline void
shutdown(socket_handle handle)
{
  return shutdown(handle, shutdown_type::read_write);
}

#ifdef PDNNET_UNIX
/**
 * Return a new `sockaddr_in`.
 *
 * Inputs should be in host byte order.
 *
 * @param address Internet address, e.g. `INADDR_ANY`
 * @param port Port number, e.g. `8888`
 */
inline auto
socket_address(in_addr_t address, in_port_t port)
{
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htons(address);
  addr.sin_port = htons(port);
  return addr;
}
#endif  // PDNNET_UNIX

/**
 * Socket class maintaining unique ownership of a socket handle.
 *
 * To ensure unique ownership, copying is prohibited.
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
   * Construct directly using the `socket` function but with default protocol.
   *
   * @param af_domain Socket address family/domain, e.g. `AF_INET`, `AF_UNIX`
   * @param type Socket type, e.g. `SOCK_STREAM`, `SOCK_RAW`
   */
  unique_socket(int af_domain, int type) : unique_socket{af_domain, type, 0} {}

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
    if (handle_ != bad_socket_handle)
      close_handle(handle_);
  }

  /**
   * Return underlying socket handle.
   *
   * Provided to mimic the STL `unique_ptr` interface. If `release` has been
   * called or if default-constructed, this returns `bad_socket_handle`.
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
   * @note Cannot use `auto` return type since it is used in move ctor.
   *
   * Once released, destroying the `unique_socket` will not close the handle.
   */
  socket_handle
  release() noexcept
  {
    auto old_handle = handle_;
    handle_ = bad_socket_handle;
    return old_handle;
  }

  /**
   * Return `true` if a valid socket handle is owned.
   *
   * `false` if `release` has been called or after default construction.
   */
  auto
  valid() const noexcept { return handle_ == bad_socket_handle; }

  /**
   * Syntactic sugar for compatibility with C socket functions.
   */
  operator socket_handle() const { return handle_; }

private:
  socket_handle handle_;
};

#ifdef PDNNET_UNIX
/**
 * Socket reader class for abstracting raw socket reads.
 *
 * Can be streamed to an output stream or as a way to initialize a string, e.g.
 *
 * @code{.cc}
 * pdnnet::unique_socket socket{AF_INET, SOCK_STREAM};
 * pdnnet::socket_reader reader{socket};
 * std::cout << reader;
 * // reader.write(std::cout);
 * // auto output = reader.operator std::string();
 * @endcode
 *
 * An lvalue for the reader need not even be created, e.g.
 *
 * @code{.cc}
 * pdnnet::unique_socket socket{AF_INET, SOCK_STREAM};
 * std::cout << pdnnet::socket_reader{socket};
 * @endcode
 */
class socket_reader {
public:
  /**
   * Ctor.
   *
   * @param handle Socket handle
   */
  socket_reader(socket_handle handle) : socket_reader{handle, socket_read_size}
  {}

  /**
   * Ctor.
   *
   * @param handle Socket handle
   * @param buf_size Read buffer size, i.e. number of bytes per chunk read
   */
  socket_reader(socket_handle handle, std::size_t buf_size)
    : handle_{handle},
      buf_size_{buf_size},
      buf_{std::make_unique<unsigned char[]>(buf_size_)}
  {}

  /**
   * Read from socket until end of transmission and write to stream.
   *
   * To write binary data, use a stream with `CharT = char`.
   *
   * @tparam CharT Char type
   * @tparam Traits Char traits
   *
   * @param out Stream to write received data to
   * @returns `*this` to support method chaining
   */
  template <typename CharT, typename Traits>
  auto&
  write(std::basic_ostream<CharT, Traits>& out) const
  {
    // number of bytes read
    ssize_t n_read;
    // until client signals end of transmission
    do {
      // read and handle errors
      if ((n_read = ::read(handle_, buf_.get(), sizeof(CharT) * buf_size_)) < 0)
        throw std::runtime_error{
          "read() failure: " + std::string{std::strerror(errno)}
        };
      // write to stream + clear buffer
      out.write(reinterpret_cast<const CharT*>(buf_.get()), buf_size_);
      memset(buf_.get(), 0, buf_size_);
    }
    while (n_read);
    return *this;
  }

  /**
   * Read from socket until end of transmission and return bytes as a string.
   *
   * @note Usually this does not need to be invoked directly.
   *
   * @tparam CharT Char type
   * @tparam Traits Char traits
   */
  template <typename CharT, typename Traits>
  operator std::basic_string<CharT, Traits>() const
  {
    std::basic_stringstream<CharT, Traits> ss;
    write(ss);
    return ss.str();
  }

private:
  socket_handle handle_;
  std::size_t buf_size_;
  std::unique_ptr<unsigned char[]> buf_;
};

/**
 * Read from socket until end of transmission and write to stream.
 *
 * @tparam CharT Char type
 * @tparam Traits Char traits
 *
 * @param out Stream to write received data to
 * @param reader Socket reader
 */
template <typename CharT, typename Traits>
inline auto&
operator<<(std::basic_ostream<CharT, Traits>& out, const socket_reader& reader)
{
  reader.write(out);
  return out;
}

template <typename CharT = char, typename Traits = std::char_traits<CharT>>
inline auto
read(socket_handle handle, std::size_t buf_size)
{
  return socket_reader{handle, buf_size}.operator std::basic_string<CharT, Traits>();
}

template <typename CharT = char, typename Traits = std::char_traits<CharT>>
inline auto
read(socket_handle handle)
{
  return read<CharT, Traits>(handle, socket_read_size);
}

/**
 * Socket writer class from abstracting writes to raw sockets.
 *
 * Can be streamed from an input stream or write from string contents, e.g.
 *
 * @code{.cc}
 * pdnnet::unique_socket socket{AF_INET, SOCK_STREAM};
 * std::stringstream ss;
 * ss << "text content";
 * ss >> pdnnet::socket_writer{socket};
 * @endcode
 *
 * The `shutdown` function should be called afterwards on the socket handle
 * appropriately to signal end of transmission to the socket receiver.
 */
class socket_writer {
public:
  /**
   * Ctor.
   *
   * @param handle Socket handle
   */
  socket_writer(socket_handle handle) : handle_{handle} {}

  /**
   * Write string view contents to socket.
   *
   * @tparam CharT Char type
   * @tparam Traits Char traits
   *
   * @param text String view to read input from
   * @returns `*this` to support method chaining
   */
  template <typename CharT, typename Traits>
  auto&
  read(const std::basic_string_view<CharT, Traits>& text) const
  {
    if (::write(handle_, text.data(), sizeof(CharT) * text.size()) < 0)
      throw std::runtime_error{
        "write() failure: " + std::string{std::strerror(errno)}
      };
    return *this;
  }

  /**
   * Write string stream contents to socket.
   *
   * @tparam CharT Char type
   * @tparam Traits Char traits
   *
   * @param in String stream to read input from
   * @returns `*this` to support method chaining
   */
  template <typename CharT, typename Traits>
  auto&
  read(std::basic_stringstream<CharT, Traits>& in) const
  {
    return read(static_cast<std::basic_string_view<CharT, Traits>>(in.str()));
  }

private:
  socket_handle handle_;
};

/**
 * Write string stream contents to socket.
 *
 * @param CharT Char type
 * @tparam Traits Char traits
 *
 * @param in String stream to write
 * @param writer Socket writer
 * @returns `in` to support additional streaming
 */
template <typename CharT, typename Traits>
inline auto&
operator>>(
  std::basic_stringstream<CharT, Traits>& in, const socket_writer& writer)
{
  writer.read(in);
  return in;
}
#endif  // PDNNET_UNIX

}  // namespace pdnnet

#endif  // PDNNET_SOCKET_H_PP_