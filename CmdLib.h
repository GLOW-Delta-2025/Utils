// CmdLib.h
// Command builder + parser simplified: no positional params — all params MUST be named (key=value).
// Format:
//   !![MSG_KIND:]COMMAND{key=value,key=value}##
// Examples:
//   !!REQUEST:MAKE_STAR{speed=100,color=red,brightness=80,size=20}##
//   !!CONFIRM:MAKE_STAR##   (no params)
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
  #include <vector>
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

struct NamedParam {
  String key;
  String value;
};

struct Command {
  // Message kind (optional) and command name
  String msgKind;
  String command;

  // Only named params (no positional)
  NamedParam namedParams[CMDLIB_MAX_PARAMS];
  int namedCount = 0;

  void clear() {
    msgKind = "";
    command = "";
    namedCount = 0;
    for (int i = 0; i < CMDLIB_MAX_PARAMS; ++i) {
      namedParams[i].key = "";
      namedParams[i].value = "";
    }
  }

  // Named param helpers
  bool setParam(const String &k, const String &v) {
    for (int i = 0; i < namedCount; ++i) {
      if (namedParams[i].key == k) { namedParams[i].value = v; return true; }
    }
    if (namedCount >= CMDLIB_MAX_PARAMS) return false;
    namedParams[namedCount].key = k;
    namedParams[namedCount].value = v;
    namedCount++;
    return true;
  }
  String getNamed(const String &k, const String &def = "") const {
    for (int i = 0; i < namedCount; ++i) if (namedParams[i].key == k) return namedParams[i].value;
    return def;
  }

  // Build string (only named params)
  String toString() const {
    String out = "!!";
    if (msgKind.length() > 0) {
      out += msgKind;
      out += ":";
    }
    out += command;
    if (namedCount > 0) {
      out += "{";
      for (int i = 0; i < namedCount; ++i) {
        out += namedParams[i].key; out += "="; out += namedParams[i].value;
        if (i < namedCount - 1) out += ",";
      }
      out += "}";
    }
    out += "##";
    return out;
  }
};

// Utility trim
static inline String trimStr(const String &s) {
  int a = 0;
  while (a < s.length() && isspace(s.charAt(a))) ++a;
  int b = s.length() - 1;
  while (b >= a && isspace(s.charAt(b))) --b;
  if (b < a) return "";
  return s.substring(a, b + 1);
}

// Parse (Arduino). Requires named params (key=value). Rejects tokens without '='.
static inline bool parse(const String &input, Command &out, String &error) {
  out.clear();
  error = "";

  if (!input.startsWith("!!")) { error = "Missing prefix '!!'"; return false; }
  if (!input.endsWith("##")) { error = "Missing suffix '##'"; return false; }

  int braceOpen = input.indexOf('{');
  int braceClose = input.lastIndexOf('}');
  int headerEnd = (braceOpen != -1) ? braceOpen : input.lastIndexOf("##");
  if (headerEnd == -1) { error = "Malformed header"; return false; }

  String header = input.substring(2, headerEnd);
  if (header.endsWith(":")) header = header.substring(0, header.length() - 1);

  // split header into (optional) msgKind and command using last colon
  int lastColon = header.lastIndexOf(':');
  if (lastColon == -1) {
    out.msgKind = "";
    out.command = trimStr(header);
  } else {
    out.msgKind = trimStr(header.substring(0, lastColon));
    out.command = trimStr(header.substring(lastColon + 1));
  }

  if (out.command.length() == 0) { error = "Empty command"; return false; }

  // parse params if present — **ONLY named params allowed**
  if (braceOpen != -1) {
    if (braceClose == -1 || braceClose < braceOpen) { error = "Malformed braces"; return false; }
    String inside = input.substring(braceOpen + 1, braceClose);

    int i = 0;
    while (i < inside.length()) {
      while (i < inside.length() && isspace(inside.charAt(i))) i++;
      if (i >= inside.length()) break;
      int startKey = i;
      // read until '=' or ','
      while (i < inside.length() && inside.charAt(i) != '=' && inside.charAt(i) != ',') i++;
      if (i >= inside.length() || inside.charAt(i) == ',') {
        // token without '=' -> positional or malformed; positional not supported
        String token = trimStr(inside.substring(startKey, i));
        if (token.length() > 0) {
          error = "Positional params not supported; expected key=value";
          return false;
        }
        if (i < inside.length() && inside.charAt(i) == ',') i++;
        continue;
      }

      String key = trimStr(inside.substring(startKey, i));
      i++; // skip '='
      int startVal = i;
      while (i < inside.length() && inside.charAt(i) != ',') i++;
      String val = trimStr(inside.substring(startVal, i));
      if (key.length() == 0) { error = "Empty param key"; return false; }
      out.setParam(key, val);
      if (i < inside.length() && inside.charAt(i) == ',') i++;
    }
  }

  return true;
}

