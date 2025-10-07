// CmdLib.h
// Simple command builder + parser for format:
// !!device:type:command:{key=val,key2=val2}##
//
// Define CMDLIB_ARDUINO (or let ARDUINO be defined) to compile the Arduino-friendly small-footprint version.
// Otherwise the STL version is used.

#ifndef CMDLIB_H
#define CMDLIB_H

// ---------------- CONFIG ----------------
// #define CMDLIB_ARDUINO   // uncomment to force Arduino mode if needed
// ----------------------------------------

#if defined(ARDUINO) && !defined(CMDLIB_ARDUINO)
#define CMDLIB_ARDUINO
#endif

#ifdef CMDLIB_ARDUINO
  #include <WString.h> // Arduino String
#else
  #include <string>
  #include <vector>
  #include <unordered_map>
  #include <cctype>
  #include <sstream>
#endif

namespace cmdlib {

#ifdef CMDLIB_ARDUINO

// Arduino-friendly tiny implementation using fixed-size param array.
// Configure MAX_PARAMS if desired before including.
#ifndef CMDLIB_MAX_PARAMS
#define CMDLIB_MAX_PARAMS 12
#endif

struct Param {
  String key;
  String value;
};

struct Command {
  String device;
  String type;
  String command;
  Param params[CMDLIB_MAX_PARAMS];
  int paramCount = 0;

  Command() = default;

  void clear() {
    device = type = command = "";
    for (int i = 0; i < paramCount; ++i) { params[i].key = ""; params[i].value = ""; }
    paramCount = 0;
  }

  bool setParam(const String &k, const String &v) {
    // update if exists
    for (int i=0;i<paramCount;++i) {
      if (params[i].key == k) { params[i].value = v; return true; }
    }
    if (paramCount >= CMDLIB_MAX_PARAMS) return false;
    params[paramCount].key = k;
    params[paramCount].value = v;
    ++paramCount;
    return true;
  }

  String getParam(const String &k, const String &def = "") const {
    for (int i=0;i<paramCount;++i) if (params[i].key == k) return params[i].value;
    return def;
  }

  String toString() const {
    String out = "!!";
    out += device; out += ":"; out += type; out += ":"; out += command; out += ":{";
    for (int i=0;i<paramCount;++i) {
      out += params[i].key; out += "="; out += params[i].value;
      if (i < paramCount - 1) out += ",";
    }
    out += "}##";
    return out;
  }
};

// Helper: trim spaces
static inline String trimStr(const String &s) {
  int start = 0;
  while (start < s.length() && isspace(s.charAt(start))) ++start;
  int end = s.length() - 1;
  while (end >= start && isspace(s.charAt(end))) --end;
  if (end < start) return String("");
  return s.substring(start, end + 1);
}

// Parse returns true on success; on failure returns false and error contains message
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

  // part before ":{"
  String header = input.substring(2, braceOpen); // exclude leading !!
  if (header.endsWith(":")) header = header.substring(0, header.length()-1);

  // header parts
  int p1 = header.indexOf(':');
  int p2 = header.indexOf(':', p1+1);
  if (p1 == -1 || p2 == -1) { error = "Header must be 'device:type:command'"; return false; }
  out.device = trimStr(header.substring(0, p1));
  out.type   = trimStr(header.substring(p1+1, p2));
  out.command= trimStr(header.substring(p2+1));

  // parse params inside braces
  String inside = input.substring(braceOpen+1, braceClose);
  int i = 0;
  while (i < inside.length()) {
    // parse key
    // skip whitespace
    while (i < inside.length() && isspace(inside.charAt(i))) ++i;
    if (i >= inside.length()) break;
    int startKey = i;
    while (i < inside.length() && inside.charAt(i) != '=' && inside.charAt(i) != ',' ) ++i;
    if (i >= inside.length() || inside.charAt(i) == ',') {
      // a single flag (no value)
      String k = trimStr(inside.substring(startKey, i));
      if (k.length() > 0) { out.setParam(k, ""); }
      if (i < inside.length() && inside.charAt(i) == ',') ++i;
      continue;
    }
    String key = trimStr(inside.substring(startKey, i));
    ++i; // skip '='
    // parse value; support quoted values
    String value = "";
    if (i < inside.length() && (inside.charAt(i) == '"' || inside.charAt(i) == '\'')) {
      char q = inside.charAt(i);
      ++i;
      int startVal = i;
      while (i < inside.length() && inside.charAt(i) != q) ++i;
      value = inside.substring(startVal, i);
      if (i < inside.length() && inside.charAt(i) == q) ++i;
    } else {
      int startVal = i;
      while (i < inside.length() && inside.charAt(i) != ',') ++i;
      value = trimStr(inside.substring(startVal, i));
    }
    out.setParam(key, value);
    if (i < inside.length() && inside.charAt(i) == ',') ++i;
  }

