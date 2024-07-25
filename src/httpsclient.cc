/**
 * @file httpsclient.cc
 * @author Derek Huang
 * @brief Toy HTTPS client that makes a simple GET request
 * @copyright MIT License
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define SECURITY_WIN32  // for user-mode security
#include <schannel.h>
#include <security.h>
#endif  // _WIN32

#include <climits>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#define PDNNET_HAS_PROGRAM_USAGE
#define PDNNET_ADD_CLIOPT_HOST
#define PDNNET_ADD_CLIOPT_PATH
#define PDNNET_ADD_CLIOPT_VERBOSE
#define PDNNET_CLIOPT_HOST_DEFAULT "cs.nyu.edu"
#define PDNNET_CLIOPT_PATH_DEFAULT "/~gottlieb/almasiGottlieb.html"

#include "pdnnet/client.hh"
#include "pdnnet/cliopt.h"
#include "pdnnet/error.h"
#include "pdnnet/error.hh"
#include "pdnnet/platform.h"

#ifdef PDNNET_UNIX
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#endif  // PDNNET_UNIX

#include "pdnnet/tls.hh"

/**
 * Platform-specific program note.
 *
 * @note Preprocessor directives in macro expansions is undefined behavior.
 */
#if defined(_WIN32)
#define EXTRA_NOTE \
  "\n\n" \
  "WIP on Windows, only performing the Schannel TLS handshake with the server."
#else
#define EXTRA_NOTE ""
#endif  // !defined(_WIN32)

PDNNET_PROGRAM_USAGE_DEF
(
  "Simple HTTPS client that makes a GET request and prints text to stdout.\n"
  "\n"
  "OpenSSL is used for TLS on *nix systems with Schannel used on Windows."
  EXTRA_NOTE
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
    // note: HTTP/1.0 disables chunked transfer + implies Connection: close
    "GET " + path.string() + " HTTP/1.1\r\n"
    // only interested in receiving text
    "Accept: text/html,application/xhtml+xml,application/xml\r\n"
    // host required for HTTP 1.1 requests
    "Host: " + host + "\r\n"
    // custom user agent string
    "User-Agent: pdnnet-" + std::string{PDNNET_PROGRAM_NAME} + "/0.0.1\r\n\r\n";
}
#endif  // PDNNET_UNIX

#ifdef _WIN32
/**
 * Perform Schannel TLS handshake.
 */
