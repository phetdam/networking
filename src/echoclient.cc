/**
 * @file echoclient.cc
 * @author Derek Huang
 * @brief Toy echo client
 * @copyright MIT License
 */

// on Windows, we use Windows Sockets 2
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif  // !defined(_WIN32)

#include <cerrno>
#include <cstdio>
#include <iostream>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_HOST
#define PDNNET_ADD_CLIOPT_PORT
#define PDNNET_CLIOPT_PORT_DEFAULT 8888
#define PDNNET_ADD_CLIOPT_MESSAGE_BYTES

#include "pdnnet/cliopt.h"
#include "pdnnet/error.h"
#include "pdnnet/error.hh"
#include "pdnnet/features.h"
#include "pdnnet/socket.hh"

/**
 * Platform-specific note on program execution.
 *
 * @note Preprocessor directives in macro expansions is undefined behavior.
 */
#if defined(_WIN32)
#define EXEC_NOTE \
  "\n\n" \
  "On Windows, extra blank lines are often written when printing the server's\n" \
  "response to stdout. No explanation for this behavior has been found yet."
#else
#define EXEC_NOTE
#endif  // !defined(_WIN32)

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple echoserver client that sends a message and expects a response.\n"
  "\n"
  "The message is read from stdin and the server response is printed to stdout."
  EXEC_NOTE
)

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // open IPv4 TCP/IP socket + resolve IPv4 host
  pdnnet::unique_socket socket{AF_INET, SOCK_STREAM};
  // TODO: write C++ wrapper for this
  auto serv_ent = gethostbyname(PDNNET_CLIOPT(host));
  if (!serv_ent)
#if defined(PDNNET_BSD_DEFAULT_SOURCE)
    PDNNET_H_ERRNO_EXIT_EX(h_errno, "No such host %s", PDNNET_CLIOPT(host));
#else
    PDNNET_ERROR_EXIT_EX("No such host %s", PDNNET_CLIOPT(host));
#endif  // !defined(PDNNET_BSD_DEFAULT_SOURCE)
  // create socket address struct
  auto serv_addr = pdnnet::socket_address(serv_ent, PDNNET_CLIOPT(port));
  // attempt connection
  if (!pdnnet::connect(socket, serv_addr))
#if defined(_WIN32)
    PDNNET_ERROR_EXIT(pdnnet::winsock_error("Could not connect to socket").c_str());
#else
    PDNNET_ERRNO_EXIT(errno, "Could not connect to socket");
#endif  // !defined(_WIN32)
  // read from stream and write to socket + signal end of transmission
  std::cin >> pdnnet::socket_writer{socket};
  pdnnet::shutdown(socket, pdnnet::shutdown_type::write);
  // read from socket and write to output stream, include trailing newline
  std::cout << pdnnet::socket_reader{socket} << std::endl;
  return EXIT_SUCCESS;
}
