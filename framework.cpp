/*
Copyright 2020. Siwei Wang.

Implementation for unit testing framework.
*/
#include "framework.h"
using std::count_if;
using std::exception;
using std::function;
using std::optional;
using std::ostream;
using std::out_of_range;
using std::string;

size_t Framework::total_size() const noexcept { return tests.size(); }

size_t Framework::executed_size() const noexcept { return results.size(); }

bool Framework::contains(const string& name) const noexcept {
  return tests.find(name) != tests.end();
}

bool Framework::executed(const string& name) const noexcept {
  return results.find(name) != results.end();
}

void Framework::emplace(const string& name, const function<void()>& func) {
  tests.emplace(name, func);
  results.erase(name);
}

void Framework::run(const string& name) {
  if (!contains(name)) throw out_of_range("Provided name is not registered.");
  if (executed(name)) return;
  const auto& test = tests.at(name);
  try {
    test();
  } catch (const exception& err) {
    results.emplace(name, err.what());
    return;
  } catch (...) {
    results.emplace(name, "Caught unknown exception.");
    return;
  }
  results.emplace(name, optional<string>());
}

void Framework::run_all() {
  for (const auto& test : tests) run(test.first);
}

bool Framework::passed(const string& name) const {
  const auto& out = results.at(name);
  return !out.has_value();
}

bool Framework::failed(const string& name) const {
  const auto& out = results.at(name);
  return out.has_value();
}

size_t Framework::passed() const noexcept {
  return count_if(results.begin(), results.end(),
                  [](const auto& res) { return !res.second.has_value(); });
}

size_t Framework::failed() const noexcept {
  return count_if(results.begin(), results.end(),
                  [](const auto& res) { return res.second.has_value(); });
}

void Framework::result(const string& name, ostream& os) const {
  if (!contains(name)) throw out_of_range("Provided name is not registered.");
  os << "Test " << name;
  if (!executed(name)) {
    os << " has not been executed.";
  } else if (passed(name)) {
    os << " passed.";
  } else {
    os << " failed.\n";
    os << "\tError: " << error_msg(name);
  }
  os << '\n';
}

ostream& operator<<(ostream& os, const Framework& fr) {
  for (const auto& test : fr.tests) {
    fr.result(test.first, os);
  }
  return os;
}

string Framework::error_msg(const string& name) const {
  const auto& out = results.at(name);
  return out.value();
}

test_error::test_error(const string& message) : msg(message) {}

const char* test_error::what() const throw() { return msg.c_str(); }
