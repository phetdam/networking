/**
 * @file tls.hh
 * @author Derek Huang
 * @brief C++ header implementing TLS object wrappers
 * @copyright MIT License
 */

#ifndef PDNNET_TLS_HH_
#define PDNNET_TLS_HH_

#include <optional>
#include <string>

#include "pdnnet/error.hh"
#include "pdnnet/platform.h"
#include "pdnnet/socket.hh"

#ifdef PDNNET_UNIX
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#endif  // PDNNET_UNIX

namespace pdnnet {

#ifdef PDNNET_UNIX
/**
 * Initialize OpenSSL error strings and library globals.
 *
 * Although since OpenSSL 1.1.0+ initialization need not be done explicitly,
 * for portability and compatibility reasons this is still good to have.
 *
 * This function is thread-safe.
 */
void init_openssl() noexcept
{
  /**
   * Private class responsible for doing OpenSSL initialization in its ctor.
   */
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

/**
 * Return an error string from an OpenSSL error code.
 *
 * @param err Error code, e.g. as returned from `ERR_get_error`.
 */
inline std::string openssl_error_string(unsigned long err)
{
  return ERR_error_string(err, nullptr);
}

/**
 * Return an error string from an OpenSSL error code prefixed with a message.
 *
 * @param err Error code, e.g. as returned from `ERR_get_error`.
 * @param message Message to prefix with
 */
inline auto openssl_error_string(unsigned long err, const std::string& message)
{
  return (message + ": ") + openssl_error_string(err);
}

/**
 * Return an error string from the last OpenSSL error prefixed with a message.
 *
 * The error is popped from the thread's error queue with `ERR_get_error`.
 *
 * @param message Message to prefix with
 */
inline auto openssl_error_string(const std::string& message)
{
  return openssl_error_string(ERR_get_error(), message);
}

/**
 * Return an error string from the last OpenSSL error.
 *
 * The error is popped from the thread's error queue with `ERR_get_error`.
 */
inline auto openssl_error_string()
{
  return openssl_error_string(ERR_get_error());
}

inline std::string openssl_ssl_error_string(int ssl_error)
{
  switch (ssl_error) {
    case SSL_ERROR_NONE:
      return "No error";
    case SSL_ERROR_ZERO_RETURN:
      return "Connection for writing closed by peer";
    case SSL_ERROR_WANT_READ:
      return "Unable to complete retryable nonblocking read";
    case SSL_ERROR_WANT_WRITE:
      return "Unable to complete retryable nonblocking write";
    case SSL_ERROR_WANT_CONNECT:
      return "Unable to complete retryable connect";
    case SSL_ERROR_WANT_ACCEPT:
      return "Unable to complete retryable accept";
    // TODO: not done handling all the SSL error values
    case SSL_ERROR_SYSCALL:
      return errno_error("Fatal I/O error");
    case SSL_ERROR_SSL:
      return openssl_error_string("Fatal OpenSSL error");
    default:
      return "Unknown OpenSSL SSL error value " + std::to_string(ssl_error);
  }
}

/**
 * TLS context class with unique ownership.
 *
 * This is typically used for initializing a TLS connection layer. On UNIX-like
 * systems the TLS provider used is OpenSSL.
 */
class unique_tls_context {
public:
  /**
   * Default ctor.
   *
   * Uses the default OpenSSL flexible TLS method for initialization.
   */
  unique_tls_context() : unique_tls_context{TLS_method} {}

  /**
   * Ctor.
   *
   * The callable returns the TLS protocol to use for connection.
   *
   * @param method_getter Callable returning a `const SSL_METHOD*`
   */
  unique_tls_context(const std::function<const SSL_METHOD*()>& method_getter)
  {
    // ensure OpenSSL is initialized (thread-safe call)
    init_openssl();
    // initialize context
    context_ = SSL_CTX_new(method_getter());
    // check for error
    if (!context_)
      throw std::runtime_error{openssl_error_string("Failed to create SSL_CTX")};
  }

  /**
   * Deleted copy ctor.
   */
  unique_tls_context(const unique_tls_context&) = delete;

  /**
   * Move ctor.
   *
   * @param other TLS context to move from
   */
  unique_tls_context(unique_tls_context&& other) noexcept
    : context_{other.release()}
  {}

  /**
   * Dtor.
   */
  ~unique_tls_context()
  {
    // no-op if context_ is nullptr
    SSL_CTX_free(context_);
  }

  /**
   * Move assignment operator.
   *
   * @param other TLS context to move from
   */
  unique_tls_context& operator=(unique_tls_context&& other) noexcept
  {
    // free current context (no-op if nullptr) and take ownership
    SSL_CTX_free(context_);
    context_ = other.release();
    return *this;
  }

  /**
   * Return handle to TLS context.
   */
  auto context() const noexcept { return context_; }

  /**
   * Return the TLS method used to create the TLS context.
   */
  auto method() const noexcept { return SSL_CTX_get_ssl_method(context_); }

  /**
   * Release ownership of the TLS context handle.
   */
  SSL_CTX* release() noexcept
  {
    auto old_context = context_;
    context_ = nullptr;
    return old_context;
  }

  /**
   * Implicitly convert to a `SSL_CTX*` TLS context handle.
   */
  operator SSL_CTX*() const noexcept { return context_; }

private:
  SSL_CTX* context_;
};

/**
 * Return const reference to the default TLS context.
 *
 * This uses the default TLS method when creating the TLS context.
 */
inline const auto& default_tls_context()
{
  static unique_tls_context context;
  return context;
}

/**
 * Return const reference to the default TLS 1.3 context.
 *
 * For some servers that don't correctly support secure renegotiation for TLS
 * 1.2 or lower, it may be better to use TLS 1.3 instead.
 */
inline const auto& default_tls1_3_context()
{
  static auto context = []
  {
    unique_tls_context ctx;
    if (!SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION))
      throw std::runtime_error{"Failed to create default TLS 1.3 context"};
    return ctx;
  }();
  return context;
}

class unique_tls_layer {
public:
  unique_tls_layer() noexcept : layer_{} {}

