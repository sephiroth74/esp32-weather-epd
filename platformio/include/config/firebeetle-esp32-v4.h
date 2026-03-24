
#include <Arduino.h>
#include <stdint.h>

#define WAVESHARE_ESP32_C6
#define BOARD_NAME "Waveshare ESP32-C6"

// sda=22 scl=23
// MOSI pin: 19
// MISO pin: 20
// SCK pin: 21
// SS pin: 18

#define PIN_EPD_BUSY (int8_t)14
#define PIN_EPD_RST (int8_t)21
#define PIN_EPD_DC (int8_t)22
#define PIN_EPD_CS (int8_t)13
#define PIN_EPD_SCK (int8_t)18
#define PIN_EPD_MOSI (int8_t)23
#define PIN_EPD_MISO (int8_t)-1 // unused
#define PIN_EPD_PWR -1
#define PIN_BAT_ADC 1

#define PIN_BME_SDA (int8_t)25 // 22
#define PIN_BME_SCL (int8_t)26 // 23
#define PIN_BME_PWR -1

// Wakeup pin
#define PIN_WAKEUP GPIO_NUM_2

// Wakeup method (if not defined EXT0 will be used)
// Not used if PIN_WAKEUP is -1
#define USE_EXT1_WAKEUP

// BME I2C Address
// 0x76 if SDO -> GND; 0x77 if SDO -> VCC
#define BME_ADDRESS 0x76

#define USE_HTTP

// Display type
// #define DISP_7C_F
// #define DISP_6C_F
#define DISP_BW_V2

// Display driver
#define DRIVER_DESPI_C02
// #define DRIVER_WAVESHARE

// Accent color
#define ACCENT_COLOR GxEPD_BLACK
// #define ACCENT_COLOR GxEPD_RED
// #define ACCENT_COLOR GxEPD_GREEN
// #define ACCENT_COLOR GxEPD_BLUE
// #define ACCENT_COLOR GxEPD_ORANGE
// #define ACCENT_COLOR GxEPD_YELLOW

// #define ACCENT_COLOR2 GxEPD_GREEN
#define ACCENT_COLOR2 GxEPD_BLACK

// HSPI for EPD
// #define USE_HSPI_FOR_EPD

// Temperature sensor
#define SENSOR_BME280
// #define SENSOR_BME680

// Battery monitoring
#define DISABLE_DEEP_SLEEP
// #define BATTERY_MONITORING 1
// #define BATTERY_POWER_SAVING 1

// R1 = 330kΩ, R2 = 180kΩ (Vout = Vin * R2 / (R1 + R2))
// #define BATTERY_RESISTOR_DIVIDER 0.31984678
// #define BATTERY_RESISTOR_DIVIDER 0.3219753086 // casa gavirate
#define BATTERY_RESISTOR_DIVIDER 0.2527075812

// Debug level
#define DEBUG_LEVEL 1

// Delay before sleep
#define DELAY_BEFORE_SLEEP 5000
// #define DISABLE_DEEP_SLEEP

#define LOCALE it_IT
#define FONT_HEADER "fonts/Roboto_Regular.h"
