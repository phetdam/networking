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

#include "pdnnet/common.h"
#include "pdnnet/error.h"
#include "pdnnet/features.h"
#include "pdnnet/inet.h"
#include "pdnnet/socket.h"

// program name
#ifndef PROGRAM_NAME
#define PROGRAM_NAME "ackserver"
#endif  // PROGRAM_NAME

// maximum read chunk size in bytes
#ifndef MAX_READ_SIZE
#define MAX_READ_SIZE BUFSIZ
#endif  // MAX_READ_SIZE

// default values for command-line arguments
#define PORT_DEFAULT 8888
#define MAX_CONNECT_DEFAULT 5
#define READ_SIZE_DEFAULT 256

// globals for command-line arguments
bool print_usage_value = false;
uint16_t port_value = PORT_DEFAULT;
unsigned int max_connect_value = MAX_CONNECT_DEFAULT;
size_t read_size_value = READ_SIZE_DEFAULT;

/**
 * Enum for command-line option parsing mode.
 */
typedef enum {
  cliopt_parse_default,
  cliopt_parse_port,
  cliopt_parse_max_connect,
  cliopt_parse_read_size
} cliopt_parse_mode;

/**
 * Print program usage.
 */
static void
print_usage()
{
  printf(
    "Usage: %s [-h] [-p PORT] [-m MAX_CONNECT] [-r READ_SIZE]\n"
    "\n"
    "Simple server that sends an acknowledgement to every connected client.\n"
    "\n"
    "Reads an arbitrary amount of bytes from a client connected via IPv4 and\n"
    "sends an acknowledgement, forking to handle each client separately. The\n"
    "client is expected to signal end of tranmission after writing, e.g. with\n"
    "shutdown(sockfd, SHUT_WR), to inform " PROGRAM_NAME " it is done writing.\n"
    "\n"
    "Options:\n"
    "\n"
    "  -p, --port PORT     Port number to bind to, default "
      PDNNET_STRINGIFY(PORT_DEFAULT) "\n"
    "  -m, --max-connect MAX_CONNECT\n"
    "                      Max number of connections to accept, default "
      PDNNET_STRINGIFY(MAX_CONNECT_DEFAULT) "\n"
    "  -r, --read-size READ_SIZE\n"
    "                      Number of bytes requested per read() to a client,\n"
    "                      default " PDNNET_STRINGIFY(READ_SIZE_DEFAULT)
      " bytes, maximum " PDNNET_STRINGIFY(MAX_READ_SIZE) " bytes\n",
    PROGRAM_NAME
  );
}

/**
 * Parse port value.
 */
static bool
parse_port_value(const char *arg)
{
  int value = atoi(arg);
  // on error, zero
  if (!value) {
    fprintf(stderr, "Error: Unable to convert %s to a port number\n", arg);
    return false;
  }
  // can't be greater than UINT16_MAX
  if (value > UINT16_MAX) {
    fprintf(
      stderr,
      "Error: Port number %d exceeds max port number %d\n",
      value,
      UINT16_MAX
    );
    return false;
  }
  // update port_value + return
  port_value = (uint16_t) value;
  return true;
}

/**
 * Parse max accepted connections value.
 */
static bool
parse_max_connect_value(const char *arg)
{
  int value = atoi(arg);
  // zero on error
  if (!value) {
    fprintf(stderr, "Error: Can't convert %s to number of max connects\n", arg);
    return false;
  }
  // must be positive
  if (value < 1) {
    fprintf(stderr, "Error: Max connection value must be positive\n");
    return false;
  }
  // must be within integer range
  if (value > INT_MAX) {
    fprintf(
      stderr,
      "Error: Max connect value %u exceeds allowed max %d\n",
      value,
      INT_MAX
    );
    return false;
  }
  // update max_connect_value + return
  max_connect_value = (unsigned int) value;
  return true;
}

/**
 * Parse number of bytes to `read()` from client sockets.
 */
