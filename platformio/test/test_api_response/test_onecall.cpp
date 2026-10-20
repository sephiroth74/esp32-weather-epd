/* Unit tests for deserializeOneCall() (One Call 3.0 API response parsing).
 */

#include <string>
#include <unity.h>
#include "api_response.h"
#include "config.h"
#include "fixtures.h"
#include "test_api_response.h"

#define TEST_ASSERT_FLOAT(expected, actual) \
  TEST_ASSERT_FLOAT_WITHIN(0.001f, (expected), (actual))

// owm_resp_onecall_t is several kB, too large for the Arduino loop task stack.
static owm_resp_onecall_t *r = nullptr;

void setUpOneCall()
{
  r = new owm_resp_onecall_t();
}

void tearDownOneCall()
{
  delete r;
  r = nullptr;
}

static DeserializationError parseOneCall(const char *json)
{
  MemoryStream stream(json);
  return deserializeOneCall(stream, *r);
}

static void parseFixture()
{
  TEST_ASSERT_EQUAL_STRING("Ok", parseOneCall(owmOneCallFixture()).c_str());
}

static void assertWeather(int id, const char *main, const char *description,
                          const char *icon, const owm_weather_t &weather)
{
  TEST_ASSERT_EQUAL_INT(id, weather.id);
  TEST_ASSERT_EQUAL_STRING(main, weather.main.c_str());
  TEST_ASSERT_EQUAL_STRING(description, weather.description.c_str());
  TEST_ASSERT_EQUAL_STRING(icon, weather.icon.c_str());
}

void test_onecall_fixture_is_available(void)
{
  TEST_ASSERT_TRUE_MESSAGE(strlen(owmOneCallFixture()) > 0,
                           "test/owm_onecall.json missing or empty");
}

void test_onecall_location(void)
{
  parseFixture();
  TEST_ASSERT_FLOAT(47.3779f, r->lat);
  TEST_ASSERT_FLOAT(8.5403f, r->lon);
  TEST_ASSERT_EQUAL_STRING("Europe/Zurich", r->timezone.c_str());
  TEST_ASSERT_EQUAL_INT(7200, r->timezone_offset);
}

void test_onecall_current(void)
{
  parseFixture();
  const owm_current_t &c = r->current;
  TEST_ASSERT_EQUAL_INT64(1792482291, c.dt);
  TEST_ASSERT_EQUAL_INT64(1792475460, c.sunrise);
  TEST_ASSERT_EQUAL_INT64(1792513812, c.sunset);
  TEST_ASSERT_FLOAT(284.0f, c.temp); // integer in the JSON
  TEST_ASSERT_FLOAT(283.36f, c.feels_like);
  TEST_ASSERT_EQUAL_INT(1014, c.pressure);
  TEST_ASSERT_EQUAL_INT(85, c.humidity);
  TEST_ASSERT_FLOAT(281.58f, c.dew_point);
  TEST_ASSERT_EQUAL_INT(91, c.clouds);
  TEST_ASSERT_FLOAT(0.54f, c.uvi);
  TEST_ASSERT_EQUAL_INT(10000, c.visibility);
  TEST_ASSERT_FLOAT(1.03f, c.wind_speed);
  TEST_ASSERT_EQUAL_INT(0, c.wind_deg);
  assertWeather(804, "Clouds", "cielo coperto", "04d", c.weather);
}

// wind_gust, rain and snow are only sent "where available".
void test_onecall_current_absent_fields_are_zero(void)
{
  parseFixture();
  TEST_ASSERT_FLOAT(0.0f, r->current.wind_gust);
  TEST_ASSERT_FLOAT(0.0f, r->current.rain_1h);
  TEST_ASSERT_FLOAT(0.0f, r->current.snow_1h);
}

void test_onecall_hourly_fills_all_slots(void)
{
  parseFixture();
  TEST_ASSERT_EQUAL_INT64(1792479600, r->hourly[0].dt);
  for (int i = 0; i < OWM_NUM_HOURLY; ++i)
  {
    TEST_ASSERT_EQUAL_INT64(r->hourly[0].dt + 3600LL * i, r->hourly[i].dt);
    TEST_ASSERT_FALSE(r->hourly[i].weather.icon.isEmpty());
  }
  TEST_ASSERT_EQUAL_INT64(1792648800, r->hourly[OWM_NUM_HOURLY - 1].dt);
}

