/**
 * @file socket.hh
 * @author Derek Huang
 * @brief C++ header for sockets helpers
 * @copyright MIT License
 */

#ifndef PDNNET_SOCKET_HH_
#define PDNNET_SOCKET_HH_

// on Windows, we use Windows Sockets 2, so WIN32_LEAN_AND_MEAN must be defined
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <ws2ipdef.h>
// for *nix systems, use standard socket API
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <istream>
#include <optional>
#include <ostream>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "pdnnet/error.hh"
#include "pdnnet/features.h"
#include "pdnnet/platform.h"
#include "pdnnet/warnings.h"

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
 * Signed integral type returned by `read`, `recv`, etc.
 */
#if defined(_WIN32)
using ssize_type = int;
#else
using ssize_type = ssize_t;
#endif  // !defined(_WIN32)

#ifdef _WIN32
/**
 * [Re]initialize Windows Sockets 2.
 *
 * After the first call, the parameters are ignored and this function can be
 * used to retrieve a const reference to the relevant `WSADATA` struct.
 *
 * @param major Windows Sockets major version
 * @param minor Windows Socket minor version
 * @returns `WSADATA` const reference
 */
inline const auto& winsock_init(BYTE major, BYTE minor)
{
#ifndef PDNNET_DOXYGEN
  /**
   * Private class for initializing Windows Sockets.
   */
  class wsa_init {
  public:
    /**
     * Ctor.
     *
     * @param major Windows Sockets major version
     * @param minor Windows Sockets minor version
     */
    wsa_init(BYTE major, BYTE minor)
    {
      auto status = WSAStartup(MAKEWORD(major, minor), &wsa_data_);
      if (status)
        throw std::runtime_error{winsock_error("WSAStartup() failed")};
    }

    /**
     * Dtor.
     *
     * Performs the final cleanup of Windows Sockets.
     */
    ~wsa_init() { WSACleanup(); }

    /**
     * Return const reference to the `WSADATA` struct.
     */
    const auto& wsa_data() const noexcept { return wsa_data_; }

  private:
    WSADATA wsa_data_;
  };
#endif  // PDNNET_DOXYGEN

  static wsa_init init{major, minor};
  return init.wsa_data();
}

/**
 * [Re]initialize Windows Sockets 2.2.
 *
 * @returns `WSADATA` const reference
 */
inline const auto& winsock_init() { return winsock_init(2, 2); }
#endif  // _WIN32

/**
 * Close the socket handle.
 *
 * Prefer using `unique_socket` instead of using raw socket handles directly.
 *
 * @param handle Socket handle to close
 * @returns 0 on success, -1 (*nix) or SOCKET_ERROR (Win32) on failure
 */
inline int close_handle(socket_handle handle) noexcept
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
inline auto shutdown_value(shutdown_type how)
{
  return static_cast<std::underlying_type_t<decltype(how)>>(how);
}

/**
 * Shut down a socket handle.
 *
 * @param handle Socket handle
 * @param how Shutdown method
 */
inline void shutdown(socket_handle handle, shutdown_type how)
{
  if (::shutdown(handle, shutdown_value(how)) < 0)
    throw std::runtime_error{
      "shutdown() with how=" + std::to_string(shutdown_value(how)) +
#if defined(_WIN32)
      winsock_error(" failed")
#else
      errno_error(" failed")
#endif  // !defined(_WIN32)
    };
}

/**
 * Shut down both receives and sends for a socket handle.
 *
 * @param handle Socket handle
 */
inline void shutdown(socket_handle handle)
{
  return shutdown(handle, shutdown_type::read_write);
}

/**
 * Internet address integral type.
 *
 * This also determines the size of `in_addr`.
 */
#if defined(_WIN32)
using inet_addr_type = ULONG;
#else
using inet_addr_type = in_addr_t;
#endif  // !defined(_WIN32)

/**
 * Internet port integral type.
 */
#if defined(_WIN32)
using inet_port_type = USHORT;
#else
using inet_port_type = in_port_t;
#endif  // !defined(_WIN32)

/**
 * Return a new `sockaddr_in`.
 *
 * Inputs should be in host byte order.
 *
 * @param address Internet address, e.g. `INADDR_ANY`
 * @param port Port number, e.g. `8888`
 */
