/**
 * @file server.hh
 * @author Derek Huang
 * @brief C++ header for simple TCP/IP servers
 * @copyright MIT License
 */

#ifndef PDNNET_SERVER_HH_
#define PDNNET_SERVER_HH_

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#endif  // !defined(_WIN32)

#include <atomic>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>

#include "pdnnet/error.hh"
#include "pdnnet/features.h"
#include "pdnnet/socket.hh"

namespace pdnnet {

/**
 * Helper class holding parameters used when starting a socket-based server.
 *
 * Named parameter idiom can be used after default construction.
 */
class server_params {
public:
  /**
   * Default ctor.
   *
   * Sets port to zero to indicate that the next free port should be used and
   * uses hardware thread concurrency as the max length of the pending queue as
   * well as the max number of concurrent connections handled at once.
   */
  server_params() noexcept
    : server_params{std::thread::hardware_concurrency()}
  {}

  /**
   * Ctor.
   *
   * Sets port to zero to indicate that the next free port should be used. The
   * hardware thread concurrency is used to set the max concurrent connections.
   *
   * @param max_pending Maximum length of pending connections queue
   */
  server_params(unsigned int max_pending) noexcept
    : server_params{0U, max_pending, std::thread::hardware_concurrency()}
  {}

  /**
   * Ctor.
   *
   * @param port Port number in host byte order
   * @param max_pending Maximum length of pending connections queue
   * @param max_concurrency Maximum number of connections to handle concurrently
   */
  server_params(
    inet_port_type port,
    unsigned int max_pending,
    unsigned int max_concurrency) noexcept
    : port_{port}, max_pending_{max_pending}, max_concurrency_{max_concurrency}
  {}

  /**
   * Return port number.
   *
   * This may be zero to tell the server to use the next free port.
   */
  auto port() const noexcept { return port_; }

  /**
   * Set the port number.
   *
   * @param new_port New port number to use
   * @returns `*this` to allow method chaining
   */
  auto& port(inet_port_type new_port) noexcept
  {
    port_ = new_port;
    return *this;
  }

  /**
   * Return maximum length of pending connections queue.
   */
  auto max_pending() const noexcept { return max_pending_; }

  /**
   * Set the maximum length of the pending connections queue.
   *
   * @param new_max New max length of pending connections queue to use
   * @returns `*this` to allow method chaining
   */
  auto& max_pending(unsigned int new_max) noexcept
  {
    max_pending_ = new_max;
    return *this;
  }

  /**
   * Return maximum number of concurrent connections to handle.
   */
  auto max_concurrency() const noexcept { return max_concurrency_; }

  /**
   * Set maximum number of concurrent connections to handle.
   *
   * @param new_max New max number of concurrent connections to handle
   * @returns `*this` to allow method chaining
   */
  auto& max_concurrency(unsigned int new_max) noexcept
  {
    max_concurrency_ = new_max;
    return *this;
  }

private:
  inet_port_type port_;
  unsigned int max_pending_;
  unsigned int max_concurrency_;
};

/**
 * Generic IPv4 server interface.
 *
 * Uses a nonblocking full-duplex stream socket to listen for connections.
 */
class ipv4_server {
public:
  /**
   * Ctor.
   */
  ipv4_server() : running_{} {}

  /**
   * Virtual dtor.
   *
   * If the server is running in a background thread it is joined.
   */
  virtual ~ipv4_server()
  {
    try { join(); }
    catch (std::system_error&) {}
  }

  /**
   * Return const reference to the `unique_socket` owned by the server.
   *
   * Socket handle is invalid unless the server is running.
   */
  const auto& socket() const noexcept { return socket_; }

  /**
   * Return const reference to `sockaddr_in` socket address struct.
   *
   * Value returned is unspecified unless server is running.
   */
  const auto& address() const noexcept { return address_; }

  /**
   * Return maximum pending connection queue length.
   *
   * Value returned is unspecified unless server is running.
   */
  auto max_pending() const noexcept { return max_pending_; }

  /**
   * Return whether the server is currently running.
   *
   * This is set to `true` when the `start` member function is called and is
   * set to `false` when the `stop` member function is called.
   */
  bool running() const noexcept { return running_; }