void test_onecall_hourly_first_hour(void)
{
  parseFixture();
  const owm_hourly_t &h = r->hourly[0];
  TEST_ASSERT_FLOAT(283.65f, h.temp);
  TEST_ASSERT_FLOAT(283.03f, h.feels_like);
  TEST_ASSERT_EQUAL_INT(1014, h.pressure);
  TEST_ASSERT_EQUAL_INT(87, h.humidity);
  TEST_ASSERT_FLOAT(281.58f, h.dew_point);
  TEST_ASSERT_EQUAL_INT(93, h.clouds);
  TEST_ASSERT_FLOAT(0.11f, h.uvi);
  TEST_ASSERT_EQUAL_INT(10000, h.visibility);
  TEST_ASSERT_FLOAT(1.7f, h.wind_speed);
  TEST_ASSERT_FLOAT(3.92f, h.wind_gust);
  TEST_ASSERT_EQUAL_INT(46, h.wind_deg);
  TEST_ASSERT_FLOAT(0.0f, h.pop);
  TEST_ASSERT_FLOAT(0.0f, h.rain_1h);
  TEST_ASSERT_FLOAT(0.0f, h.snow_1h);
  assertWeather(804, "Clouds", "cielo coperto", "04d", h.weather);
}

// Hourly rain is nested: "rain": {"1h": 0.14}
void test_onecall_hourly_rain_volume(void)
{
  parseFixture();
  const owm_hourly_t &h = r->hourly[12];
  TEST_ASSERT_FLOAT(0.14f, h.rain_1h);
  TEST_ASSERT_FLOAT(0.21f, h.pop);
  TEST_ASSERT_EQUAL_INT(9779, h.visibility);
  assertWeather(500, "Rain", "pioggia leggera", "10n", h.weather);
  TEST_ASSERT_FLOAT(2.73f, r->hourly[30].rain_1h);
}

void test_onecall_daily_fills_all_slots(void)
{
  parseFixture();
  TEST_ASSERT_EQUAL_INT64(1792494000, r->daily[0].dt);
  for (int i = 0; i < OWM_NUM_DAILY; ++i)
  {
    TEST_ASSERT_EQUAL_INT64(r->daily[0].dt + 86400LL * i, r->daily[i].dt);
    TEST_ASSERT_FALSE(r->daily[i].weather.icon.isEmpty());
  }
  TEST_ASSERT_EQUAL_INT64(1793098800, r->daily[OWM_NUM_DAILY - 1].dt);
}

void test_onecall_daily_today(void)
{
  parseFixture();
  const owm_daily_t &d = r->daily[0];
  TEST_ASSERT_EQUAL_INT64(1792475460, d.sunrise);
  TEST_ASSERT_EQUAL_INT64(1792513812, d.sunset);
  TEST_ASSERT_EQUAL_INT64(1792505280, d.moonrise);
  TEST_ASSERT_EQUAL_INT64(1792450680, d.moonset);
  TEST_ASSERT_FLOAT(0.3f, d.moon_phase);
  TEST_ASSERT_FLOAT(282.58f, d.temp.morn);
  TEST_ASSERT_FLOAT(284.88f, d.temp.day);
  TEST_ASSERT_FLOAT(285.61f, d.temp.eve);
  TEST_ASSERT_FLOAT(284.13f, d.temp.night);
  TEST_ASSERT_FLOAT(281.92f, d.temp.min);
  TEST_ASSERT_FLOAT(287.34f, d.temp.max);
  TEST_ASSERT_FLOAT(282.58f, d.feels_like.morn);
  TEST_ASSERT_FLOAT(284.25f, d.feels_like.day);
  TEST_ASSERT_FLOAT(285.24f, d.feels_like.eve);
  TEST_ASSERT_FLOAT(283.79f, d.feels_like.night);
  TEST_ASSERT_EQUAL_INT(1013, d.pressure);
  TEST_ASSERT_EQUAL_INT(82, d.humidity);
  TEST_ASSERT_FLOAT(281.91f, d.dew_point);
  TEST_ASSERT_EQUAL_INT(84, d.clouds);
  TEST_ASSERT_FLOAT(2.4f, d.uvi);
  TEST_ASSERT_FLOAT(1.74f, d.wind_speed);
  TEST_ASSERT_FLOAT(5.01f, d.wind_gust);
  TEST_ASSERT_EQUAL_INT(54, d.wind_deg);
  TEST_ASSERT_FLOAT(0.45f, d.pop);
  TEST_ASSERT_FLOAT(0.14f, d.rain); // daily rain is a plain number
  TEST_ASSERT_FLOAT(0.0f, d.snow);
  assertWeather(500, "Rain", "pioggia leggera", "10d", d.weather);
}

void test_onecall_daily_without_rain(void)
{
  parseFixture();
  TEST_ASSERT_FLOAT(0.0f, r->daily[4].rain);
  TEST_ASSERT_FLOAT(0.05f, r->daily[4].pop);
  const owm_daily_t &last = r->daily[OWM_NUM_DAILY - 1];
  TEST_ASSERT_FLOAT(0.0f, last.rain);
  TEST_ASSERT_FLOAT(0.0f, last.pop);
  assertWeather(803, "Clouds", "nubi sparse", "04d", last.weather);
}

