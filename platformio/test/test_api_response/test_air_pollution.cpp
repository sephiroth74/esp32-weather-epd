/* Unit tests for deserializeAirQuality() (Air Pollution history API response
 * parsing).
 */

#include <unity.h>
#include "api_response.h"
#include "fixtures.h"
#include "test_api_response.h"

#define TEST_ASSERT_FLOAT(expected, actual) \
  TEST_ASSERT_FLOAT_WITHIN(0.001f, (expected), (actual))

static owm_resp_air_pollution_t *r = nullptr;

void setUpAirPollution()
{
  r = new owm_resp_air_pollution_t();
}

void tearDownAirPollution()
{
  delete r;
  r = nullptr;
}

static DeserializationError parseAirPollution(const char *json)
{
  MemoryStream stream(json);
  return deserializeAirQuality(stream, *r);
}

static void parseFixture()
{
  TEST_ASSERT_EQUAL_STRING("Ok",
                           parseAirPollution(owmAirPollutionFixture()).c_str());
}

void test_air_pollution_fixture_is_available(void)
{
  TEST_ASSERT_TRUE_MESSAGE(strlen(owmAirPollutionFixture()) > 0,
                           "test/air_pollution.json missing or empty");
}

void test_air_pollution_success_flag(void)
{
  TEST_ASSERT_FALSE(r->success);
  parseFixture();
  TEST_ASSERT_TRUE(r->success);
}

void test_air_pollution_coord(void)
{
  parseFixture();
  TEST_ASSERT_FLOAT(47.3779f, r->coord.lat);
  TEST_ASSERT_FLOAT(8.5403f, r->coord.lon);
}

// The fixture holds 26 hourly entries, oldest first: only the first
// OWM_NUM_AIR_POLLUTION are kept.
void test_air_pollution_fills_all_slots(void)
{
  parseFixture();
  TEST_ASSERT_EQUAL_INT64(1792396800, r->dt[0]);
  for (int i = 0; i < OWM_NUM_AIR_POLLUTION; ++i)
  {
    TEST_ASSERT_EQUAL_INT64(r->dt[0] + 3600LL * i, r->dt[i]);
    TEST_ASSERT_NOT_EQUAL(0, r->main_aqi[i]);
  }
  TEST_ASSERT_EQUAL_INT64(1792479600, r->dt[OWM_NUM_AIR_POLLUTION - 1]);
}

void test_air_pollution_oldest_hour(void)
{
  parseFixture();
  const owm_components_t &c = r->components;
  TEST_ASSERT_EQUAL_INT(2, r->main_aqi[0]);
  TEST_ASSERT_FLOAT(161.2f, c.co[0]);
  TEST_ASSERT_FLOAT(0.14f, c.no[0]);
  TEST_ASSERT_FLOAT(3.08f, c.no2[0]);
  TEST_ASSERT_FLOAT(61.79f, c.o3[0]);
  TEST_ASSERT_FLOAT(0.25f, c.so2[0]);
  TEST_ASSERT_FLOAT(4.64f, c.pm2_5[0]);
  TEST_ASSERT_FLOAT(4.81f, c.pm10[0]);
  TEST_ASSERT_FLOAT(0.42f, c.nh3[0]);
}

// calc_aqi() reads the last index as the most recent hourly concentration.
void test_air_pollution_most_recent_hour(void)
{
  parseFixture();
  const int last = OWM_NUM_AIR_POLLUTION - 1;
  const owm_components_t &c = r->components;
  TEST_ASSERT_EQUAL_INT(1, r->main_aqi[last]);
  TEST_ASSERT_FLOAT(141.01f, c.co[last]);
  TEST_ASSERT_FLOAT(0.01f, c.no[last]);
  TEST_ASSERT_FLOAT(2.67f, c.no2[last]);
  TEST_ASSERT_FLOAT(58.14f, c.o3[last]);
  TEST_ASSERT_FLOAT(0.24f, c.so2[last]);
  TEST_ASSERT_FLOAT(3.48f, c.pm2_5[last]);
  TEST_ASSERT_FLOAT(3.84f, c.pm10[last]);
  TEST_ASSERT_FLOAT(0.57f, c.nh3[last]);
}

void test_air_pollution_truncated_response(void)
{
  const char *json = owmAirPollutionFixture();
  MemoryStream stream(json, strlen(json) / 2);
  DeserializationError err = deserializeAirQuality(stream, *r);
  TEST_ASSERT_EQUAL_STRING("IncompleteInput", err.c_str());
  TEST_ASSERT_FALSE(r->success);
  TEST_ASSERT_EQUAL_INT64(0, r->dt[0]);
}

void test_air_pollution_non_json_response(void)
{
  DeserializationError err = parseAirPollution("<html>502 Bad Gateway</html>");
  TEST_ASSERT_EQUAL_STRING("InvalidInput", err.c_str());
  TEST_ASSERT_FALSE(r->success);
}

void test_air_pollution_empty_response(void)
{
  TEST_ASSERT_EQUAL_STRING("EmptyInput", parseAirPollution("").c_str());
  TEST_ASSERT_FALSE(r->success);
}

void runAirPollutionTests()
{
  UnitySetTestFile(__FILE__); // report failures against this file
  RUN_TEST(test_air_pollution_fixture_is_available);
  RUN_TEST(test_air_pollution_success_flag);
  RUN_TEST(test_air_pollution_coord);
  RUN_TEST(test_air_pollution_fills_all_slots);
  RUN_TEST(test_air_pollution_oldest_hour);
  RUN_TEST(test_air_pollution_most_recent_hour);
  RUN_TEST(test_air_pollution_truncated_response);
  RUN_TEST(test_air_pollution_non_json_response);
  RUN_TEST(test_air_pollution_empty_response);
}
