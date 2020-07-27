# Thread Pool

The class `thread_pool` is an efficient FIFO thread pool for modern C++. This project was inspired by Python's `multiprocessing.Pool`. To use, simply `#include "pool.h"`.

## Construction

The thread pool may be specified to either join or terminate on destruction.

- Joined thread pools will finish all tasks in their queue before terminating.
- Terminated thread pools will stop as soon as the threads are done with their current tasks.

The constructor for `thread_pool` also takes an `unsigned` argument to specify the number of worker threads to spawn. By default, this number is `std::thread::hardware_concurrency()`.

## Applying

To apply a function `f` to arguments `args...`, use either `apply` (if `f` returns `void`) or `apply_get` (if `f` has a return value).

Calling `pool.apply(f, args...)` will schedule the task of running `f(args...)` onto the task queue. Then `apply_get` takes this concept further by returning a `std::future` with the type of `f(args...)`.

## Mapping

To apply a function `f` over a range `[begin, end)` of arguments, use either `map` (if `f` returns void) or `map_get` (if `f` has a return value).

Calling `pool.map(f, begin, end)` will schedule the task of running `f(*it)` for each `begin <= it < end`. On top of this, using `map_get` will yield a `std::vector` of `std::future` each with the type of `f(*it)`. The size of this `std::vector` is the same as `std::distance(begin, end)`.

## Testing

The `benchmark.cpp` file includes 4 unit tests for both versions of applying and mapping. It also features benchmarks the performance of the thread library by pitting the multi-threaded `thread_pool::map` against the single-threaded `std::for_each`.