#else
// -------------------- Standard C++ Version --------------------
using std::string;
using std::vector;
using std::unordered_map;
using std::size_t;

struct Command {
  string msgKind;
  string command;

  // Only named params
  unordered_map<string, string> namedParams;

  void clear() {
    msgKind.clear();
    command.clear();
    namedParams.clear();
  }

  void setParam(const string &k, const string &v) { namedParams[k] = v; }
  string getNamed(const string &k, const string &def = "") const {
    auto it = namedParams.find(k);
    return (it == namedParams.end()) ? def : it->second;
  }

  string toString() const {
    std::ostringstream ss;
    ss << "!!";
    if (!msgKind.empty()) ss << msgKind << ":";
    ss << command;
    if (!namedParams.empty()) {
      ss << "{";
      bool first = true;
      for (auto &kv : namedParams) {
        if (!first) ss << ",";
        first = false;
        ss << kv.first << "=" << kv.second;
      }
      ss << "}";
    }
    ss << "##";
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
  size_t headerEnd = (braceOpen != string::npos) ? braceOpen : input.size() - 2;
  string header = input.substr(2, headerEnd - 2);
  if (!header.empty() && header.back() == ':') header.pop_back();

  // split by last ':'
  size_t lastColon = header.rfind(':');
  if (lastColon == string::npos) {
    out.msgKind.clear();
    out.command = trim(header);
  } else {
    out.msgKind = trim(header.substr(0, lastColon));
    out.command = trim(header.substr(lastColon + 1));
  }

  if (out.command.empty()) { error = "Empty command"; return false; }

  // parse params (named only)
  if (braceOpen != string::npos) {
    if (braceClose == string::npos || braceClose < braceOpen) { error = "Malformed braces"; return false; }
    string inside = input.substr(braceOpen + 1, braceClose - (braceOpen + 1));
    size_t p = 0;
    while (p < inside.size()) {
      while (p < inside.size() && isspace((unsigned char)inside[p])) ++p;
      if (p >= inside.size()) break;
      size_t startKey = p;
      while (p < inside.size() && inside[p] != '=' && inside[p] != ',') ++p;
      if (p >= inside.size() || inside[p] == ',') {
        // token without '=' -> positional style -> not allowed
        string token = trim(inside.substr(startKey, p - startKey));
        if (!token.empty()) {
          error = "Positional params not supported; expected key=value";
          return false;
        }
        if (p < inside.size() && inside[p] == ',') ++p;
        continue;
      }
      string key = trim(inside.substr(startKey, p - startKey));
      ++p; // skip '='
      size_t startVal = p;
      while (p < inside.size() && inside[p] != ',') ++p;
      string val = trim(inside.substr(startVal, p - startVal));
      if (key.empty()) { error = "Empty param key"; return false; }
      out.setParam(key, val);
      if (p < inside.size() && inside[p] == ',') ++p;
    }
  }

  return true;
}

#endif // CMDLIB_ARDUINO

} // namespace cmdlib

#endif // CMDLIB_H
