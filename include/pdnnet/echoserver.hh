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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "pdnnet/error.hh"
#include "pdnnet/features.h"
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
   * Binds the created socket to the local address with a random free port.
   */
  echoserver() : echoserver{0U} {}

  /**
   * Ctor.
   *
   * Maximum number of server threads used is set to the hardware concurrency.
   *
   * @param port Port number in host byte order
   */
  echoserver(inet_port_type port)
    : echoserver{
        port,
        static_cast<unsigned short>(std::thread::hardware_concurrency())
      }
  {}

  /**
   * Ctor.
   *
   * @param port Port number in host byte order
   * @param max_threads Maximum number of server threads
   */
  echoserver(inet_port_type port, unsigned short max_threads)
    : socket_{AF_INET, SOCK_STREAM}, address_{}, max_threads_{max_threads}
  {
    // set to local address with specified port
    address_ = socket_address(INADDR_ANY, port);
    // attempt to bind socket
    if (!bind(socket_, address_))
      throw std::runtime_error{socket_error("Could not bind socket")};
    // get the actual socket address, e.g. if port is 0 it is resolved
    if (!getsockname(socket_, address_))
      throw std::runtime_error{socket_error("Could not retrieve socket address")};
  }

  /**
   * Dtor.
   *
   * Ensures that all threads are joined.
   */
  ~echoserver()
  {
    for (auto& thread : thread_queue_) {
      try { thread.join(); }
      catch (const std::system_error&) {}
    }
  }

  /**
   * Return const reference to the `unique_socket` owned by the server.
   */
  const auto& socket() const noexcept { return socket_; }

  /**
   * Return max number of server threads that can exist at any time.
   */
  auto max_threads() const noexcept { return max_threads_; }

  /**
   * Return const reference to `sockaddr_in` socket address struct.
   */
  const auto& address() const noexcept { return address_; }

  /**
   * Return const reference to the thread deque.
   */
  const auto& thread_queue() const noexcept { return thread_queue_; }

  /**
   * Return the host address as an IPv4 decimal-dotted string.
   *
   * On platforms without `inet_ntoa`, this returns `"[unknown]"` instead.
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
   */
  auto port() const noexcept
  {
    return ntohs(address_.sin_port);
  }

  /**
   * Return current number of threads in the thread deque.
   */
  auto n_threads() const { return thread_queue_.size(); }

  /**
   * Start listening for up to `max_connect` connections.
   *
   * Each client connection is handled via a thread, with up to `max_threads`
   * threads running at any time. If the number of connections exceeds the
   * number of threads, one of the threads will join before another thread is
   * started to handle the incoming client connection.
   *
   * @param max_connect Maximum number of connections to accept
   * @returns `EXIT_SUCCESS` on success
   */
  int start(unsigned int max_connect = std::thread::hardware_concurrency())
  {
    // start listening for connections
    if (!listen(socket_, max_connect))
      throw std::runtime_error{socket_error("listen() failed")};
    // event loop
    // TODO: consider adding a boolean member to allow starting/stopping
    while (true) {
      // poll for events on socket, accepting client if there is data to read
      if (!(poll(socket_, POLLIN) & POLLIN))
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
            // read from socket and echo back to socket. each end of the socket
            // is shut down automatically after the full read + write
            std::stringstream stream;
            stream << socket_reader{socket, true};
            // TODO: if we want to add server print, we need synchronization
            stream >> socket_writer{socket, true};
          }
        }
      );
    }
    return EXIT_SUCCESS;
  }

private:
  unique_socket socket_;
  sockaddr_in address_;
  unsigned short max_threads_;
  std::deque<std::thread> thread_queue_;
};

}  // namespace pdnnet

#endif  // PDNNET_ECHOSERVER_HH_
