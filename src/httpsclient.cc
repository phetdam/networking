/**
 * @file httpsclient.cc
 * @author Derek Huang
 * @brief Toy HTTPS client that makes a simple GET request
 * @copyright MIT License
 */

#include <cstdint>
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

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define SECURITY_WIN32  // for user-mode security
#include <schannel.h>
#include <security.h>
#else
#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>

#include "pdnnet/tls.hh"
#endif  // !defined(_WIN32)

/**
 * Platform-specific program note.
 *
 * @note Preprocessor directives in macro expansions is undefined behavior.
 */
#if defined(_WIN32)
#define EXTRA_NOTE \
  "\n\n" \
  "WIP on Windows since TLS handshake logic using Schannel is incomplete."
#else
#define EXTRA_NOTE \
  ""
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

#ifdef _WIN32
pdnnet::optional_error schannel_acquire_creds(
  CredHandle& cred, const SCHANNEL_CRED& sc_cred)
{
  // acquire credentials handle for Schannel
  auto status = AcquireCredentialsHandle(
    NULL,
    UNISP_NAME,
    SECPKG_CRED_OUTBOUND,
    NULL,  // pvLogonID
    (PVOID) &sc_cred,
    NULL,  // pGetKeyFn
    NULL,  // pvGetKeyArgument
    &cred,
    NULL
  );
  // if success, no error, otherwise get error string
  if (status == SEC_E_OK)
    return {};
  return pdnnet::windows_error(status);
}

// max TLS message size + overhead for header/mac/padding (overestimated).
// from https://gist.github.com/mmozeiko/c0dfcc8fec527a90a02145d2cc0bfb6d
// constexpr std::size_t max_tls_message_size = 16384 + 512;

pdnnet::optional_error schannel_perform_handshake(
  CtxtHandle& context, pdnnet::socket_handle handle, const CredHandle& cred)
{
  // true if first InitializeSecurityContext call succeeded
  bool building_context = false;
  // socket writer
  pdnnet::socket_writer writer{handle};
  // handshake loop
  while (true) {
    // flags sent and received that indicate requests for the context
    ULONG context_flags =
      ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_REPLAY_DETECT;
    // input security buffers
    SecBuffer input_bufs[2]{};
    input_bufs[0].BufferType = SECBUFFER_TOKEN;
    // TODO: have to set the buffer pointer
    input_bufs[1].BufferType = SECBUFFER_EMPTY;
    // output security buffer
    SecBuffer output_buf{};
    output_buf.BufferType = SECBUFFER_TOKEN;
    // input + output security buffer desc
    SecBufferDesc input_desc{SECBUFFER_VERSION, 2, input_bufs};
    SecBufferDesc output_desc{SECBUFFER_VERSION, 1, &output_buf};
    // perform context initialization/build call
    auto status = InitializeSecurityContext(
      (!building_context) ? const_cast<PCredHandle>(&cred) : NULL,
      NULL,
      // cast away const to please C++ compiler
      const_cast<SEC_CHAR*>(PDNNET_CLIOPT(host)),
      context_flags,
      0,     // Reserved1
      NULL,  // TargetDataRep
      (!building_context) ? NULL : &input_desc,
      0,     // Reserved2
      &context,
      &output_desc,
      &context_flags,
      NULL
    );
    // switch off of status value
    switch (status) {
      // done, return with no error
      case SEC_E_OK:
        break;
      // send token to server and read (part of) a return token
      case SEC_I_CONTINUE_NEEDED:
        // send token to server + free the token buffer when done
        writer.read(output_buf.pvBuffer, output_buf.cbBuffer);
        status = FreeContextBuffer(output_buf.pvBuffer);
        if (status != SEC_E_OK)
          return pdnnet::windows_error(status, "Failed to free token buffer");
        // TODO: add read call here to write to buffer
        break;
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
  CredHandle cred;
  // Schannel credentials struct. not using SCH_CREDENTIALS since it refuses
  // to be defined correctly despite the documentation
  SCHANNEL_CRED sc_cred{};
  sc_cred.dwVersion = SCHANNEL_CRED_VERSION;
  // TLS 1.3 doesn't work correctly in Schannel, need TLS 1.2 or lower
  // sc_cred.grbitEnabledProtocols = SP_PROT_TLS1_3_CLIENT;
  // acquire credential handle for Schannel, exit on error
  schannel_acquire_creds(cred, sc_cred).exit_on_error();
  // security context to use later
  CtxtHandle context;
  // build security context by performing TLS handshake
  schannel_perform_handshake(context, client.socket(), cred).exit_on_error();
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
  // PDNNET_ERROR_EXIT_IF(
  //   SSL_shutdown(layer) < 0,
  //   pdnnet::openssl_error_string("Failed to close write end").c_str()
  // );
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
#endif  // !defined(_WIN32)
  // send simple GET request
  return EXIT_SUCCESS;
}
