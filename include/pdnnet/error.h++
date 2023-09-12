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
#include <WinSock2.h>
#undef WIN32_LEAN_AND_MEAN
#endif  // _WIN32

#include <cerrno>
#include <cstring>
#include <string>

// currently only needed on Windows
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

/**
 * Return string error message corresponding to a Windows Sockets error code.
 *
 * If error code is unknown, a generic invalid error code message is returned.
 *
 * @param wsa_err Windows Sockets error code
 */
inline auto
winsock_error_string(int wsa_err)
{
  static std::unordered_map<decltype(wsa_err), std::string> err_map{
    {WSA_INVALID_HANDLE, "Specified event object handle is invalid"},
    {WSA_NOT_ENOUGH_MEMORY, "Insufficient memory available"},
    {WSA_INVALID_PARAMETER, "One or more parameters are invalid"},
    {WSA_OPERATION_ABORTED, "Overlapping operation aborted"},
    {WSA_IO_INCOMPLETE, "Overlapped I/O event object not in signaled state"},
    {WSA_IO_PENDING, "Overlapped operations will complete late"},
    {WSAEINTR, "Blocking operation interrupted by WSACancelBlockingCall"},
    {WSAEBADF, "File handle is not valid"}
  };
  if (err_map.find() != err_map.end())
    return err_map[wsa_err];
  return decltype(err_map)::mapped_type{"Invalid Windows Sockets error code"};
}

/**
 * Return string error message for a Windows Sockets error code.
 *
 * @param wsa_err Windows Sockets error code
 * @param message Message describing the current error
 */
inline auto
winsock_error_string(int wsa_err, const std::string& message)
{
  return message + ": " + winsock_error_string(wsa_err);
}

/**
 * Return string error message describing the `WSAGetLastError` value.
 *
 * @param message Message describing the current error
 */
inline auto
winsock_error(const std::string& message)
{
  return winsock_error_string(WSAGetLastError(), message);
}

/**
 * Return string error message describing the `WSAGetLastError` value.
 */
inline auto
winsock_error() { return winsock_error_string(WSAGetLastError()); }
#endif  // _WIN32

}  // namespace pdnnet

#endif  // PDNNET_ERRNO_H_PP
