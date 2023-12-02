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
#include <cstring>
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
 * Return string error message corresponding to a Windows Sockets error code.
 *
 * @param wsa_err Windows Sockets error code
 */
inline auto winsock_error_string(int wsa_err)
{
  return std::system_category().message(wsa_err);
}

/**
 * Return string describing the Windows Sockets error.
 */
inline auto winsock_error() { return winsock_error_string(WSAGetLastError()); }

/**
 * Return string describing the Windows Sockets error, prefixed with a message.
 *
 * @param wsa_err Windows Sockets error code
 * @param message Message to prefix the Windows Sockets error description with
 */
inline auto winsock_error(int wsa_err, const std::string& message)
{
  return message + ": " + winsock_error_string(wsa_err);
}

/**
 * Return string describing last Windows Sockets error, prefixed with a message.
 *
 * @param message Message to prefix the Windows Sockets error description with
 */
inline auto winsock_error(const std::string& message)
{
  return winsock_error(WSAGetLastError(), message);
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
  return winsock_error(err, message);
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

}  // namespace pdnnet

#endif  // PDNNET_ERROR_HH_
