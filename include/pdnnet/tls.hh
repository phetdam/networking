/**
 * @file tls.hh
 * @author Derek Huang
 * @brief C++ header implementing TLS object wrappers
 * @copyright MIT License
 */

#ifndef PDNNET_TLS_HH_
#define PDNNET_TLS_HH_

#ifdef _WIN32
// consistently use WIN32_LEAN_AND_MEAN like the other headers do
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <schannel.h>
// by default, use Win32 security APIs in user application mode
#if !defined(SECURITY_WIN32) && !defined(SECURITY_KERNEL)
#define SECURITY_WIN32
#endif  // defined(SECURITY_WIN32) || defined(SECURITY_KERNEL)
// don't allow use of the obsolete (?) SECURITY_MAC macro
#ifdef SECURITY_MAC
#error "Only SECURITY_WIN32 or SECURITY_KERNEL may be defined"
#endif  // SECURITY_MAC
#include <security.h>
#endif  // _WIN32

#include <optional>
#include <string>

#include "pdnnet/error.hh"
#include "pdnnet/platform.h"
#include "pdnnet/socket.hh"

// OpenSSL used for *nix systems
#ifdef PDNNET_UNIX
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#endif  // PDNNET_UNIX

namespace pdnnet {

/**
 * Maximum TLS record size with extra space for header, MAC address, padding.
 *
 * See RFC 8449. Value used here from the following GitHub Gist:
 *
 * https://gist.github.com/mmozeiko/c0dfcc8fec527a90a02145d2cc0bfb6d
 */
constexpr std::size_t tls_record_size_limit = 16384 + 512;

#ifdef _WIN32
/**
 * Return a new `SCHANNEL_CRED` from the given inputs.
 *
 * @note TLS 1.3 doesn't seem to work correctly in Schannel, e.g. for clients
 *  `SP_PROT_TLS1_3_CLIENT` cannot be used as a protocol. One therefore needs
 *  to allow protocol negotation to TLS 1.2 or lower.
 *
 * @param protocols Allowed SSL/TLS protocols, zero to let Schannel decide
 * @param op_flags Schannel operation flags, e.g. `SCH_CRED_NO_DEFAULT_CREDS`
 */
auto create_schannel_cred(DWORD protocols, DWORD op_flags)
{
  SCHANNEL_CRED cred{};
  cred.dwVersion = SCHANNEL_CRED_VERSION;  // always SCHANNEL_CRED_VERSION
  cred.grbitEnabledProtocols = protocols;
  cred.dwFlags = op_flags;
  return cred;
}

/**
 * Return a new `SCHANNEL_CRED` from the given inputs.
 *
 * Allows Schannel to decided what the SSL/TLS protocol used will be.
 *
 * @param op_flags Schannel operation flags, e.g. `SCH_CRED_NO_DEFAULT_CREDS`
 */
inline auto create_schannel_cred(DWORD op_flags = 0u)
{
  return create_schannel_cred(0u, op_flags);
}

/**
 * Windows SSPI credential handle class with unique ownership.
 */
class unique_cred_handle {
public:
  /**
   * Default ctor.
   *
   * Calling `valid()` on a default-constructed instance will return `false`.
   */
  unique_cred_handle() noexcept : handle_{} {}

  /**
   * Ctor.
   *
   * @handle Credential handle struct to take ownership of
   */
  explicit unique_cred_handle(const CredHandle& handle) noexcept
    : handle_{handle}
  {}

  /**
   * Deleted copy ctor.
   */
  unique_cred_handle(const unique_cred_handle&) = delete;

  /**
   * Move ctor.
   */
  unique_cred_handle(unique_cred_handle&& other) noexcept
    : handle_{other.release()}
  {}

  /**
   * Move assignment operator.
   */
  auto& operator=(unique_cred_handle&& other) noexcept
  {
    destroy_handle();
    handle_ = other.release();
    return *this;
  }

  /**
   * Dtor.
   */
  ~unique_cred_handle()
  {
    destroy_handle();
  }

  /**
   * Return a const reference to the managed `CredHandle` struct.
   */
  const auto& handle() const noexcept { return handle_; }

  /**
   * Release ownership of the managed `CredHandle`.
   *
   * After calling `release()`, calling `valid()` will return `false`.
   */
  CredHandle release() noexcept
  {
    auto old_handle = handle_;
    handle_ = {};
    return old_handle;
  }

  /**
   * Implicitly convert to a `PCredHandle`.
   *
   * This is mostly useful when working with the Win32 SSPI functions.
   */
  operator PCredHandle() noexcept
  {
    return &handle_;
  }

  /**
   * Implicitly convert to a `CredHandle` const reference.
   */
  operator const CredHandle&() const noexcept
  {
    return handle_;
  }

