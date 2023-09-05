/**
 * @file socket.h
 * @author Derek Huang
 * @brief C header for socket helpers
 * @copyright MIT License
 */

#ifndef PDNNET_SOCKET_H_
#define PDNNET_SOCKET_H_

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>

#include "pdnnet/dllexport.h"
#include "pdnnet/error.h"
#include "pdnnet/common.h"
#include "pdnnet/platform.h"
#include "pdnnet/sa.h"

#ifdef PDNNET_UNIX
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#endif  // PDNNET_UNIX

PDNNET_EXTERN_C_BEGIN

#ifdef PDNNET_UNIX
/**
 * Struct holding state information when performing a socket read.
 *
 * Below is a description of the members.
 *
 * `sockfd`
 *    Socket file descriptor being read from
 * `msg_buf`
 *    Buffer to whom each `read()` will write bytes to. Contains an extra null
 *    terminator so it can be treated as a string if expected to contain text.
 * `msg_buf_size`
 *    Buffer size, not including final null terminator
 * `n_reads`
 *    Number of successful `read()` calls made
 * `n_read_msg`
 *    Number of bytes last read, i.e. `read()` return value
 * `n_read_total`
 *    Total number of bytes read so far
 */
typedef struct {
  int sockfd;
  void *msg_buf;
  size_t msg_buf_size;
  size_t n_reads;
  ssize_t n_read_msg;
  size_t n_read_total;
} pdnnet_socket_read_state;

/**
 * Function pointer typedef for use with `pdnnet_socket_onlread[2]`.
 *
 * Describes the declaration of the functions passed to the `read_action` and
 * `post_action` parameters. These functions take the address of the
 * `pdnnet_socket_read_state` describing the read state. External state can be
 * passed through the `void *` second parameter, which depending on the use can
 * be `NULL`. If unused, then `NULL` should always be passed.
 *
 * On success, the function should return 0, otherwise the negation of a valid
 * `errno` value, e.g. -ENOMEM, -EINVAL, etc. If a function called within the
 * function errors and sets `errno`, then -errno` should be returned.
 */
typedef int (*pdnnet_socket_onlread_func)(
  PDNNET_SA(In) pdnnet_socket_read_state *, PDNNET_SA(Opt(In_Out)) void *);

/**
 * Declare a `pdnnet_socket_onlread_func`.
 *
 * Read state is accessed via `state`, external state through `data`.
 *
 * @param name Function name
 */
#define PDNNET_SOCKET_ONLREAD_FUNC(name) \
  int \
  name( \
    PDNNET_SA(In) pdnnet_socket_read_state *state, \
    PDNNET_SA(Opt(In_Out)) void *data)

/**
 * Read from a socket until end of transmission.
 *
 * @param sockfd Socket file descriptor to read from
 * @param read_action Function to invoke after each successful `read` call
 * @param read_action_param Parameter to pass to `read_action`
 * @returns 0 on success, -ENOMEM on buffer allocation failure, -errno on error
 */
PDNNET_PUBLIC
int
pdnnet_socket_onlread(
  int sockfd,
  PDNNET_SA(In) pdnnet_socket_onlread_func read_action,
  PDNNET_SA(Opt(In_Out)) void *read_action_param);

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
 * @returns 0 on success, -ENOMEM on buffer allocation failure, -errno on error
 */
PDNNET_PUBLIC
int
pdnnet_socket_onlread2(
  int sockfd,
  size_t read_size,
  PDNNET_SA(In) pdnnet_socket_onlread_func read_action,
  PDNNET_SA(Opt(In_Out)) void *read_action_param,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func post_action,
  PDNNET_SA(Opt(In_Out)) void *post_action_param);

/**
 * Read from a socket until end of transmission and write bytes to a stream.
 *
 * @param sockfd Socket file descriptor to read from
 * @param f File stream to write bytes read to
 * @returns 0 on success, -ENOMEM on buffer allocation failure, -EINVAL if `f`
 *  is `NULL`, -EIO if writing to `f` fails, -errno on error
 */
PDNNET_PUBLIC
int
pdnnet_socket_fwrite(int sockfd, PDNNET_SA(Out) FILE *f);
#endif  // PDNNET_UNIX

PDNNET_EXTERN_C_END

#endif  // PDNNET_SOCKET_H_
