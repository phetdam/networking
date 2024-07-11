/**
 * @file ackclient.c
 * @author Derek Huang
 * @brief Toy client for sending message to the toy acknowledgment server
 * @copyright MIT License
 *
 * This is based off of a heavily modified version of a toy client program by
 * Robert Ingalls which can be downloaded from the following link:
 *
 * https://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/client.c
 */

// include first for platform detection macros
#include "pdnnet/platform.h"

#ifndef PDNNET_UNIX
#error "ackclient.c cannot be compiled for non-Unix platforms"
#endif  // PDNNET_UNIX

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_HOST
#define PDNNET_ADD_CLIOPT_PORT
#define PDNNET_CLIOPT_PORT_DEFAULT 8888
#define PDNNET_ADD_CLIOPT_MESSAGE_BYTES

#include "pdnnet/cliopt.h"
#include "pdnnet/error.h"
#include "pdnnet/features.h"
#include "pdnnet/socket.h"

#define MESSAGE_BUFFER_SIZE 256

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple ackserver client that sends a message and expects a response.\n"
  "\n"
  "The message is read from stdin " PDNNET_STRINGIFY(MESSAGE_BUFFER_SIZE - 1)
    " characters at a time."
)

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // open IPv4 TCP/IP socket
  pdnnet_socket sockfd = PDNNET_TCP_SOCKET(AF_INET);
  if (!PDNNET_SOCKET_VALID(sockfd))
    PDNNET_ERRNO_EXIT(errno, "Failed to open socket");
  // convert numeric port number to string for use with getaddrinfo. since the
  // port number cannot exceed 65536, we need 6 chars (includes terminator)
  char port_buf[6];
  snprintf(port_buf, sizeof port_buf, "%u", PDNNET_CLIOPT(port));
  // address hint struct + head of address struct linked list
  struct addrinfo hints = {0, AF_INET, SOCK_STREAM, IPPROTO_TCP};
  struct addrinfo *addrs;
  // resolve host name and port to get server address list
  int status;
  if ((status = getaddrinfo(PDNNET_CLIOPT(host), port_buf, &hints, &addrs)))
    PDNNET_ERROR_EXIT_EX(
      "Could not resolve host %s with port %s: %s",
      PDNNET_CLIOPT(host),
      // use string port buffer since this is passed to getaddrinfo
      port_buf,
      gai_strerror(status)
    );
  // copy first address as sockaddr_in + free address list
  struct sockaddr_in serv_addr;
  memcpy(&serv_addr, addrs->ai_addr, sizeof *addrs->ai_addr);
  freeaddrinfo(addrs);
  // attempt connection
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    PDNNET_ERRNO_EXIT(errno, "Could not connect to socket");
  // read MESSAGE_BUFFER_SIZE - 1 chars
  char buffer[MESSAGE_BUFFER_SIZE];
  while (!feof(stdin)) {
    memset(buffer, 0, sizeof buffer);
    fread(buffer, 1U, sizeof buffer - 1, stdin);
    // check for error if any
    if (ferror(stdin))
      PDNNET_ERRNO_EXIT(EIO, "Error reading from stdin");
    if (write(sockfd, buffer, strlen(buffer)) < 0)
      PDNNET_ERRNO_EXIT(errno, "Socket write failed");
  }
  // close write end to signal end of transmission
  if (shutdown(sockfd, SHUT_WR) < 0)
    PDNNET_ERRNO_EXIT(errno, "Shutdown with SHUT_WR failed");
  // read and print each received message chunk
#if defined(PDNNET_BSD_DEFAULT_SOURCE)
  printf(
    "%s: Received from %s: ",
    PDNNET_PROGRAM_NAME,
    inet_ntoa(serv_addr.sin_addr)
  );
#else
  printf("%s: Received from [unknown]: ", PDNNET_PROGRAM_NAME);
#endif  // !defined(PDNNET_BSD_DEFAULT_SOURCE)
  if (pdnnet_socket_fwrite_s(sockfd, PDNNET_CLIOPT(message_bytes), stdout) < 0) {
    if (shutdown(sockfd, SHUT_RDWR) < 0)
      PDNNET_ERRNO_EXIT(errno, "Shutdown with SHUT_RDWR after read failed");
    PDNNET_ERRNO_EXIT(errno, "Read failed");
  }
  // trailing newline to finish off
  puts("");
  // done with the socket, so just close
  if (close(sockfd) < 0)
    PDNNET_ERRNO_EXIT(errno, "Failed to close socket");
  return EXIT_SUCCESS;
}
