/**
 * @file tls.hh
 * @author Derek Huang
 * @brief C++ header implementing TLS object wrappers
 * @copyright MIT License
 */

#ifndef PDNNET_TLS_HH_
#define PDNNET_TLS_HH_

#include <string>

#include "pdnnet/platform.h"

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
void init_openssl()
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

class unique_tls_context {
public:
  unique_tls_context() : unique_tls_context{TLS_method} {}

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

  unique_tls_context(const unique_tls_context&) = delete;

  ~unique_tls_context()
  {
    // no-op if context_ is nullptr
    SSL_CTX_free(context_);
  }

  unique_tls_context& operator=(unique_tls_context&& other)
  {
    // free current context (no-op if nullptr) and take ownership
    SSL_CTX_free(context_);
    context_ = other.release();
    return *this;
  }

  auto context() const noexcept { return context_; }

  auto method() const noexcept { return SSL_CTX_get_ssl_method(context_); }

  SSL_CTX* release() noexcept
  {
    auto old_context = context_;
    context_ = nullptr;
    return old_context;
  }

  operator SSL_CTX*() const noexcept { return context_; }

private:
  SSL_CTX* context_;
};

inline const auto& default_tls_context()
{
  static unique_tls_context context;
  return context;
}
#endif  // PDNNET_UNIX

}  // namespace pdnnet

#endif  // PDNNET_TLS_HH_
