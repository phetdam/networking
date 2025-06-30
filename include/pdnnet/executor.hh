/**
 * @file executor.hh
 * @author Derek Huang
 * @brief C++ task executors
 * @copyright MIT License
 */

#ifndef PDNNET_EXECUTOR_HH_
#define PDNNET_EXECUTOR_HH_

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace pdnnet {

/**
 * Thread-based task executor.
 *
 * This executor manages a pool of worker threads that are notified of new
 * tasks as they arrive. These tasks are run in a FIFO order at a fixed level
 * of concurrency that can be specified during executor creation.
 */
class thread_executor {
public:
  using task_type = std::function<void()>;

  /**
   * Default ctor.
   *
   * The system's hardware concurrency will be used as the thread count.
   */
  thread_executor() : thread_executor{std::thread::hardware_concurrency()} {}

  /**
   * Ctor.
   *
   * Create an executor with a given number of worker threads.
   *
   * @param n_threads Number of worker threads to reserve
   */
  thread_executor(unsigned n_threads) : threads_{n_threads}
  {
    start();
  }

  /**
   * Dtor.
   *
   * Join all threads and ignore any errors.
   */
  ~thread_executor()
  {
    // stop and join all threads
    try {
      stop();
      join();
    }
    catch (const std::system_error&) {
      // TODO: add noexcept logging or printf to stderr
    }
  }

  /**
   * Indicate if the worker threads are running or not.
   *
   * @note This member function is thread-safe.
   */
  bool running() const
  {
    std::lock_guard lk{mut_};
    return running_;
  }

  /**
   * Start all worker thread event loops.
   *
   * If the worker threads are already running then nothing is done.
   *
   * @note This function is thread-safe.
   */
  void start()
  {
    std::lock_guard lk{mut_};
    // running, so break
    if (running_)
      return;
    // otherwise, mark as running
    running_ = true;
    // reset worker threads
    for (auto& thread : threads_) {
      thread = std::thread{
        // worker thread loop
        [this]
        {
          // empty task to move assign to
          task_type task;
          // event loop
          while (true) {
            // wait until there are pending tasks or we stopped running
            {
              std::unique_lock lk{mut_};
              cond_.wait(lk, [this] { return !tasks_.empty() || !running_; });
              // if not running, terminate loop
              if (!running_)
                return;
              // otherwise, obtain task from the queue
              task = std::move(tasks_.front());
              tasks_.pop_front();
            }
            // after unlocking we execute task
            task();
          }
        }
      };
    }
  }

  /**
   * Issue a stop to all worker thread event loops.
   *
   * @note This function is thread-safe.
   *
   * @todo Return `*this` when we have a `start()` member function.
   */
  void stop()
  {
    // mark as not running
    {
      std::lock_guard lk{mut_};
      running_ = false;
    }
    // ensure all workers are notified of change
    cond_.notify_all();
  }

  /**
   * Schedule a task for execution.
   *
   * @note This function is thread-safe.
   *
   * @todo Decide whether posting tasks when not running is allowed.
   */
  template <typename F, typename = std::enable_if_t<std::is_invocable_r_v<void, F>> >
  auto& post(F&& func)
  {
    // post new task
    {
      std::lock_guard lk{mut_};
      tasks_.push_back(std::forward<F>(func));
    }
    // notify a thread about the new task
    // TODO: what if a thread that is already running a task is notified?
    cond_.notify_one();
    return *this;
  }

  /**
   * Return the number of worker threads.
   *
   * @note This function is thread-safe.
   */
  auto workers() const
  {
    std::lock_guard lk{mut_};
    return threads_.size();
  }

private:
  // task deque, mutex, and condition variable for scheduling on threads. the
  // mutex is used to control all state read/write
  std::deque<task_type> tasks_;
  std::mutex mut_;
  std::condition_variable cond_;
  // worker threads
  std::vector<std::thread> threads_;
  // indicate if executor is running or not
  bool running_{};

  /**
   * Join all worker threads.
   *
   * If `stop()` was not called the calling thread will be blocked forever.
   */
  void join()
  {
    for (auto& thread : threads_)
      thread.join();
  }
};

}  // namespace pdnnet

#endif  // PDNNET_EXECUTOR_HH_
