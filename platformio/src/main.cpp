/* Main program for esp32-weather-epd.
 * Copyright (C) 2022-2026  Luke Marzen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <time.h>
#include <WiFi.h>
#include <Wire.h>

#include "_locale.h"
#include "api_response.h"
#include "client_utils.h"
#include "config.h"
#include "display_utils.h"
#include "display_utils_extra.h"
#include "board_utils.h"
#include "icons/icons_196x196.h"
#include "renderer.h"

#if defined(SENSOR_BME280)
#include <Adafruit_BME280.h>
#endif
#if defined(SENSOR_BME680)
#include <Adafruit_BME680.h>
#endif
#if defined(USE_HTTPS_WITH_CERT_VERIF) || defined(USE_HTTPS_WITH_CERT_VERIF)
#include <WiFiClientSecure.h>
#endif
#ifdef USE_HTTPS_WITH_CERT_VERIF
#include "cert.h"
#endif

#include <hal/gpio_types.h>
#include "esp_log.h"

// too large to allocate locally on stack
static owm_resp_onecall_t owm_onecall;
static owm_resp_air_pollution_t owm_air_pollution;

Preferences prefs;

// consecutive deep sleeps with the clock not set; kept across deep sleep
static RTC_DATA_ATTR uint8_t clockUnsetRetries = 0;

/* Deep-sleeps for sleepDuration seconds, see enterDeepSleep().
 */
static void sleepFor(unsigned long startTime, uint64_t sleepDuration)
{
#if DEBUG_LEVEL >= 1
    printHeapUsage();
#endif

    esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL);
    Serial.print(TXT_AWAKE_FOR);
    Serial.println(" " + String((millis() - startTime) / 1000.0, 3) + "s");
    Serial.print(TXT_ENTERING_DEEP_SLEEP_FOR);
    Serial.println(" " + String(sleepDuration) + "s");
    enterDeepSleep();
} // end sleepFor

/* Put esp32 into ultra low-power deep sleep (<11μA).
 * Aligns wake time to the minute, or retries soon if the clock was never set.
 * Sleep times defined in config.cpp.
 */
void beginDeepSleep(unsigned long startTime, tm* timeInfo)
{
    // 0 ms: don't wait, only SNTP earlier in this boot could have set it
    if (!getLocalTime(timeInfo, 0)) {
        // The clock was never set, e.g. the first boot after a power loss
        // with WiFi or NTP failing. The schedule can't be computed (the unset
        // clock reads as midnight and would wait until WAKE_TIME): retry soon,
        // doubling the wait while it keeps failing, up to SLEEP_DURATION.
        uint64_t retryMinutes = (uint64_t)TIME_UNKNOWN_RETRY_MINUTES
            << (clockUnsetRetries < 8 ? clockUnsetRetries : 8);
        if (retryMinutes > (uint64_t)SLEEP_DURATION) {
            retryMinutes = SLEEP_DURATION;
        }
        if (clockUnsetRetries < UINT8_MAX) {
            ++clockUnsetRetries;
        }
        ESP_LOGW(LOG_TAG, "Clock not set, retrying in %u min", (unsigned)retryMinutes);
        sleepFor(startTime, retryMinutes * 60);
        return;
    }
    clockUnsetRetries = 0;

    // To simplify sleep time calculations, the current time stored by timeInfo
    // will be converted to time relative to the WAKE_TIME. This way if a
    // SLEEP_DURATION is not a multiple of 60 minutes it can be more trivially,
    // aligned and it can easily be deterimined whether we must sleep for
    // additional time due to bedtime.
    // i.e. when curHour == 0, then timeInfo->tm_hour == WAKE_TIME
    int bedtimeHour = INT_MAX;
    if (BED_TIME != WAKE_TIME) {
        bedtimeHour = (BED_TIME - WAKE_TIME + 24) % 24;
    }

    ESP_LOGD(LOG_TAG, "bedTimeHour: %d", bedtimeHour);

    // time is relative to wake time
    int curHour = (timeInfo->tm_hour - WAKE_TIME + 24) % 24;
    const int curMinute = curHour * 60 + timeInfo->tm_min;
    const int curSecond = curHour * 3600
        + timeInfo->tm_min * 60
        + timeInfo->tm_sec;
    const int desiredSleepSeconds = SLEEP_DURATION * 60;
    const int offsetMinutes = curMinute % SLEEP_DURATION;
    const int offsetSeconds = curSecond % desiredSleepSeconds;

    // align wake time to nearest multiple of SLEEP_DURATION
    int sleepMinutes = SLEEP_DURATION - offsetMinutes;
    if (desiredSleepSeconds - offsetSeconds < 120
        || offsetSeconds / (float)desiredSleepSeconds > 0.95f) { // if we have a sleep time less than 2 minutes OR less 5% SLEEP_DURATION,
        // skip to next alignment
        sleepMinutes += SLEEP_DURATION;
    }

    // estimated wake time, if this falls in a sleep period then sleepDuration
    // must be adjusted
    const int predictedWakeHour = ((curMinute + sleepMinutes) / 60) % 24;

    uint64_t sleepDuration;
    if (predictedWakeHour < bedtimeHour) {
        sleepDuration = sleepMinutes * 60 - timeInfo->tm_sec;
    } else {
        const int hoursUntilWake = 24 - curHour;
        sleepDuration = hoursUntilWake * 3600ULL
            - (timeInfo->tm_min * 60ULL + timeInfo->tm_sec);
    }

    // add extra delay to compensate for esp32's with fast RTCs.
    sleepDuration += 3ULL;
    sleepDuration *= 1.0015f;

    sleepFor(startTime, sleepDuration);
} // end beginDeepSleep

