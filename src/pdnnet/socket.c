/**
 * @file socket.c
 * @author Derek Huang
 * @brief C source for socket helpers
 * @copyright MIT License
 */

#include "pdnnet/socket.h"

#if defined(_WIN32)
// must define WIN32_LEAN_AND_MEAN to use Windows Sockets 2
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winerror.h>
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <errno.h>
#include <limits.h>  // note: only INT_MAX used when _WIN32 defined
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdnnet/error.h"
#include "pdnnet/sa.h"

pdnnet_socket
pdnnet_socket_create(int domain, int type, int protocol)
{
  // both Windows and POSIX have the same argument types
  return socket(domain, type, protocol);
}

int
pdnnet_socket_destroy(pdnnet_socket socket)
{
#if defined(_WIN32)
  return (closesocket(socket) == SOCKET_ERROR) ? -WSAGetLastError() : 0;
#else
  return (close(socket) < 0) ? -errno : 0;
#endif  // !defined(_WIN32)
}

/*
const char *
pdnnet_strerror(int err)
{
#if defined(_WIN32)
  // note: Windows Sockets error codes start from 10000
  // note: should use TLS APIs for Windows OSes prior to Vista. see
  // https://learn.microsoft.com/en-us/cpp/parallel/thread-local-storage-tls
  __declspec(thread) static msg[512];
#else
  // otherwise, just call strerror
  return strerror(err);
#endif  // !defined(_WIN32)
}
*/

int
pdnnet_socket_onlread(
  pdnnet_socket sockfd,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func read_action,
  PDNNET_SA(Opt(In_Out)) void *read_action_param)
{
  return pdnnet_socket_onlread_s(
    sockfd,
    PDNNET_SOCKET_ONLREAD_SIZE,
    read_action,
    read_action_param
  );
}

int
pdnnet_socket_onlread_s(
  pdnnet_socket sockfd,
  size_t read_size,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func read_action,
  PDNNET_SA(Opt(In_Out)) void *read_action_param)
{
  return pdnnet_socket_onlread2(
    sockfd,
    read_size,
    read_action,
    read_action_param,
    NULL,
    NULL
  );
}

int
pdnnet_socket_onlread2(
  pdnnet_socket sockfd,
  size_t read_size,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func read_action,
  PDNNET_SA(Opt(In_Out)) void *read_action_param,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func post_action,
  PDNNET_SA(Opt(In_Out)) void *post_action_param)
{
  if (!read_size)
    return -EINVAL;
// on Windows, the max read size must not exceed INT_MAX
#ifdef _WIN32
  if (read_size > INT_MAX)
    return -EINVAL;
#endif  // _WIN32
  // return status
  int status = 0;
  // socket read state struct
  pdnnet_socket_read_state rs;
  rs.sockfd = sockfd;
  rs.msg_buf_size = read_size;
  // allocated buffer has an extra byte so it can be treated like a string
  rs.msg_buf = malloc(read_size + 1);
  if (!rs.msg_buf)
    return -ENOMEM;
  rs.n_reads = rs.n_read_total = 0;
  // until client signals end of transmission
  do {
    // clear and read client message
    memset(rs.msg_buf, 0, read_size + 1);
#if defined(_WIN32)
    // note: recv returns int but gets promoted to SSIZE_T (LONG_PTR)
    // TODO: should we try to map the error codes to errno values?
    if ((rs.n_read_msg = recv(sockfd, rs.msg_buf, read_size, 0)) == SOCKET_ERROR)
      return -WSAGetLastError();
#else
    if ((rs.n_read_msg = read(sockfd, rs.msg_buf, read_size)) < 0)
      return -errno;
#endif // !defined(_WIN32)
    // update number of reads and total bytes read
    rs.n_reads += 1;
    rs.n_read_total += rs.n_read_msg;
    // call user-specified action if any
    if (read_action && (status = read_action(&rs, read_action_param)) < 0)
      goto end;
  } while (rs.n_read_msg);
  // if there is a post action, call that
  if (post_action && (status = post_action(&rs, post_action_param)) < 0)
    goto end;
end:
  free(rs.msg_buf);
  return status;
}

/**
 * Post-`read` action function for `pdnnet_socket_fwrite`.
 *
 * @returns 0 on success, -EINVAL if `data` is `NULL`, -EIO if `fwrite` fails
 */
static
PDNNET_SOCKET_ONLREAD_FUNC(pdnnet_socket_onlread_fwrite)
{
  if (!data)
    return -EINVAL;
  FILE *f = data;
  fwrite(state->msg_buf, 1, state->n_read_msg, f);
  if (ferror(f))
    return -EIO;
  return 0;
}

int
pdnnet_socket_fwrite(pdnnet_socket sockfd, PDNNET_SA(Out) FILE *f)
{
  return pdnnet_socket_fwrite_s(sockfd, PDNNET_SOCKET_ONLREAD_SIZE, (void *) f);
}

int
pdnnet_socket_fwrite_s(
  pdnnet_socket sockfd, size_t read_size, PDNNET_SA(Out) FILE *f)
{
  return pdnnet_socket_onlread_s(
    sockfd,
    read_size,
    pdnnet_socket_onlread_fwrite,
    (void *) f
  );
}
