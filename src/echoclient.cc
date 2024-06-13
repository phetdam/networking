/**
 * @file echoclient.cc
 * @author Derek Huang
 * @brief Toy echo client
 * @copyright MIT License
 */

#include <cstdlib>
#include <iostream>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_HOST
#define PDNNET_ADD_CLIOPT_PORT
#define PDNNET_CLIOPT_PORT_DEFAULT 8888
#define PDNNET_ADD_CLIOPT_TIMEOUT
#define PDNNET_CLIOPT_TIMEOUT_DEFAULT 10'000  // 10s

#include "pdnnet/client.hh"
#include "pdnnet/cliopt.h"
#include "pdnnet/error.hh"
#include "pdnnet/features.h"
#include "pdnnet/socket.hh"

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple echoserver client that sends a message and expects a response.\n"
  "\n"
  "The message is read from stdin and the server response is printed to stdout."
)

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // create IPv4 TCP/IP client + attempt connection
  pdnnet::ipv4_client client{};
  client.connect(PDNNET_CLIOPT(host), PDNNET_CLIOPT(port)).exit_on_error();
  // read from stdin and write to socket
  std::cin >> pdnnet::client_writer{client};
  // block until server response is detected
  if (!pdnnet::wait_pollin(client.socket(), PDNNET_CLIOPT(timeout))) {
    std::cerr << "Error: Operation timed out after " <<
      PDNNET_CLIOPT(timeout) << " ms " << std::endl;
    return EXIT_FAILURE;
  }
  // read from socket until end of transmission and write to output stream,
  // include trailing newline. socket is fully closed on exit
  std::cout << pdnnet::client_reader{client} << std::endl;
  return EXIT_SUCCESS;
}