pdnnet::optional_error schannel_perform_handshake(
  CtxtHandle& context,
  pdnnet::socket_handle handle,
  const CredHandle& cred,
  // TODO: do we really need all these flags?
  ULONG ctx_init_flags =
    ISC_REQ_USE_SUPPLIED_CREDS |
    ISC_REQ_ALLOCATE_MEMORY |
    ISC_REQ_CONFIDENTIALITY |
    ISC_REQ_REPLAY_DETECT |
    ISC_REQ_SEQUENCE_DETECT |
    ISC_REQ_STREAM)
{
  // true if first InitializeSecurityContext call succeeded
  bool building_context = false;
  // socket writer
  pdnnet::socket_writer writer{handle};
  // raw buffer to receive security context data + current written size
  // TODO: this buffer has to persist past the function's scope
  char ctx_buffer[pdnnet::tls_record_size_limit];
  unsigned long ctx_bufsize = 0;  // largest type needed is ULONG
  // handshake loop
  while (true) {
    // flags sent and received that indicate requests for the context
    auto context_flags = ctx_init_flags;
    // input security buffers
    SecBuffer input_bufs[2]{};
    // token buffer, points to ctx_buffer with size ctx_bufsize
    input_bufs[0].BufferType = SECBUFFER_TOKEN;
    input_bufs[0].pvBuffer = ctx_buffer;
    // sentinel buffer, can be SECBUFFER_EXTRA after InitializeSecurityContext
    // if server was not able to process all of the provided input
    input_bufs[0].cbBuffer = ctx_bufsize;
    input_bufs[1].BufferType = SECBUFFER_EMPTY;
    // output security buffer
    SecBuffer output_buf{};
    output_buf.BufferType = SECBUFFER_TOKEN;
    // input + output security buffer desc
    SecBufferDesc input_desc{SECBUFFER_VERSION, 2, input_bufs};
    SecBufferDesc output_desc{SECBUFFER_VERSION, 1, &output_buf};
    // perform context initialization/build call
    auto status = InitializeSecurityContext(
      // MS documentation is incorrect, PCredHandle must always be passed
      const_cast<PCredHandle>(&cred),
      // supply PCtxtHandle after first InitializeSecurityContext call
      (!building_context) ? NULL : &context,
      const_cast<SEC_CHAR*>(PDNNET_CLIOPT(host)),
      context_flags,
      0,     // Reserved1
      NULL,  // TargetDataRep
      // supply input SecBufferDesc after first InitializeSecurityContext call
      (!building_context) ? NULL : &input_desc,
      0,     // Reserved2
      &context,
      &output_desc,
      &context_flags,
      NULL
    );
    // might be too much input for Schannel to handle, in which case the second
    // buffer is now SECBUFFER_EXTRA and indicates number of unprocessed bytes
    if (input_bufs[1].BufferType == SECBUFFER_EXTRA) {
      // shift unprocessed bytes and update buffer size
      std::memcpy(
        ctx_buffer,
        ctx_buffer + (ctx_bufsize - input_bufs[1].cbBuffer),
        input_bufs[1].cbBuffer
      );
      ctx_bufsize = input_bufs[1].cbBuffer;
    }
    // reset buffer size, we are going to write into it from the beginning
    else
      ctx_bufsize = 0;
    // switch off of status value
    switch (status) {
      // done, return with no error
      case SEC_E_OK:
        return {};
      // send token to server and read (part of) a return token
      case SEC_I_CONTINUE_NEEDED: {
        // send token to server, returning the error if any
        auto err = writer.read(output_buf.pvBuffer, output_buf.cbBuffer);
        if (err)
          return err;
        // free the token buffer when done + handle error
        if ((status = FreeContextBuffer(output_buf.pvBuffer)) != SEC_E_OK)
          return pdnnet::windows_error(status, "Failed to free token buffer");
        // buffer full, so server could be misbehaving
        if (ctx_bufsize == sizeof ctx_buffer)
          return "Error: Token buffer full, server not following TLS protocol";
        // read data back from server to ctx_buffer
        auto n_read = ::recv(
          handle,
          ctx_buffer + ctx_bufsize,
          // note: due to precedence, read as (sizeof ctx_buffer) - ctx_bufsize
          static_cast<int>(sizeof ctx_buffer - ctx_bufsize),
          0
        );
        // server closed connection. not what we want when doing handshake
        if (!n_read)
          return "Server closed connection gracefully during handshake";
        // error
        if (n_read < 0)
          return pdnnet::winsock_error("Could not read token back from server");
        // partial read, keep updating
        ctx_bufsize += n_read;
        break;
      }
      // handle error
      default:
        return pdnnet::windows_error(status, "Security context creation failed");
    }
    // if we made it here, building_context should be set to true
    if (!building_context)
      building_context = true;
  }
  return {};
}
#endif  // _WIN32

#ifdef PDNNET_UNIX
/**
 * TLS reader/writer CRTP base class.
 *
 * Allows setting the TLS layer and some shared members.
 *
 * @note We use CRTP here because otherwise the named parameter idiom usage of
 *  returning `*this` results in derived classes returning base class refs.
 *
 * @tparam Impl TLS reader/writer implementation class
 */
template <typename Impl>
class tls_reader_writer_base {
public:
  /**
   * Ctor.
   *
   * @param layer TLS connection layer handle
   */
  tls_reader_writer_base(SSL* layer) noexcept
    : layer_{layer}, allow_retry_{true}, message_sink_{}
  {}

  /**
   * Return TLS connection layer handle.
   */
  auto layer() const noexcept { return layer_; }

  /**
   * Indicate if TLS read/write retries are allowed.
   */
  auto allow_retry() const noexcept { return allow_retry_; }

  /**
   * Return pointer to the message sink for any messages (can be `nullptr`).
   */
  auto message_sink() const noexcept { return message_sink_; }

