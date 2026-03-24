/* Main program for esp32-weather-epd.
 * Copyright (C) 2022-2025  Luke Marzen
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

#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>
#include <time.h>

#include "_locale.h"
#include "api_response.h"
#include "battery.h"
#include "client_utils.h"
#include "config.h"
#include "display_utils.h"
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

#ifndef USE_EXT1_WAKEUP
#include "driver/rtc_io.h" // For RTC GPIO functions needed for EXT0 wakeup
#endif

// too large to allocate locally on stack
static owm_resp_onecall_t owm_onecall;
static owm_resp_air_pollution_t owm_air_pollution;

Preferences prefs;

void printWakeupReason()
{
    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();
    switch (wakeupCause) {
    case ESP_SLEEP_WAKEUP_EXT0:
        ESP_LOGD(LOG_TAG, "Wakeup caused by external signal using EXT0");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        ESP_LOGD(LOG_TAG, "Wakeup caused by external signal using EXT1");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        ESP_LOGD(LOG_TAG, "Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        ESP_LOGD(LOG_TAG, "Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        ESP_LOGD(LOG_TAG, "Wakeup caused by ULP");
        break;
    default:
        ESP_LOGD(LOG_TAG, "Wakeup caused by unknown reason");
        break;
    }

    if (wakeupCause == ESP_SLEEP_WAKEUP_EXT1) {
        uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
        // Print the raw value returned by esp_sleep_get_ext1_wakeup_status. This is the bitmask of the pin/s that triggered wake up
        ESP_LOGD(LOG_TAG, "Raw bitmask value returned: %llu", GPIO_reason);

        // Using log method to work out trigger pin. This is the method used by Random Nerd Tutorials
        ESP_LOGD(LOG_TAG, "GPIO that triggered the wake up calculated using log method: ");
        int wakeupPin = log(GPIO_reason) / log(2);
        ESP_LOGD(LOG_TAG, "Wakeup pin: %d", wakeupPin);
    }
}

void enterDeepSleep()
{
    // Stop LED pulsing and turn off LED
    stopLEDPulsing();

#if defined(DELAY_BEFORE_SLEEP) && DELAY_BEFORE_SLEEP > 0
    ESP_LOGD(LOG_TAG, "Delaying before deep sleep for %d seconds", DELAY_BEFORE_SLEEP / 1000);
    delay(DELAY_BEFORE_SLEEP);
#endif // DELAY_BEFORE_SLEEP

#ifdef PIN_WAKEUP
    // enable wakeup on GPIO
    ESP_LOGD(LOG_TAG, "Enabling wakeup on GPIO %d", PIN_WAKEUP);

#ifdef USE_EXT1_WAKEUP
    ESP_LOGD(LOG_TAG, "Using EXT1 wakeup method");
    // Configure GPIO for EXT1 wakeup
    // Button connected to GPIO2 and GND: when pressed GPIO goes LOW
    // IMPORTANT: Add external 10kΩ pull-up resistor between GPIO2 and 3V3 for stability!
    pinMode(PIN_WAKEUP, INPUT_PULLUP);

    esp_err_t result = esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(PIN_WAKEUP), ESP_EXT1_WAKEUP_ANY_LOW);

    if (result != ESP_OK) {
        ESP_LOGE(LOG_TAG, "[error] Failed to enable EXT1 wakeup");
    }
#endif // USE_EXT1_WAKEUP

#else
    ESP_LOGD(LOG_TAG, "No wakeup pin defined, deep sleep will not be exited until reset or power cycle");
#endif // PIN_WAKEUP

#if !defined(DISABLE_DEEP_SLEEP)
    ESP_LOGD(LOG_TAG, "Entering deep sleep now!");
    Serial.flush();
    esp_deep_sleep_start();
#else
    ESP_LOGD(LOG_TAG, "Deep sleep disabled. Staying awake.");
#endif // DISABLE_DEEP_SLEEP
}

/* Put esp32 into ultra low-power deep sleep (<11μA).
 * Aligns wake time to the minute. Sleep times defined in config.cpp.
 */
