/**
 * @file memory.hh
 * @author Derek Huang
 * @brief C++ header for memory management helpers
 * @copyright MIT License
 */

#ifndef PDNNET_MEMORY_HH_
#define PDNNET_MEMORY_HH_

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <type_traits>

namespace pdnnet {

/**
 * Type alias for a single raw byte of data.
 */
using byte = unsigned char;

/**
 * Byte buffer class template with unique ownership.
 *
 * @tparam Deleter Buffer deleter, defaults to `std::default_delete<byte>`
 */
template <typename Deleter = std::default_delete<byte[]>>
class byte_buffer {
public:
  // deleter consistency check
  static_assert(std::is_invocable_v<Deleter, byte*>);

  /**
   * Default ctor.
   *
   * Constructs a null buffer with size zero.
   */
  byte_buffer() : byte_buffer{nullptr, 0u} {}

  /**
   * Ctor.
   *
   * @param size Number of bytes to allocate using `operator new[]`
   */
  byte_buffer(std::size_t size) : buf_{new byte[size]}, size_{size} {}

  /**
   * Ctor.
   *
   * @param buf Allocated buffer to be deleted via `Deleter` to own
   * @param size Number of bytes in the allocated buffer
   */
  explicit byte_buffer(const byte* buf, std::size_t size) noexcept
    : buf_{buf}, size_{size}
  {}

  /**
   * Deleted copy ctor.
   */
  byte_buffer(const byte_buffer&) = delete;

  /**
   * Move ctor.
   */
  byte_buffer(byte_buffer&& other) noexcept
    : byte_buffer{other.release(), other.size()}
  {}

  /**
   * Move assignment operator.
   */
  auto& operator=(byte_buffer&& other) noexcept
  {
    buf_ = std::move(other.buf_);
    size_ = other.size_;
    return *this;
  }

  /**
   * Return the `unique_ptr` managing the buffer.
   */
  const auto& buf() const noexcept { return buf_; }

  /**
   * Return number of bytes in the buffer.
   */
  auto size() const noexcept { return size_; }

  /**
   * Return pointer to the first byte in the bfufer.
   */
  auto get() const noexcept
  {
    return buf_.get();
  }

  /**
   * Release ownership of the buffer.
   */
  byte* release() noexcept
  {
    return buf_.release();
  }

  /**
   * Implicitly convert the `byte_buffer` into a `byte*`.
   */
  operator byte*() const noexcept
  {
    return buf_.get();
  }

private:
  std::unique_ptr<byte[], Deleter> buf_;
  std::size_t size_;
};

}  // namespace pdnnet

#endif  // PDNNET_MEMORY_HH_
