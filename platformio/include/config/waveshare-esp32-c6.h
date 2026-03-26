
#include <Arduino.h>
#include <stdint.h>

#define WAVESHARE_ESP32_C6
#define BOARD_NAME "Waveshare ESP32-C6"

#define PIN_EPD_BUSY (int8_t)14
#define PIN_EPD_RST (int8_t)15
#define PIN_EPD_DC (int8_t)22
#define PIN_EPD_CS (int8_t)18
#define PIN_EPD_SCK (int8_t)21
#define PIN_EPD_MOSI (int8_t)19
#define PIN_EPD_MISO (int8_t)-1 // unused
#define PIN_EPD_PWR -1
#define PIN_BAT_ADC 1

#define PIN_BME_SDA (int8_t)4
#define PIN_BME_SCL (int8_t)5
#define PIN_BME_PWR -1

// Wakeup pin
#define PIN_WAKEUP GPIO_NUM_2

// Wakeup method (if not defined EXT0 will be used)
// Not used if PIN_WAKEUP is -1
#define USE_EXT1_WAKEUP

// Reset button to GND and PIN to PIN_WAKEUP with a 10kΩ pull-down resistor to VCC, so wakeup is triggered when button is pressed and GPIO goes LOW
#define WAKEUP_PIN_MODE ESP_EXT1_WAKEUP_ANY_LOW

// BME I2C Address
// 0x76 if SDO -> GND; 0x77 if SDO -> VCC
#define BME_ADDRESS 0x76

#define USE_HTTPS_NO_CERT_VERIF

// Display type
#define DISP_BW_V2

// Display driver
#define DRIVER_DESPI_C02

// Accent color
#define ACCENT_COLOR GxEPD_BLACK
#define ACCENT_COLOR2 GxEPD_BLACK

// Temperature sensor
#define SENSOR_BME280

// Battery monitoring
#define BATTERY_MONITORING 1
#define DELAY_BEFORE_SLEEP 5000

// R1 = 680kΩ, R2 = 330kΩ (Vout = Vin * R2 / (R1 + R2))
// #define BATTERY_RESISTOR_DIVIDER 0.31984678
// #define BATTERY_RESISTOR_DIVIDER 0.3219753086 // casa gavirate
#define BATTERY_RESISTOR_DIVIDER 0.3261845387

// Debug level
#define DEBUG_LEVEL 1

#define LOCALE it_IT
#define FONT_HEADER "fonts/Roboto_Regular.h"