inline auto make_sockaddr_in(inet_addr_type address, inet_port_type port) noexcept
{
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(address);
  addr.sin_port = htons(port);
  return addr;
}

/**
 * Return a new `sockaddr_in`.
 *
 * Inputs should be in host byte order.
 *
 * @param ent Host entry description
 * @param port Port number, e.g. `8888`
 */
inline auto make_sockaddr_in(const hostent* ent, inet_port_type port)
{
  if (!ent)
    throw std::runtime_error{"hostent struct pointer is null"};
  // in_addr integral type for address
  inet_addr_type address;
  // error if address length is not equal to in_addr size
  if (ent->h_length != sizeof address)
    throw std::runtime_error{
      "hostent address length in bytes " + std::to_string(ent->h_length) +
      " not equal to inet_addr_type size " + std::to_string(sizeof address)
    };
  // otherwise, copy first address from hostent + return sockaddr_in
  std::memcpy(
    &address,
#if defined(h_addr)
    ent->h_addr,
#else
    ent->h_addr_list[0],
#endif  // !defined(h_addr)
    // already checked that this is equal to ent.h_length
    sizeof address
  );
  return make_sockaddr_in(ntohl(address), port);
}

/**
 * Socket class maintaining unique ownership of a socket handle.
 *
 * To ensure unique ownership, copying is prohibited.
 */
class unique_socket {
public:
  /**
   * Default ctor.
   *
   * Uses `bad_socket_handle` as the socket handle.
   */
  unique_socket() noexcept : unique_socket{bad_socket_handle} {}

  /**
   * Ctor.
   *
   * Construct from a raw socket handle. The handle's validity is not checked.
   *
   * @param handle Socket handle
   */
  explicit unique_socket(socket_handle handle) noexcept : handle_{handle} {}

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
   * Construct directly using the `socket` function. On Windows, this ctor also
   * ensures that Windows Sockets 2 is correctly initialized.
   *
   * @param af_domain Socket address family/domain, e.g. `AF_INET`, `AF_UNIX`
   * @param type Socket type, e.g. `SOCK_STREAM`, `SOCK_RAW`
   * @param protocol Socket protocol, e.g. `0`, `IPPROTO_TCP`
   */
  unique_socket(int af_domain, int type, int protocol)
  {
    // attempt to create socket handle
#ifdef _WIN32
    winsock_init();
#endif  // _WIN32
    handle_ = ::socket(af_domain, type, protocol);
#if defined(_WIN32)
    if (handle_ == INVALID_SOCKET)
      throw std::runtime_error{winsock_error("Could not open socket")};
#else
    if (handle_ < 0)
      throw std::runtime_error{errno_error("Could not open socket")};
#endif  // !defined(_WIN32)
  }

  /**
   * Deleted copy ctor.
   */
  unique_socket(const unique_socket& socket) = delete;

  /**
   * Move ctor.
   */
  unique_socket(unique_socket&& socket) noexcept
  {
    handle_ = socket.release();
  }

  /**
   * Dtor.
   *
   * Close the raw socket handle if not released.
   */
  ~unique_socket()
  {
    destroy_handle();
  }

  /**
   * Move assignment operator.
   */
  unique_socket& operator=(unique_socket&& socket) noexcept
  {
    destroy_handle();
    handle_ = socket.release();
    return *this;
  }

  /**
   * Return underlying socket handle.
   *
   * Provided to mimic the STL `unique_ptr` interface. If `release` has been
   * called or if default-constructed, this returns `bad_socket_handle`.
   */
  auto get() const noexcept { return handle_; }

  /**
   * Return underlying socket handle.
   */
  auto handle() const noexcept { return handle_; }

  /**
   * Release ownership of the underlying socket handle.
   *
   * @note Cannot use `auto` return type since it is used in move ctor.
   *
   * Once released, destroying the `unique_socket` will not close the handle.
   */
  socket_handle release() noexcept
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
  bool valid() const noexcept { return handle_ != bad_socket_handle; }

  /**
   * Syntactic sugar for compatibility with C socket functions.
   */
  operator socket_handle() const noexcept { return handle_; }

private:
  socket_handle handle_;

  /**
   * Close the managed socket handle if valid.
   */
  void destroy_handle() noexcept
  {
    if (valid())
      close_handle(handle_);
  }
};

/**
 * Class template helper for checking supported socket address types.
 *
 * Currently only supports `sockaddr_in`, `sockaddr_in6` address types.
 *
 * @tparam AddressType Socket address type
 */
