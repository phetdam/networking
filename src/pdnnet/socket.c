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

#include "pdnnet/error.h"
#include "pdnnet/platform.h"
#include "pdnnet/sa.h"

#ifdef PDNNET_UNIX
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // PDNNET_UNIX

int
pdnnet_socket_onlread(
  pdnnet_socket_handle sockfd,
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
  pdnnet_socket_handle sockfd,
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
  pdnnet_socket_handle sockfd,
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
  } while (read_state.n_read_msg);
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

int
pdnnet_socket_fwrite(pdnnet_socket_handle sockfd, PDNNET_SA(Out) FILE *f)
{
  return pdnnet_socket_fwrite_s(sockfd, PDNNET_SOCKET_ONLREAD_SIZE, (void *) f);
}

int
pdnnet_socket_fwrite_s(
  pdnnet_socket_handle sockfd, size_t read_size, PDNNET_SA(Out) FILE *f)
{
  return pdnnet_socket_onlread_s(
    sockfd,
    read_size,
    pdnnet_socket_onlread_fwrite,
    (void *) f
  );
}
