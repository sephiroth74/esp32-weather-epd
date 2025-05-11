#ifndef BATTERY_H__
#define BATTERY_H__

#include <stdlib.h>

#define BATTERY_READINGS   10      // Number of readings to average
#define BATTERY_ADJUSTMENT 1.06548 // Adjust for voltage divider

typedef struct {
    uint32_t raw;          // raw ADC value
    uint32_t inputVoltage; // in mV
    uint32_t voltage;         // in mV
    float percent;
    void print();
} battery_t;

void read_battery_with_resistor(battery_t* bat,
                                uint8_t pin,
                                uint32_t r1,
                                uint32_t r2,
                                uint32_t minV,
                                uint32_t maxV);

void read_battery(battery_t* bat, uint8_t pin, uint32_t minV, uint32_t maxV);

#endif // BATTERY_H__