template <typename AddressType>
struct is_addr_supported : std::bool_constant<
  std::is_same_v<AddressType, sockaddr_in> ||
  std::is_same_v<AddressType, sockaddr_in6>
> {};

/**
 * Boolean helper for checking supported socket address types.
 *
 * @tparam AddressType Socket address type
 */
template <typename AddressType>
inline constexpr bool is_addr_supported_v = is_addr_supported<AddressType>::value;

#ifdef PDNNET_HAS_CC_20
/**
 * Concept for supported socket address struct types.
 *
 * @param T type
 */
template <typename T>
concept inet_sockaddr = is_addr_supported_v<T>;
#endif  // PDNNET_HAS_CC_20

/**
 * Restricted template parameter macro for supported socket address types.
 *
 * @param T type placeholder
 */
#if defined(PDNNET_HAS_CC_20)
#define PDNNET_INET_SOCKADDR(T) pdnnet::inet_sockaddr T
#else
#define PDNNET_INET_SOCKADDR(T) \
  typename T, typename = std::enable_if_t<pdnnet::is_addr_supported_v<T>>
#endif // !defined(PDNNET_HAS_CC_20)

/**
 * Perform a blocking accept of the next connection in the client queue.
 *
 * @tparam AddressType Socket address type
 *
 * @param handle Socket handle
 * @param addr Socket address structure, e.g. `sockaddr_in`, `sockaddr_in6`,
 *  that will receive the address of the peer (client) socket
 * @param addr_len `socklen_t` that will receive the size of the socket address
 * @returns `unique_socket` managing the client socket
 */
template <PDNNET_INET_SOCKADDR(AddressType)>
inline auto accept(socket_handle handle, AddressType& addr, socklen_t& addr_len)
{
  // input address length, i.e. sizeof addr
  socklen_t addr_len_in = sizeof addr;
  // accept next client connection, returning managed socket
  unique_socket socket{::accept(handle, static_cast<sockaddr*>(&addr), &addr_len_in)};
  // socket handle is invalid on error
  if (!socket.valid())
#if defined(_WIN32)
    throw std::runtime_error{winsock_error("accept() failed")};
#else
    throw std::runtime_error{errno_error("accept() failed")};
  // if buffer is too small, address is truncated, which is still an error. the
  // Windows Sockets version of accept checks this and WSAGetLastError will
  // return WSAEFAULT if sizeof cli_addr is too small.
  if (addr_len_in > sizeof addr)
    throw std::runtime_error{"Client address buffer truncated"};
#endif  // !defined(_WIN32)
  // done, so set addr_len and return socket
  addr_len = addr_len_in;
  return socket;
}

/**
 * Perform a blocking accept of the next connection in the client queue.
 *
 * A more convenient overload of `accept` that does not require struct size.
 *
 * @tparam AddressType Socket address type
 *
 * @param handle Socket handle
 * @param addr Socket address structure, e.g. `sockaddr_in`, `sockaddr_in6`,
 *  that will receive the address of the peer (client) socket
 * @returns `unique_socket` managing the client socket
 */
template <PDNNET_INET_SOCKADDR(AddressType)>
inline auto accept(socket_handle handle, AddressType& addr)
{
  socklen_t addr_len = sizeof addr;
  return accept(handle, addr, addr_len);
}

/**
 * Perform a blocking accept of the next connection in the client queue.
 *
 * @note Neither address nor address length are passed to the actual `::accept`
 *  call, i.e. both parameters are `nullptr`, so no template param is needed.
 *
 * @param handle Socket handle
 * @returns `unique_socket` managing the client socket
 */
inline auto accept(socket_handle handle)
{
  // accept next client connection, returning managed socket. there is no use
  // of the address structure or its length here
  unique_socket socket{::accept(handle, nullptr, nullptr)};
  // socket handle is invalid on error
  if (!socket.valid())
    throw std::runtime_error{socket_error("accept() failed")};
  // return optional with unique_socket
  return socket;
}

/**
 * Bind a created socket handle to an address.
 *
 * On error, `errno` (*nix) or `WSAGetLastError` (Windows) should be checked.
 *
 * @tparam AddressType Socket address type
 *
 * @param handle Socket handle
 * @param addr Socket address structure, e.g. `sockaddr_in`, `sockaddr_in6`
 * @returns `true` on success, `false` on error
 */
