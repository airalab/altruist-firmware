#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <algorithm>
#include <deque>
#include <string>
#include <cstdio>
#define F(text) text
#define HEX 16
using boolean = bool;
extern bool refuseStringReserve;
class String {
  std::string value;
public:
  String(const char *text = "") : value(text ? text : "") {}
  String(uint32_t number, int base) {
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), base == HEX ? "%x" : "%u", number);
    value = buffer;
  }
  size_t length() const { return value.size(); }
  const char *c_str() const { return value.c_str(); }
  bool reserve(size_t n) {
    if (refuseStringReserve) return false;
    value.reserve(n);
    return true;
  }
  bool concat(char c) { value += c; return true; }
  bool concat(const char *text) { value += text; return true; }
  String operator+(const char *text) const { String out(*this); out.value += text; return out; }
  String operator+(const String &text) const { return *this + text.c_str(); }
  String &operator+=(char text) { value += text; return *this; }
  bool operator==(const char *text) const { return value == text; }
};
class StringSumHelper : public String {};
extern uint64_t fakeNow;
inline unsigned long millis() { return static_cast<unsigned long>(fakeNow); }
struct FakeSerial {
  void printf(const char *, ...) {}
  void println() {}
  std::string output;
  std::deque<size_t> limits;
  size_t writes = 0;
  size_t write(uint8_t byte) { return write(&byte, 1); }
  size_t write(const uint8_t *data, size_t length) {
    if (!limits.empty()) {
      length = std::min(length, limits.front());
      limits.pop_front();
    }
    output.append(reinterpret_cast<const char *>(data), length);
    ++writes;
    return length;
  }
};
extern FakeSerial Serial;
inline String get_chipid() { return "1051DB010C70"; }

template <typename T> inline T sq(T number) { return number * number; }
