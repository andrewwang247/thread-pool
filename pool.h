/*
Thread pool of variable size. Performs work
asynchronously and yields std::future objects.

Copyright 2020. Siwei Wang.
*/

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

// Sets the destructor behavior for thread_pool.
namespace Policy {
// Finish all tasks on pool destruction.
static inline constexpr bool JOIN = true;

// Terminate all threads on pool destruction.
static inline constexpr bool TERMINATE = false;
}  // namespace Policy

/**
 * Thread pool interface.
 */
template <bool Policy = Policy::JOIN>
class thread_pool {
 public:
  /**
   * Initializes number of workers to maximal hardware concurrency.
   */
  thread_pool();

  /**
   * Spawns a predetermined user-specified number of workers.
   *
   * @param num_workers   The number of workers to spawn.
   */
  explicit thread_pool(unsigned num_workers);

  /**
   * Destructor terminates all worker threads.
   */
  ~thread_pool();

  /**
   * Yields the size of the thread pool.
   *
   * @return    The number of workers
   */
  size_t size() const;

  /**
   * Block until tasks queue is empty.
   */
  void join();

  /**
   * Terminates all worker threads.
   */
  void terminate();

  /**
   * Apply a void function with args.
   *
   * @param func    The function to apply
   * @param args    Arguments to func
   */
  template <typename Func, typename... Args>
  void apply(Func&& func, Args&&... args);

  /**
   * Apply a function with args. Get a future to its return value.
   *
   * @param func    The function to apply
   * @param args    Arguments to func
   * @return        A future for func(args...)
   */
  template <typename Func, typename... Args>
  std::future<typename std::result_of<Func(Args...)>::type> apply_get(
      Func&& func, Args&&... args);

  /**
   * Map a void function over a range.
   *
   * REQUIRES: Iter is forward iterable.
   *
   * @param func    The function to map
   * @param begin   The begin iterator to argument range
   * @param end     The end iterator to argument range
   */
  template <typename Func, typename Iter>
  void map(Func&& func, const Iter& begin, const Iter& end);

  /**
   * Map a function over a range.
   *
   * REQUIRES: Iter is forward iterable.
   *
   * @param func    The function to map
   * @param begin   The begin iterator to argument range
   * @param end     The end iterator to argument range
   * @return        A vector containing results
   */
  template <typename Func, typename Iter>
  std::vector<std::future<typename std::result_of<
      Func(typename std::iterator_traits<Iter>::value_type)>::type>>
  map_get(Func&& func, const Iter& begin, const Iter& end);

 private:
  // The worker threads.
  std::vector<std::thread> workers;

  // A queue of tasks to complete.
  std::queue<std::function<void()>> tasks;

  // Lock for tasks queue.
  std::mutex task_mutex;

  // CV to wake up workers.
  std::condition_variable worker_cv;

  // CV to wake up parent thread when joining.
  std::condition_variable join_cv;

  // Signal flag for workers to exit.
  std::atomic_bool stop;
};

template <bool Policy>
thread_pool<Policy>::thread_pool()
    : thread_pool(std::thread::hardware_concurrency()) {}

template <bool Policy>
thread_pool<Policy>::thread_pool(unsigned num_workers) : stop(false) {
  if (num_workers == 0) {
    throw std::length_error("Number of workers must be positive.");
  }
  workers.reserve(num_workers);

  // All workers initialized with the same function.
  const auto worker_func = [this]() {
    while (true) {
      std::function<void()> job;
      // Lock task queue, block while empty (watching for stop signal).
      // Remove top of queue and execute task after releasing lock.
      {
        std::unique_lock<std::mutex> lock(this->task_mutex);
        while (!this->stop.load() && this->tasks.empty()) {
          this->worker_cv.wait(lock);
        }
        if (this->stop.load()) return;
        job = std::move(this->tasks.front());
        this->tasks.pop();
      }
      job();
      this->join_cv.notify_one();
    }
  };

  for (unsigned i = 0; i < num_workers; ++i) {
    workers.emplace_back(worker_func);
  }
}

template <bool Policy>
size_t thread_pool<Policy>::size() const {
  return workers.size();
}

template <bool Policy>
void thread_pool<Policy>::join() {
  if (stop.load()) {
    throw std::runtime_error("Thread pool invoked after it was terminated.");
  }
  std::unique_lock<std::mutex> lock(this->task_mutex);
  while (!tasks.empty()) {
    join_cv.wait(lock);
  }
}

template <bool Policy>
void thread_pool<Policy>::terminate() {
  if (stop.exchange(true)) return;
  worker_cv.notify_all();
  for (auto& worker : workers) {
    worker.join();
  }
}

template <bool Policy>
template <typename Func, typename... Args>
void thread_pool<Policy>::apply(Func&& func, Args&&... args) {
  if (stop.load()) {
    throw std::runtime_error("Thread pool invoked after it was terminated.");
  }
  auto job = std::make_shared<std::packaged_task<void()>>(
      std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
  std::unique_lock<std::mutex> lock(task_mutex);
  tasks.push([job]() { (*job)(); });
  worker_cv.notify_one();
}

template <bool Policy>
template <typename Func, typename... Args>
std::future<typename std::result_of<Func(Args...)>::type>
thread_pool<Policy>::apply_get(Func&& func, Args&&... args) {
  using result_type = typename std::result_of<Func(Args...)>::type;
  if (stop.load()) {
    throw std::runtime_error("Thread pool invoked after it was terminated.");
  }

  auto job = std::make_shared<std::packaged_task<result_type()>>(
      std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

  std::unique_lock<std::mutex> lock(task_mutex);
  tasks.push([job]() { (*job)(); });

  worker_cv.notify_one();
  return job->get_future();
}

template <bool Policy>
template <typename Func, typename Iter>
void thread_pool<Policy>::map(Func&& func, const Iter& begin, const Iter& end) {
  if (stop.load()) {
    throw std::runtime_error("Thread pool invoked after it was terminated.");
  }
  std::unique_lock<std::mutex> lock(task_mutex);
  for (Iter it = begin; it != end; ++it) {
    auto job = std::make_shared<std::packaged_task<void()>>(
        std::bind(std::forward<Func>(func), *it));
    tasks.push([job]() { (*job)(); });
  }

  worker_cv.notify_all();
}

template <bool Policy>
template <typename Func, typename Iter>
std::vector<std::future<typename std::result_of<
    Func(typename std::iterator_traits<Iter>::value_type)>::type>>
thread_pool<Policy>::map_get(Func&& func, const Iter& begin, const Iter& end) {
  using result_type = typename std::result_of<Func(
      typename std::iterator_traits<Iter>::value_type)>::type;
  if (stop.load()) {
    throw std::runtime_error("Thread pool invoked after it was terminated.");
  }
  std::vector<std::future<result_type>> futures;

  std::unique_lock<std::mutex> lock(task_mutex);
  for (Iter it = begin; it != end; ++it) {
    auto job = std::make_shared<std::packaged_task<result_type()>>(
        std::bind(std::forward<Func>(func), *it));
    tasks.push([job]() { (*job)(); });
    futures.push_back(job->get_future());
  }

  worker_cv.notify_all();
  return futures;
}

template <bool Policy>
thread_pool<Policy>::~thread_pool() {
  if (Policy == Policy::JOIN) join();
  terminate();
}
