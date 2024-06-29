/**
 * @file echoserver_test.cc
 * @author Derek Huang
 * @brief echoserver.hh integration tests
 * @copyright MIT License
 */

#include "pdnnet/echoserver.hh"

#include <chrono>
#include <future>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "pdnnet/client.hh"
#include "pdnnet/common.h"
#include "pdnnet/socket.hh"
#include "pdnnet/warnings.h"

namespace {

/**
 * `echoserver` base testing fixture for single-connection tests.
 *
 * This is a CRTP template base to allow plugging in of derived classes that
 * will be used as type inputs for Google Test typed tests. Therefore, we can
 * combine the use of both `TEST_P` and `TYPED_TEST`.
 *
 * @tparam TestType Test class type
 */
template <typename TestType>
class ConnectionTest : public ::testing::Test {
public:
  /**
   * Number of pending connections allowed in connection backlog.
   *
   * This must be implemented by CRTP derived classes.
   */
  auto max_pending() const noexcept
  {
    return static_cast<const TestType*>(this)->max_pending();
  }

  /**
   * Connection timeout for a single connection.
   *
   * This must be implemented by CRTP derived classes.
   *
   * @note `std::chrono::duration` ctors are not `noexcept`.
   */
  auto timeout() const
  {
    return static_cast<const TestType*>(this)->timeout();
  }

  /**
   * Number of client connections that will be made.
   *
   * This must be implemented by CRTP derived classes.
   */
  auto connections() const noexcept
  {
    return static_cast<const TestType*>(this)->connections();
  }

protected:
  /**
   * Set up the server tests.
   *
   * Only one thread is allowed to connect at a time.
   */
  void SetUp() override
  {
    // start server on background thread; no connection backlog allowed
    // TODO: consider allowing server to start itself on separate thread
    server_thread_ = std::thread{[this] { server_.start(max_pending()); }};
    // block until server started
    while (!server_.running());
  }

  /**
   * Stop the running server and join.
   */
  void TearDown() override
  {
    server_.stop();
    server_thread_.join();
  }

  pdnnet::echoserver server_;
  std::thread server_thread_;
};

/**
 * Convenience macro for defining a connection test type/test input type.
 *
 * The number of max pending connections is `connections_` minus 1.
 *
 * @param type Test type name
 * @param timeout_ Timeout duration for a single connection
 * @param connections_ Number of connections made. If zero, then the number of
 *  connections used by a test will be `std::thread::hardware_concurrency()`.
 */
#define CONNECTION_TEST(type, timeout_, connections_) \
  class type : public ConnectionTest<type> { \
  public: \
    auto connections() const noexcept \
    { \
      /* suppress MSVC warning that expression is always true. this is expected \
       * because the static assert must be true (else compile error). */ \
  PDNNET_MSVC_WARNING_DISABLE(4296) \
      static_assert(connections_ >= 0, "connection count must be positive"); \
  PDNNET_MSVC_WARNING_ENABLE() \
      /* if zero, use hardware concurrency */ \
      if constexpr (!connections_) \
        return std::thread::hardware_concurrency(); \
      else \
        return connections_; \
    } \
    auto max_pending() const noexcept \
    { \
      return connections() - 1; \
    } \
    auto timeout() const \
    { \
      using namespace std::chrono; \
      /* extra parentheses allows use of chrono literals */ \
      static_assert((timeout_).count() > 0, "timeout must be positive"); \
      return timeout_; \
    } \
  }

/**
 * Single connection.
 */
CONNECTION_TEST(OneConnectTest, 1000ms, 1U);

/**
 * Connections equal to `std::thread::hardware_concurrency()`.
 */
CONNECTION_TEST(AllThreadsTest, 2000ms, 0U);

/**
 * 100 connections to increase server loead.
 */
CONNECTION_TEST(LoadTest, 5000ms, 100U);

TYPED_TEST_SUITE(
  ConnectionTest,
  PDNNET_IDENTITY(::testing::Types<OneConnectTest, AllThreadsTest, LoadTest>)
);

// old test code
#if 0
/**
 * `echoserver` base testing fixture for single-connection tests.
 */
class SingleConnectionTest : public ::testing::Test {
protected:
  /**
   * Set up the server tests.
   *
   * Only one thread is allowed to connect at a time.
   */
  void SetUp() override
  {
    // start server on background thread; no connection backlog allowed
    // TODO: consider allowing server to start itself on separate thread
    server_thread_ = std::thread{[this] { server_.start(max_pending()); }};
    // block until server started
    while (!server_.running());
  }

  /**
   * Stop the running server and join.
   */
  void TearDown() override
  {
    server_.stop();
    server_thread_.join();
  }

  /**
   * Number of pending connections allowed in connection backlog.
   *
   * This is virtual so test subclasses can override it.
   */
  virtual unsigned int max_pending() const noexcept
  {
    return 0U;
  }

