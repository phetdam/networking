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
#define PDNNET_ADD_CLIOPT_MESSAGE_BYTES
#include "pdnnet/cliopt.h"
#include "pdnnet/error.h"
#include "pdnnet/socket.h"

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple ackserver client that sends a message and expects a response.\n"
  "\n"
  "The message is defined by the user and truncated at 255 characters."
)

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_ARGS();

  int sockfd;

  struct sockaddr_in serv_addr;
  struct hostent *server;

  char buffer[256];
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    PDNNET_ERRNO_EXIT(errno, "Failed to open socket");
  server = gethostbyname(PDNNET_CLIOPT(host));
  if (!server)
    PDNNET_ERRNO_EXIT_EX(errno, "No such host %s", PDNNET_CLIOPT(host));
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  serv_addr.sin_port = htons(PDNNET_CLIOPT(port));
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    PDNNET_ERRNO_EXIT(errno, "Could not connect to socket");
  printf("Please enter the message: ");
  memset(buffer, 0, sizeof buffer);
  // fgets reads sizeof buffer - 1 chars at max and adds a '\0'
  fgets(buffer, sizeof buffer, stdin);
  // don't write an extra newline if any
  size_t buf_size = strlen(buffer);
  if (buffer[buf_size - 1] == '\n')
    buffer[buf_size - 1] = '\0';
  if (write(sockfd, buffer, strlen(buffer)) < 0)
    PDNNET_ERRNO_EXIT(errno, "Socket write failed");
  // close write end to signal end of transmission
  if (shutdown(sockfd, SHUT_WR) < 0)
    PDNNET_ERRNO_EXIT(errno, "Shutdown with SHUT_WR failed");
  // read and print each received message chunk
  if (pdnnet_socket_fwrite(sockfd, stdout) < 0)
  {
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