void beginDeepSleep(unsigned long startTime, tm* timeInfo)
{
    if (!getLocalTime(timeInfo)) {
        ESP_LOGD(LOG_TAG, "%s", TXT_REFERENCING_OLDER_TIME_NOTICE);
    }

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
    const int curSecond = curHour * 3600 + timeInfo->tm_min * 60 + timeInfo->tm_sec;
    const int desiredSleepSeconds = SLEEP_DURATION * 60;
    const int offsetMinutes = curMinute % SLEEP_DURATION;
    const int offsetSeconds = curSecond % desiredSleepSeconds;

    ESP_LOGD(LOG_TAG, "curHour: %d", curHour);
    ESP_LOGD(LOG_TAG, "curMinute: %d", curMinute);
    ESP_LOGD(LOG_TAG, "curSecond: %d", curSecond);
    ESP_LOGD(LOG_TAG, "desiredSleepSeconds: %d", desiredSleepSeconds);
    ESP_LOGD(LOG_TAG, "offsetMinutes: %d", offsetMinutes);
    ESP_LOGD(LOG_TAG, "offsetSeconds: %d", offsetSeconds);

    // align wake time to nearest multiple of SLEEP_DURATION
    int sleepMinutes = SLEEP_DURATION - offsetMinutes;
    ESP_LOGD(LOG_TAG, "sleepMinutes: %d", sleepMinutes);
    if (desiredSleepSeconds - offsetSeconds < 120 || offsetSeconds / (float)desiredSleepSeconds > 0.95f) { // if we have a sleep time less than 2 minutes OR less 5% SLEEP_DURATION,
        // skip to next alignment
        sleepMinutes += SLEEP_DURATION;
        ESP_LOGD(LOG_TAG, "Adjusted sleepMinutes: %d", sleepMinutes);
    }

    // estimated wake time, if this falls in a sleep period then sleepDuration
    // must be adjusted
    const int predictedWakeHour = ((curMinute + sleepMinutes) / 60) % 24;
    ESP_LOGD(LOG_TAG, "predictedWakeHour: %d", predictedWakeHour);

    uint64_t sleepDuration;
    if (predictedWakeHour < bedtimeHour) {
        sleepDuration = sleepMinutes * 60 - timeInfo->tm_sec;
        ESP_LOGD(LOG_TAG, "Sleep duration (before bedtime): %llu", sleepDuration);
    } else {
        const int hoursUntilWake = 24 - curHour;
        sleepDuration = hoursUntilWake * 3600ULL - (timeInfo->tm_min * 60ULL + timeInfo->tm_sec);
        ESP_LOGD(LOG_TAG, "Sleep duration (after bedtime): %llu", sleepDuration);
    }

    // add extra delay to compensate for esp32's with fast RTCs.
    sleepDuration += 3ULL;
    sleepDuration *= 1.0015f;

    esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL);

    ESP_LOGD(LOG_TAG, "%s %.3f s", TXT_AWAKE_FOR, (millis() - startTime) / 1000.0);
    ESP_LOGD(LOG_TAG, "%s %llu s", TXT_ENTERING_DEEP_SLEEP_FOR, sleepDuration);

#if DEBUG_LEVEL >= 1
    // Print next wakeup time for debugging purposes
    time_t nextWakeTime = time(nullptr) + sleepDuration;
    struct tm* nextWakeTimeInfo = localtime(&nextWakeTime);
    ESP_LOGD(LOG_TAG, "Next wakeup time: %s", asctime(nextWakeTimeInfo));
#endif

    enterDeepSleep();
} // end beginDeepSleep

/* Program entry point.
 */