  /**
   * Connection timeout.
   *
   * This is virtual so test subclasses can override it.
   *
   * @note `std::chrono::duration` ctors are not `noexcept`.
   */
  virtual std::chrono::milliseconds timeout() const
  {
    using namespace std::chrono;
    return 1000ms;
  }

  pdnnet::echoserver server_;
  std::thread server_thread_;
};

/**
 * Test the echo server and client work properly for a single connection.
 */
TEST_F(SingleConnectionTest, Test)
{
  // create and connect client
  pdnnet::ipv4_client client;
  {
    auto err = client.connect("localhost", server_.port());
    // note: *err branch is never evaluated on success
    ASSERT_FALSE(err) << *err;
  }
  // write data to server
  std::string data{"hello world"};
  {
    auto err = pdnnet::client_writer{client}.read(data.c_str());
    ASSERT_FALSE(err) << *err;
  }
  // block until server response
  ASSERT_TRUE(pdnnet::wait_pollin(client.socket(), timeout())) <<
    "server failed to respond within " << timeout().count() << " ms";
  // check response is identical to data
  EXPECT_EQ(data, static_cast<std::string>(pdnnet::client_reader{client}));
}

/**
 * `echoserver` base testing fixture for multi-connection tests.
 */
class MultiConnectionTest : public SingleConnectionTest {
protected:
  /**
   * Number of pending connections allowed in connection backlog.
   */
  unsigned int max_pending() const noexcept override
  {
    return 100U;
  }

  /**
   * Connection timeout.
   */
  std::chrono::milliseconds timeout() const override
  {
    using namespace std::chrono;
    return 5000ms;
  }
};
#endif  // 0

/**
 * Make a client connection to the localhost echo server.
 *
 * @param port Echo server port number
 * @param message Message to send echo server
 * @param timeout Timeout before considering server response failure
 * @returns The message on success, error info on failure
 */
std::string echo_client_connect(
  pdnnet::inet_port_type port,
  std::string_view message,
  std::chrono::milliseconds timeout)
{
  // create and connect client. since Google Test is thread-safe only
  // on POSIX systems, we return the error text directly on error
  pdnnet::ipv4_client client;
  {
    auto err = client.connect("localhost", port);
    if (err)
      return *err;
  }
  // write message to server
  {
    auto err = pdnnet::client_writer{client}.read(message);
    if (err)
      return *err;
  }
  // block until server response
  if (!pdnnet::wait_pollin(client.socket(), timeout))
    return "No server response within " + std::to_string(timeout.count()) + " ms";
  // return string data
  return pdnnet::client_reader{client};
}

// old test code
#if 0
/**
 * Test the echo server and client work properly for multiple connections.
 *
 * The number of connections opened is equal to the hardware concurrency.
 */
TEST_F(MultiConnectionTest, AllThreadsTest)
{
  using future_type = std::future<std::string>;
  using value_type = decltype(std::declval<future_type>().get());
  // vector of futures holding data to send to server
  std::vector<value_type> messages;
  for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++)
    messages.push_back("hello world " + std::to_string(i));
  // create and connect clients using std::async
  std::vector<future_type> futures;
  for (const auto& message : messages)
    futures.push_back(
      std::async(echo_client_connect, server_.port(), message, timeout())
    );
  // collect futures into separate vector
  decltype(messages) results;
  for (auto& future : futures)
    results.emplace_back(future.get());
  // compare results
  EXPECT_EQ(messages, results);
}
#endif // 0

/**
 * Test that echo server and client work properly for multiple connections.
 *
 * The number of connections opened is determined by the test type.
 *
 * @note On WSL1 Ubuntu 22.04 it has been observed that when the number of
 *  threads is `std::thread::hardware_concurrency()` or higher that around
 *  5-10% of the time, it seems like a `POLLIN` event is not properly returned
 *  by `poll`. This means that sometimes that client's response from the server
 *  is just a `write()` of zero. On native Windows there is absolutely no issue
 *  (or the incidence is extremely rare) in comparison to WSL1.
 */
TYPED_TEST(ConnectionTest, Test)
{
  using future_type = std::future<std::string>;
  using value_type = decltype(std::declval<future_type>().get());
  // vector of futures holding data to send to server
  std::vector<value_type> messages;
  for (unsigned int i = 0; i < this->connections(); i++)
    messages.push_back("hello world " + std::to_string(i));
  // create and connect clients using std::async
  std::vector<future_type> futures;
  for (const auto& message : messages)
    futures.push_back(
      std::async(
        echo_client_connect, this->server_.port(), message, this->timeout()
      )
    );
  // collect futures into separate vector
  decltype(messages) results;
  for (auto& future : futures)
    results.emplace_back(future.get());
  // compare results
  EXPECT_EQ(messages, results);
}

}  // namespace
