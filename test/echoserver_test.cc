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
#include <vector>

#include <gtest/gtest.h>

#include "pdnnet/client.hh"
#include "pdnnet/socket.hh"

namespace {

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
    return std::chrono::milliseconds{1000};
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
    return std::chrono::milliseconds{5000};
  }
};

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
    return "Server failed to respond within " +
      std::to_string(timeout.count()) + " ms";
  // return string data
  return pdnnet::client_reader{client};
}

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

}  // namespace
