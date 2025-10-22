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
  cmd.headers.push_back("MASTER");
  cmd.headers.push_back("ARM#1");
  cmd.msgKind = "CONFIRM";
  cmd.command = "SEND_STAR";
  cmd.setNamed("speed", "3");
  cmd.setNamed("color", "red");
  cmd.setNamed("brightness", "80");
  cmd.setNamed("size", "10");

  const std::string serialized = cmd.toString();
  expect_true(serialized.rfind("!!MASTER:ARM#1:CONFIRM:SEND_STAR{", 0) == 0, "Serialized command should start with full header");
  expect_true(serialized.find("speed=3") != std::string::npos, "Serialized command should contain speed parameter");
  expect_true(serialized.find("color=red") != std::string::npos, "Serialized command should contain color parameter");
  expect_true(serialized.find("brightness=80") != std::string::npos, "Serialized command should contain brightness parameter");
  expect_true(serialized.find("size=10") != std::string::npos, "Serialized command should contain size parameter");

  Command roundTrip;
  std::string error;
  expect_true(parse(serialized, roundTrip, error), "Round-trip parse should succeed");
  expect_eq(roundTrip.msgKind, "CONFIRM", "Round-trip msgKind");
  expect_eq(roundTrip.command, "SEND_STAR", "Round-trip command");
  expect_eq(roundTrip.getNamed("size"), "10", "Round-trip size param");
  expect_eq(roundTrip.getNamed("color"), "red", "Round-trip color param");
  expect_eq(roundTrip.headers[0], "MASTER", "Round-trip header 0");
  expect_eq(roundTrip.headers[1], "ARM#1", "Round-trip header 1");
}

void expect_parse_with_flags() {

  const std::string payload = "!!SRC:DEST:ALERT:RAISE{flag,level=5,with_spaces=hello_world}##";
  Command parsed;
  std::string error;
  expect_true(parse(payload, parsed, error), "Parse should succeed");
  expect_eq(parsed.msgKind, "ALERT", "msgKind");
  expect_eq(parsed.command, "RAISE", "Command");
  expect_eq(parsed.getNamed("flag"), "", "Flag should produce empty value");
  expect_eq(parsed.getNamed("level"), "5", "Level param");
  expect_eq(parsed.getNamed("with_spaces"), "hello_world", "Param with underscores");
  expect_eq(parsed.headers[0], "SRC", "Header SRC");
  expect_eq(parsed.headers[1], "DEST", "Header DEST");
}

void expect_error_detection() {

  Command parsed;
  std::string error;
  expect_true(!parse("!BAD:FORMAT{key=val}##", parsed, error), "Missing prefix should fail");
  expect_true(error.find("prefix") != std::string::npos, "Error message should mention prefix");

  expect_true(!parse("!!TYPE:CMD key=val}##", parsed, error), "Missing braces should fail");
  expect_true(error.find("brace") != std::string::npos || error.find("Malformed") != std::string::npos, "Error message should mention braces");

  expect_true(!parse("!!TYPE{key=val}##", parsed, error), "Missing command should fail");
  expect_true(error.find("header") != std::string::npos, "Error message should mention header");
}

void expect_clear_and_overwrite() {

  Command cmd;
  cmd.headers.push_back("INIT");
  cmd.msgKind = "SETUP";
  cmd.command = "START";
  cmd.setNamed("foo", "bar");
  cmd.clear();

  expect_eq(cmd.msgKind, "", "msgKind cleared");
  expect_eq(cmd.command, "", "Command cleared");
  expect_eq(cmd.getNamed("foo", "default"), "default", "Params cleared");

  cmd.headers.push_back("CONFIRM");
  cmd.msgKind = "DONE";
  cmd.command = "FINISH";
  cmd.setNamed("foo", "first");
  cmd.setNamed("foo", "second");
  expect_eq(cmd.getNamed("foo"), "second", "Latest value wins");
}
void expect_multi_header_parse() {
  const std::string payload = "!!A:B:C:D:KIND:CMD{p1=1,p2=2}##";
  Command parsed;
  std::string error;
  expect_true(parse(payload, parsed, error), "Parse should succeed");
  expect_eq(parsed.msgKind, "KIND", "msgKind");
  expect_eq(parsed.command, "CMD", "Command");
  expect_eq(parsed.getNamed("p1"), "1", "Param p1");
  expect_eq(parsed.getNamed("p2"), "2", "Param p2");
  expect_eq(std::to_string(parsed.headers.size()), "4", "Header count");
  expect_eq(parsed.headers[0], "A", "Header 0");
  expect_eq(parsed.headers[3], "D", "Header 3");
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
     {"multi-header parse", expect_multi_header_parse},
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
