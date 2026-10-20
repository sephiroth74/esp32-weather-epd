/* Minimal Arduino API shim for host-side unit tests (pio test -e native).
 *
 * Provides only what the sources under test need: String, Printable/Print/
 * Stream (the interfaces ArduinoJson's Arduino Stream support relies on, with
 * the same virtuals as the ESP32 core) and a Serial that writes to stdout.
 * Extend it when a new source under test needs more of the Arduino API.
 *
 * This directory is only on the include path of the native env, so it never
 * shadows the real Arduino.h on the device.
 */

#ifndef __NATIVE_SHIM_ARDUINO_H__
#define __NATIVE_SHIM_ARDUINO_H__

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

class String
{
public:
  String() = default;
  String(const char *s) : s_(s ? s : "") {}
  String(const std::string &s) : s_(s) {}
  explicit String(char c) : s_(1, c) {}
  explicit String(int v) : s_(std::to_string(v)) {}
  explicit String(unsigned int v) : s_(std::to_string(v)) {}
  explicit String(long v) : s_(std::to_string(v)) {}
  explicit String(unsigned long v) : s_(std::to_string(v)) {}
  explicit String(long long v) : s_(std::to_string(v)) {}
  explicit String(unsigned long long v) : s_(std::to_string(v)) {}
  explicit String(double v, unsigned int decimals = 2)
  {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", static_cast<int>(decimals), v);
    s_ = buf;
  }

  const char *c_str() const { return s_.c_str(); }
  unsigned int length() const { return s_.length(); }
  bool isEmpty() const { return s_.empty(); }

  String &operator+=(const String &rhs) { s_ += rhs.s_; return *this; }
  friend String operator+(String lhs, const String &rhs) { return lhs += rhs; }
  friend bool operator==(const String &lhs, const String &rhs)
  {
    return lhs.s_ == rhs.s_;
  }
  friend bool operator!=(const String &lhs, const String &rhs)
  {
    return lhs.s_ != rhs.s_;
  }

private:
  std::string s_;
};

class Print;

class Printable
{
public:
  virtual ~Printable() = default;
  virtual size_t printTo(Print &p) const = 0;
};

class Print
{
public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t *buffer, size_t size)
  {
    size_t n = 0;
    while (n < size && write(buffer[n]))
    {
      ++n;
    }
    return n;
  }
  virtual void flush() {}
};

class Stream : public Print
{
public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  virtual size_t readBytes(char *buffer, size_t length)
  {
    size_t n = 0;
    while (n < length)
    {
      int c = read();
      if (c < 0)
      {
        break;
      }
      buffer[n++] = static_cast<char>(c);
    }
    return n;
  }
};

class StdoutSerial : public Print
{
public:
  void begin(unsigned long) {}
  using Print::write;
  size_t write(uint8_t c) override { return std::fputc(c, stdout) == EOF ? 0 : 1; }
  size_t print(const String &s) { return std::fputs(s.c_str(), stdout) < 0 ? 0 : s.length(); }
  size_t println(const String &s) { return print(s) + print("\n"); }
  size_t println() { return print("\n"); }
};

inline StdoutSerial Serial;

#endif
