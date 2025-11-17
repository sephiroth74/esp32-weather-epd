
#include <Arduino.h>
#include <stdint.h>

#define SEEDSTUDIO_ESP32_C6

#define PIN_EPD_BUSY (int8_t)D9 // 20
#define PIN_EPD_RST (int8_t)D7 // 17
#define PIN_EPD_DC (int8_t)D3 // 21
#define PIN_EPD_CS (int8_t)D6 // 16
#define PIN_EPD_SCK (int8_t)D8 // 19
#define PIN_EPD_MOSI (int8_t)D10 // 18
#define PIN_EPD_PWR -1
#define PIN_BAT_ADC D0 // D0

#define PIN_BME_SDA (int8_t)D4 // 22
#define PIN_BME_SCL (int8_t)D5 // 23
#define PIN_BME_PWR -1

// Wakeup pin
#define PIN_WAKEUP GPIO_NUM_2

// Wakeup method (if not defined EXT0 will be used)
// Not used if PIN_WAKEUP is -1
#define USE_EXT1_WAKEUP

// BME I2C Address
// 0x76 if SDO -> GND; 0x77 if SDO -> VCC
#define BME_ADDRESS 0x76

// Built-in LED
// #define HAS_BUILTIN_LED

#define USE_HTTP

// Display type
#define DISP_7C_F
// #define DISP_BW_V2

// Display driver
#define DRIVER_DESPI_C02
// #define DRIVER_WAVESHARE

// Accent color
// #define ACCENT_COLOR GxEPD_BLACK
#define ACCENT_COLOR GxEPD_RED
// #define ACCENT_COLOR GxEPD_GREEN
// #define ACCENT_COLOR GxEPD_BLUE
// #define ACCENT_COLOR GxEPD_ORANGE
// #define ACCENT_COLOR GxEPD_YELLOW


#define ACCENT_COLOR2 GxEPD_GREEN

// HSPI for EPD
// #define USE_HSPI_FOR_EPD

// Temperature sensor
#define SENSOR_BME280
// #define SENSOR_BME680

// Battery monitoring
#define BATTERY_POWER_SAVING 1

// R1 = 680kΩ, R2 = 330kΩ (Vout = Vin * R2 / (R1 + R2))
#define BATTERY_RESISTOR_DIVIDER 0.3266666667

// Debug level
#define DEBUG_LEVEL 1
#define DEBUG_BATTERY 1

// Delay before sleep
#define DELAY_BEFORE_SLEEP 10000
#define DISABLE_DEEP_SLEEP 1

// Sleep configuration
#define CONFIG_SLEEP_DURATION 120
#define CONFIG_BED_TIME 22
#define CONFIG_WAKE_TIME 5

#define CONFIG_HOURLY_GRAPH_MAX 24

#define LOCALE it_IT

#define FONT_HEADER "fonts/Ubuntu_R.h"

// Location(s)
#define NUM_LOCATIONS 1
