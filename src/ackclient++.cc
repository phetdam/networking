/**
 * @file ackclient++.cc
 * @author Derek Huang
 * @brief C++ toy client for messaging the C++ toy acknowledgement serer
 * @copyright MIT License
 */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif  // !defined(_WIN32)

#include <cstdlib>
#include <cstring>
#include <iostream>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_HOST
#define PDNNET_ADD_CLIOPT_PORT
#define PDNNET_CLIOPT_PORT_DEFAULT 8888

#include "pdnnet/cliopt.h"
#include "pdnnet/error.h"
#include "pdnnet/error.hh"
#include "pdnnet/features.h"
#include "pdnnet/socket.hh"

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple ackserver++ client that sends a message and expects a response.\n"
  "\n"
  "An improved C++ version of the original ackclient program."
)

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // open IPv4 TCP/IP socket
  pdnnet::unique_socket socket{AF_INET, SOCK_STREAM};
  // attempt to resolve host name to server address
  auto serv_ent = gethostbyname(PDNNET_CLIOPT(host));
  // nullptr on error. _DEFAULT_SOURCE or _BSD_SOURCE required for h_errno
  if (!serv_ent)
#if defined(_WIN32)
    PDNNET_ERROR_EXIT(
      pdnnet::winsock_error(
        "Error resolving " + std::string{PDNNET_CLIOPT(host)}
      ).c_str()
    );
#elif defined(PDNNET_BSD_DEFAULT_SOURCE)
    PDNNET_H_ERRNO_EXIT_EX(h_errno, "Error resolving %s", PDNNET_CLIOPT(host));
#else
    PDNNET_ERROR_EXIT_EX("Error resolving %s", PDNNET_CLIOPT(host));
#endif  // !defined(_WIN32) && !defined(PDNNET_BSD_DEFAULT_SOURCE)
  // create socket address struct + attempt connection
  auto serv_addr = pdnnet::socket_address(serv_ent, PDNNET_CLIOPT(port));
  if (!pdnnet::connect(socket, serv_addr))
#if defined(_WIN32)
    PDNNET_ERROR_EXIT(pdnnet::winsock_error("Could not connect to socket").c_str());
#else
    PDNNET_ERRNO_EXIT(errno, "Could not connect to socket");
#endif  // !defined(_WIN32)
  // read from stdin and write to socket + signal end of transmission
  std::cin >> pdnnet::socket_writer{socket};
  pdnnet::shutdown(socket, pdnnet::shutdown_type::write);
  // print identifying header like original ackclient
  std::cout << PDNNET_PROGRAM_NAME << ": Received from " <<
#if defined(_WIN32) || defined(PDNNET_BSD_DEFAULT_SOURCE)
    inet_ntoa(serv_addr.sin_addr) <<
#else
    "[unknown]" <<
#endif  // !defined(_WIN32) && !defined(PDNNET_BSD_DEFAULT_SOURCE)
  // we flush instead of using std::endl since we don't want newline
    ": " << std::flush;
  // read from socket and write to output stream, include trailing newline
  std::cout << pdnnet::socket_reader{socket} << std::endl;
  return EXIT_SUCCESS;
}
