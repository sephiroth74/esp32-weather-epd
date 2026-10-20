/* Hooks each test file of the api_response suite exposes to test_main.cpp.
 */

#ifndef __TEST_API_RESPONSE_H__
#define __TEST_API_RESPONSE_H__

void setUpOneCall();
void tearDownOneCall();
void runOneCallTests();

void setUpAirPollution();
void tearDownAirPollution();
void runAirPollutionTests();

#endif
