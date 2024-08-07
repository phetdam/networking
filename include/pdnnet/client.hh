/**
 * @file client.hh
 * @author Derek Huang
 * @brief C++ header for a simple TCP/IP client
 * @copyright MIT License
 */

#ifndef PDNNET_CLIENT_HH_
#define PDNNET_CLIENT_HH_

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cerrno>
#endif  // !defined(_WIN32)

#include <chrono>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>

#include "pdnnet/error.hh"
#include "pdnnet/features.h"
#include "pdnnet/socket.hh"

namespace pdnnet {

/**
 * Simple IPv4 client managing a socket connection.
 */
class ipv4_client {
public:
  /**
   * Ctor.
   *
   * Uses `SOCK_STREAM` as the socket type with the default protocol of `0`.
   */
  ipv4_client() : ipv4_client{SOCK_STREAM} {}

  /**
   * Ctor.
   *
   * Uses the default protocol of `0`, the same as `IPPROTO_TCP`.
   *
   * @param type Socket tpye, e.g. `SOCK_STREAM`, `SOCK_RAW`
   */
  ipv4_client(int type) : ipv4_client{type, 0} {}

  /**
   * Ctor.
   *
   * @param type Socket type, e.g. `SOCK_STREAM`, `SOCK_RAW`
   * @param protocol Socket protocol, e.g. `0`, `IPPROTO_TCP`
   */
  ipv4_client(int type, int protocol)
    : socket_{AF_INET, type, protocol},
      type_{type},
      protocol_{protocol},
      connected_{},
      host_addr_{}
  {}

  /**
   * Return const reference to the managed `unique_socket`.
   */
  const auto& socket() const noexcept { return socket_; }

  /**
   * Return integer socket type, e.g. `SOCK_STREAM`.
   */
  auto type() const noexcept { return type_; }

  /**
   * Return integer socket protocol, e.g. `IPPROTO_TCP`.
   */
  auto protocol() const noexcept { return protocol_; }

  /**
   * Return `true` if socket is connected to a host, `false` otherwise.
   */
  auto connected() const noexcept { return connected_; }

  /**
   * Return the `sockaddr_in` holding host address information.
   */
  const auto& host_addr() const noexcept { return host_addr_; }

  /**
   * Return the host name as a string.
   *
   * On platforms where `inet_ntoa` is not available, "[unknown]" is returned.
   */
  std::string host_name() const
  {
#if defined(_WIN32) || defined(PDNNET_BSD_DEFAULT_SOURCE)
    return inet_ntoa(host_addr_.sin_addr);
#else
    return "[unknown]";
#endif  // !defined(_WIN32) && !defined(PDNNET_BSD_DEFAULT_SOURCE)
  }

  /**
   * Return the host port value in the local byte order.
   */
  inet_port_type host_port() const noexcept
  {
    return ntohs(host_addr_.sin_port);
  }

  /**
   * Connect to the specified TCP/IP endpoint.
   *
   * @param host IPv4 host name or address
   * @param port Port number in local byte order
   * @returns Error wrapper empty on success, with error message on failure
   */
  optional_error connect(const std::string& host, inet_port_type port)
  {
    // get list of IPv4 address structs (catch possible exception)
    unique_addrinfo addrs;
    try {
      addrs = getaddrinfo(host, port);
    }
    catch (const std::runtime_error& exc) {
      return exc.what();
    }
    // create address + attempt connection
    auto serv_addr = addrs.addr_in();
    if (!pdnnet::connect(socket_, serv_addr))
      return "Socket connect error: " + socket_error();
    // update host address, mark as connected, and return
    host_addr_ = serv_addr;
    connected_ = true;
    return {};
  }

private:
  unique_socket socket_;
  int type_;
  int protocol_;
  bool connected_;
  sockaddr_in host_addr_;
};

/**
 * Client writer class for abstracting raw socket writes.
 *
 * Can be used as a drop-in for `socket_writer` when using clients.
 */
class client_writer : public socket_writer {
public:
  /**
   * Ctor.
   *
   * @param client Client instance
   * @param close_write `true` to close socket write end after writing
   */
  client_writer(const ipv4_client& client, bool close_write = false)
    : socket_writer{client.socket(), close_write}
  {}
};

/**
 * Client reader class for abstracting raw socket reads.
 *
 * Can be used as a drop-in for `socket_reader` when using clients.
 */
class client_reader : public socket_reader {
public:
  /**
   * Ctor.
   *
   * @param client Client instance
   * @param poll_timeout Timeout to use when polling socket for input
   */
  client_reader(
    const ipv4_client& client,
    std::chrono::milliseconds poll_timeout = poll_timeout_default)
    : socket_reader{client.socket(), poll_timeout}
  {}
};

}  // namespace pdnnet

#endif  // PDNNET_CLIENT_HH_
