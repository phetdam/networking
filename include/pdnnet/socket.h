/**
 * @file socket.h
 * @author Derek Huang
 * @brief C header for socket helpers
 * @copyright MIT License
 */

#ifndef PDNNET_SOCKET_H_
#define PDNNET_SOCKET_H_

#if defined(_WIN32)
// must define WIN32_LEAN_AND_MEAN to use Windows Sockets 2
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#endif  // !defined(_WIN32)

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>

#include "pdnnet/common.h"
#include "pdnnet/dllexport.h"
#include "pdnnet/error.h"
#include "pdnnet/features.h"
#include "pdnnet/sa.h"

PDNNET_EXTERN_C_BEGIN

/**
 * Socket handle type.
 */
#if defined(_WIN32)
typedef SOCKET pdnnet_socket;
#else
typedef int pdnnet_socket;
#endif  // !defined(_WIN32)

/**
 * Signed size type.
 */
#if defined(_WIN32)
// TODO: Windows socket functions still use int, maybe typedef to int
typedef SSIZE_T pdnnet_ssize_t;
// ssize_t is POSIX, defined in sys/types.h
#elif defined(PDNNET_HAS_SYS_TYPES)
typedef ssize_t pdnnet_ssize_t;
// fallback for non-POSIX, non-Windows systems
#else
typedef int pdnnet_ssize_t;
#endif  // !defined(_WIN32)

/**
 * Invalid socket handle.
 */
#if defined(_WIN32)
#define PDNNET_INVALID_SOCKET INVALID_SOCKET
#else
#define PDNNET_INVALID_SOCKET -1
#endif  // !defined(_WIN32)

/**
 * Check if a socket descriptor is valid or not.
 *
 * @param socket `pdnnet_socket` socket descriptor
 */
#if defined(_WIN32)
#define PDNNET_SOCKET_VALID(socket) ((socket) != INVALID_SOCKET)
#else
#define PDNNET_SOCKET_VALID(socket) ((socket) >= 0)
#endif  // !defined(_WIN32)

/**
 * Check if a native socket function has returned an error.
 *
 * POSIX functions typically return -1 while Windows Sockets functions will
 * instead return `SOCKET_ERROR`. Having this macro smooths things over.
 *
 * @param expr Socket function expression
 */
#if defined(_WIN32)
#define PDNNET_SOCKET_ERROR(expr) ((expr) == SOCKET_ERROR)
#else
#define PDNNET_SOCKET_ERROR(expr) ((expr) < 0)
#endif  // !defined(_WIN32)

/**
 * Create a socket descriptor.
 *
 * @note Check `errno` for POSIX, `WSAGetLastError` for Windows.
 *
 * @param domain Socket communication domain, e.g. `AF_INET`, `AF_INET6`
 * @param type Socket type, e.g. `SOCK_STREAM`, `SOCK_DGRAM`
 * @param protocol Socket communication protocol, e.g. `0` for the default
 * @returns Socket descriptor on success, `PDNNET_INVALID_SOCKET` on error
 */
PDNNET_PUBLIC pdnnet_socket
pdnnet_socket_create(int domain, int type, int protocol) PDNNET_NOEXCEPT;

/**
 * Close a socket descriptor.
 *
 * @param socket Socket descriptor to close
 * @returns 0 on success, on error `-errno` (POSIX), `-WSAGetLastError()` (Win32)
 */
PDNNET_PUBLIC int
pdnnet_socket_destroy(pdnnet_socket socket) PDNNET_NOEXCEPT;

/**
 * Create a TCP socket using the default communication protocol.
 *
 * @note Check `errno` for POSIX, `WSAGetLastError` for Windows.
 *
 * @param domain Socket communication domain, e.g. `AF_INET`, `AF_INET6`
 * @returns Socket descriptor on success, `PDNNET_INVALID_SOCKET` on error
 */
#define PDNNET_TCP_SOCKET(domain) pdnnet_socket_create(domain, SOCK_STREAM, 0)

/**
 * Return a string message for the given error code.
 *
 * This is equivalent to `strerror` for POSIX systems while on Windows it is
 * equivalent to a `FormatMessage` call on system error codes.
 *
 * @param err Error code, e.g. from `errno` on POSIX, `[WSA]GetLastError` Win32
 */
/*
PDNNET_PUBLIC const char *
pdnnet_strerror(int err) PDNNET_NOEXCEPT;
*/

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
 * Struct holding state information when performing a socket read.
 *
 * Below is a description of the members.
 *
 * @todo Fix docs, `recv` is used on Windows
 *
 * @param sockfd Socket file descriptor being read from
 * @param msg_buf Buffer to whom each `read()` will write bytes to. Contains a
 *  null terminator so it can be treated as a string if appropriate.
 * @param msg_buf_size Buffer size, not including final null terminator
 * @param n_reads Number of successful `read()` calls made
 * @param n_read_msg Number of bytes last read, i.e. `read()` return value
 * @param n_read_total Total number of bytes read so far
 */
typedef struct {
  pdnnet_socket sockfd;
  void *msg_buf;
  size_t msg_buf_size;
  size_t n_reads;
  pdnnet_ssize_t n_read_msg;
  size_t n_read_total;
} pdnnet_socket_read_state;

// TODO: update docs for any *onlread functions to indicate that negated Windows
// Sockets error codes will be returned instead of -errno on Windows

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
  PDNNET_SA(In) pdnnet_socket_read_state *,
  PDNNET_SA(Opt(In_Out)) void *) PDNNET_NOEXCEPT;

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
PDNNET_PUBLIC int
pdnnet_socket_onlread(
  pdnnet_socket sockfd,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func read_action,
  PDNNET_SA(Opt(In_Out)) void *read_action_param) PDNNET_NOEXCEPT;

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
PDNNET_PUBLIC int
pdnnet_socket_onlread_s(
  pdnnet_socket sockfd,
  size_t read_size,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func read_action,
  PDNNET_SA(Opt(In_Out)) void *read_action_param) PDNNET_NOEXCEPT;

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
PDNNET_PUBLIC int
pdnnet_socket_onlread2(
  pdnnet_socket sockfd,
  size_t read_size,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func read_action,
  PDNNET_SA(Opt(In_Out)) void *read_action_param,
  PDNNET_SA(Opt(In)) pdnnet_socket_onlread_func post_action,
  PDNNET_SA(Opt(In_Out)) void *post_action_param) PDNNET_NOEXCEPT;

/**
 * Read from a socket until end of transmission and write bytes to a stream.
 *
 * @param sockfd Socket file descriptor to read from
 * @param f File stream to write bytes read to
 * @returns 0 on success, -ENOMEM on buffer allocation failure, -EINVAL if `f`
 *  is `NULL`, -EIO if writing to `f` fails, -errno on error
 */
PDNNET_PUBLIC int
pdnnet_socket_fwrite(pdnnet_socket sockfd, PDNNET_SA(Out) FILE *f) PDNNET_NOEXCEPT;

/**
 * Read from a socket until end of transmission and write bytes to a stream.
 *
 * @param sockfd Socket file descriptor to read from
 * @param read_size Number of bytes each `read` call should request
 * @param f File stream to write bytes read to
 * @returns 0 on success, -ENOMEM on buffer allocation failure, -EINVAL if `f`
 *  is `NULL`, -EIO if writing to `f` fails, -errno on error
 */
PDNNET_PUBLIC int
pdnnet_socket_fwrite_s(
  pdnnet_socket sockfd, size_t read_size, PDNNET_SA(Out) FILE *f) PDNNET_NOEXCEPT;

PDNNET_EXTERN_C_END

#endif  // PDNNET_SOCKET_H_
