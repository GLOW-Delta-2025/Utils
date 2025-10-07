#include "CmdLib.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cmdlib;

namespace {

void expect_true(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void expect_eq(const std::string &actual, const std::string &expected, const std::string &label) {
  if (actual != expected) {
    throw std::runtime_error(label + " | expected: '" + expected + "' got: '" + actual + "'");
  }
}

void expect_build_round_trip() {
  Command cmd;
  cmd.type = "CONFIRM";
  cmd.command = "SEND_STAR";
  cmd.setParam("speed", "3");
  cmd.setParam("color", "red");
  cmd.setParam("brightness", "80");
  cmd.setParam("size", "10");

  const std::string serialized = cmd.toString();
  expect_true(serialized.rfind("!!CONFIRM:SEND_STAR:{", 0) == 0, "Serialized command should start with header");
  expect_true(serialized.find("speed=3") != std::string::npos, "Serialized command should contain speed parameter");
  expect_true(serialized.find("color=red") != std::string::npos, "Serialized command should contain color parameter");
  expect_true(serialized.find("brightness=80") != std::string::npos, "Serialized command should contain brightness parameter");
  expect_true(serialized.find("size=10") != std::string::npos, "Serialized command should contain size parameter");

  Command roundTrip;
  std::string error;
  expect_true(parse(serialized, roundTrip, error), "Round-trip parse should succeed");
  expect_eq(roundTrip.type, "CONFIRM", "Round-trip type");
  expect_eq(roundTrip.command, "SEND_STAR", "Round-trip command");
  expect_eq(roundTrip.getParam("size"), "10", "Round-trip size param");
}

void expect_parse_with_flags() {
  const std::string payload = "!!ALERT:RAISE:{flag,level=5,with spaces=hello world}##";
  Command parsed;
  std::string error;
  expect_true(parse(payload, parsed, error), "Parse should succeed");
  expect_eq(parsed.type, "ALERT", "Type");
  expect_eq(parsed.command, "RAISE", "Command");
  expect_eq(parsed.getParam("flag"), "", "Flag should produce empty value");
  expect_eq(parsed.getParam("level"), "5", "Level param");
  expect_eq(parsed.getParam("with spaces"), "hello world", "Param with spaces");
}

void expect_error_detection() {
  Command parsed;
  std::string error;
  expect_true(!parse("!BAD:FORMAT:{key=val}##", parsed, error), "Missing prefix should fail");
  expect_true(error.find("prefix") != std::string::npos, "Error message should mention prefix");

  expect_true(!parse("!!TYPE:CMD key=val}##", parsed, error), "Missing braces should fail");
  expect_true(error.find("brace") != std::string::npos, "Error message should mention braces");

  expect_true(!parse("!!TYPE:{key=val}##", parsed, error), "Missing command should fail");
  expect_true(error.find("Header") != std::string::npos, "Error message should mention header");
}

void expect_clear_and_overwrite() {
  Command cmd;
  cmd.type = "INIT";
  cmd.command = "SETUP";
  cmd.setParam("foo", "bar");
  cmd.clear();

  expect_eq(cmd.type, "", "Type cleared");
  expect_eq(cmd.command, "", "Command cleared");
  expect_eq(cmd.getParam("foo", "default"), "default", "Params cleared");

  cmd.type = "CONFIRM";
  cmd.command = "DONE";
  cmd.setParam("foo", "first");
  cmd.setParam("foo", "second");
  expect_eq(cmd.getParam("foo"), "second", "Latest value wins");
}

} // namespace

int main() {
  struct TestCase {
    std::string name;
    std::function<void()> fn;
  };

  const std::vector<TestCase> tests = {
      {"build round-trip", expect_build_round_trip},
      {"parse with flags", expect_parse_with_flags},
      {"error detection", expect_error_detection},
      {"clear and overwrite", expect_clear_and_overwrite},
  };

  int passed = 0;
  int failed = 0;

  for (const auto &test : tests) {
    try {
      test.fn();
      std::cout << "[PASS] " << test.name << '\n';
      ++passed;
    } catch (const std::exception &ex) {
      std::cout << "[FAIL] " << test.name << " -> " << ex.what() << '\n';
      ++failed;
    }
  }

  std::cout << "\nSummary: " << passed << " passed, " << failed << " failed" << std::endl;
  return failed == 0 ? 0 : 1;
}