  unique_tls_layer(const unique_tls_context& context)
    : layer_{SSL_new(context)}
  {
    if (!layer_)
      throw std::runtime_error{openssl_error_string("Failed to create SSL")};
  }

  unique_tls_layer(const unique_tls_layer&) = delete;

  unique_tls_layer(unique_tls_layer&& other) noexcept
    : layer_{other.release()}
  {}

  ~unique_tls_layer()
  {
    // no-op if layer_ is nullptr
    SSL_free(layer_);
  }

  unique_tls_layer& operator=(unique_tls_layer&& other) noexcept
  {
    SSL_free(layer_);
    layer_ = other.release();
    return *this;
  }

  auto layer() const noexcept { return layer_; }

  SSL* release() noexcept
  {
    auto old_layer = layer_;
    layer_ = nullptr;
    return old_layer;
  }

  operator SSL*() const noexcept { return layer_; }

  std::optional<std::string> handshake(socket_handle handle)
  {
    // set I/O facility using the connected socket handle
    if (!SSL_set_fd(layer_, handle))
      return openssl_error_string("Failed to set socket handle");
    // perform TLS handshake with server
    auto status = SSL_connect(layer_);
    // 1 on success
    if (status == 1)
      return {};
    // otherwise, failure. use SSL_get_error to get TLS layer error code
    auto ssl_error = SSL_get_error(layer_, status);
    // 0 for controlled failure, otherwise fatal
    if (!status)
      return
        "Controlled TLS handshake error: " + openssl_ssl_error_string(ssl_error);
    return "Fatal TLS handshake error: " + openssl_ssl_error_string(ssl_error);
  }

private:
  SSL* layer_;
};
#endif  // PDNNET_UNIX

}  // namespace pdnnet

#endif  // PDNNET_TLS_HH_