static bool
parse_read_size(const char *arg)
{
  long long value = atoll(arg);
  // zero on error
  if (!value) {
    fprintf(stderr, "Error: Unable to convert %s to read size\n", arg);
    return false;
  }
  // must be positive
  if (value < 1) {
    fprintf(stderr, "Error: Read size value must be positive\n");
    return false;
  }
  // must not exceed MAX_READ_SIZE
  if (value > MAX_READ_SIZE) {
    fprintf(
      stderr,
      "Error: Read value %lld exceeds allowed max %zu\n",
      value,
      (size_t) MAX_READ_SIZE
    );
    return false;
  }
  // update read_size_value + return
  read_size_value = (size_t) value;
  return true;
}

/**
 * Parse command line arguments.
 *
 * @param argc Arg count
 * @param argv Arg vector
 */
static bool
parse_args(int argc, char **argv)
{
  cliopt_parse_mode mode = cliopt_parse_default;
  for (int i = 1; i < argc; i++) {
    // help flag
    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      print_usage_value = true;
      return true;
    }
    // port option
    else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port"))
      mode = cliopt_parse_port;
    // max accepted connections
    else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--max-connect"))
      mode = cliopt_parse_max_connect;
    // read size
    else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--read-size"))
      mode = cliopt_parse_read_size;
    // parse based on parse mode
    else {
      switch (mode) {
        // parse port value
        case cliopt_parse_port:
          if (!parse_port_value(argv[i]))
            return false;
          break;
        // parse max connect value
        case cliopt_parse_max_connect:
          if (!parse_max_connect_value(argv[i]))
            return false;
          break;
        // parse read size
        case cliopt_parse_read_size:
          if (!parse_read_size(argv[i]))
            return false;
          break;
        // don't know this option
        case cliopt_parse_default:
          fprintf(stderr, "Error: Unknown option %s\n", argv[i]);
          return false;
        // unreachable
        default:
          fprintf(stderr, "Error: Unreachable branch reached\n");
          return false;
      }
    }
  }
  return true;
}

/**
 * Used by the `pdnnet_socket_onlread` call to print the client message chunks.
 */
static
PDNNET_SOCKET_ONLREAD_FUNC(print_client_msg)
{
  // for first chunk, print header with client address if possible
  if (state->n_reads == 1)
#ifdef PDNNET_BSD_DEFAULT_SOURCE
    printf(
      "%s: Received from %s: ",
      PROGRAM_NAME,
      inet_ntoa(((const struct sockaddr_in *) data)->sin_addr)
    );
#else
    printf("%s: Received from [unknown]: ", PROGRAM_NAME);
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
  // static buffers for acknowledgment message
  // static char msg_buf[MAX_READ_SIZE + 1];
  static const char ack_buf[] = "Acknowledged message received";
  // read client message and print each received chunk
  if (
    pdnnet_socket_onlread_s(
      cli_sock,
      read_size_value,
      print_client_msg,
      (void *) cli_addr  // not modified by print_client_msg
    ) < 0
  ) {
    PDNNET_ERRNO_RETURN(shutdown(cli_sock, SHUT_RDWR));
    return -errno;
  }
  // trailing newline to finish off
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

int
main(int argc, char **argv)
{
  // parse command-line args
  if (!parse_args(argc, argv))
    return EXIT_FAILURE;
  // print usage if specified
  if (print_usage_value) {
    print_usage();
    return EXIT_SUCCESS;
  }
  // run in background as a daemon automatically
#ifdef PDNNET_BSD_DEFAULT_SOURCE
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
#endif  // PDNNET_BSD_DEFAULT_SOURCE
  // create and bind socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    PDNNET_ERRNO_EXIT(errno, "Could not open socket");
  // zero and fill in server socket address struct (IPv4)
  struct sockaddr_in serv_addr;
  pdnnet_set_sockaddr_in(&serv_addr, INADDR_ANY, port_value);
  // bind socket + listen for connections
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof serv_addr) < 0)
    PDNNET_ERRNO_EXIT(errno, "Could not bind socket");
  if (listen(sockfd, (int) max_connect_value) < 0)
    PDNNET_ERRNO_EXIT(errno, "listen() failed");
  // run event loop
  return event_loop(sockfd);
}
