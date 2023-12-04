/**
 * @file ackserver.c
 * @author Derek Huang
 * @brief Toy server that prints client messages and sends acknowledgment
 * @copyright MIT License
 *
 * This is based off of a heavily modified version of a toy server program by
 * Robert Ingalls which can be downloaded from the following link:
 *
 * https://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/server2.c
 */

// include first for platform detection macros
#include "pdnnet/platform.h"

#ifndef PDNNET_UNIX
#error "ackserver.c cannot be compiled for non-Unix platforms"
#endif  // PDNNET_UNIX

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_VERBOSE
#define PDNNET_ADD_CLIOPT_PORT
#define PDNNET_CLIOPT_PORT_DEFAULT 8888
#define PDNNET_ADD_CLIOPT_MESSAGE_BYTES
#define PDNNET_ADD_CLIOPT_MAX_CONNECT

#include "pdnnet/cliopt.h"
#include "pdnnet/common.h"
#include "pdnnet/error.h"
#include "pdnnet/features.h"
#include "pdnnet/inet.h"
#include "pdnnet/socket.h"

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple server that sends an acknowledgment to every connected client.\n"
  "\n"
  "Reads an arbitrary amount of bytes from a client connected via IPv4 and\n"
  "sends an acknowledgment, forking to handle each client separately. The\n"
  "client is expected to signal end of transmission after writing, e.g. with\n"
  "shutdown(sockfd, SHUT_WR), to inform the server it is done writing."
)

/**
 * Used by the `pdnnet_socket_onlread` call to print the client message chunks.
 *
 * @note This function uses `PDNNET_PROGRAM_NAME` and so should not be called
 *  on its own without calling `PDNNET_CLIOPT_PARSE_OPTIONS` first in `main`.
 */
static
PDNNET_SOCKET_ONLREAD_FUNC(print_client_msg)
{
  // for first chunk, print header with client address if possible
  if (state->n_reads == 1)
#ifdef PDNNET_BSD_DEFAULT_SOURCE
    printf(
      "%s: Received from %s: ",
      PDNNET_PROGRAM_NAME,
      inet_ntoa(((const struct sockaddr_in *) data)->sin_addr)
    );
#else
    printf("%s: Received from [unknown]: ", PDNNET_PROGRAM_NAME);
#endif  // PDNNET_BSD_DEFAULT_SOURCE
  // print actual message content
  printf("%s", (const char *) state->msg_buf);
  return 0;
}

/**
 * Read the client's message and send an acknowledgement to the client.
 *
 * The client is expected to be well-behaved and shutdown the pipe on its end
 * with `SHUT_WR` to signal end of transmission. After sending the
 * acknowledgment, the forked server process will shutdown with `SHUT_RDWR`.
 *
 * @param cli_sock Client socket file descriptor
 * @param cli_addr Client socket address
 * @returns 0 on success, -EINVAL if argument invalid, -errno on error
 */
static int
handle_client(int cli_sock, const struct sockaddr_in *cli_addr)
{
  if (!cli_sock || cli_sock < 0 || !cli_addr)
    return -EINVAL;
  // static buffer for acknowledgment message
  static const char ack_buf[] = "Acknowledged message received";
  // action to perform while reading client messages. if verbose, will just
  // print, otherwise nothing will be done
  pdnnet_socket_onlread_func read_action;
  read_action = (PDNNET_CLIOPT(verbose)) ? print_client_msg : NULL;
  // read client message and print each received chunk if running verbosely
  if (
    pdnnet_socket_onlread_s(
      cli_sock,
      PDNNET_CLIOPT(message_bytes),
      read_action,
      (void *) cli_addr  // not modified by print_client_msg
    ) < 0
  ) {
    PDNNET_ERRNO_RETURN(shutdown(cli_sock, SHUT_RDWR));
    return -errno;
  }
  // trailing newline to finish off
  if (PDNNET_CLIOPT(verbose))
    puts("");
  // send acknowledgment + shutdown completely to end transmission
  PDNNET_ERRNO_RETURN(write(cli_sock, ack_buf, sizeof ack_buf - 1));
  PDNNET_ERRNO_RETURN(shutdown(cli_sock, SHUT_RDWR));
  return 0;
}

/**
 * Main event loop.
 *
 * After accepting a client connection, a fork is performed to handle the
 * client with `handle_client`, with the child reaped later.
 *
 * @param sockfd Server socket file descriptor
 * @returns `EXIT_SUCCESS` from child, unreachable from parent
 */
static int
event_loop(int sockfd)
{
  // client socket address and size of address
  struct sockaddr_in cli_addr;
  socklen_t cli_len;
  // client socket fd, return status
  int cli_sockfd, status;
  // process PID
  pid_t pid;
  // accept client connections and handle them by forking
  while (true) {
    cli_len = sizeof cli_addr;
    cli_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
    if (cli_sockfd < 0)
      PDNNET_ERRNO_EXIT(errno, "Could not accept client socket");
    if (cli_len > sizeof cli_addr)
      PDNNET_ERRNO_EXIT(errno, "Client address buffer truncated");
    // fork to handle incoming client request
    if ((pid = fork()) < 0)
      PDNNET_ERRNO_EXIT(errno, "Fork failed");
    // child: close duplicated server socket file descriptor + do stuff
    if (!pid) {
      if (close(sockfd) < 0)
        PDNNET_ERRNO_EXIT(errno, "Failed to close server socket fd");
      if ((status = handle_client(cli_sockfd, &cli_addr)))
        PDNNET_ERRNO_EXIT(-status, "handle_client() error");
      // don't forget to close child socket too
      close(cli_sockfd);
      return EXIT_SUCCESS;
    }
    // parent: close socket but also reap the child
    else {
      close(cli_sockfd);
      // note: ignoring the status of the child
      if (wait(NULL) < 0)
        PDNNET_ERRNO_EXIT(errno, "wait() failed");
    }
  }
  // unreachable
  return EXIT_SUCCESS;
}

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // run in background as a daemon automatically
#if defined(PDNNET_BSD_DEFAULT_SOURCE)
  if (daemon(true, true) < 0)
    PDNNET_ERRNO_EXIT(errno, "daemon() failed");
#else
  // fallback using fork() call
  switch (fork()) {
    case -1: PDNNET_ERRNO_EXIT(errno, "fork() failed");
    case  0: break;
    // parent exits immediately to orphan child
    default: _exit(EXIT_SUCCESS);
  }
#endif  // !defined(PDNNET_BSD_DEFAULT_SOURCE)
  // create and bind socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    PDNNET_ERRNO_EXIT(errno, "Could not open socket");
  // zero and fill in server socket address struct (IPv4)
  struct sockaddr_in serv_addr;
  pdnnet_set_sockaddr_in(&serv_addr, INADDR_ANY, PDNNET_CLIOPT(port));
  // bind socket + listen for connections
  if (bind(sockfd, (const struct sockaddr *) &serv_addr, sizeof serv_addr) < 0)
    PDNNET_ERRNO_EXIT(errno, "Could not bind socket");
  if (listen(sockfd, (int) PDNNET_CLIOPT(max_connect)) < 0)
    PDNNET_ERRNO_EXIT(errno, "listen() failed");
  // run event loop
  return event_loop(sockfd);
}
