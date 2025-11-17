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

#ifndef __BATTERY_READER_H__
#define __BATTERY_READER_H__

#include <Arduino.h>
#include <stdint.h>

namespace battery {

/**
 * Calculates the battery percentage based on the voltage.
 * @param v The voltage in millivolts.
 * @return The battery percentage (0-100).
 */
uint8_t calc_battery_percentage(uint32_t v);

typedef struct battery_step {
    uint8_t percent;
    uint16_t voltage;

    constexpr battery_step(uint16_t percent, uint16_t voltage)
        : percent(percent)
        , voltage(voltage)
    {
    }
} battery_step_t;

typedef struct battery_info {
public:
    uint32_t raw_analog; // raw ADC reading
    uint32_t raw_millivolts; // raw battery voltage in millivolts
    uint32_t millivolts;
    uint8_t percent;

    float batteryVoltage1; // battery voltage after applying resistor divider

    constexpr battery_info(
        uint32_t raw,
        uint32_t raw_millivolts,
        uint32_t millivolts,
        uint8_t percent,
        float batteryVoltage1 = 0)
        : raw_analog(raw)
        , raw_millivolts(raw_millivolts)
        , millivolts(millivolts)
        , percent(percent)
        , batteryVoltage1(batteryVoltage1)
    {
    }

    constexpr battery_info()
        : battery_info(0, 0, 0, 0, 0)
    {
    }

    constexpr battery_info(const battery_step_t& step)
        : battery_info(0, step.voltage, step.voltage, step.percent)
    {
    }

    constexpr bool operator==(const battery_info& other) const
    {
        return (raw_millivolts == other.raw_millivolts && millivolts == other.millivolts && percent == other.percent);
    }

    static inline constexpr battery_info empty() { return battery_info(0, 0, 0, 0); }

    static inline constexpr battery_info full() { return battery_info(0, 4200, 4200, 100); }

    bool is_low() const;

    bool is_critical() const;

    bool is_empty() const;

    /**
     * Checks if the battery is currently charging.
     * @return True if the battery is charging, false otherwise.
     */
    bool is_charging() const;

    String to_string() const;

    static battery_step_t from_percent(uint8_t percent, uint16_t voltage = 0);

} battery_info_t;

// Battery steps for voltage to percentage mapping
constexpr battery_step_t steps[22] = {
    battery_step(0, 3220),
    battery_step(5, 3560),
    battery_step(8, 3600),
    battery_step(10, 3640),
    battery_step(15, 3660),
    battery_step(20, 3680),
    battery_step(25, 3700),
    battery_step(30, 3720),
    battery_step(35, 3740),
    battery_step(40, 3750),
    battery_step(45, 3770),
    battery_step(50, 3790),
    battery_step(55, 3800),
    battery_step(60, 3820),
    battery_step(65, 3860),
    battery_step(70, 3900),
    battery_step(75, 3930),
    battery_step(80, 3970),
    battery_step(85, 4030),
    battery_step(90, 4070),
    battery_step(95, 4100),
    battery_step(100, 4150),
};

constexpr uint8_t total_steps = 22;

class BatteryReader {
public:
    uint8_t pin;
    double resistor_ratio;
    uint8_t num_readings;
    uint32_t delay_between_readings;

    constexpr BatteryReader(uint8_t pin,
        double resistor_ratio,
        uint8_t num_readings,
        uint32_t delay)
        : pin(pin)
        , resistor_ratio(resistor_ratio)
        , num_readings(num_readings)
        , delay_between_readings(delay)
    {
    }

    /**
     * Initializes the battery reader by setting the pin mode and ADC attenuation.
     */
    void init() const;

    /**
     * Reads the battery voltage and returns a battery_info_t structure containing
     * the raw value, millivolts, and percentage.
     * @return A battery_info_t structure with the battery information.
     */
    battery_info_t read() const;

}; // BatteryReader

} // namespace battery

#endif // __BATTERY_READER_H__