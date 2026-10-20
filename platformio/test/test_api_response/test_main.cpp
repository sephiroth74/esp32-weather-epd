/* Unit tests for api_response.cpp (OpenWeatherMap response parsing).
 *
 *   pio test -e native        on the host
 *   pio test -e test_device   on the board
 */

#include <unity.h>
#include "test_api_response.h"

void setUp(void)
{
  setUpOneCall();
  setUpAirPollution();
}

void tearDown(void)
{
  tearDownOneCall();
  tearDownAirPollution();
}

int runUnityTests(void)
{
  UNITY_BEGIN();
  runOneCallTests();
  runAirPollutionTests();
  return UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>

void setup()
{
  delay(2000); // give the USB serial port time to come up
  runUnityTests();
}

void loop()
{
}
#else
int main(void)
{
  return runUnityTests();
}
#endif
