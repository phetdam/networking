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
#include <iostream>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_HOST
#define PDNNET_ADD_CLIOPT_PORT
#define PDNNET_CLIOPT_PORT_DEFAULT 8888

#include "pdnnet/client.hh"
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
  // create IPv4 TCP/IP client + attempt connection
  pdnnet::ipv4_client client{};
  auto error = client.connect(PDNNET_CLIOPT(host), PDNNET_CLIOPT(port));
  // if connection fails, print error and exit nonzero
  if (error)
    PDNNET_ERROR_EXIT(error->c_str());
  // read from stdin and write to socket + signal end of transmission
  std::cin >> pdnnet::client_writer{client, true};
  // print identifying header like original ackclient. we flush instead of
  // using std::endl since we don't want newline
  std::cout << PDNNET_PROGRAM_NAME << ": Received from " <<
    client.host_name() << ": " << std::flush;
  // read from socket and write to output stream, include trailing newline. no
  // need to signal end of transmission since we close socket on exit
  std::cout << pdnnet::client_reader{client} << std::endl;
  return EXIT_SUCCESS;
}