  return true;
}

#else // STL version

using std::string;
using std::vector;
using std::unordered_map;

struct Command {
  string device;
  string type;
  string command;
  unordered_map<string,string> params;

  Command() = default;

  void clear() { device.clear(); type.clear(); command.clear(); params.clear(); }

  void setParam(const string &k, const string &v) { params[k] = v; }
  string getParam(const string &k, const string &def = "") const {
    auto it = params.find(k);
    return (it == params.end() ? def : it->second);
  }

  string toString() const {
    std::ostringstream ss;
    ss << "!!" << device << ":" << type << ":" << command << ":{";
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

// helpers
static inline string trim(const string &s) {
  size_t a = 0;
  while (a < s.size() && isspace((unsigned char)s[a])) ++a;
  if (a == s.size()) return "";
  size_t b = s.size() - 1;
  while (b > a && isspace((unsigned char)s[b])) --b;
  return s.substr(a, b - a + 1);
}

// parse function: returns true on success; fills error on failure
static inline bool parse(const string &input, Command &out, string &error) {
  out.clear();
  error.clear();

  if (input.rfind("!!", 0) != 0) { error = "Missing prefix '!!'"; return false; }
  if (input.size() < 4 || input.substr(input.size()-2) != "##") { error = "Missing suffix '##'"; return false; }

  auto braceOpen = input.find('{');
  auto braceClose = input.rfind('}');
  if (braceOpen == string::npos || braceClose == string::npos || braceClose < braceOpen) {
    error = "Malformed braces";
    return false;
  }

  // header portion between !! and the ':' before '{'
  string header = input.substr(2, braceOpen - 2);
  if (!header.empty() && header.back() == ':') header.pop_back();

  // split header into device:type:command
  size_t p1 = header.find(':');
  size_t p2 = header.find(':', p1 == string::npos ? string::npos : p1 + 1);
  if (p1 == string::npos || p2 == string::npos) { error = "Header must be 'device:type:command'"; return false; }
  out.device = trim(header.substr(0, p1));
  out.type   = trim(header.substr(p1 + 1, p2 - (p1 + 1)));
  out.command= trim(header.substr(p2 + 1));

  // parse inside braces
  string inside = input.substr(braceOpen + 1, braceClose - (braceOpen + 1));
  size_t i = 0, n = inside.size();
  while (i < n) {
    while (i < n && isspace((unsigned char)inside[i])) ++i;
    if (i >= n) break;
    size_t startKey = i;
    while (i < n && inside[i] != '=' && inside[i] != ',') ++i;
    if (i >= n || inside[i] == ',') {
      // flag without value
      string k = trim(inside.substr(startKey, i - startKey));
      if (!k.empty()) out.setParam(k, "");
      if (i < n && inside[i] == ',') ++i;
      continue;
    }
    string key = trim(inside.substr(startKey, i - startKey));
    ++i; // skip '='

    string value;
    if (i < n && (inside[i] == '"' || inside[i] == '\'')) {
      char q = inside[i++];
      size_t startVal = i;
      while (i < n && inside[i] != q) ++i;
      value = inside.substr(startVal, i - startVal);
      if (i < n && inside[i] == q) ++i;
    } else {
      size_t startVal = i;
      while (i < n && inside[i] != ',') ++i;
      value = trim(inside.substr(startVal, i - startVal));
    }
    out.setParam(key, value);
    if (i < n && inside[i] == ',') ++i;
  }

  return true;
}

#endif // CMDLIB_ARDUINO

} // namespace cmdlib

#endif // CMDLIB_H