void setup()
{
    unsigned long startTime = millis();
    Serial.begin(115200);

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(LOG_TAG, ESP_LOG_DEBUG);

    delay(2000); // wait for serial monitor to open

    ESP_LOGI(LOG_TAG, "Starting up...");

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

    // Start LED pulsing to indicate activity
    startLEDPulsing();

    // Print heap usage for debugging purposes
    printHeapUsage();

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

#if DEBUG_LEVEL >= 1
    ESP_LOGI(LOG_TAG, "Location index: %d", locationsIndex);
    ESP_LOGI(LOG_TAG, "Location: %s", currentCity.c_str());
    ESP_LOGI(LOG_TAG, "Latitude: %s", currentLat.c_str());
    ESP_LOGI(LOG_TAG, "Longitude: %s", currentLon.c_str());
    ESP_LOGI(LOG_TAG, "Timezone: %s", currentTimeZone.c_str());
#endif

#if PIN_BAT_ADC > -1
    ESP_LOGI(LOG_TAG, "Battery ADC pin: %d", PIN_BAT_ADC);
    battery::battery_info_t battery_info;

    ESP_LOGI(LOG_TAG, "Battery resistor divider: %.8f", BATTERY_RESISTOR_DIVIDER);

    battery::BatteryReader battery_reader(PIN_BAT_ADC,
        BATTERY_RESISTOR_DIVIDER,
        BATTERY_NUM_SAMPLES,
        BATTERY_DELAY_MS);

    battery_reader.init();
    battery_info = battery_reader.read();

    ESP_LOGI(LOG_TAG, "Battery info: %s", battery_info.to_string().c_str());

    // When the battery is low, the display should be updated to reflect that, but
    // only the first time we detect low voltage. The next time the display will
    // refresh is when voltage is no longer low. To keep track of that we will
    // make use of non-volatile storage.
    bool lowBat = prefs.getBool("lowBat", false);

    ESP_LOGI(LOG_TAG, "lowBat: %d", lowBat);

#if BATTERY_POWER_SAVING
    auto batteryVoltage = battery_info.millivolts;

    // low battery, deep sleep now
    if (batteryVoltage <= LOW_BATTERY_VOLTAGE) {
        ESP_LOGI(LOG_TAG, "%d <= %d", batteryVoltage, LOW_BATTERY_VOLTAGE);

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
            ESP_LOGI(LOG_TAG, "%d <= %d", batteryVoltage, CRIT_LOW_BATTERY_VOLTAGE);

            // don't set esp_sleep_enable_timer_wakeup();
            // We won't wake up again until someone manually presses the RST button.
            ESP_LOGI(LOG_TAG, "%s", TXT_CRIT_LOW_BATTERY_VOLTAGE);
            ESP_LOGI(LOG_TAG, "%s", TXT_HIBERNATING_INDEFINITELY_NOTICE);
        } else if (batteryVoltage <= VERY_LOW_BATTERY_VOLTAGE) { // very low battery
            ESP_LOGI(LOG_TAG, "%d <= %d", batteryVoltage, VERY_LOW_BATTERY_VOLTAGE);

            esp_sleep_enable_timer_wakeup(VERY_LOW_BATTERY_SLEEP_INTERVAL * 60ULL * 1000000ULL);
            ESP_LOGI(LOG_TAG, "%s", TXT_VERY_LOW_BATTERY_VOLTAGE);
            ESP_LOGI(LOG_TAG, "%s %d min", TXT_ENTERING_DEEP_SLEEP_FOR, VERY_LOW_BATTERY_SLEEP_INTERVAL);
        } else { // low battery
            ESP_LOGI(LOG_TAG, "%d <= %d", batteryVoltage, LOW_BATTERY_VOLTAGE);

            esp_sleep_enable_timer_wakeup(LOW_BATTERY_SLEEP_INTERVAL * 60ULL * 1000000ULL);
            ESP_LOGI(LOG_TAG, "%s", TXT_LOW_BATTERY_VOLTAGE);
            ESP_LOGI(LOG_TAG, "%s %d min", TXT_ENTERING_DEEP_SLEEP_FOR, LOW_BATTERY_SLEEP_INTERVAL);
        }
        delay(DELAY_BEFORE_SLEEP);
        enterDeepSleep();
    }
    // battery is no longer low, reset variable in non-volatile storage
    if (lowBat == true) {
        prefs.putBool("lowBat", false);
    }