template <PDNNET_INET_SOCKADDR(AddressType)>
inline bool bind(socket_handle handle, const AddressType& addr) noexcept
{
#if defined(_WIN32)
  if (
    ::bind(
      handle,
      reinterpret_cast<const sockaddr*>(&addr),
      static_cast<int>(sizeof addr)
    ) == SOCKET_ERROR
  )
#else
  if (::bind(handle, reinterpret_cast<const sockaddr*>(&addr), sizeof addr) < 0)
#endif  // !defined(_WIN32)
    return false;
  return true;
}

/**
 * Connect to a socket given an open socket handle and an address.
 *
 * On error, `errno` (*nix) or `WSAGetLastError` (Windows) should be checked.
 *
 * @note Currently only supports `sockaddr_in`, `sockaddr_in6` address types.
 *
 * @tparam AddressType Socket address type
 *
 * @param handle Socket handle
 * @param addr Socket address structure, e.g. `sockaddr_in`, `sockaddr_in6`
 * @returns `true` on success, `false` on error
 */
template <PDNNET_INET_SOCKADDR(AddressType)>
inline bool connect(socket_handle handle, const AddressType& addr) noexcept
{
#if defined(_WIN32)
  if (
    ::connect(
      handle,
      reinterpret_cast<const sockaddr*>(&addr),
      static_cast<socklen_t>(sizeof addr)
    ) == SOCKET_ERROR
  )
#else
  if (
    ::connect(
      handle,
      reinterpret_cast<const sockaddr*>(&addr),
      static_cast<socklen_t>(sizeof addr)
    ) < 0
  )
#endif  // !defined(_WIN32)
    return false;
  return true;
}

/**
 * Place a bound, unconnected socket handle in listening mode.
 *
 * On error, `errno` (*nix) or `WSAGetLastError` (Windows) should be checked.
 *
 * @param handle Bound socket handle
 * @param max_pending Maximum number of pending connections in backlog
 * @returns `true` on success, `false` on error
 */
inline bool listen(socket_handle handle, unsigned int max_pending) noexcept
{
#if defined(_WIN32)
  if (::listen(handle, static_cast<int>(max_pending)) == SOCKET_ERROR)
#else
  if (::listen(handle, static_cast<int>(max_pending)) < 0)
#endif  // !defined(_WIN32)
    return false;
  return true;
}

/**
 * Get the address struct of the specified socket handle.
 *
 * On error, `errno` (*nix) or `WSAGetLastError` (Windows) should be checked.
 *
 * @tparam AddressType Socket address type
 *
 * @param handle Socket handle
 * @param addr Socket address structure, e.g. `sockaddr_in`, `sockaddr_in6`
 * @returns `true` on success, `false` on error
 */
template <PDNNET_INET_SOCKADDR(AddressType)>
inline bool getsockname(socket_handle handle, AddressType& addr) noexcept
{
  // note: sizeof(T&) == sizeof(T)
  socklen_t addr_size = sizeof addr;
#if defined(_WIN32)
  if (::getsockname(handle, (sockaddr*) &addr, &addr_size) == SOCKET_ERROR)
#else
  if (::getsockname(handle, (sockaddr*) &addr, &addr_size) < 0)
#endif  // !defined(_WIN32)
    return false;
  return true;
}

/**
 * Poll a single socket for events.
 *
 * @note `timeout` is cast to `int` before being passed to `::poll`.
 *
 * @tparam Rep Arithmetic type
 *
 * @param handle Socket handle
 * @param events Events to poll for, e.g. `POLLIN | POLLOUT`
 * @param timeout Timeout in milliseconds to block for when waiting for events.
 *  If zero, returns immediately, and if negative, timeout is infinite.
 * @returns Bitmask indicating the events that have occurred, zero if no events
 */
template <typename Rep, typename = std::enable_if_t<std::is_arithmetic_v<Rep>>>
short poll(socket_handle handle, short events, Rep timeout)
{
  // poll events for socket handle
  pollfd res{handle, events};
#if defined(_WIN32)
  auto status = WSAPoll(&res, 1, static_cast<int>(timeout));
#else
  auto status = ::poll(&res, 1, static_cast<int>(timeout));
#endif  // !defined(_WIN32)
  // if negative, throw, as this is usually fatal, e.g. EFAULT, EINTR, ENOMEM
  // for *nix systems or WSAENETDOWN, WSAEFAULT, WSAEINVAL, WSAENOBUFS for
  // Windows Sockets 2. better to just terminate in this case
  if (status < 0)
    throw std::runtime_error{socket_error("poll() failed")};
  // otherwise if 0, no events
  if (!status)
    return 0;
  // otherwise positive, i.e. 1, so return the received events
  return res.revents;
}

