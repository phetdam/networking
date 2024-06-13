/**
 * @file echoserver.cc
 * @author Derek Huang
 * @brief Toy echo server
 * @copyright MIT License
 */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <cstdlib>
#include <iostream>
#include <thread>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_PORT
#define PDNNET_ADD_CLIOPT_MAX_CONNECT
#include "pdnnet/cliopt.h"
#include "pdnnet/echoserver.hh"
#include "pdnnet/process.hh"
#include "pdnnet/server.hh"

/**
 * Platform-specific note on program execution.
 *
 * @note Preprocessor directives in macro expansions is undefined behavior.
 */
#if defined(_WIN32)
#define EXEC_NOTE \
  "This program will run in the current shell as there is no fork() on Windows."
#else
#define EXEC_NOTE \
  "This program will run as a system daemon automatically."
#endif  // !defined(_WIN32)

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple server that echoes received data back to the sending client.\n"
  "\n"
  "Reads an arbitrary amount of bytes from a client connected via IPv4 and\n"
  "sends the same data back. Client is expected to signal end of tranmission\n"
  "after writing with a call to shutdown() or pdnnet::shutdown().\n"
  "\n"
  EXEC_NOTE
)

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // run in background as daemon on *nix
#if defined(_WIN32)
  // don't use FreeConsole otherwise you cannot interact with the console.
  // TODO: still looking for an easy way to run this as a daemon automatically
  // if (!FreeConsole())
  //   throw std::runtime_error{pdnnet::hresult_error("Failed to detach from console")};
#else
  pdnnet::daemonize();
#endif  // !defined(_WIN32)
  // create new server
  pdnnet::echoserver server;
  // start server in new thread with given parameters. we need to do this so we
  // can print the state of the running server from the current thread
  std::thread server_thread{
    [&server]
    {
      auto params = pdnnet::server_params{}
          .port(PDNNET_CLIOPT(port))
          .max_pending(PDNNET_CLIOPT(max_connect));
      server.start(params);
    }
  };
  // wait until the server is running to prevent race condition
  while (!server.running());
  // on WSL Ubuntu 22.04.2 LTS we have to flush stdout otherwise the terminal
  // does not properly print on a new line + then display the new prompt
#ifndef _WIN32
  std::cout << std::flush;
#endif  // _WIN32
  // print address and port for debugging
  std::cout << PDNNET_PROGRAM_NAME << ": max_threads=" <<
    server.max_threads() << ", address=" << server.dot_address() << ":" <<
    server.port() << std::endl;
  // join + return (unreachable)
  server_thread.join();
  return EXIT_SUCCESS;
}
