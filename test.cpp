/*
Demo for thread pool.

Copyright 2020. Siwei Wang.
*/

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>
#include "framework.h"
#include "pool.h"

using std::atomic;
using std::cout;
using std::endl;
using std::iota;
using std::transform;
using std::vector;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;

namespace pool_test {
void test_apply();
void test_apply_get();
void test_map();
void test_map_get();
}  // namespace pool_test

// Make test cases instead.
int main() {
  {
    thread_pool<Policy::TERMINATE> pool;
    cout << "Hardward support for " << pool.size() << " threads." << endl;
  }
  Framework tests;
  tests.emplace("apply", pool_test::test_apply);
  tests.emplace("apply_get", pool_test::test_apply_get);
  tests.emplace("map", pool_test::test_map);
  tests.emplace("map_get", pool_test::test_map_get);
  tests.run_all();
  cout << tests;
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
  auto collatz = [](unsigned x) { return x % 2 == 0 ? x / 2 : 3 * x + 1; };

  constexpr size_t sz = 50;
  vector<unsigned> nums(sz);
  iota(nums.begin(), nums.end(), 1);
  vector<unsigned> expected(sz);
  transform(nums.begin(), nums.end(), expected.begin(), collatz);

  thread_pool<Policy::TERMINATE> pool;
  auto actual = pool.map_get(collatz, nums.begin(), nums.end());

  for (unsigned i = 0; i < sz; ++i) {
    assert_eq(actual[i].get(), expected[i]);
  }
}