  /**
   * Enable or disable TLS read/write retries.
   *
   * For OpenSSL, a retryable write is when the `SSL_write` error retrieved via
   * `SSL_get_error` is `SSL_ERROR_WANT_WRITE`. A retryable read is when the
   * `SSL_read` error retrieved via `SSL_get_error` is `SSL_ERROR_WANT_READ`.
   *
   * @param retry `true` to allow retrying reads/writes, `false` to disallow
   * @returns `*this` to allow method chaining
   */
  auto& allow_retry(bool retry) noexcept
  {
    allow_retry_ = retry;
    return *static_cast<Impl*>(this);
  }

  /**
   * Set or unset the message sink.
   *
   * @param sink Address to an output stream
   * @returns `*this` to allow method chaining
   */
  auto& message_sink(std::ostream* sink) noexcept
  {
    message_sink_ = sink;
    return *static_cast<Impl*>(this);
  }

private:
  SSL* layer_;
  bool allow_retry_;
  std::ostream* message_sink_;
};

/**
 * TLS writer class for abstracting TLS socket writes.
 */
class tls_writer : public tls_reader_writer_base<tls_writer> {
public:
  // note: class name injection works with CRTP here
  using tls_reader_writer_base::tls_reader_writer_base;

  /**
   * Write string view contents to socket.
   *
   * @tparam CharT Char type
   * @tparam Traits Char traits
   *
   * @param text String view to read input from
   * @returns Optional empty on success, with error message on failure
   */
  template <typename CharT, typename Traits>
  pdnnet::optional_error operator()(std::basic_string_view<CharT, Traits> text) const
  {
    // total and remaining bytes to write
    auto n_total = sizeof(CharT) * text.size();
    auto n_remain = n_total;
    // if total is too large, error
    if (n_total > INT_MAX)
      return "Message length " + std::to_string(n_total) +
        " exceeds max allowed length " + std::to_string(INT_MAX);
    // until done, write bytes to server through TLS layer
    while (n_remain) {
      auto n_written = SSL_write(
        layer(),
        text.data() + (n_total - n_remain),
        static_cast<int>(n_remain)
      );
      // unsucessful, returned zero
      if (n_written <= 0) {
        auto err = SSL_get_error(layer(), n_written);
        // write is retryable
        if (err == SSL_ERROR_WANT_WRITE) {
          // no retry allowed
          if (!allow_retry())
            return "TLS write retryable but writer has disabled retries";
          // non-null sink
          if (message_sink())
            *message_sink() << "TLS write failed: retrying..." <<
              std::endl;
          continue;
        }
        // else give up
        return pdnnet::openssl_ssl_error_string(err, "TLS write failed");
      }
      // decrement remaining
      n_remain -= n_written;
    }
    return {};
  }

  /**
   * Write string contents to socket.
   *
   * @note Overload necessary since implicit conversions are not deduced.
   *
   * @tparam CharT Char type
   * @tparam Traits Char traits
   *
   * @param text String to read input from
   * @returns Optional empty on success, with error message on failure
   */
  template <typename CharT, typename Traits>
  auto operator()(const std::basic_string<CharT, Traits>& text) const
  {
    return (*this)(static_cast<std::basic_string_view<CharT, Traits>>(text));
  }
};

/**
 * TLS reader class for abstracting TLS socket reads.
 */
class tls_reader : public tls_reader_writer_base<tls_reader> {
public:
  /**
   * Ctor.
   *
   * @param layer TLS connection layer handle
   * @param buf_size Read buffer size, i.e. max number of bytes per read
   */
  tls_reader(SSL* layer, std::size_t buf_size = 512U)
    : tls_reader_writer_base(layer),
      buf_size_{buf_size},
      buf_{std::make_unique<decltype(buf_)::element_type[]>(buf_size_)}
  {
    // buffer size cannot exceed INT_MAX since SSL_read uses int
    if (buf_size_ > INT_MAX)
      throw std::invalid_argument{"buf_size parameter cannot exceed INT_MAX"};
  }

  /**
   * Return size of reader buffer.
   */
  auto buf_size() const noexcept { return buf_size_; }

