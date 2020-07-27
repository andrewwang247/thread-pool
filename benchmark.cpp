/*
Demo for thread pool.

Copyright 2020. Siwei Wang.
*/

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <vector>
#include "framework.h"
#include "pool.h"

using std::accumulate;
using std::cout;
using std::flush;
using std::default_random_engine;
using std::for_each;
using std::ios_base;
using std::iota;
using std::transform;
using std::uniform_int_distribution;
using std::vector;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;
using TimeUnit = microseconds;

namespace pool_test {
void test_apply();
void test_apply_get();
void test_map();
void test_map_get();
}  // namespace pool_test

namespace perf_test {
microseconds::rep benchmark_control(const vector<TimeUnit>&);
microseconds::rep benchmark_pool(const vector<TimeUnit>&);
}  // namespace perf_test

/**
 * Generate a random vector of task times.
 *
 * @param sz    The size of the returned vector.
 */
vector<TimeUnit> generate_tasks(size_t sz);

/**
 * A task to consume time, measured in TimeUnit.
 *
 * @param duration    The amount of time to sleep for.
 */
void task(TimeUnit duration);

// Make test cases instead.
int main() {
  ios_base::sync_with_stdio(false);
  {
    cout << "--- UNIT TESTS ---\n";
    Framework tests;
    tests.emplace("apply", pool_test::test_apply);
    tests.emplace("apply_get", pool_test::test_apply_get);
    tests.emplace("map", pool_test::test_map);
    tests.emplace("map_get", pool_test::test_map_get);
    tests.run_all();
    cout << tests << '\n';
  }
  {
    cout << "--- PERFORMANCE BENCHMARK ---\n";
    cout << "Your computer has hardware support for "
         << std::thread::hardware_concurrency() << " threads.\n";
    const auto data = generate_tasks(75'000);
    cout << "All time measurements are made in microseconds.\n\n";

    cout << "Single threaded (std::for_each): " << flush;
    const auto single_time = perf_test::benchmark_control(data);
    cout << single_time << '\n';

    cout << "Thread pool (thread_pool::map): " << flush;
    const auto pool_time = perf_test::benchmark_pool(data);
    cout << pool_time << '\n';

    const auto ratio = single_time / pool_time;
    cout << "The thread pool was around " << ratio << " times faster.\n";
  }
}

void pool_test::test_apply() {
  thread_pool<Policy::TERMINATE> pool(3);
  auto f = []() { sleep_for(milliseconds(15)); };

  auto tic = high_resolution_clock::now();
  for (int i = 0; i < 8; ++i) {
    pool.apply(f);
  }
  pool.join();
  auto toc = high_resolution_clock::now();

  // Given 3 workers and 8 sets of 15 ms waits, we expect it to take ~30 ms.

  auto elapsed = duration_cast<milliseconds>(toc - tic).count();
  unsigned epsilon = 4;
  assert_leq(30 - epsilon, elapsed);
  assert_leq(elapsed, 30 + epsilon);
}

void pool_test::test_apply_get() {
  thread_pool<Policy::JOIN> pool(1);

  auto add = [](int x, int y) { return x + y; };

  auto fut = pool.apply_get(add, 7, 11);
  assert_eq(fut.get(), 18);
}

void pool_test::test_map() {
  thread_pool<Policy::TERMINATE> pool(2);
  auto f = [](unsigned x) { sleep_for(milliseconds(x)); };
  const vector<unsigned> times{10, 10, 10, 10, 10, 10};

  auto tic = high_resolution_clock::now();
  pool.map(f, times.begin(), times.end());
  pool.join();
  auto toc = high_resolution_clock::now();

  auto elapsed = duration_cast<milliseconds>(toc - tic).count();
  unsigned epsilon = 4;
  assert_leq(20 - epsilon, elapsed);
  assert_leq(elapsed, 20 + epsilon);
}

void pool_test::test_map_get() {
  constexpr size_t sz = 50;
  vector<unsigned> nums(sz);
  iota(nums.begin(), nums.end(), 1);

  const auto collatz = [](unsigned x) {
    return x % 2 == 0 ? x / 2 : 3 * x + 1;
  };

  vector<unsigned> expected(sz);
  transform(nums.begin(), nums.end(), expected.begin(), collatz);

  thread_pool<Policy::JOIN> pool;
  auto actual = pool.map_get(collatz, nums.begin(), nums.end());

  for (unsigned i = 0; i < sz; ++i) {
    assert_eq(actual[i].get(), expected[i]);
  }
}

vector<TimeUnit> generate_tasks(size_t sz) {
  const auto seed = high_resolution_clock::now().time_since_epoch().count();
  default_random_engine gen(seed);
  uniform_int_distribution<TimeUnit::rep> distr(60, 75);

  vector<TimeUnit> times;
  times.reserve(sz);
  for (size_t i = 0; i < sz; ++i) {
    times.emplace_back(distr(gen));
  }
  cout << "Generated " << sz << " tasks for benchmarking.\n";
  return times;
}

void task(TimeUnit duration) { sleep_for(duration); }

typename TimeUnit::rep perf_test::benchmark_control(
    const vector<TimeUnit>& data) {
  const auto tic = high_resolution_clock::now();
  for_each(data.begin(), data.end(), task);
  const auto toc = high_resolution_clock::now();
  return duration_cast<TimeUnit>(toc - tic).count();
}

typename TimeUnit::rep perf_test::benchmark_pool(const vector<TimeUnit>& data) {
  thread_pool<Policy::TERMINATE> pool;
  const auto tic = high_resolution_clock::now();
  pool.map(task, data.begin(), data.end());
  const auto toc = high_resolution_clock::now();
  return duration_cast<TimeUnit>(toc - tic).count();
}
