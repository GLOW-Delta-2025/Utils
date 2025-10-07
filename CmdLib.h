// CmdLib.h
// Simple command builder + parser for format:
// !!type:command:{key=value,key=value}##
//
// Define CMDLIB_ARDUINO (or let ARDUINO be defined) for the Arduino version.

#ifndef CMDLIB_H
#define CMDLIB_H

#if defined(ARDUINO) && !defined(CMDLIB_ARDUINO)
#define CMDLIB_ARDUINO
#endif

#ifdef CMDLIB_ARDUINO
  #include <WString.h>
#else
  #include <string>
  #include <unordered_map>
  #include <sstream>
  #include <cctype>
#endif

namespace cmdlib {

#ifdef CMDLIB_ARDUINO
// -------------------- Arduino Version --------------------
#ifndef CMDLIB_MAX_PARAMS
#define CMDLIB_MAX_PARAMS 12
#endif

struct Param {
  String key;
  String value;
};

struct Command {
  String type;
  String command;
  Param params[CMDLIB_MAX_PARAMS];
  int paramCount = 0;

  void clear() {
    type = command = "";
    for (int i = 0; i < paramCount; ++i) {
      params[i].key = "";
      params[i].value = "";
    }
    paramCount = 0;
  }

  bool setParam(const String &k, const String &v) {
    for (int i = 0; i < paramCount; ++i) {
      if (params[i].key == k) { params[i].value = v; return true; }
    }
    if (paramCount >= CMDLIB_MAX_PARAMS) return false;
    params[paramCount].key = k;
    params[paramCount].value = v;
    paramCount++;
    return true;
  }

  String getParam(const String &k, const String &def = "") const {
    for (int i = 0; i < paramCount; ++i)
      if (params[i].key == k) return params[i].value;
    return def;
  }

  String toString() const {
    String out = "!!";
    out += type; out += ":"; out += command; out += ":{";
    for (int i = 0; i < paramCount; ++i) {
      out += params[i].key; out += "="; out += params[i].value;
      if (i < paramCount - 1) out += ",";
    }
    out += "}##";
    return out;
  }
};

// Utility: trim spaces
static inline String trimStr(const String &s) {
  int start = 0;
  while (start < s.length() && isspace(s.charAt(start))) start++;
  int end = s.length() - 1;
  while (end >= start && isspace(s.charAt(end))) end--;
  if (end < start) return "";
  return s.substring(start, end + 1);
}

// Parse function
static inline bool parse(const String &input, Command &out, String &error) {
  out.clear();
  error = "";

  if (!input.startsWith("!!")) { error = "Missing prefix '!!'"; return false; }
  if (!input.endsWith("##")) { error = "Missing suffix '##'"; return false; }

  int braceOpen = input.indexOf('{');
  int braceClose = input.lastIndexOf('}');
  if (braceOpen == -1 || braceClose == -1 || braceClose < braceOpen) {
    error = "Malformed braces";
    return false;
  }

  String header = input.substring(2, braceOpen);
  if (header.endsWith(":")) header = header.substring(0, header.length() - 1);

  int p1 = header.indexOf(':');
  if (p1 == -1) { error = "Header must be 'type:command'"; return false; }
  out.type = trimStr(header.substring(0, p1));
  out.command = trimStr(header.substring(p1 + 1));

  // Parse params
  String inside = input.substring(braceOpen + 1, braceClose);
  int i = 0;
  while (i < inside.length()) {
    while (i < inside.length() && isspace(inside.charAt(i))) i++;
    if (i >= inside.length()) break;

    int startKey = i;
    while (i < inside.length() && inside.charAt(i) != '=' && inside.charAt(i) != ',') i++;

    if (i >= inside.length() || inside.charAt(i) == ',') {
      String key = trimStr(inside.substring(startKey, i));
      if (key.length() > 0) out.setParam(key, "");
      if (i < inside.length() && inside.charAt(i) == ',') i++;
      continue;
    }

    String key = trimStr(inside.substring(startKey, i));
    i++;
    int startVal = i;
    while (i < inside.length() && inside.charAt(i) != ',') i++;
    String val = trimStr(inside.substring(startVal, i));
    out.setParam(key, val);
    if (i < inside.length() && inside.charAt(i) == ',') i++;
  }

  return true;
}

#else
// -------------------- Standard C++ Version --------------------
using std::string;
using std::unordered_map;

struct Command {
  string type;
  string command;
  unordered_map<string, string> params;

  void clear() { type.clear(); command.clear(); params.clear(); }

  void setParam(const string &k, const string &v) { params[k] = v; }
  string getParam(const string &k, const string &def = "") const {
    auto it = params.find(k);
    return (it == params.end()) ? def : it->second;
  }

  string toString() const {
    std::ostringstream ss;
    ss << "!!" << type << ":" << command << ":{";
    bool first = true;
    for (auto &kv : params) {
      if (!first) ss << ",";
      first = false;
      ss << kv.first << "=" << kv.second;
    }
    ss << "}##";
    return ss.str();
  }
};

static inline string trim(const string &s) {
  size_t a = 0;
  while (a < s.size() && isspace((unsigned char)s[a])) ++a;
  if (a == s.size()) return "";
  size_t b = s.size() - 1;
  while (b > a && isspace((unsigned char)s[b])) --b;
  return s.substr(a, b - a + 1);
}

static inline bool parse(const string &input, Command &out, string &error) {
  out.clear();
  error.clear();

  if (input.rfind("!!", 0) != 0) { error = "Missing prefix '!!'"; return false; }
  if (input.size() < 4 || input.substr(input.size() - 2) != "##") { error = "Missing suffix '##'"; return false; }

  auto braceOpen = input.find('{');
  auto braceClose = input.rfind('}');
  if (braceOpen == string::npos || braceClose == string::npos || braceClose < braceOpen) {
    error = "Malformed braces";
    return false;
  }

  string header = input.substr(2, braceOpen - 2);
  if (!header.empty() && header.back() == ':') header.pop_back();

  size_t p1 = header.find(':');
  if (p1 == string::npos) { error = "Header must be 'type:command'"; return false; }
  out.type = trim(header.substr(0, p1));
  out.command = trim(header.substr(p1 + 1));

  string inside = input.substr(braceOpen + 1, braceClose - (braceOpen + 1));
  size_t i = 0;
  while (i < inside.size()) {
    while (i < inside.size() && isspace((unsigned char)inside[i])) ++i;
    if (i >= inside.size()) break;

    size_t startKey = i;
    while (i < inside.size() && inside[i] != '=' && inside[i] != ',') ++i;

    if (i >= inside.size() || inside[i] == ',') {
      string key = trim(inside.substr(startKey, i - startKey));
      if (!key.empty()) out.setParam(key, "");
      if (i < inside.size() && inside[i] == ',') ++i;
      continue;
    }

    string key = trim(inside.substr(startKey, i - startKey));
    ++i;
    size_t startVal = i;
    while (i < inside.size() && inside[i] != ',') ++i;
    string val = trim(inside.substr(startVal, i - startVal));
    out.setParam(key, val);
    if (i < inside.size() && inside[i] == ',') ++i;
  }

  return true;
}

#endif // CMDLIB_ARDUINO

} // namespace cmdlib

#endif // CMDLIB_H