/**
 * Poll a single socket for events with a timeout of 1 ms.
 *
 * @param handle Socket handle
 * @param events Events to poll for, e.g. `POLLIN | POLLOUT`
 * @returns Bitmask indicating the events that have occurred, zero if no events
 */
inline auto poll(socket_handle handle, short events)
{
  return poll(handle, events, 1);
}

/**
 * Duration representing infinite `pdnnet::poll` timeout.
 *
 * Use signed `int` since `::poll` treats negative timeout as infinite.
 */
inline constexpr std::chrono::duration<int, std::milli> infinite_poll_timeout{-1};

/**
 * Poll a single socket for events.
 *
 * @note `timeout` is cast to `int` before being passed to `::poll`.
 *
 * @tparam Rep Arithmetic type
 *
 * @param handle Socket handle
 * @param events Events to poll for, e.g. `POLLIN | POLLOUT`
 * @param timeout Timeout to block for when waiting for events. If zero,
 *  returns immediately, and if negative, timeout is infinite.
 * @returns Bitmask indicating the events that have occurred, zero if no events
 */
template <typename Rep, typename = std::enable_if_t<std::is_arithmetic_v<Rep>>>
inline auto poll(
  socket_handle handle,
  short events,
  std::chrono::duration<Rep, std::milli> timeout)
{
  return poll(handle, events, timeout.count());
}

/**
 * Block until socket is ready for reading or until timeout elapses.
 *
 * Useful for waiting until the `POLLIN` event has occurred for the socket.
 *
 * @note `timeout` is cast to `int` before being passed to `::poll`.
 *
 * @tparam Rep Arithmetic type
 *
 * @param handle Socket handle
 * @param timeout Timeout in milliseconds to block for when waiting. If zero,
 *  returns immediately, and if negative, timeout is infinite.
 * @returns `true` if `POLLIN` has occurred, `false` if timed out
 */
template <typename Rep, typename = std::enable_if_t<std::is_arithmetic_v<Rep>>>
inline bool wait_pollin(socket_handle handle, Rep timeout)
{
  return poll(handle, POLLIN, timeout) & POLLIN;
}

/**
 * Block until socket is ready for reading or until 1 ms timeout elapses.
 *
 * Useful for waiting until the `POLLIN` event has occurred for the socket.
 *
 * @param handle Socket handle
 * @param timeout Timeout in milliseconds to block for when waiting. If zero,
 *  returns immediately, and if negative, timeout is infinite.
 * @returns `true` if `POLLIN` has occurred, `false` if timed out
 */
inline bool wait_pollin(socket_handle handle)
{
  return wait_pollin(handle, 1);
}

/**
 * Block until socket is ready for reading or until timeout elapses.
 *
 * Useful for waiting until the `POLLIN` event has occurred for the socket.
 *
 * @note `timeout` is cast to `int` before being passed to `::poll`.
 *
 * @tparam Rep Arithmetic type
 *
 * @param handle Socket handle
 * @param timeout Timeout to block for when waiting for events. If zero,
 *  returns immediately, and if negative, timeout is infinite.
 * @returns `true` if `POLLIN` has occurred, `false` if timed out
 */