  /**
   * Implicitly convert to a `CredHandle` reference.
   */
  operator CredHandle&() noexcept
  {
    return handle_;
  }

  /**
   * `true` if the object is managing a valid `CredHandle`, `false` otherwise.
   *
   * @note The `unique_cred_handle` takes a zeroed `CredHandle` to be invalid,
   *  but the `SecInvalidateHandle` and `SecIsValidHandle` macros defined in
   *  the Windows `sspi.h` header imply that an invalid `CredHandle` is one
   *  that has all its bits set (due to casting -1 to `ULONG_PTR`).
   */
  bool valid() const noexcept
  {
    return handle_.dwLower && handle_.dwUpper;
  }

  /**
   * `true` if the object is managing a valid `CredHandle`, `false` otherwise.
   */
  operator bool() const noexcept
  {
    return valid();
  }

private:
  CredHandle handle_;

  /**
   * Free the `CredHandle` if it is valid.
   *
   * Return value ignored since a managed `CredHandle` should not be invalid.
   */
  void destroy_handle() noexcept
  {
    if (valid())
      FreeCredentialsHandle(&handle_);
  }
};

/**
 * Windows SSPI context handle class with unique ownership.
 *
 * @note Although `CredHandle` and `CtxtHandle` are both typedefs of the SSPI
 *  `SecHandle` type, their methods of deletion are different.
 */
class unique_ctxt_handle {
public:
  /**
   * Default ctor.
   *
   * Calling `valid()` on a default-constructed instance will return `false`.
   */
  unique_ctxt_handle() noexcept : handle_{} {}

  /**
   * Ctor.
   *
   * @handle Credential handle struct to take ownership of
   */
  explicit unique_ctxt_handle(const CtxtHandle& handle) noexcept
    : handle_{handle}
  {}

  /**
   * Deleted copy ctor.
   */
  unique_ctxt_handle(const unique_ctxt_handle&) = delete;

  /**
   * Move ctor.
   */
  unique_ctxt_handle(unique_ctxt_handle&& other) noexcept
    : handle_{other.release()}
  {}

  /**
   * Move assignment operator.
   */
  auto& operator=(unique_ctxt_handle&& other) noexcept
  {
    destroy_handle();
    handle_ = other.release();
    return *this;
  }

  /**
   * Dtor.
   */
  ~unique_ctxt_handle()
  {
    destroy_handle();
  }

  /**
   * Return a const reference to the managed `CtxtHandle` struct.
   */
  const auto& handle() const noexcept { return handle_; }

  /**
   * Release ownership of the managed `CtxtHandle`.
   *
   * After calling `release()`, calling `valid()` will return `false`.
   */
  CtxtHandle release() noexcept
  {
    auto old_handle = handle_;
    handle_ = {};
    return old_handle;
  }

  /**
   * Implicitly convert to a `PCtxtHandle`.
   *
   * This is mostly useful when working with the Win32 SSPI functions.
   */
  operator PCtxtHandle() noexcept
  {
    return &handle_;
  }

  /**
   * Implicitly convert to a `CtxtHandle` const reference.
   */
  operator const CtxtHandle&() const noexcept
  {
    return handle_;
  }

  /**
   * Implicitly convert to a `CtxtHandle` reference.
   */
  operator CtxtHandle&() noexcept
  {
    return handle_;
  }

  /**
   * `true` if the object is managing a valid `CtxtHandle`, `false` otherwise.
   *
   * @note See the note in the `unique_cred_handle::valid` comments.
   */
  bool valid() const noexcept
  {
    return handle_.dwLower && handle_.dwUpper;
  }

  /**
   * `true` if the object is managing a valid `CtxtHandle`, `false` otherwise.
   */
  operator bool() const noexcept
  {
    return valid();
  }

private:
  CtxtHandle handle_;

