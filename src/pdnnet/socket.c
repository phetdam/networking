/**
 * @file socket.c
 * @author Derek Huang
 * @brief C source for socket helpers
 * @copyright MIT License
 */

#include "pdnnet/socket.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdnnet/cerror.h"
#include "pdnnet/platform.h"
#include "pdnnet/sa.h"

#ifdef PDNNET_UNIX
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Max number of bytes `read` at once by an "online" socket read.
 *
 * Functions like `pdnnet_socket_onlread` and `pdnnet_socket_fwrite` use this
 * value to determine how large their internal message buffer should be.
 *
 * This can be redefined at compile time if necessary.
 */
#ifndef PDNNET_SOCKET_ONLREAD_SIZE
#define PDNNET_SOCKET_ONLREAD_SIZE 512
#endif  // PDNNET_SOCKET_ONLREAD_SIZE

/**
 * Read from a socket until end of transmission.
 *
 * @param sockfd Socket file descriptor to read from
 * @param read_action Function to invoke after each successful `read` call
 * @param read_action_param Parameter to pass to `read_action`
 * @returns 0 on success, -ENOMEM on buffer allocation failure, -errno on error
 */
int
pdnnet_socket_onlread(
  int sockfd,
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

/**
 * Read from a socket until end of transmission.
 *
 * Allows manual specification of max bytes each `read` call should request.
 *
 * @param sockfd Socket file descriptor to read from
 * @param read_size Number of bytes each `read` call should request
 * @param read_action Function to invoke after each successful `read` call
 * @param read_action_param Parameter to pass to `read_action`
 * @returns 0 on success, -EINVAL if `read_size` is zero, -ENOMEM on buffer
 *  allocation failure, -errno for other errors
 */
int
pdnnet_socket_onlread_s(
  int sockfd,
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

/**
 * Read from a socket until end of transmission.
 *
 * This function provides more control over the reads and allows calling the
 * `read_action_param` function after all the reads have been completed.
 *
 * @param sockfd Socket file descriptor to read from
 * @param read_size Number of bytes each `read` call should request
 * @param read_action Function to invoke after each successful `read` call
 * @param read_action_param Parameter to pass to `read_action`
 * @param post_action Function to invoke after the final `read_action`
 * @param post_action_param Parameter to pass to `post_action`
 * @returns 0 on success, -EINVAL if `read_size` is zero, -ENOMEM on buffer
 *  allocation failure, -errno for other errors
 */
int
pdnnet_socket_onlread2(
  int sockfd,
  size_t read_size,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func read_action,
  PDNNET_SA(Opt(In_Out)) void *read_action_param,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func post_action,
  PDNNET_SA(Opt(In_Out)) void *post_action_param)
{
  if (!read_size)
    return -EINVAL;
  // return status
  int status;
  // socket read state struct
  pdnnet_socket_read_state read_state;
  read_state.sockfd = sockfd;
  read_state.msg_buf_size = read_size;
  // allocated buffer has an extra byte so it can be treated like a string
  read_state.msg_buf = malloc(read_size + 1);
  if (!read_state.msg_buf)
    return -ENOMEM;
  read_state.n_reads = read_state.n_read_total = 0;
  // until client signals end of transmission
  do {
    // clear and read client message
    memset(read_state.msg_buf, 0, read_size + 1);
    if ((read_state.n_read_msg = read(sockfd, read_state.msg_buf, read_size)) < 0)
      return -errno;
    // update number of reads and total bytes read
    read_state.n_reads += 1;
    read_state.n_read_total += read_state.n_read_msg;
    // call user-specified action if any
    if (read_action && (status = read_action(&read_state, read_action_param)) < 0)
      goto end;
  } while(read_state.n_read_msg);
  // if there is a post action, call that
  if (post_action && (status = post_action(&read_state, post_action_param)) < 0)
    goto end;
end:
  free(read_state.msg_buf);
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

/**
 * Read from a socket until end of transmission and write bytes to a stream.
 *
 * @param sockfd Socket file descriptor to read from
 * @param f File stream to write bytes read to
 * @returns 0 on success, -ENOMEM on buffer allocation failure, -EINVAL if `f`
 *  is `NULL`, -EIO if writing to `f` fails, -errno on error
 */
int
pdnnet_socket_fwrite(int sockfd, PDNNET_SA(Out) FILE *f)
{
  return pdnnet_socket_fwrite_s(sockfd, PDNNET_SOCKET_ONLREAD_SIZE, (void *) f);
}

/**
 * Read from a socket until end of transmission and write bytes to a stream.
 *
 * @param sockfd Socket file descriptor to read from
 * @param read_size Number of bytes each `read` call should request
 * @param f File stream to write bytes read to
 * @returns 0 on success, -ENOMEM on buffer allocation failure, -EINVAL if `f`
 *  is `NULL`, -EIO if writing to `f` fails, -errno on error
 */
int
pdnnet_socket_fwrite_s(int sockfd, size_t read_size, PDNNET_SA(Out) FILE *f)
{
  return pdnnet_socket_onlread_s(
    sockfd,
    read_size,
    pdnnet_socket_onlread_fwrite,
    (void *) f
  );
}
#endif  // PDNNET_UNIX