void test_onecall_fixture_has_no_alerts(void)
{
  parseFixture();
  TEST_ASSERT_EQUAL_UINT(0, r->alerts.size());
}

#if DISPLAY_ALERTS
void test_onecall_alert_fields(void)
{
  const char *json = R"json({"alerts": [{
      "sender_name": "MeteoSvizzera",
      "event": "Thunderstorm warning",
      "start": 1792500000,
      "end": 1792540000,
      "description": "Strong thunderstorms with hail are expected.",
      "tags": ["Thunderstorm", "Wind"]
    }]})json";
  TEST_ASSERT_EQUAL_STRING("Ok", parseOneCall(json).c_str());
  TEST_ASSERT_EQUAL_UINT(1, r->alerts.size());
  const owm_alerts_t &a = r->alerts[0];
  TEST_ASSERT_EQUAL_STRING("Thunderstorm warning", a.event.c_str());
  TEST_ASSERT_EQUAL_INT64(1792500000, a.start);
  TEST_ASSERT_EQUAL_INT64(1792540000, a.end);
  TEST_ASSERT_EQUAL_STRING("Thunderstorm", a.tags.c_str()); // first tag only
  // not stored, to save memory
  TEST_ASSERT_TRUE(a.sender_name.isEmpty());
  TEST_ASSERT_TRUE(a.description.isEmpty());
}

void test_onecall_alerts_capped_at_OWM_NUM_ALERTS(void)
{
  std::string json = "{\"alerts\":[";
  for (int i = 0; i < OWM_NUM_ALERTS + 2; ++i)
  {
    if (i > 0)
    {
      json += ",";
    }
    json += "{\"event\":\"Alert " + std::to_string(i)
          + "\",\"start\":0,\"end\":0,\"tags\":[]}";
  }
  json += "]}";

  TEST_ASSERT_EQUAL_STRING("Ok", parseOneCall(json.c_str()).c_str());
  TEST_ASSERT_EQUAL_UINT(OWM_NUM_ALERTS, r->alerts.size());
  const owm_alerts_t &last = r->alerts[OWM_NUM_ALERTS - 1];
  TEST_ASSERT_EQUAL_STRING(
    ("Alert " + std::to_string(OWM_NUM_ALERTS - 1)).c_str(), last.event.c_str());
  TEST_ASSERT_TRUE(last.tags.isEmpty());
}
#endif

// A connection dropped mid-body. getOWMonecall() retries on error, so nothing
// may be written to the response before the document is complete.
void test_onecall_truncated_response(void)
{
  const char *json = owmOneCallFixture();
  MemoryStream stream(json, strlen(json) / 2);
  DeserializationError err = deserializeOneCall(stream, *r);
  TEST_ASSERT_EQUAL_STRING("IncompleteInput", err.c_str());
  TEST_ASSERT_TRUE(r->timezone.isEmpty());
  TEST_ASSERT_EQUAL_INT64(0, r->current.dt);
}

void test_onecall_non_json_response(void)
{
  DeserializationError err = parseOneCall("<html>502 Bad Gateway</html>");
  TEST_ASSERT_EQUAL_STRING("InvalidInput", err.c_str());
}

void test_onecall_empty_response(void)
{
  TEST_ASSERT_EQUAL_STRING("EmptyInput", parseOneCall("").c_str());
}

void runOneCallTests()
{
  UnitySetTestFile(__FILE__); // report failures against this file
  RUN_TEST(test_onecall_fixture_is_available);
  RUN_TEST(test_onecall_location);
  RUN_TEST(test_onecall_current);
  RUN_TEST(test_onecall_current_absent_fields_are_zero);
  RUN_TEST(test_onecall_hourly_fills_all_slots);
  RUN_TEST(test_onecall_hourly_first_hour);
  RUN_TEST(test_onecall_hourly_rain_volume);
  RUN_TEST(test_onecall_daily_fills_all_slots);
  RUN_TEST(test_onecall_daily_today);
  RUN_TEST(test_onecall_daily_without_rain);
  RUN_TEST(test_onecall_fixture_has_no_alerts);
#if DISPLAY_ALERTS
  RUN_TEST(test_onecall_alert_fields);
  RUN_TEST(test_onecall_alerts_capped_at_OWM_NUM_ALERTS);
#endif
  RUN_TEST(test_onecall_truncated_response);
  RUN_TEST(test_onecall_non_json_response);
  RUN_TEST(test_onecall_empty_response);
}
