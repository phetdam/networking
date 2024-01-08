/**
 * @file ackserver++.cc
 * @author Derek Huang
 * @brief C++ toy acknowledgment server
 * @copyright MIT License
 */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif  // !defined(_WIN32)

#include <cerrno>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_VERBOSE
#define PDNNET_ADD_CLIOPT_PORT
#define PDNNET_CLIOPT_PORT_DEFAULT 8888
#define PDNNET_ADD_CLIOPT_MESSAGE_BYTES
#define PDNNET_ADD_CLIOPT_MAX_CONNECT

#include "pdnnet/cliopt.h"
#include "pdnnet/error.h"
#include "pdnnet/error.hh"
#include "pdnnet/features.h"
#include "pdnnet/process.hh"
#include "pdnnet/socket.hh"

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple server that sends an acknowledgment to every connected client.\n"
  "\n"
  "Reads an arbitrary amount of bytes from a client connected via IPv4 and\n"
  "sends an acknowledgment, forking to handle each client separately. The\n"
  "client is expected to signal end of transmission after writing, e.g. with\n"
  "shutdown(sockfd, SHUT_WR), to inform the server it is done writing.\n"
  "\n"
  "This is a C++ version of the original C program that uses threads instead\n"
  "of forking in order to be more portable, i.e. so it can run on Windows."
)

namespace {

/**
 * Read the client's message and send an acknowledgment to the client.
 *
 * Client is expected to be well-behaved and shut down its write end of the
 * pipe to signal end of transmission. The server will close its read end after
 * reading and close its write end after writing to the client.
 *
 * If the server was specified to run verbosely, the text received from the
 * client will be printed to standard output.
 *
 * @param sockfd Client socket file descriptor
 * @returns 0 on success, -EINVAL if argument invalid, -errno on error
 */
void handle_client(pdnnet::socket_handle sockfd)
{
  // own the handle to automatically close later
  pdnnet::unique_socket socket{sockfd};
  // get the client address via getsockname, throw on error
  sockaddr_in cli_addr;
  if (!pdnnet::getsockname(socket, cli_addr))
    throw std::runtime_error{
      pdnnet::socket_error("Could not get client socket address")
    };
  // read text from socket into string
  auto read_text = pdnnet::read(socket);
  // if verbose, print the stream contents to stdout with header
  if (PDNNET_CLIOPT(verbose)) {
    std::cout << PDNNET_PROGRAM_NAME << ": Received from " <<
#if defined(_WIN32) || defined(PDNNET_BSD_DEFAULT_SOURCE)
      inet_ntoa(cli_addr.sin_addr) <<
#else
      "[unknown]" <<
#endif  // !defined(_WIN32) && !defined(PDNNET_BSD_DEFAULT_SOURCE)
      ": " << read_text << std::flush;
  }
  // write our acknowledgment message
  // TODO: a write() counterpart to read() would be less verbose
  pdnnet::socket_writer{socket}.read("Acknowledged message received");
}

/**
 * Main event loop.
 *
 * After accepting a client connection, a thread is created to handle the
 * client, joining any older threads first if necessary. Clients are served in
 * a FIFO manner, with earlier threads joined once the max thread count is hit.
 *
 * @param sockfd Server socket file descriptor
 * @param thread_queue Deque of threads for managing client connections
 * @param max_threads Maximum number of threads that can be in the queue
 * @returns `EXIT_SUCCESS`
 */
int event_loop(
  pdnnet::socket_handle sockfd,
  std::deque<std::thread>& thread_queue,
  unsigned int max_threads = std::thread::hardware_concurrency())
{
  while (true) {
    // perform blocking accept for next client connection
    auto cli_socket = pdnnet::accept(sockfd);
    // if thread queue is at capacity, force the first thread to join
    if (thread_queue.size() == max_threads) {
      thread_queue.front().join();
      thread_queue.pop_front();
    }
    // emplace new running thread to manage client socket and connection. we
    // need to release the socket handle so it is not closed on scope exit
    thread_queue.emplace_back(std::thread{handle_client, cli_socket.release()});
  }
  return EXIT_SUCCESS;
}

}  // namespace

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // on *nix systems we can run the program as a daemon immediately
#ifdef PDNNET_UNIX
  pdnnet::daemonize();
#endif  // PDNNET_UNIX
  // create owned socket handle and address
  pdnnet::unique_socket socket{AF_INET, SOCK_STREAM};
  auto addr = pdnnet::make_sockaddr_in(INADDR_ANY, PDNNET_CLIOPT(port));
  // bind socket to address
  PDNNET_ERROR_EXIT_IF(
    !pdnnet::bind(socket, addr),
    pdnnet::socket_error("Could not bind socket").c_str()
  );
  // listen for connections
  PDNNET_ERROR_EXIT_IF(
    !pdnnet::listen(socket, PDNNET_CLIOPT(max_connect)),
    pdnnet::socket_error("Listening failed").c_str()
  );
  // create thread queue for managing client connections. we use a very simple
  // FIFO thread scheduling scheme to manage connections instead of a pool
  std::deque<std::thread> thread_queue;
  return event_loop(socket, thread_queue);
}
