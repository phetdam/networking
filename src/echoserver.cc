/**
 * @file echoserver.cc
 * @author Derek Huang
 * @brief Toy echo server
 * @copyright MIT License
 */

// include first for platform detection macros
#include "pdnnet/platform.h"

#ifndef PDNNET_UNIX
#error "echoserver.cc cannot be compiled for non-Unix platforms"
#endif  // PDNNET_UNIX

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <iostream>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_PORT
#define PDNNET_ADD_CLIOPT_MAX_CONNECT
#include "pdnnet/cliopt.h"
#include "pdnnet/echoserver.h++"
#include "pdnnet/error.h"
#include "pdnnet/features.h"
#include "pdnnet/process.h++"

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple server that received data back to the sending client.\n"
  "\n"
  "Reads an arbitrary amount of bytes from a client connected via IPv4 and\n"
  "sends the same data back. Client is expected to signal end of tranmission\n"
  "after writing with a call to pdnnet::shutdown."
)

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // run in background as daemon
  pdnnet::daemonize();
  // create server + print address and port for debugging
  pdnnet::echoserver server{PDNNET_CLIOPT(port)};
  std::cout << PDNNET_PROGRAM_NAME << ": max_threads=" <<
    server.max_threads() << ", address=" <<
#if defined(PDNNET_BSD_DEFAULT_SOURCE)
    inet_ntoa(server.address().sin_addr) <<
#else
    "[unknown]: " <<
#endif  // !defined(PDNNET_BSD_DEFAULT_SOURCE)
    ":" << ntohs(server.address().sin_port) << std::endl;
  // start server
  return server.start(PDNNET_CLIOPT(max_connect));
}
