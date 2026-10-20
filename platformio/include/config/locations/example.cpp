// Template for a location file: the deployment-specific values and credentials
// that config.cpp includes via LOCATION_CONFIG_FILE.
//
// Copy it next to this file (e.g. config/locations/home.cpp), fill it in and
// point your env's LOCATION_CONFIG_FILE at it. Location files are gitignored,
// only this template is committed: never put real credentials here.

#include <Arduino.h>

// OpenWeatherMap API key, https://openweathermap.org/
const String OWM_APIKEY = "abcdefghijklmnopqrstuvwxyz012345";

// Location(s)
// The display cycles through them, one per refresh.
// For time zones see
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const uint8_t NUM_LOCATIONS = 1;
const String LAT[NUM_LOCATIONS] = { "40.7128" };
const String LON[NUM_LOCATIONS] = { "-74.0060" };
const String CITY_STRING[NUM_LOCATIONS] = { "New York" };
const String TIMEZONE[NUM_LOCATIONS] = { "EST5EDT,M3.2.0,M11.1.0" };

// WiFi credentials
const char* WIFI_SSID = "ssid";
const char* WIFI_PASSWORD = "password";

const int SLEEP_DURATION = 30; // sleep duration between updates (in minutes)
const int BED_TIME = 0; // Last update at 00:00 (midnight) until WAKE_TIME.
const int WAKE_TIME = 6; // Hour of first update after BED_TIME, 06:00.
const int HOURLY_GRAPH_MAX = 24; // Number of hours to display on the outlook graph.(range : [8 - 48])
