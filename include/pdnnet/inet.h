/**
 * @file inet.h
 * @author Derek Huang
 * @brief C/C++ header for Internet helpers
 * @copyright MIT License
*/

#ifndef PDNNET_INET_H_
#define PDNNET_INET_H_

#include <string.h>

#include "pdnnet/common.h"

#ifdef __unix__
#include <arpa/inet.h>
#include <netinet/in.h>
#endif  // __unix__

PDNNET_EXTERN_C_BEGIN

#ifdef __unix__
/**
 * Populate the `sockaddr_in` members to represent an IPv4 address.
 *
 * @param sa Address to `sockaddr_in` struct
 * @param addr IP host address, e.g. `INADDR_ANY`, `INADDR_LOOPBACK`
 * @param port Port number in host byte order
 */
PDNNET_INLINE void
pdnnet_set_sockaddr_in(struct sockaddr_in *sa, in_addr_t addr, in_port_t port)
{
  memset(sa, 0, sizeof *sa);
  sa->sin_family = AF_INET;
  sa->sin_addr.s_addr = addr;
  sa->sin_port = htons(port);
}
#endif  // __unix__

PDNNET_EXTERN_C_END

#endif  // PDNNET_INET_H_