template <typename Rep, typename = std::enable_if_t<std::is_arithmetic_v<Rep>>>
inline bool wait_pollin(
  socket_handle handle, std::chrono::duration<Rep, std::milli> timeout)
{
  return wait_pollin(handle, timeout.count());
}

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
   * Default timeout duration to use when polling socket for input.
   */
  static inline constexpr std::chrono::milliseconds poll_timeout_default{1};

  /**
   * Ctor.
   *
   * Buffer read size is given by `socket_read_size`.
   *
   * @param handle Socket handle
   * @param poll_timeout Timeout to use when polling socket for input
   */
  socket_reader(
    socket_handle handle,
    std::chrono::milliseconds poll_timeout = poll_timeout_default)
    : socket_reader{handle, socket_read_size, poll_timeout}
  {}

  /**
   * Ctor.
   *
   * @param handle Socket handle
   * @param buf_size Read buffer size, i.e. number of bytes per read
   * @param poll_timeout Timeout to use when polling socket for input
   */
  socket_reader(
    socket_handle handle,
    std::size_t buf_size,
    std::chrono::milliseconds poll_timeout = poll_timeout_default)
    : handle_{handle},
      buf_size_{buf_size},
      buf_{std::make_unique<unsigned char[]>(buf_size_)},
      poll_timeout_{poll_timeout}
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
   * @returns Optional empty on success, with error message on failure
   */
  template <typename CharT, typename Traits>
  optional_error write(std::basic_ostream<CharT, Traits>& out) const
  {
    // number of bytes read
    ssize_type n_read;
    // loop until we read zero bytes
    do {
      // poll to check if there is anything to read. if not, return. note that
      // if write end is not closed, POLLIN is possible with read() returning 0
      // TODO: see poll() todo. might use std::chrono::duration for timeout
      if (!(poll(handle_, POLLIN, static_cast<int>(poll_timeout_.count())) & POLLIN))
        return {};
      // read and handle errors
#if defined(_WIN32)
      n_read = ::recv(
        handle_,
        reinterpret_cast<char*>(buf_.get()),
        static_cast<int>(buf_size_),
        0
      );
      if (n_read == SOCKET_ERROR)
        return winsock_error("recv() failure");
#else
      if ((n_read = ::read(handle_, buf_.get(), buf_size_)) < 0)
        return errno_error("read() failure");
#endif  // !defined(_WIN32)
      // write to stream + clear buffer
      out.write(reinterpret_cast<const CharT*>(buf_.get()), n_read / sizeof(CharT));
      memset(buf_.get(), 0, buf_size_);
    }
    while (n_read);
    // no bytes left in buffer (n_read is 0), so done
    return {};
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
    write(ss).throw_on_error();
    return ss.str();
  }

private:
  socket_handle handle_;
  std::size_t buf_size_;
  std::unique_ptr<unsigned char[]> buf_;
  std::chrono::milliseconds poll_timeout_;
};

/**
 * Read from socket and write to stream.
 *
 * @tparam CharT Char type
 * @tparam Traits Char traits
 *
 * @param out Stream to write received data to
 * @param reader Socket reader
 */
template <typename CharT, typename Traits>
inline auto& operator<<(
  std::basic_ostream<CharT, Traits>& out, const socket_reader& reader)
{
  reader.write(out).throw_on_error();
  return out;
}

/**
 * Read from socket and return contents as string.
 *
 * @tparam CharT Char type
 * @tparam Traits Char traits
 *
 * @param handle Socket handle
 * @param buf_size Read buffer size, i.e. number of bytes per read
 * @param poll_timeout Timeout to use when polling socket for input
 */
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
inline auto read(
  socket_handle handle,
  std::size_t buf_size,
  std::chrono::milliseconds poll_timeout = socket_reader::poll_timeout_default)
{
  return socket_reader{handle, buf_size, poll_timeout}
    .operator std::basic_string<CharT, Traits>();
}

/**
 * Read from socket and return contents as string.
 *
 * Buffer read size is given by `socket_read_size`.
 *
 * @tparam CharT Char type
 * @tparam Traits Char traits
 *
 * @param handle Socket handle
 * @param poll_timeout Timeout to use when polling socket for input
 */
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
inline auto read(
  socket_handle handle,
  std::chrono::milliseconds poll_timeout = socket_reader::poll_timeout_default)
{
  return read<CharT, Traits>(handle, socket_read_size, poll_timeout);
}

