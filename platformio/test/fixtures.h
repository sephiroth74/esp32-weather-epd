/* Test helpers shared by the test suites, on both the native and device envs.
 */

#ifndef __TEST_FIXTURES_H__
#define __TEST_FIXTURES_H__

#include <cstring>
#include <Arduino.h>

/* Serves an in-memory buffer through the Arduino Stream interface, standing in
 * for HTTPClient::getStream() so the deserializers can be fed canned responses.
 */
class MemoryStream : public Stream
{
public:
  MemoryStream(const char *data, size_t length)
    : data_(data), length_(length) {}
  explicit MemoryStream(const char *data)
    : MemoryStream(data, strlen(data)) {}

  int available() override { return static_cast<int>(length_ - pos_); }
  int read() override
  {
    return pos_ < length_ ? static_cast<unsigned char>(data_[pos_++]) : -1;
  }
  int peek() override
  {
    return pos_ < length_ ? static_cast<unsigned char>(data_[pos_]) : -1;
  }
  using Stream::readBytes;
  size_t readBytes(char *buffer, size_t length) override
  {
    size_t n = length_ - pos_ < length ? length_ - pos_ : length;
    memcpy(buffer, data_ + pos_, n);
    pos_ += n;
    return n;
  }
  using Stream::write;
  size_t write(uint8_t) override { return 0; }

private:
  const char *data_;
  size_t length_;
  size_t pos_ = 0;
};

/* Real API responses for Zurich, requested with the same queries the client
 * uses:
 *   test/owm_onecall.json    One Call 3.0 as in getOWMonecall() (lang=it,
 *                            units=standard, exclude=minutely), no alerts.
 *   test/air_pollution.json  Air Pollution history as in getOWMairpollution(),
 *                            26 hourly entries.
 */
#ifdef ARDUINO
// Embedded into the test firmware via board_build.embed_txtfiles.
extern const char owm_onecall_json[] asm("_binary_test_owm_onecall_json_start");
extern const char air_pollution_json[] asm("_binary_test_air_pollution_json_start");

inline const char *owmOneCallFixture()
{
  return owm_onecall_json;
}

inline const char *owmAirPollutionFixture()
{
  return air_pollution_json;
}
#else
#include <fstream>
#include <sstream>
#include <string>

// OWM_FIXTURES_DIR is set to the test directory by the native env.
inline std::string readFixture(const char *name)
{
  std::ifstream file(std::string(OWM_FIXTURES_DIR) + "/" + name);
  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

inline const char *owmOneCallFixture()
{
  static const std::string json = readFixture("owm_onecall.json");
  return json.c_str();
}

inline const char *owmAirPollutionFixture()
{
  static const std::string json = readFixture("air_pollution.json");
  return json.c_str();
}
#endif

#endif