#endif
#else
    battery::battery_info battery_info = battery::battery_info::full();
    auto batteryVoltage = battery_info.millivolts;
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
    }

    // Print current local time for debugging purposes
    ESP_LOGI(LOG_TAG, "Current local time: %s", asctime(&timeInfo));

    // MAKE API REQUESTS
#ifdef USE_HTTP
    WiFiClient client;
#elif defined(USE_HTTPS_NO_CERT_VERIF)
    WiFiClientSecure client;
    client.setInsecure();
#elif defined(USE_HTTPS_WITH_CERT_VERIF)
    WiFiClientSecure client;
    client.setCACert(cert_Sectigo_RSA_Organization_Validation_Secure_Server_CA);
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
    }
    rxStatus = getOWMairpollution(client, owm_air_pollution, currentLat, currentLon);

    ESP_LOGI(LOG_TAG, "Air Pollution Success: %d", owm_air_pollution.success);

    killWiFi(); // WiFi no longer needed

    // GET INDOOR TEMPERATURE AND HUMIDITY, start BMEx80...

#if PIN_BME_PWR > -1
    ESP_LOGI(LOG_TAG, "Powering on BME sensor on pin: %d", PIN_BME_PWR);
    pinMode(PIN_BME_PWR, OUTPUT);
    digitalWrite(PIN_BME_PWR, HIGH);
#endif // PIN_BME_PWR

    float inTemp = NAN;
    float inHumidity = NAN;

#if defined(SENSOR_BME280) || defined(SENSOR_BME680)

#if defined(SENSOR_BME280)
    ESP_LOGI(LOG_TAG, "%s BME280... ", TXT_READING_FROM);
    Adafruit_BME280 bme;

    TwoWire I2C_bme = TwoWire(0);

    ESP_LOGI(LOG_TAG, "Initializing I2C bus on pins: %d, %d", PIN_BME_SDA, PIN_BME_SCL);

#ifdef CONFIG_IDF_TARGET_ESP32C6
    I2C_bme.setPins(PIN_BME_SDA, PIN_BME_SCL);
#else
    I2C_bme.begin(PIN_BME_SDA, PIN_BME_SCL, 100'000); // 100kHz
#endif

    if (bme.begin(BME_ADDRESS, &I2C_bme)) {
#endif
#if defined(SENSOR_BME680)
        ESP_LOGI(LOG_TAG, "%s BME680... ", TXT_READING_FROM);
        Adafruit_BME680 bme(&I2C_bme);

        if (bme.begin(BME_ADDRESS)) {
#endif
            inTemp = bme.readTemperature(); // Celsius
            inHumidity = bme.readHumidity(); // %

            // check if BME readings are valid
            // note: readings are checked again before drawing to screen. If a reading
            //       is not a number (NAN) then an error occurred, a dash '-' will be
            //       displayed.
            if (std::isnan(inTemp) || std::isnan(inHumidity)) {
                statusStr = "BME " + String(TXT_READ_FAILED);
                ESP_LOGI(LOG_TAG, "%s", statusStr.c_str());
            } else {
                ESP_LOGI(LOG_TAG, "%s", TXT_SUCCESS);
            }
        } else {
            statusStr = "BME " + String(TXT_NOT_FOUND); // check wiring
            ESP_LOGI(LOG_TAG, "%s", statusStr.c_str());
        }

#if PIN_BME_PWR > -1
        ESP_LOGI(LOG_TAG, "Powering off BME sensor.");
        digitalWrite(PIN_BME_PWR, LOW);
#endif // PIN_BME_PWR > 0

#endif // SENSOR_BME280 || SENSOR_BME680

        Wire.end();

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
            drawCurrentConditions(
                owm_onecall.current, owm_onecall.daily[0], owm_air_pollution, inTemp, inHumidity);
            drawOutlookGraph(owm_onecall.hourly, owm_onecall.daily, timeInfo);
            drawForecast(owm_onecall.daily, timeInfo);
            drawLocationDate(currentCity, dateStr);
#if DISPLAY_ALERTS
            drawAlerts(owm_onecall.alerts, currentCity, dateStr);
#endif
            drawStatusBar(statusStr, refreshTimeStr, wifiRSSI, battery_info);
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
