
#include <Arduino.h>
#include <stdint.h>

#define WAVESHARE_ESP32_S3_ZERO

#define PIN_EPD_BUSY (int8_t)13
#define PIN_EPD_RST (int8_t)7
#define PIN_EPD_DC (int8_t)4
#define PIN_EPD_CS (int8_t)SS // 10
#define PIN_EPD_SCK (int8_t)SCK // 12
#define PIN_EPD_MOSI (int8_t)MOSI // 11
#define PIN_EPD_PWR -1
#define PIN_BAT_ADC 1

#define PIN_BME_SDA (int8_t)SDA // 8
#define PIN_BME_SCL (int8_t)SCL // 9
#define PIN_BME_PWR 6

// Wakeup pin
#define PIN_WAKEUP GPIO_NUM_2

// BME I2C Address
// 0x76 if SDO -> GND; 0x77 if SDO -> VCC
#define BME_ADDRESS 0x76

// Built-in LED
// #define HAS_BUILTIN_LED

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
#define BATTERY_RESISTOR_DIVIDER 0.3247030879 

// Debug level
#define DEBUG_LEVEL 1
#define DEBUG_BATTERY 0

// Delay before sleep
#define DELAY_BEFORE_SLEEP 0

// Sleep configuration
#define CONFIG_SLEEP_DURATION 120
#define CONFIG_BED_TIME 23
#define CONFIG_WAKE_TIME 06

#define CONFIG_HOURLY_GRAPH_MAX 24

#define LOCALE it_IT

#define FONT_HEADER "fonts/OpenSans_Regular.h"

// Location(s)
#define NUM_LOCATIONS 1
