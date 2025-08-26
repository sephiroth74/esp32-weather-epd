// MIT License
//
// Copyright (c) 2025 Alessandro Crugnola
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "battery.h"
#include "config.h"

#include <Arduino.h>
#include <cmath>

#ifdef CONFIG_IDF_TARGET_ESP32C6
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_oneshot.h>
#else
#include <driver/adc.h>
#include <esp_adc_cal.h>
#endif // CONFIG_IDF_TARGET_ESP32C6

namespace battery {

uint8_t calc_battery_percentage(uint32_t v)
{
    if (v >= steps[total_steps - 1].voltage)
        return steps[total_steps - 1].percent;
    if (v <= steps[0].voltage)
        return steps[0].percent;

    for (int8_t i = total_steps - 1; i > 0; i--) {
        battery_step_t current = steps[i];
        battery_step_t previous = steps[i - 1];
        if (v >= previous.voltage && v <= current.voltage) {
            return map(v, previous.voltage, current.voltage, previous.percent, current.percent);
        }
    }
    return 0;
} // calc_battery_percentage

bool battery_info_t::is_low() const { return millivolts <= battery_info::from_percent(10, 3690).voltage; }

bool battery_info_t::is_critical() const { return millivolts <= battery_info::from_percent(8, 3650).voltage; }

bool battery_info_t::is_empty() const { return millivolts <= battery_info::from_percent(5, 3610).voltage; }

bool battery_info_t::is_charging() const { return millivolts > battery_info::from_percent(100, 4200).voltage + 100; }

String battery_info_t::to_string() const
{
    String result;
    result += "Battery Info:\n";
    result += "  Raw Analog: " + String(raw_analog) + "\n";
    result += "  Raw Millivolts: " + String(raw_millivolts) + "\n";
    result += "  Millivolts: " + String(millivolts) + "\n";
    result += "  Percent: " + String(percent) + "\n";
    return result;
}

void BatteryReader::init() const
{
    Serial.print(F("Initializing BatteryReader on pin "));
    Serial.println(pin);
    pinMode(pin, INPUT);
    // Set the pin to use the ADC with no attenuation
    analogSetPinAttenuation(pin, ADC_11db);
    delay(200); // Allow some time for the ADC to stabilize

#if DEBUG_LEVEL > 0
    Serial.print(F("BatteryReader initialized on pin "));
    Serial.println(pin);
#endif // DEBUG_LEVEL
} // init

battery_info_t BatteryReader::read() const
{
    uint32_t raw_millivolts = 0;
    uint32_t raw_analog = 0;
    for (int i = 0; i < num_readings; i++) {
        raw_millivolts += analogReadMilliVolts(pin);
        raw_analog += analogRead(pin);
        delay(delay_between_readings);
    }

    raw_millivolts /= num_readings;

    uint32_t voltage = raw_millivolts / resistor_ratio;
    uint8_t percent = calc_battery_percentage(voltage);

#if DEBUG_LEVEL > 1
    Serial.println("Battery readings: ");
    Serial.print("resistor_ratio: ");
    Serial.printf("%4f", resistor_ratio);
    Serial.print("raw: ");
    Serial.println(raw_analog);
    Serial.print("millivolts: ");
    Serial.println(raw_millivolts);
    Serial.print("voltage (millivolts/ratio): ");
    Serial.println(voltage);
    Serial.print("percent: ");
    Serial.println(percent);
    Serial.println("--------------------------------------");
#endif // DEBUG_LEVEL

    return battery_info(
        raw_analog /* raw_analog */,
        raw_millivolts /* raw_millivolts */,
        voltage /* adjusted millivolts */,
        percent /* percent */);
} // read

} // namespace battery