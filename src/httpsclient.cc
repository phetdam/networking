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
#include "pdnnet/platform.h"

#ifdef PDNNET_UNIX
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
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

void init_openssl()
{
  class openssl_init {
  public:
    openssl_init()
    {
      // no explicit init required for OpenSSL 1.1.0+
#if OPENSSL_VERSION_NUMBER < 0x10100000L
      SSL_load_error_strings();
      SSL_library_init();
#endif  // OPENSSL_VERSION_NUMBER >= 0x10100000L
    }
  };

  // thread-safe under C++11
  static openssl_init init;
}

inline std::string openssl_error_string(unsigned long err)
{
  return ERR_error_string(err, nullptr);
}

inline auto openssl_error_string()
{
  return openssl_error_string(ERR_get_error());
}

class tls_context {
public:
  tls_context() : tls_context{TLS_method} {}

  tls_context(const std::function<const SSL_METHOD*()>& method_getter)
  {
    // ensure OpenSSL is initialized (thread-safe call)
    init_openssl();
    // initialize method and context
    method_ = method_getter();
    context_ = SSL_CTX_new(method_);
    // check for error
    if (!context_)
      throw std::runtime_error{
        "Failed to create SSL_CTX: " + openssl_error_string()
      };
  }

  tls_context(const tls_context&) = delete;

  ~tls_context()
  {
    // no-op if context_ is NULL
    SSL_CTX_free(context_);
  }

  tls_context& operator=(tls_context&& other)
  {
    // copy pointers
    context_ = other.context_;
    method_ = other.method_;
    // set other's pointers to NULL
    other.context_ = nullptr;
    other.method_ = nullptr;
    return *this;
  }

  operator SSL_CTX*() const noexcept { return context_; }

  auto context() const noexcept { return context_; }

  auto method() const noexcept { return method_; }

private:
  SSL_CTX* context_;
  const SSL_METHOD* method_;
};

class tls_connection {
public:
  tls_connection(pdnnet::socket_handle handle, const tls_context& context)
    : handle_{handle}, ssl_{SSL_new(context)}, context_{context}
  {
    if (!ssl_)
      throw std::runtime_error{
        "Failed to create SSL: " + openssl_error_string()
      };
    // set socket handle as I/O facility for transport layer
    if (!SSL_set_fd(ssl_, handle_))
      throw std::runtime_error{
        "Failed to set socket handle: " + openssl_error_string()
      };
  }

  tls_connection(const tls_connection&) = delete;

  ~tls_connection()
  {
    SSL_free(ssl_);
  }

  auto handle() const noexcept { return handle_; }

  auto ssl() const noexcept { return ssl_; }

  const auto& context() const noexcept { return context_; }

  std::optional<std::string> connect() const
  {
    auto status = SSL_connect(ssl_);
    // 1 on success
    if (status == 1)
      return {};
    // 0 is controlled failure
    if (status == 0)
      return "Controlled TLS handshake error: " + openssl_error_string();
    // otherwise, fatal error
    return "Fatal TLS handshake error: " + openssl_error_string();
  }

  auto operator()() const
  {
    return connect();
  }

private:
  pdnnet::socket_handle handle_;
  SSL* ssl_;
  const tls_context& context_;
};
#endif  // PDNNET_UNIX

}  // namespace

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // create IPv4 TCP/IP client + attempt connection
  pdnnet::ipv4_client client{};
  auto error = client.connect(PDNNET_CLIOPT(host), 443);  // HTTPS is port 443
  // if connection fails, print error and exit nonzero
  if (error)
    PDNNET_ERROR_EXIT(error->c_str());
  // HTTPS request logic on *nix only for now
#ifdef PDNNET_UNIX
  // create OpenSSL TLS context
  tls_context context;
  // create OpenSSL TLS connection + attempt to connect
  tls_connection connection{client.socket(), context};
  error = connection();
  // if TLS handshake fails, print error and exit nonzero
  if (error)
    PDNNET_ERROR_EXIT(error->c_str());
  // HTTP/1.1 GET request we will make
  auto get_request = http_get_request(PDNNET_CLIOPT(host), PDNNET_CLIOPT(path));
  // print request if verbose
  if (PDNNET_CLIOPT(verbose))
    std::cout << PDNNET_PROGRAM_NAME << ": Making request...\n" <<
      get_request << std::endl;
  // write request to server
  auto remaining = get_request.size();
  while (remaining) {
    auto n_written = SSL_write(
      connection.ssl(),
      get_request.c_str() + (get_request.size() - remaining),
      // write parameter is int, need cast
      static_cast<int>(remaining)
    );
    // if unsuccesful, throw only if we can't retry
    if (n_written <= 0) {
      auto err = SSL_get_error(connection.ssl(), n_written);
      if (err == SSL_ERROR_WANT_WRITE) {
        std::cout << "GET request write failed: retrying..." << std::endl;
        continue;
      }
      throw std::runtime_error{
        "GET request write failed: " + openssl_error_string(err)
      };
    }
    // decrement remaining
    remaining -= n_written;
  }
  // done, shut down write end
  if (SSL_shutdown(connection.ssl()) < 0)
    PDNNET_ERROR_EXIT(
      ("Failed to close write end: " + openssl_error_string()).c_str()
    );
  // read contents from server and print to stdout
  char buf[512];
  int n_read;
  int read_err;
  do {
    n_read = SSL_read(connection.ssl(), buf, sizeof buf);
    // if successful (read some bytes), print to stdout
    if (n_read > 0)
      std::cout.write(buf, n_read);
    // if unsuccessful, continue if we can retry
    else {
      read_err = SSL_get_error(connection.ssl(), n_read);
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
  if (read_err != SSL_ERROR_ZERO_RETURN)
    PDNNET_ERROR_EXIT(
      ("Content read failed: OpenSSL error " + openssl_error_string()).c_str()
    );
  // TODO: do nothing otherwise
#else
  std::cout << "Nothing done" << std::endl;
#endif  //
  // send simple GET request
  return EXIT_SUCCESS;
}