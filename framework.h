/*
Copyright 2020. Siwei Wang.

Interface for user-friendly testing framework.
*/
#pragma once
#include <algorithm>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <utility>

/* --- USER LEVEL ASSERTION STATEMENTS --- */

/**
 * Checks that Pred is true.
 * THROWS: test_error with the given message string.
 * Must be used in conjunction with the framework.
 */
template <typename Pred>
void assert_true(Pred, const std::string& = "Default assert_true message.");

/**
 * Checks that Pred is true.
 * THROWS: test_error with the given message string.
 * Must be used in conjunction with the framework.
 */
template <typename Pred>
void assert_false(Pred, const std::string& = "Default assert_false message.");

/**
 * Checks that Obj1 and Obj2 are equal.
 * THROWS: test_error with the given message string.
 * Must be used in conjunction with the framework.
 */
template <typename Obj1, typename Obj2>
void assert_eq(Obj1, Obj2, const std::string& = "Default assert_eq message.");

/**
 * Checks that Obj1 and Obj2 are not equal.
 * THROWS: test_error with the given message string.
 * Must be used in conjunction with the framework.
 */
template <typename Obj1, typename Obj2>
void assert_neq(Obj1, Obj2, const std::string& = "Default assert_neq message.");

/**
 * Checks that Obj1 is less than Obj2.
 * THROWS: test_error with the given message string.
 * Must be used in conjunction with the framework.
 */
template <typename Obj1, typename Obj2>
void assert_less(Obj1, Obj2,
                 const std::string& = "Default assert_less message.");

/**
 * Checks that Obj1 is less than or equal to Obj2.
 * THROWS: test_error with the given message string.
 * Must be used in conjunction with the framework.
 */
template <typename Obj1, typename Obj2>
void assert_leq(Obj1, Obj2, const std::string& = "Default assert_leq message.");

/**
 * Checks that Obj1 is greater than Obj2.
 * THROWS: test_error with the given message string.
 * Must be used in conjunction with the framework.
 */
template <typename Obj1, typename Obj2>
void assert_greater(Obj1, Obj2,
                    const std::string& = "Default assert_greater message.");

/**
 * Checks that Obj1 is greater than or equal to Obj2.
 * THROWS: test_error with the given message string.
 * Must be used in conjunction with the framework.
 */
template <typename Obj1, typename Obj2>
void assert_geq(Obj1, Obj2, const std::string& = "Default assert_geq message.");

/**
 * A custom excpetion class used by the framework.
 */
class test_error : public std::exception {
 private:
  const std::string msg;

 public:
  explicit test_error(const std::string&);
  const char* what() const throw();
};

/* --- FRAMEWORK INTERFACE --- */

/**
 * Stores test functions and their names.
 * Executes tests on command.
 * Allows for querying of results.
 * Runs tests in alphabetic order by name.
 */
class Framework {
 private:
  // Match function names to the corresponding unit test.
  std::map<std::string, std::function<void()>> tests;

  // If failed, associated test name with error message.
  std::map<std::string, std::optional<std::string>> results;

 public:
  /**
   * Returns the number of tests registered by the framework.
   */
  size_t total_size() const noexcept;

  /**
   * Returns the number of tests that have been executed.
   */
  size_t executed_size() const noexcept;

  /**
   * Returns whether or not test name is
   * registered by the framework.
   */
  bool contains(const std::string&) const noexcept;

  /**
   * Returns whether or not the test has been
   * executed by the framework.
   */
  bool executed(const std::string&) const noexcept;

  /**
   * Associate the function name with the unit test.
   * Note that this function will overwrite.
   */
  void emplace(const std::string&, const std::function<void()>&);

  /**
   * Execute the given function and record its result.
   * Does not execute if the test has already been run.
   */
  void run(const std::string&);

  /**
   * Executes all tests registered by the framework.
   * Skips tests that have already been run.
   */
  void run_all();

  /**
   * Returns the result associated with this name.
   * THROWS: if the test has not been run.
   */
  bool passed(const std::string&) const;

  /**
   * Returns the result associated with this name.
   * THROWS: if the test has not been run.
   */
  bool failed(const std::string&) const;

  /**
   * Returns the error message associated with this name.
   * THROWS: if the test has not been run or it passed.
   */
  std::string error_msg(const std::string&) const;

  /**
   * Returns the number of tests that passed.
   */
  size_t passed() const noexcept;

  /**
   * Returns the number of tests that failed.
   */
  size_t failed() const noexcept;

  // NOTE: passed() + failed() == executed().

  /**
   * Formats the result of the given test to the ostream.
   */
  void result(const std::string&, std::ostream&) const;

  /**
   * Formats the result of all tests to the ostream.
   */
  friend std::ostream& operator<<(std::ostream&, const Framework&);
};

/* --- TEMPLATE FUNCTION IMPLEMENTATIONS --- */

template <typename Pred>
inline void assert_true(Pred p, const std::string& msg) {
  if (!p) throw test_error(msg);
}

template <typename Pred>
inline void assert_false(Pred p, const std::string& msg) {
  if (p) throw test_error(msg);
}

template <typename Obj1, typename Obj2>
inline void assert_eq(Obj1 X, Obj2 Y, const std::string& msg) {
  if (!(X == Y)) throw test_error(msg);
}

template <typename Obj1, typename Obj2>
inline void assert_neq(Obj1 X, Obj2 Y, const std::string& msg) {
  if (!(X != Y)) throw test_error(msg);
}

template <typename Obj1, typename Obj2>
inline void assert_less(Obj1 X, Obj2 Y, const std::string& msg) {
  if (!(X < Y)) throw test_error(msg);
}

template <typename Obj1, typename Obj2>
inline void assert_leq(Obj1 X, Obj2 Y, const std::string& msg) {
  if (!(X <= Y)) throw test_error(msg);
}

template <typename Obj1, typename Obj2>
inline void assert_greater(Obj1 X, Obj2 Y, const std::string& msg) {
  if (!(X > Y)) throw test_error(msg);
}

template <typename Obj1, typename Obj2>
inline void assert_geq(Obj1 X, Obj2 Y, const std::string& msg) {
  if (!(X >= Y)) throw test_error(msg);
}
