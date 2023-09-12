/**
 * @file error.h++
 * @author Derek Huang
 * @brief C++ header for error handling helpers
 * @copyright MIT License
 */

#ifndef PDNNET_ERRNO_H_PP
#define PDNNET_ERRNO_H_PP

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif  // _WIN32

#include <cerrno>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <sstream>
#endif  // _WIN32

namespace pdnnet {

/**
 * Return string describing the given error code.
 *
 * @param error `errno` error code
 */
inline std::string
error_string(int error) { return std::strerror(error); }

/**
 * Return string describing the given error code, prefixed with a message.
 *
 * @param error `errno` error code
 * @param message Message to prefix `errno` error description with
 */
inline auto
error_string(int error, const std::string& message)
{
  return message + ": " + error_string(error);
}

/**
 * Return string describing the `errno` error value, prefixed with a message.
 *
 * @param message Message to prefix `errno` error description with
 */
inline auto
errno_error(const std::string& message)
{
  return error_string(errno, message);
}

/**
 * Return string giving the `errno` error description.
 */
inline auto
errno_error() { return error_string(errno); }

#ifdef _WIN32
/**
 * Return string error message with corresponding `HRESULT` in hex.
 *
 * @param error `HRESULT` status value
 * @param message Message describing the current error
 */
inline auto
hresult_error(HRESULT error, const std::string& message)
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
inline auto
hresult_error(const std::string& message)
{
  return hresult_error(HRESULT_FROM_WIN32(GetLastError()), message);
}
#endif  // _WIN32

}  // namespace pdnnet

#endif  // PDNNET_ERRNO_H_PP