/* Program entry point.
 */
void setup()
{
    unsigned long startTime = millis();
    Serial.begin(115200);

#if defined(DELAY_AFTER_WAKEUP) && DELAY_AFTER_WAKEUP > 0
    waitForSerialMonitor(DELAY_AFTER_WAKEUP); // only with a computer attached
#endif // DELAY_AFTER_WAKEUP

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(LOG_TAG, ESP_LOG_DEBUG);

    ESP_LOGI(LOG_TAG, "Starting up...");

#if DEBUG_LEVEL >= 1
    printHeapUsage();
#endif

#if defined(BOARD_NAME)
    ESP_LOGI(LOG_TAG, "Board: %s", BOARD_NAME);
#endif

    ESP_LOGI(LOG_TAG, "Board Cpu Frequency: %d MHz", getCpuFrequencyMhz());
    ESP_LOGI(LOG_TAG, "SDK Version: %s", ESP.getSdkVersion());
    ESP_LOGI(LOG_TAG, "Chip Model: %s", ESP.getChipModel());
    ESP_LOGI(LOG_TAG, "Chip Revision: %d", ESP.getChipRevision());
    ESP_LOGI(LOG_TAG, "Flash Size: %d MB", ESP.getFlashChipSize() / (1024 * 1024));
    ESP_LOGI(LOG_TAG, "Flash Speed: %d MHz", ESP.getFlashChipSpeed() / 1000000);
    ESP_LOGI(LOG_TAG, "Free Heap: %.2f KB", ESP.getFreeHeap() / 1024.0);
    ESP_LOGI(LOG_TAG, "Sketch Size: %d KB", ESP.getSketchSize() / 1024);

    ESP_LOGI(LOG_TAG, "MOSI pin: %d", MOSI);
    ESP_LOGI(LOG_TAG, "MISO pin: %d", MISO);
    ESP_LOGI(LOG_TAG, "SCK pin: %d", SCK);
    ESP_LOGI(LOG_TAG, "SS pin: %d", SS);

    disableBuiltinLED();

    // Start LED pulsing to indicate activity
    startLEDPulsing();

    // Print wakeup reason for debugging purposes
    printWakeupReason();

    // Open namespace for read/write to non-volatile storage
    prefs.begin(NVS_NAMESPACE, false);
    int locationsIndex = prefs.getInt("locationsIndex", 0);

    // static_assert(NUM_LOCATIONS > 0, "NUM_LOCATIONS must be greater than 0");

    if (locationsIndex < 0 || locationsIndex >= NUM_LOCATIONS) {
        locationsIndex = 0;
    }

    prefs.putInt("locationsIndex", locationsIndex + 1);

    String currentLat = LAT[locationsIndex];
    String currentLon = LON[locationsIndex];
    String currentCity = CITY_STRING[locationsIndex];
    String currentTimeZone = TIMEZONE[locationsIndex];

    // Local time also on the paths that fail before SNTP (configTzTime() sets
    // it again below): the RTC keeps the time across deep sleep.
    setenv("TZ", currentTimeZone.c_str(), 1);
    tzset();

#if DEBUG_LEVEL >= 1
    ESP_LOGI(LOG_TAG, "Location index: %d", locationsIndex);
    ESP_LOGI(LOG_TAG, "Location: %s", currentCity.c_str());
    ESP_LOGI(LOG_TAG, "Latitude: %s", currentLat.c_str());
    ESP_LOGI(LOG_TAG, "Longitude: %s", currentLon.c_str());
    ESP_LOGI(LOG_TAG, "Timezone: %s", currentTimeZone.c_str());
#endif

#if BATTERY_MONITORING
    uint32_t batteryVoltage = readBatteryVoltage();
    Serial.print(TXT_BATTERY_VOLTAGE);
    Serial.println(": " + String(batteryVoltage) + "mv");

    ESP_LOGI(LOG_TAG, "Battery Pin: %d", PIN_BAT_ADC);
    ESP_LOGI(LOG_TAG, "Battery resistor divider: %.8f", BATTERY_RESISTOR_DIVIDER);
    ESP_LOGD(LOG_TAG, "Battery Voltage: %d mV", batteryVoltage);

    if (batteryVoltage < MIN_VALID_BATTERY_VOLTAGE) {
        ESP_LOGW(LOG_TAG, "Ignoring implausible battery voltage: %u mV",
            (unsigned)batteryVoltage);
        batteryVoltage = UINT32_MAX; // no reading, see drawStatusBar()
    }

    // When the battery is low, the display should be updated to reflect that, but
    // only the first time we detect low voltage. The next time the display will
    // refresh is when voltage is no longer low. To keep track of that we will
    // make use of non-volatile storage.
    bool lowBat = prefs.getBool("lowBat", false);

    ESP_LOGI(LOG_TAG, "lowBat: %d", lowBat);

    // low battery, deep sleep now
    if (batteryVoltage <= LOW_BATTERY_VOLTAGE) {
        if (lowBat == false) { // battery is now low for the first time
            prefs.putBool("lowBat", true);
            prefs.end();
            initDisplay();
            do {
                drawError(battery_alert_0deg_196x196, TXT_LOW_BATTERY);
            } while (display.nextPage());
            powerOffDisplay();
        }

        if (batteryVoltage <= CRIT_LOW_BATTERY_VOLTAGE) { // critically low battery
                                                          // don't set esp_sleep_enable_timer_wakeup();
                                                          // We won't wake up again until someone manually presses the RST button.
            ESP_LOGD(LOG_TAG, "%s", TXT_CRIT_LOW_BATTERY_VOLTAGE);
            ESP_LOGD(LOG_TAG, "%s", TXT_HIBERNATING_INDEFINITELY_NOTICE);
        } else if (batteryVoltage <= VERY_LOW_BATTERY_VOLTAGE) { // very low battery
            esp_sleep_enable_timer_wakeup(VERY_LOW_BATTERY_SLEEP_INTERVAL
                * 60ULL * 1000000ULL);
            ESP_LOGD(LOG_TAG, "%s", TXT_VERY_LOW_BATTERY_VOLTAGE);
            ESP_LOGD(LOG_TAG, "%s", TXT_ENTERING_DEEP_SLEEP_FOR);
            ESP_LOGD(LOG_TAG, "%d min", VERY_LOW_BATTERY_SLEEP_INTERVAL);
        } else { // low battery
            esp_sleep_enable_timer_wakeup(LOW_BATTERY_SLEEP_INTERVAL
                * 60ULL * 1000000ULL);
            ESP_LOGD(LOG_TAG, "%s", TXT_LOW_BATTERY_VOLTAGE);
            ESP_LOGD(LOG_TAG, "%s", TXT_ENTERING_DEEP_SLEEP_FOR);
            ESP_LOGD(LOG_TAG, "%d min", LOW_BATTERY_SLEEP_INTERVAL);
        }
        enterDeepSleep();
        return;
    }
    // battery is no longer low, reset variable in non-volatile storage
    if (lowBat == true && batteryVoltage != UINT32_MAX) {
        prefs.putBool("lowBat", false);
    }
#else
    uint32_t batteryVoltage = UINT32_MAX;
#endif

    // All data should have been loaded from NVS. Close filesystem.
    prefs.end();

    String statusStr = { };
    String tmpStr = { };
    tm timeInfo = { };

    // START WIFI
    int wifiRSSI = 0; // “Received Signal Strength Indicator"
    wl_status_t wifiStatus = startWiFi(wifiRSSI);
    if (wifiStatus != WL_CONNECTED) { // WiFi Connection Failed
        killWiFi();
        initDisplay();
        if (wifiStatus == WL_NO_SSID_AVAIL) {
            ESP_LOGI(LOG_TAG, "%s", TXT_NETWORK_NOT_AVAILABLE);
            do {
                drawError(wifi_x_196x196, TXT_NETWORK_NOT_AVAILABLE);
            } while (display.nextPage());
        } else {
            ESP_LOGI(LOG_TAG, "%s", TXT_WIFI_CONNECTION_FAILED);
            do {
                drawError(wifi_x_196x196, TXT_WIFI_CONNECTION_FAILED);
            } while (display.nextPage());
        }
        powerOffDisplay();
        beginDeepSleep(startTime, &timeInfo);
        return;
    }

    // TIME SYNCHRONIZATION
    configTzTime(currentTimeZone.c_str(), NTP_SERVER_1, NTP_SERVER_2);
    bool timeConfigured = waitForSNTPSync(&timeInfo);
    if (!timeConfigured) {
        ESP_LOGI(LOG_TAG, "%s", TXT_TIME_SYNCHRONIZATION_FAILED);
        killWiFi();
        initDisplay();
        do {
            drawError(wi_time_4_196x196, TXT_TIME_SYNCHRONIZATION_FAILED);
        } while (display.nextPage());
        powerOffDisplay();
        beginDeepSleep(startTime, &timeInfo);
        return;
    }

    // API CALL RATE LIMIT, see MIN_API_CALL_INTERVAL in config.h. The time of
    // the last attempt is kept in NVS, so it survives resets and brownouts, and
    // it is saved before the requests: a reboot during them counts too.
    const int64_t now = time(nullptr);
    prefs.begin(NVS_NAMESPACE, false);
    const int64_t lastApiCall = prefs.getLong64("lastApiCall", 0);
    if (lastApiCall > 0 && now >= lastApiCall
        && now - lastApiCall < MIN_API_CALL_INTERVAL) {
        ESP_LOGW(LOG_TAG, "Last API call %lld s ago, less than %d s: skipping this update",
            (long long)(now - lastApiCall), MIN_API_CALL_INTERVAL);
        prefs.putInt("locationsIndex", locationsIndex); // not shown, keep it next
        prefs.end();
        killWiFi();
        beginDeepSleep(startTime, &timeInfo);
        return;
    }
    prefs.putLong64("lastApiCall", now);
    prefs.end();

    // MAKE API REQUESTS
#ifdef USE_HTTP
    WiFiClient client;
#elif defined(USE_HTTPS_NO_CERT_VERIF)
    WiFiClientSecure client;
    client.setInsecure();
#elif defined(USE_HTTPS_WITH_CERT_VERIF)
    WiFiClientSecure client;
    client.setCACert(cert_Sectigo_Public_Server_Authentication_Root_R46);
#endif
    int rxStatus = getOWMonecall(client, owm_onecall, currentLat, currentLon);
    if (rxStatus != HTTP_CODE_OK) {
        killWiFi();
        statusStr = "One Call " + OWM_ONECALL_VERSION + " API";
        tmpStr = String(rxStatus, DEC) + ": " + getHttpResponsePhrase(rxStatus);
        initDisplay();
        do {
            drawError(wi_cloud_down_196x196, statusStr, tmpStr);
        } while (display.nextPage());
        powerOffDisplay();
        beginDeepSleep(startTime, &timeInfo);
        return;
    }
    rxStatus = getOWMairpollution(client, owm_air_pollution, currentLat, currentLon);
    //   if (rxStatus != HTTP_CODE_OK)
    //   {
    //     killWiFi();
    //     statusStr = "Air Pollution API";
    //     tmpStr = String(rxStatus, DEC) + ": " + getHttpResponsePhrase(rxStatus);
    //     initDisplay();
    //     do
    //     {
    //       drawError(wi_cloud_down_196x196, statusStr, tmpStr);
    //     } while (display.nextPage());
    //     powerOffDisplay();
    //     beginDeepSleep(startTime, &timeInfo);
    //   }
    killWiFi(); // WiFi no longer needed

    // GET INDOOR TEMPERATURE AND HUMIDITY, start BMEx80...

#if PIN_BME_PWR > -1
    ESP_LOGI(LOG_TAG, "Powering on BME sensor on pin: %d", PIN_BME_PWR);
    pinMode(PIN_BME_PWR, OUTPUT);
    digitalWrite(PIN_BME_PWR, HIGH);

#if defined(SENSOR_INIT_DELAY_MS) && SENSOR_INIT_DELAY_MS > 0
    delay(SENSOR_INIT_DELAY_MS);
#endif

#endif // PIN_BME_PWR

    float inTemp = NAN;
    float inHumidity = NAN;

#if defined(SENSOR_BME280) || defined(SENSOR_BME680)
    TwoWire I2C_bme = TwoWire(0);

    ESP_LOGI(LOG_TAG, "Initializing I2C bus on pins: %d, %d", PIN_BME_SDA, PIN_BME_SCL);

    // the sensor library's begin() starts the bus (100kHz) on these pins
    I2C_bme.setPins(PIN_BME_SDA, PIN_BME_SCL);

#if defined(SENSOR_BME280)
    ESP_LOGI(LOG_TAG, "%s BME280... ", TXT_READING_FROM);
    Adafruit_BME280 bme;
    bool bmeFound = bme.begin(BME_ADDRESS, &I2C_bme);
#else
    ESP_LOGI(LOG_TAG, "%s BME680... ", TXT_READING_FROM);
    Adafruit_BME680 bme(&I2C_bme);
    bool bmeFound = bme.begin(BME_ADDRESS);
#endif

    if (bmeFound) {
        inTemp = bme.readTemperature(); // Celsius
        inHumidity = bme.readHumidity(); // %

        // check if BME readings are valid
        // note: readings are checked again before drawing to screen. If a reading
        //       is not a number (NAN) then an error occurred, a dash '-' will be
        //       displayed.
        if (std::isnan(inTemp) || std::isnan(inHumidity)) {
            statusStr = "BME " + String(TXT_READ_FAILED);
        }
    } else {
        statusStr = "BME " + String(TXT_NOT_FOUND); // check wiring
        ESP_LOGI(LOG_TAG, "%s", statusStr.c_str());
    }

    I2C_bme.end();

#if PIN_BME_PWR > -1
    ESP_LOGI(LOG_TAG, "Powering off BME sensor.");
    digitalWrite(PIN_BME_PWR, LOW);
#endif // PIN_BME_PWR > -1

#endif // SENSOR_BME280 || SENSOR_BME680

#if DEBUG_LEVEL > 0
    ESP_LOGI(LOG_TAG, "BME readings: ");
    ESP_LOGI(LOG_TAG, "  Temperature: %.2f °C", inTemp);
    ESP_LOGI(LOG_TAG, "  Humidity: %.2f %%", inHumidity);
#endif

    String refreshTimeStr;
    getRefreshTimeStr(refreshTimeStr, timeConfigured, &timeInfo);
    String dateStr;
    getDateStr(dateStr, &timeInfo);

    // RENDER FULL REFRESH
    initDisplay();
    do {
        drawCurrentConditions(owm_onecall.current, owm_onecall.daily[0],
            owm_air_pollution, inTemp, inHumidity);
        drawOutlookGraph(owm_onecall.hourly, owm_onecall.daily, timeInfo);
        drawForecast(owm_onecall.daily, timeInfo);
        drawLocationDate(currentCity, dateStr);
#if DISPLAY_ALERTS
        drawAlerts(owm_onecall.alerts, currentCity, dateStr);
#endif
        drawStatusBar(statusStr, refreshTimeStr, wifiRSSI, batteryVoltage);
    } while (display.nextPage());
    powerOffDisplay();

    // DEEP SLEEP
    beginDeepSleep(startTime, &timeInfo);
} // end setup

/* This will never run
 */
void loop()
{
} // end loop
