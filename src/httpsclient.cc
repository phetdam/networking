/**
 * @file httpsclient.cc
 * @author Derek Huang
 * @brief Toy HTTPS client that makes a simple GET request
 * @copyright MIT License
 */

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_HOST
#define PDNNET_ADD_CLIOPT_PATH
#define PDNNET_ADD_CLIOPT_VERBOSE
#define PDNNET_CLIOPT_HOST_DEFAULT "cs.nyu.edu"
#define PDNNET_CLIOPT_PATH_DEFAULT "/~gottlieb/shortBio.html"

#include "pdnnet/client.hh"
#include "pdnnet/cliopt.h"
#include "pdnnet/error.h"
#include "pdnnet/error.hh"
#include "pdnnet/platform.h"

#ifdef PDNNET_UNIX
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>

#include "pdnnet/tls.hh"
#endif  // PDNNET_UNIX

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple HTTPS client that makes a GET request and prints text to stdout.\n"
  "\n"
  "On *nix systems the TLS implementation used is OpenSSL.\n"
  "\n"
  "This program does not do anything besides create a socket on Windows."
)

namespace {

#ifdef PDNNET_UNIX
/**
 * Return a valid HTTP GET request for the specified host path.
 *
 * @note Do not call before `PDNNET_PROGRAM_NAME` is initialized.
 *
 * @param host Host name
 * @param path Path to host resource
 */
std::string http_get_request(
  const std::string& host, const std::filesystem::path& path)
{
  return
    // note: HTTP/1.0 disables chunked transfer
    "GET " + path.string() + " HTTP/1.0\r\n"
    // only interested in receiving text
    "Accept: text/html,application/xhtml+xml,application/xml\r\n"
    // host required for HTTP 1.1 requests
    "Host: " + host + "\r\n"
    // custom user agent string
    "User-Agent: pdnnet-" + std::string{PDNNET_PROGRAM_NAME} + "/0.0.1\r\n\r\n";
}
#endif  // PDNNET_UNIX

}  // namespace

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // create IPv4 TCP/IP client + attempt connection. HTTPS is port 443
  pdnnet::ipv4_client client{};
  client.connect(PDNNET_CLIOPT(host), 443).exit_on_error();
  // HTTPS request logic on *nix only for now
#ifdef PDNNET_UNIX
  // create OpenSSL TLS layer using default context + attempt to connect
  pdnnet::unique_tls_layer layer{pdnnet::default_tls_context()};
  layer.handshake(client.socket()).exit_on_error();
  // HTTP/1.1 GET request we will make
  auto get_request = http_get_request(PDNNET_CLIOPT(host), PDNNET_CLIOPT(path));
  // print TLS version and request if verbose
  if (PDNNET_CLIOPT(verbose))
    std::cout << PDNNET_PROGRAM_NAME << ": Using " << SSL_get_version(layer) <<
      ". Making request...\n" << get_request << std::endl;
  // write request to server
  auto remaining = get_request.size();
  while (remaining) {
    auto n_written = SSL_write(
      layer,
      get_request.c_str() + (get_request.size() - remaining),
      // write parameter is int, need cast
      static_cast<int>(remaining)
    );
    // if unsuccesful, throw only if we can't retry
    if (n_written <= 0) {
      auto err = SSL_get_error(layer, n_written);
      if (err == SSL_ERROR_WANT_WRITE) {
        std::cout << "GET request write failed: retrying..." << std::endl;
        continue;
      }
      throw std::runtime_error{
        pdnnet::openssl_error_string(err, "GET request write failed")
      };
    }
    // decrement remaining
    remaining -= n_written;
  }
  // done, shut down write end
  PDNNET_ERROR_EXIT_IF(
    SSL_shutdown(layer) < 0,
    pdnnet::openssl_error_string("Failed to close write end").c_str()
  );
  // read contents from server and print to stdout
  char buf[512];
  int n_read;
  int read_err;
  do {
    n_read = SSL_read(layer, buf, sizeof buf);
    // if successful (read some bytes), print to stdout
    if (n_read > 0)
      std::cout.write(buf, n_read);
    // if unsuccessful, continue if we can retry
    else {
      read_err = SSL_get_error(layer, n_read);
      if (read_err == SSL_ERROR_WANT_READ)       {
        std::cout << "Content read failed: retrying..." << std::endl;
        continue;
      }
    }
    // otherwise, we will break out of loop
  }
  while (n_read > 0);
  // check for error. typically read_err is SSL_ERROR_SSL and we can use
  // ERR_get_error() called in openssl_error_string() for more info
  PDNNET_ERROR_EXIT_IF(
    read_err != SSL_ERROR_ZERO_RETURN,
    pdnnet::openssl_error_string("Content read failed: OpenSSL error").c_str()
  );
  // TODO: do nothing otherwise
#else
  std::cout << "Nothing done" << std::endl;
#endif  //
  // send simple GET request
  return EXIT_SUCCESS;
}
