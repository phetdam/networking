/**
 * @file socket.hh
 * @author Derek Huang
 * @brief C++ header for sockets helpers
 * @copyright MIT License
 */

#ifndef PDNNET_SOCKET_HH_
#define PDNNET_SOCKET_HH_

// on Windows, we use Windows Sockets 2
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <ws2ipdef.h>
// don't pollute translation units
#undef WIN32_LEAN_AND_MEAN
// for *nix systems, use standard socket API
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "pdnnet/error.hh"
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
inline auto socket_address(inet_addr_type address, inet_port_type port) noexcept
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
inline auto socket_address(const hostent* ent, inet_port_type port)
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
  return socket_address(ntohl(address), port);
}

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
  auto valid() const noexcept { return handle_ == bad_socket_handle; }

  /**
   * Syntactic sugar for compatibility with C socket functions.
   */
  operator socket_handle() const noexcept { return handle_; }

private:
  socket_handle handle_;
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
template <
  typename AddressType,
  typename = std::enable_if_t<is_addr_supported_v<AddressType>> >
inline bool bind(socket_handle handle, const AddressType& addr)
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
template <
  typename AddressType,
  typename = std::enable_if_t<is_addr_supported_v<AddressType>> >
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
inline bool listen(socket_handle handle, unsigned int max_pending)
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
   * Buffer read size is given by `socket_read_size`.
   *
   * @param handle Socket handle
   * @param close_read `true` to close socket read end after reading
   */
  socket_reader(socket_handle handle, bool close_read = false)
    : socket_reader{handle, socket_read_size, close_read}
  {}

  /**
   * Ctor.
   *
   * @param handle Socket handle
   * @param buf_size Read buffer size, i.e. number of bytes per chunk read
   * @param close_read `true` to close socket read end after reading
   */
  socket_reader(
    socket_handle handle, std::size_t buf_size, bool close_read = false)
    : handle_{handle},
      buf_size_{buf_size},
      buf_{std::make_unique<unsigned char[]>(buf_size_)},
      close_read_{close_read}
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
  auto& write(std::basic_ostream<CharT, Traits>& out) const
  {
    // number of bytes read
    ssize_type n_read;
    // until client signals end of transmission
    do {
      // read and handle errors
#if defined(_WIN32)
      n_read = ::recv(
        handle_,
        reinterpret_cast<char*>(buf_.get()),
        static_cast<int>(buf_size_),
        0
      );
      if (n_read == SOCKET_ERROR)
        throw std::runtime_error{winsock_error("recv() failure")};
#else
      if ((n_read = ::read(handle_, buf_.get(), buf_size_)) < 0)
        throw std::runtime_error{errno_error("read() failure")};
#endif  // !defined(_WIN32)
      // write to stream + clear buffer
      out.write(reinterpret_cast<const CharT*>(buf_.get()), n_read);
      memset(buf_.get(), 0, buf_size_);
    }
    while (n_read);
    // if requested, shut down read end of socket to signal end of transmission
    if (close_read_)
      pdnnet::shutdown(handle_, shutdown_type::read);
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
  bool close_read_;
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
inline auto& operator<<(
  std::basic_ostream<CharT, Traits>& out, const socket_reader& reader)
{
  reader.write(out);
  return out;
}

/**
 * Read from socket until end of transmission and return contents as string.
 *
 * @tparam CharT Char type
 * @tparam Traits Char traits
 *
 * @param handle Socket handle
 * @param buf_size Read buffer size, i.e. number of bytes per chunk read
 */
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
inline auto read(socket_handle handle, std::size_t buf_size)
{
  return socket_reader{handle, buf_size}
    .operator std::basic_string<CharT, Traits>();
}

/**
 * Read from socket until end of transmission and return contents as string.
 *
 * Buffer read size is given by `socket_read_size`.
 *
 * @tparam CharT Char type
 * @tparam Traits Char traits
 *
 * @param handle Socket handle
 */
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
inline auto read(socket_handle handle)
{
  return read<CharT, Traits>(handle, socket_read_size);
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
  socket_writer(socket_handle handle, bool close_write = false)
    : handle_{handle}, close_write_{close_write}
  {}

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
  auto& read(const std::basic_string_view<CharT, Traits>& text) const
  {
#if defined(_WIN32)
    // message size in bytes
    auto msg_len = sizeof(CharT) * text.size();
    // since buffer length is int, throw error if message too long
    // TODO: replace with std::numeric_limits<int>::max() once a satisfactory
    // solution for undefining the Windows.h min + max macros is found
    if (msg_len > INT_MAX)
      throw std::runtime_error{
        "message length " + std::to_string(msg_len) +
        " exceeds max allowed length " + std::to_string(INT_MAX)
      };
    // otherwise, just call send as usual
    if (::send(handle_, text.data(), static_cast<int>(msg_len), 0) == SOCKET_ERROR)
      throw std::runtime_error{winsock_error("send() failure")};
#else
    if (::write(handle_, text.data(), sizeof(CharT) * text.size()) < 0)
      throw std::runtime_error{errno_error("write() failure")};
#endif  // !defined(_WIN32)
    // if requested, shut down write end of socket to signal end of transmission
    if (close_write_)
      pdnnet::shutdown(handle_, shutdown_type::write);
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
  auto& read(std::basic_stringstream<CharT, Traits>& in) const
  {
    return read(static_cast<std::basic_string_view<CharT, Traits>>(in.str()));
  }

  /**
   * Write string view contents to socket.
   *
   * @tparam CharT Char type
   * @tparam Trait Char traits
   *
   * @param in Input stream to read line by line from
   * @returns `*this` to support method chaining
   */
  template <typename CharT, typename Traits>
  auto& read(std::basic_istream<CharT, Traits>& in) const
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
  writer.read(in);
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
  writer.read(in);
  return in;
}

}  // namespace pdnnet

#endif  // PDNNET_SOCKET_HH_