  /**
   * Return the host address as an IPv4 decimal-dotted string.
   *
   * On platforms without `inet_ntoa`, this returns `"[unknown]"` instead.
   *
   * Value returned is unspecified unless server is running.
   */
  std::string dot_address() const
  {
#if defined(_WIN32) || defined(PDNNET_BSD_DEFAULT_SOURCE)
    return inet_ntoa(address_.sin_addr);
#else
    return "[unknown]";
#endif  // !defined(_WIN32) && !defined(PDNNET_BSD_DEFAULT_SOURCE)
  }

  /**
   * Return the port number in host byte order.
   *
   * Value returned is unspecified unless server is running.
   */
  auto port() const noexcept
  {
    return ntohs(address_.sin_port);
  }

  /**
   * Start a listening socket and accept incoming connections.
   *
   * Client connection handling is left to the implementation.
   *
   * @note This function is *not* thread safe.
   *
   * @param port Port number in host byte order, zero for the next free port
   * @param max_connect Maximum number of connections to accept
   * @param background `true` to run in a background thread, `false` to block
   * @returns `EXIT_SUCCESS` on success, `EXIT_FAILURE` on failure
   */
  int start(const server_params& params, bool background = false)
  {
    // if already started, throw
    if (running_)
      throw std::runtime_error{"Server is already running"};
    // if told to run in background, run using bg_thread_ and return
    if (background) {
      bg_thread_ = std::thread{&ipv4_server::start, this, params, false};
      return EXIT_SUCCESS;
    }
    // otherwise, prepare state for listening + mark as running
    set_state(params);
    // loop until told to stop running
    while (running_) {
      // poll socket for read events. if nothing to read, continue
      if (!wait_pollin(socket_))
        continue;
      // if there is data to read, accept the socket
      auto cli_socket = accept(socket_);
      // serve the connection as defined by user. stop on error
      if (!serve(cli_socket)) {
        reset_state();
        return EXIT_FAILURE;
      }
    }
    // destroy socket, stop running, return
    reset_state();
    return EXIT_SUCCESS;
  }

  /**
   * Return if the server is joinable.
   *
   * `true` if server is running in a background thread, `false` otherwise.
   */
  bool joinable() const noexcept
  {
    return bg_thread_.joinable();
  }

  /**
   * Force the server running in the background to block the current thread.
   *
   * No-op if server is not running in a background thread.
   */
  void join()
  {
    if (joinable())
      bg_thread_.join();
  }

  /**
   * Stop accepting any new incoming connections.
   *
   * If the server is in the middle of serving a client, it will first finish
   * serving the client before it stops running for good.
   *
   * This function can be called from multiple threads safely.
   */
  void stop() noexcept
  {
    running_ = false;
  }

protected:
  /**
   * Serve the client connection.
   *
   * This function should check the return value of `running()` in order to
   * respond in a timely fashion when `stop()` is called. If interrupting a
   * client's service when `running()` returns `false`, still return `true`.
   *
   * @param cli_socket Client socket
   * @returns `true` if client was served, `false` if error
   */
  virtual bool serve(unique_socket& cli_socket) = 0;

private:
  unique_socket socket_;
  sockaddr_in address_;
  unsigned int max_pending_;
  std::atomic<bool> running_;
  std::thread bg_thread_;

  /**
   * Create listening socket, resolve its port, and mark server as running.
   *
   * The socket is bound to the local address and port 0 is correctly resolved.
   *
   * @param params Server start params
   */
  void set_state(const server_params& params)
  {
    // create new socket + set to local address with specified port
    socket_ = unique_socket{AF_INET, SOCK_STREAM};
    address_ = make_sockaddr_in(INADDR_ANY, params.port());
    // set max amount of connections that can be pending in queue
    max_pending_ = params.max_pending();
    // attempt to bind socket
    if (!bind(socket_, address_))
      throw std::runtime_error{socket_error("Could not bind socket")};
    // get the actual socket address, e.g. if port is 0 it is resolved
    if (!getsockname(socket_, address_))
      throw std::runtime_error{socket_error("Could not retrieve socket address")};
    // attempt to start listening for connections
    if (!listen(socket_, max_pending_))
      throw std::runtime_error{socket_error("Could not listen on socket")};
    // mark as running
    running_ = true;
  }

  /**
   * Destroy listening socket and mark server as not running.
   */
  void reset_state() noexcept
  {
    socket_ = {};
    running_ = false;
  }
};

}  // namespace pdnnet

#endif  // PDNNET_SERVER_HH_
