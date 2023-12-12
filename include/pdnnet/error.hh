/**
 * @file error.hh
 * @author Derek Huang
 * @brief C++ header for error handling helpers
 * @copyright MIT License
 */

#ifndef PDNNET_ERROR_HH_
#define PDNNET_ERROR_HH_

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#endif  // _WIN32

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

// currently only needed on Windows
#ifdef _WIN32
#include <sstream>
#include <system_error>
#endif  // _WIN32

namespace pdnnet {

/**
 * Return string describing the given `errno` error.
 *
 * @param error `errno` error code
 */
inline std::string errno_string(int error) { return std::strerror(error); }

/**
 * Return string describing the given `errno` error, prefixed with a message.
 *
 * @param error `errno` error code
 * @param message Message to prefix `errno` error description with
 */
inline auto errno_string(int error, const std::string& message)
{
  return message + ": " + errno_string(error);
}

/**
 * Return string describing the `errno` error value, prefixed with a message.
 *
 * @param message Message to prefix `errno` error description with
 */
inline auto errno_error(const std::string& message)
{
  return errno_string(errno, message);
}

/**
 * Return string giving the `errno` error description.
 */
inline auto errno_error() { return errno_string(errno); }

#ifdef _WIN32
/**
 * Return string error message with corresponding `HRESULT` in hex.
 *
 * @param error `HRESULT` status value
 * @param message Message describing the current error
 */
inline auto hresult_error(HRESULT error, const std::string& message)
{
  std::stringstream ss;
  ss << message << ". HRESULT: " << std::hex << error;
  return ss.str();
}

/**
 * Return string error message using `HRESULT` from `GetLastError`.
 *
 * @param message Message describing the current error
 */
inline auto hresult_error(const std::string& message)
{
  return hresult_error(HRESULT_FROM_WIN32(GetLastError()), message);
}

/**
 * Return string error message corresponding to a Windows error code.
 *
 * @param err Windows error code, e.g. from Windows Sockets, HRESULT, etc.
 */
inline auto windows_error(int err)
{
  return std::system_category().message(err);
}

/**
 * Return string describing the Windows error, prefixed with a message.
 *
 * @param err Windows error code, e.g. from Windows Sockets, HRESULT, etc.
 * @param message Message to prefix the Windows error description with
 */
inline auto windows_error(int err, const std::string& message)
{
  return message + ": " + windows_error(err);
}

/**
 * Return string describing the last Windows Sockets error.
 */
inline auto winsock_error() { return windows_error(WSAGetLastError()); }

/**
 * Return string describing last Windows Sockets error, prefixed with a message.
 *
 * @param message Message to prefix the Windows Sockets error description with
 */
inline auto winsock_error(const std::string& message)
{
  return windows_error(WSAGetLastError(), message);
}
#endif  // _WIN32

/**
 * Return string describing the socket error, prefixed with a message.
 *
 * On Windows, the socket error value should be a Windows Sockets error value,
 * e.g. from `WSAGetLastError`, otherwise an `errno` value on *nix.
 *
 * @param err Socket error value
 * @param mesage Message to prefix the socket error description with
 */
inline auto socket_error(int err, const std::string& message)
{
#if defined(_WIN32)
  // TODO: consider just using this as a generic error message functio
  return windows_error(err, message);
#else
  return errno_string(err, message);
#endif  // !defined(_WIN32)
}

/**
 * Return string describing the last socket error, prefixed with a message.
 *
 * On Windows `WSAGetLastError()` is used while for *nix `errno` is used.
 *
 * @param message Message to prefix the socket error description with
 */
inline auto socket_error(const std::string& message)
{
#if defined(_WIN32)
  return winsock_error(message);
#else
  return errno_error(message);
#endif  // !defined(_WIN32)
}

/**
 * Return string describing the last socket error.
 *
 * On Windows `WSAGetLastError()` is used while for *nix `errno` is used.
 */
inline auto socket_error()
{
#if defined(_WIN32)
  return winsock_error();
#else
  return errno_error();
#endif  // !defined(_WIN32)
}

namespace detail {

/**
 * Base class for the `optional_error`.
 *
 * Provides the main functionality used by the `optional_error`.
 */
using optional_error_base = std::optional<std::string>;

}  // namespace detail

/**
 * Error message wrapper that may or may not contain an error message.
 *
 * Can be used as a return value by methods to deliver a string error message
 * on error while automatically throwing or exiting on error.
 */
class optional_error : public detail::optional_error_base {
public:
  using detail::optional_error_base::optional_error_base;

  /**
   * If an error message is contained, exit with `EXIT_FAILURE`.
   */
  void exit_on_error() const
  {
    if (has_value()) {
      std::cerr << "Error: " << value() << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  /**
   * If an error message is contained, throw the specified exception type.
   *
   * @tparam ExceptionType Exception type, default `std::runtime_error`
   */
  template <typename ExceptionType = std::runtime_error>
  void throw_on_error() const noexcept(noexcept(ExceptionType{value()}))
  {
    if (has_value())
      throw ExceptionType{value()};
  }
};

}  // namespace pdnnet

#endif  // PDNNET_ERROR_HH_
