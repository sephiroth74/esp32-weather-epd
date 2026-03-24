#include "config.h"
#include "board_utils.h"
#include "display_utils_extra.h"

#ifndef USE_EXT1_WAKEUP
#include "driver/rtc_io.h" // For RTC GPIO functions needed for EXT0 wakeup
#endif

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