/**
 * Socket writer class for abstracting writes to raw sockets.
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
   * @param close_write `true` to close socket write end after writing
   */
  socket_writer(socket_handle handle, bool close_write = false) noexcept
    : handle_{handle}, close_write_{close_write}
  {}

  /**
   * Write string view contents to socket.
   *
   * @tparam CharT Char type
   * @tparam Traits Char traits
   *
   * @param text String view to read input from
   * @returns Optional empty on success, with error message on failure
   */
  template <typename CharT, typename Traits>
  optional_error read(const std::basic_string_view<CharT, Traits>& text) const
  {
    // remaining bytes to send, bytes last written, total bytes sent
    auto n_remain = sizeof(CharT) * text.size();
    ssize_type n_last;
    std::size_t n_sent = 0;
#ifdef _WIN32
    // since buffer length is int, throw error if message too long
    // TODO: replace with std::numeric_limits<int>::max() once a satisfactory
    // solution for undefining the Windows.h min + max macros is found
    if (n_remain > INT_MAX)
      return "Message length " + std::to_string(n_remain) +
        " exceeds max allowed length " + std::to_string(INT_MAX);
#endif  // _WIN32
    // perform standard write loop
    while (n_remain) {
#if defined(_WIN32)
      n_last = ::send(handle_, text.data() + n_sent, static_cast<int>(n_remain), 0);
      if (n_last == SOCKET_ERROR)
        return winsock_error("send() failure");
#else
      if ((n_last = ::write(handle_, text.data() + n_sent, n_remain)) < 0)
        return errno_error("write() failure");
#endif  // !defined(_WIN32)
      // increment bytes written and decrement remaining
      n_sent += n_last;
      n_remain -= n_last;
    }
    // if requested, shut down write end of socket to signal end of transmission
    if (close_write_)
      pdnnet::shutdown(handle_, shutdown_type::write);
    return {};
  }

  /**
   * Write null-terminated string contents to socket.
   *
   * @tparam CharT Char type
   *
   * @param text String literal or other null-terminated string to read from
   * @returns Optional empty on success, with error message on failure
   */
  template <typename CharT>
  auto read(const CharT* text) const
  {
    return read(std::basic_string_view{text});
  }

  /**
   * Write a buffer of characters to socket.
   *
   * @tparam CharT Char type
   *
   * @param buf Pointer to buffer of characters
   * @param size Number of characters in the buffer
   * @returns Optional empty on success, with error message on failure
   */
  template <typename CharT>
  auto read(const CharT* buf, std::size_t size) const
  {
    // handle void buffers by treating them as const char buffers
    using char_type = std::conditional_t<std::is_same_v<CharT, void>, char, CharT>;
    return read(std::basic_string_view{static_cast<const char_type*>(buf), size});
  }

  /**
   * Write string stream contents to socket.
   *
   * @tparam CharT Char type
   * @tparam Traits Char traits
   *
   * @param in String stream to read input from
   * @returns Optional empty on success, with error message on failure
   */
  template <typename CharT, typename Traits>
  auto read(std::basic_stringstream<CharT, Traits>& in) const
  {
    return read(static_cast<std::basic_string_view<CharT, Traits>>(in.str()));
  }

  /**
   * Write input stream contents to socket.
   *
   * @note A trailing newline is written regardless of whether or not the
   *  contents of the input stream are terminated with a newline.
   *
   * @tparam CharT Char type
   * @tparam Trait Char traits
   *
   * @param in Input stream to read line by line from
   * @returns Optional empty on success, with error message on failure
   */
  template <typename CharT, typename Traits>
  auto read(std::basic_istream<CharT, Traits>& in) const
  {
    std::basic_string<CharT, Traits> line;
    std::basic_stringstream<CharT, Traits> ss;
    // read until EOF, preserving newlines
    while (std::getline(in, line) && !in.eof())
      ss << line << in.widen('\n');
    // write contents all at once to socket
    return read(ss);
  }

private:
  socket_handle handle_;
  bool close_write_;
};

/**
 * Write input stream contents to socket.
 *
 * @param CharT Char type
 * @tparam Traits Char traits
 *
 * @param in Input stream to write
 * @param writer Socket writer
 * @returns `in` to support additional streaming
 */
template <typename CharT, typename Traits>
inline auto& operator>>(
  std::basic_istream<CharT, Traits>& in, const socket_writer& writer)
{
  writer.read(in).throw_on_error();
  return in;
}

/**
 * Write input stream contents to socket.
 *
 * @param CharT Char type
 * @tparam Traits Char traits
 *
 * @param in String stream to write
 * @param writer Socket writer
 * @returns `in` to support additional streaming
 */
template <typename CharT, typename Traits>
inline auto& operator>>(
  std::basic_stringstream<CharT, Traits>& in, const socket_writer& writer)
{
  writer.read(in).throw_on_error();
  return in;
}

}  // namespace pdnnet

#endif  // PDNNET_SOCKET_HH_
