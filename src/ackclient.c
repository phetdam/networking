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
  // attempt to resolve host name to server address
  struct hostent *serv_ent = gethostbyname(PDNNET_CLIOPT(host));
  if (!serv_ent)
#if defined(PDNNET_BSD_DEFAULT_SOURCE)
    PDNNET_H_ERRNO_EXIT_EX(h_errno, "No such host %s", PDNNET_CLIOPT(host));
#else
    PDNNET_ERROR_EXIT_EX("No such host %s", PDNNET_CLIOPT(host));
#endif  // !defined(PDNNET_BSD_DEFAULT_SOURCE)
  // populate socket address struct
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy(
    &serv_addr.sin_addr.s_addr,
// for glibc h_addr requires that _DEFAULT_SOURCE is defined, which is defined
// by default unless __STRICT_ANSI__, _ISOC99_SOURCE, etc. are defined. see
// feature_test_macros(7) man page for feature macro definition details. note
// that in VS Code, none of these macros are defined, so this conditional block
// is useful for avoiding the red squiggle that is otherwise caused.
#if defined(h_addr)
    serv_ent->h_addr,
#else
    serv_ent->h_addr_list[0],
#endif  // !defined(h_addr)
    serv_ent->h_length
  );
  serv_addr.sin_port = htons(PDNNET_CLIOPT(port));
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
