/**
 * @file echoserver.hh
 * @author Derek Huang
 * @brief C++ toy echo server
 * @copyright MIT License
 */

#ifndef PDNNET_ECHOSERVER_HH_
#define PDNNET_ECHOSERVER_HH_

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
#include <sys/socket.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <atomic>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <thread>

#include "pdnnet/error.hh"
#include "pdnnet/features.h"
#include "pdnnet/server.hh"
#include "pdnnet/socket.hh"

namespace pdnnet {

/**
 * Simple echo server.
 *
 * Uses a specified number of threads to handle incoming connections. A new
 * thread is created for each incoming client connection, but when the max
 * number of threads is reached, the oldest created thread is forced to join
 * before a new thread can be created for the incoming connection.
 */
class echoserver {
public:
  /**
   * Default ctor.
   *
   * Marks the server as not running.
   */
  echoserver() : running_{} {}

  /**
   * Dtor.
   *
   * Ensures that all threads are joined.
   */
  ~echoserver()
  {
    join_threads();
  }

  /**
   * Return const reference to the unique socket owned by the server.
   */
  const auto& socket() const noexcept { return socket_; }

  /**
   * Return const reference to the socket address struct.
   *
   * @note Value is unspecified unless server is running.
   */
  const auto& address() const noexcept { return address_; }

  /**
   * Return whether the server is running or not.
   *
   * @note This function is thread-safe.
   */
  bool running() const noexcept { return running_; }

  /**
   * Return max number of server threads that can exist at any time.
   *
   * @note Value is unspecified unless server is running.
   */
  auto max_threads() const noexcept { return max_threads_; }

  /**
   * Return max number of pending connections at a time.
   *
   * @note Value is unspecified unless server is running.
   */
  auto max_pending() const noexcept { return max_pending_; }

  /**
   * Return the host address as an IPv4 decimal-dotted string.
   *
   * On platforms without `inet_ntoa`, this returns `"[unknown]"` instead.
   *
   * @note Value is unspecified unless server is running.
   */
  std::string_view dot_address() const
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
   * @note Value is unspecified unless server is running.
   */
  inet_port_type port() const noexcept
  {
    return ntohs(address_.sin_port);
  }

  /**
   * Return current number of threads in the thread deque.
   *
   * @todo Consider making this function thread-safe or removing it.
   *
   * @par
   *
   * @note This function is not thread-safe.
   */
  auto n_threads() const noexcept
  {
    return thread_queue_.size();
  }

  /**
   * Start the server with the given parameters.
   *
   * Each client connection is handled via a thread, with up to `max_threads()`
   * threads running at any time. If the number of connections exceeds the
   * number of threads, one of the threads will join before another thread is
   * started to handle the incoming client connection.
   *
   * @note This function is thread-safe.
   *
   * @param params Server parameters to use when starting
   * @returns `EXIT_SUCCESS` on success, `EXIT_FAILURE` if already running
   */
  int start(const server_params& params = {})
  {
    // cannot start while running
    if (running_)
      return EXIT_FAILURE;
    // initialize state + mark as running
    set_state(params);
    // event loop
    while (running_) {
      // poll for events on socket, accepting client if there is data to read
      if (!wait_pollin(socket_))
        continue;
      // accept client connection, possibly erroring
      auto cli_socket = accept(socket_);
      // check if thread queue reached capacity. if so, join + remove first
      // thread. socket descriptor is managed partially since join() can throw
      if (thread_queue_.size() == max_threads_) {
        thread_queue_.front().join();
        thread_queue_.pop_front();
      }
      // unneeded after previous block so release before passing to thread. if
      // we release in the thread, cli_socket may be destructed before the
      // actual release is done in the thread, so the descriptor will be bad
      auto cli_sockfd = cli_socket.release();
      // emplace new running thread to manage client socket and connection. we
      // need to copy the socket handle instead of referencing to prevent
      // undefined behavior when the lambda is actually executed out of scope
      thread_queue_.emplace_back(
        std::thread{
          [cli_sockfd]
          {
            // own handle to automatically close later
            unique_socket socket{cli_sockfd};
            // read from socket until there is no more to read + echo back
            std::stringstream stream;
            stream << socket_reader{socket};
            // TODO: if we want to add server print, we need synchronization
            stream >> socket_writer{socket};
          }
        }
      );
    }
    // reset state and finish
    reset_state();
    return EXIT_SUCCESS;
  }

  /**
   * Signal that the server should stop.
   *
   * If in the middle of handling a client connection, the client connection
   * will be completed before the server exits its event loop and resets state.
   */
  void stop() noexcept
  {
    running_ = false;
  }

private:
  unique_socket socket_;
  sockaddr_in address_;
  std::atomic_bool running_;
  std::deque<std::thread> thread_queue_;
  unsigned int max_threads_;
  unsigned int max_pending_;

  /**
   * Set the server state using the given parameters.
   *
   * Creates a new listening socket, binds the socket to the address, resolves
   * the address to the actual port, places the socket in listening mode for
   * new IPv4 connections, and then marks the server as running.
   *
   * @param params Server parameters to set state with
   */
  void set_state(const server_params& params)
  {
    // create new socket + set to local address with specified port
    socket_ = unique_socket{AF_INET, SOCK_STREAM};
    address_ = make_sockaddr_in(INADDR_ANY, params.port());
    // set max amount of threads and pending connections
    max_threads_ = params.max_concurrency();
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
   * Join all running threads in the thread queue and ignore exceptions.
   */
  void join_threads() noexcept
  {
    // join all running threads + ignore any exceptions
    for (auto& thread : thread_queue_) {
      try { thread.join(); }
      catch (const std::system_error&) {}
    }
  }

  /**
   * Reset the server state.
   *
   * This joins all running threads and resets the socket, the address struct,
   * sets the server to not running, and initializes an empty thread queue.
   */
  void reset_state()
  {
    // TODO: forgot why this comment is here. we should either consider setting
    // running_ to false first before calling join_threads() but it doesn't
    // really matter in how reset_state() is being used (running_ already false)
    // first mark as not running so other threads know not to check state
    join_threads();
    socket_ = {};
    address_ = {};
    running_ = false;
    // use decltype to disambiguate from initializer list operator=
    thread_queue_ = decltype(thread_queue_){};
  }
};

}  // namespace pdnnet

#endif  // PDNNET_ECHOSERVER_HH_