  /**
   * Free the `CtxtHandle` if it is valid.
   *
   * Return value ignored since a managed `CtxtHandle` should not be invalid.
   */
  void destroy_handle() noexcept
  {
    if (valid())
      DeleteSecurityContext(&handle_);
  }
};

/**
 * Acquire unique Schannel credential handle for use in TLS handshake.
 *
 * @param cred Empty credential handle written to for use in TLS handshake
 * @param sc_cred Schannel credential struct
 * @returns Optional empty on success, with error message on failure
 */
inline optional_error acquire_schannel_creds(
  unique_cred_handle& cred, const SCHANNEL_CRED& sc_cred)
{
  CredHandle raw_cred;
  // acquire credentials handle for Schannel
  auto status = AcquireCredentialsHandle(
    NULL,
    UNISP_NAME,
    SECPKG_CRED_OUTBOUND,
    NULL,  // pvLogonID
    (PVOID) &sc_cred,
    NULL,  // pGetKeyFn
    NULL,  // pvGetKeyArgument
    &raw_cred,
    NULL
  );
  // if failure, return error string
  if (status != SEC_E_OK)
    return pdnnet::windows_error(status);
  // otherwise move + return nothing
  cred = pdnnet::unique_cred_handle{raw_cred};
  return {};
}
#endif  // _WIN32

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

/**
 * Return an error string from a `SSL_get_error` return value.
 *
 * @param ssl_error Status returned by `SSL_get_error`.
 */
inline std::string openssl_ssl_error_string(int ssl_error)
{
  switch (ssl_error) {
    case SSL_ERROR_NONE:
      return "No error";
    case SSL_ERROR_ZERO_RETURN:
      return "Connection for writing closed by peer";
    case SSL_ERROR_WANT_READ:
      return "Try again, unable to complete nonblocking BIO read";
    case SSL_ERROR_WANT_WRITE:
      return "Try again, unable to complete nonblocking BIO write";
    case SSL_ERROR_WANT_CONNECT:
      return "Try again, unable to connect to BIO without blocking";
    case SSL_ERROR_WANT_ACCEPT:
      return "Try again, unable to accept BIO without blocking";
    case SSL_ERROR_WANT_X509_LOOKUP:
      return "Try again, client certificate callback asked to be called again";
    case SSL_ERROR_WANT_ASYNC:
      return "Try again, async engine processing not yet complete";
    case SSL_ERROR_WANT_ASYNC_JOB:
      return "Try again, no async jobs in pool available to be started";
    case SSL_ERROR_WANT_CLIENT_HELLO_CB:
      return "Try again, client hello callback asked to be called again";
    case SSL_ERROR_SYSCALL:
      return errno_error("Fatal I/O error");
    case SSL_ERROR_SSL:
      return openssl_error_string("Fatal OpenSSL error");
    default:
      return "Unknown OpenSSL SSL error value " + std::to_string(ssl_error);
  }
}

/**
 * Return an error string from a `SSL_get_error` value prefixed with a message.
 *
 * @param ssl_error Status returned by `SSL_get_error`
 * @param message Message to prefix with
 */
inline auto openssl_ssl_error_string(int ssl_error, const std::string& message)
{
  return message + ": " + openssl_ssl_error_string(ssl_error);
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
   * Release ownership of the TLS context handle and return it.
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

/**
 * TLS connection layer class with unique ownership.
 *
 * On UNIX-like systems OpenSSL provides the TLS implementation.
 */
class unique_tls_layer {
public:
  /**
   * Default ctor.
   *
   * Creates an uninitialized layer.
   */
  unique_tls_layer() noexcept : layer_{} {}

  /**
   * Ctor.
   *
   * Creates the TLS connection layer from the given context.
   *
   * @param context TLS context to create connection layer from
   */
  unique_tls_layer(const unique_tls_context& context)
    : layer_{SSL_new(context)}
  {
    if (!layer_)
      throw std::runtime_error{openssl_error_string("Failed to create SSL")};
  }

  /**
   * Deleted copy ctor.
   */
  unique_tls_layer(const unique_tls_layer&) = delete;

  /**
   * Move ctor.
   *
   * @param other TLS context to move from
   */
  unique_tls_layer(unique_tls_layer&& other) noexcept
    : layer_{other.release()}
  {}

  /**
   * Dtor.
   */
  ~unique_tls_layer()
  {
    // no-op if layer_ is nullptr
    SSL_free(layer_);
  }

  /**
   * Move assignment operator.
   *
   * @param other TLS context to move from
   */
  unique_tls_layer& operator=(unique_tls_layer&& other) noexcept
  {
    SSL_free(layer_);
    layer_ = other.release();
    return *this;
  }

  /**
   * Return handle to the TLS connection layer.
   */
  auto layer() const noexcept { return layer_; }

  /**
   * Release ownership of the TLS connection layer handle and return it.
   */
  SSL* release() noexcept
  {
    auto old_layer = layer_;
    layer_ = nullptr;
    return old_layer;
  }

  /**
   * Implicitly convert to a `SSL*` TLS context handle.
   */
  operator SSL*() const noexcept { return layer_; }

  /**
   * Return the numeric TLS protocol version used for the connection.
   */
  auto protocol() const noexcept { return SSL_version(layer_); }

  /**
   * Return the TLS protocol version string used for the connection.
   */
  std::string protocol_string() const { return SSL_get_version(layer_); }

  /**
   * Perform the TLS handshake with the server through a connected socket.
   *
   * @param handle Connected socket handle
   * @returns Optional error empty on success, with error on failure
   */
  optional_error handshake(socket_handle handle)
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
