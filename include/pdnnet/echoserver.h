/**
 * @file echoserver.h
 * @author Derek Huang
 * @brief C++ toy echo server
 * @copyright MIT License
 */

#ifndef PDNNET_ECHOSERVER_H_
#define PDNNET_ECHOSERVER_H_

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "pdnnet/platform.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include "pdnnet/error.h"
#include "pdnnet/socket.h++"

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
   * @param port Port number in host byte order
   */
  echoserver(std::uint16_t port)
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
  echoserver(std::uint16_t port, unsigned short max_threads)
    : socket_{AF_INET, SOCK_STREAM}, address_{}, max_threads_{max_threads}
  {
    // set to local address with specified port
    address_ = socket_address(INADDR_ANY, port);
    // attempt to bind socket
#if defined(_WIN32)
    if (bind(socket_, (const sockaddr*) &address_, sizeof address_) == SOCKET_ERROR)
      throw std::runtime_error{winsock_error("Could not bind socket")};
#else
    if (bind(socket_, (const sockaddr*) &address_, sizeof address_) < 0)
      throw std::runtime_error{errno_error("Could not bind socket")};
#endif  // !defined(_WIN32)
    // get the actual socket address, e.g. if port is 0 it is resolved
    socklen_t addr_size = sizeof address_;
#if defined(_WIN32)
    if (getsockname(socket_, (sockaddr*) &address_, &addr_size) == SOCKET_ERROR)
      throw std::runtime_error{winsock_error("Could not retrieve socket address")};
#else
    if (getsockname(socket_, (sockaddr*) &address_, &addr_size) < 0)
      throw std::runtime_error{errno_error("Could not retrieve socket address")};
#endif  // !defined(_WIN32)
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
  const auto&
  socket() const noexcept { return socket_; }

  /**
   * Return max number of server threads that can exist at any time.
   */
  auto
  max_threads() const noexcept { return max_threads_; }

  /**
   * Return reference to `sockaddr_in` socket address struct.
   */
  auto&
  address() noexcept { return address_; }

  /**
   * Return const reference to `sockaddr_in` socket address struct.
   */
  const auto&
  address() const noexcept { return address_; }

  /**
   * Return const reference to the thread deque.
   */
  const auto&
  thread_queue() const noexcept { return thread_queue_; }

  /**
   * Return current number of threads in the thread deque.
   */
  auto
  n_threads() const { return thread_queue_.size(); }

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
  int
  start(unsigned int max_connect = std::thread::hardware_concurrency())
  {
    // start listening for connections
#if defined(_WIN32)
    if (listen(socket_, static_cast<int>(max_connect)) == SOCKET_ERROR)
      throw std::runtime_error{winsock_error("listen() failed")};
#else
    if (listen(socket_, static_cast<int>(max_connect)) < 0)
      throw std::runtime_error{errno_error("listen() failed")};
#endif  // !defined(_WIN32)
    // client socket address, address size, and socket file descriptor
    // TODO: we don't use the client socket address + size now, maybe remove
    sockaddr_in cli_addr;
    socklen_t cli_len;  // WS2tcpip.h has int typedef'd to socklen_t
    socket_handle cli_sockfd;
    // event loop
    while (true) {
      // accept client connection
      cli_len = sizeof cli_addr;
      cli_sockfd = accept(socket_, (sockaddr*) &cli_addr, &cli_len);
#if defined(_WIN32)
      // note that INVALID_SOCKET is > 0; we cannot treat it like we could
      // treat SOCKET_ERROR, which is defined as -1
      if (cli_sockfd == INVALID_SOCKET)
        throw std::runtime_error{winsock_error("accept() failed")};
#else
      if (cli_sockfd < 0)
        throw std::runtime_error{errno_error("accept() failed")};
      // if buffer is too small, address is truncated, which is still an error.
      // the Windows Sockets version of accept checks this and WSAGetLastError
      // will return WSAEFAULT if sizeof cli_addr is too small.
      if (cli_len > sizeof cli_addr)
        throw std::runtime_error{"Client address buffer truncated"};
#endif  // !defined(_WIN32)
      // success, so create unique_socket to manage the descriptor
      // check if queue reached capacity. if so, join + remove first thread.
      // note we manage the socket file descriptor since join() can throw
      unique_socket cli_socket{cli_sockfd};
      if (thread_queue_.size() == max_threads_) {
        thread_queue_.front().join();
        thread_queue_.pop_front();
      }
      // unneeded after previous block so release
      cli_socket.release();
      // emplace new running thread to manage client socket and connection
      thread_queue_.emplace_back(
        std::thread{
          [&cli_sockfd]
          {
            // own handle to automatically close later
            unique_socket socket{cli_sockfd};
            // read from socket and echo back to socket
            std::stringstream stream;
            stream << socket_reader{socket};
            // TODO: if we want to add server print, we need synchronization
            stream >> socket_writer{socket};
            // TODO: allow socket_reader + socket_writer to take shutdown_type
            shutdown(socket);
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

#endif  // PDNNET_ECHOSERVER_H_