  /**
   * Read all received messages bytes and write them to a stream.
   *
   * @note Under HTTP standard the socket read end is not automatically closed.
   *
   * @tparam CharT Char type
   * @tparam Traits Char traits
   */
  template <typename CharT, typename Traits>
  pdnnet::optional_error operator()(std::basic_ostream<CharT, Traits>& out) const
  {
    // read chunks through layer until done
    do {
      auto n_read = SSL_read(layer(), buf_.get(), static_cast<int>(buf_size_));
      // if unsuccessful, continue if we can retry
      if (n_read <= 0) {
        auto err = SSL_get_error(layer(), n_read);
        // can read some more, just retry
        if (err == SSL_ERROR_WANT_READ) {
          // no retry allowed
          if (!allow_retry())
            return "TLS read retryable but reader has disabled retries";
          // non-null sink
          if (message_sink())
            *message_sink() << "TLS read failed: retrying..."  << std::endl;
          continue;
        }
        // else give up
        return pdnnet::openssl_ssl_error_string(err, "TLS read failed");
      }
      // read is successful so write (assumes no ragged reads)
      out.write(reinterpret_cast<const CharT*>(buf_.get()), n_read / sizeof(CharT));
    }
    while (SSL_has_pending(layer()));
    // done, no error
    return {};
  }

private:
  std::size_t buf_size_;
  std::unique_ptr<unsigned char[]> buf_;
};
#endif  // PDNNET_UNIX

}  // namespace

PDNNET_ARG_MAIN
{
  PDNNET_CLIOPT_PARSE_OPTIONS();
  // create IPv4 TCP/IP client + attempt connection. HTTPS is port 443
  pdnnet::ipv4_client client{};
  client.connect(PDNNET_CLIOPT(host), 443).exit_on_error();
  // on Windows, attempt to perform handshake via Schannel
#if defined(_WIN32)
  // credential handle to get credential
  pdnnet::unique_cred_handle cred;
  // Schannel credentials struct. not using SCH_CREDENTIALS since it refuses
  // to be defined correctly despite the documentation
  auto sc_cred = pdnnet::create_schannel_cred(
    SCH_USE_STRONG_CRYPTO |
    SCH_CRED_AUTO_CRED_VALIDATION |  // enabled by default
    SCH_CRED_NO_DEFAULT_CREDS
  );
  // acquire credential handle for Schannel, exit on error
  pdnnet::acquire_schannel_creds(cred, sc_cred).exit_on_error();
  // security context to build
  pdnnet::unique_ctxt_handle context;
  // build security context by performing TLS handshake
  schannel_perform_handshake(context, client.socket(), cred).exit_on_error();
  std::cout << "TLS handshake with " << PDNNET_CLIOPT(host) << " completed" <<
    std::endl;
  // get stream size limits from context
  SecPkgContext_StreamSizes sc_sizes;
  auto status = QueryContextAttributes(
    context, SECPKG_ATTR_STREAM_SIZES, &sc_sizes
  );
  PDNNET_ERROR_EXIT_IF(
    (status != SEC_E_OK),
    pdnnet::windows_error(status, "Failed to get stream size limits").c_str()
  );
  // max message size
  std::cout << "Max TLS message size: " << sc_sizes.cbMaximumMessage << std::endl;
  // TODO: make EncryptMessage and DecryptMessage calls for communication
  // HTTPS request logic on *nix only for now
#else
  // create OpenSSL TLS layer using default context + attempt to connect
  pdnnet::unique_tls_layer layer{pdnnet::default_tls_context()};
  layer.handshake(client.socket()).exit_on_error();
  // HTTP/1.1 GET request we will make
  auto get_request = http_get_request(PDNNET_CLIOPT(host), PDNNET_CLIOPT(path));
  // print TLS version and request if verbose
  if (PDNNET_CLIOPT(verbose))
    std::cout << PDNNET_PROGRAM_NAME << ": Using " << layer.protocol_string() <<
      ". Making request...\n" << get_request << std::endl;
  // write request to server
  tls_writer{layer}.message_sink(&std::cout)(get_request).exit_on_error();
  // read contents from server until no more pending and print to stdout. under
  // HTTP standard the socket is not be closed automatically
  tls_reader{layer}.message_sink(&std::cout)(std::cout).exit_on_error();
#endif  // !defined(_WIN32)
  return EXIT_SUCCESS;
}
