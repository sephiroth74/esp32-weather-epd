#include "display_utils_extra.h"
#include "config.h"
#include <Arduino.h>

static TaskHandle_t ledPulsingTaskHandle = NULL;
static const uint32_t LED_PULSE_INTERVAL_MS = 500; // Pulse every 500ms (on/off cycle)

#if defined(LED_BUILTIN) && defined(HAS_BUILTIN_LED)
/**
 * LED pulsing task - toggles LED on/off at regular intervals
 */
void ledPulsingTask(void* parameter)
{
    bool ledState = false;
    pinMode(LED_BUILTIN, OUTPUT);

    while (true) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
        vTaskDelay(pdMS_TO_TICKS(LED_PULSE_INTERVAL_MS));
    }
} // end ledPulsingTask
#endif // defined(LED_BUILTIN) && defined(HAS_BUILTIN_LED)

void startLEDPulsing()
{
#if defined(LED_BUILTIN) && defined(HAS_BUILTIN_LED)
    ESP_LOGI("LED", "Starting LED pulsing on pin %d", LED_BUILTIN);

    if (ledPulsingTaskHandle == NULL) {
        xTaskCreate(
            ledPulsingTask, // Task function
            "LEDPulse", // Task name
            2048, // Stack size (bytes)
            NULL, // Task parameter
            1, // Priority
            &ledPulsingTaskHandle // Task handle
        );
    }
#endif // defined(LED_BUILTIN) && defined(HAS_BUILTIN_LED)
} // end startLEDPulsing

void stopLEDPulsing()
{
#if defined(LED_BUILTIN) && defined(HAS_BUILTIN_LED)
    ESP_LOGI("LED", "Stopping LED pulsing");

    if (ledPulsingTaskHandle != NULL) {
        vTaskDelete(ledPulsingTaskHandle);
        ledPulsingTaskHandle = NULL;
    }

    // Turn off LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif // defined(LED_BUILTIN) && defined(HAS_BUILTIN_LED)
} // end stopLEDPulsing

void enableBuiltinLED()
{
#if defined(LED_BUILTIN) && defined(HAS_BUILTIN_LED)
    ESP_LOGI("LED", "Enabling builtin LED on pin %d", LED_BUILTIN);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
#endif
} // end enableBuiltinLED