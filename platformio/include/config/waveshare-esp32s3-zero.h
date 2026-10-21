
#include <Arduino.h>
#include <stdint.h>

#define WAVESHARE_ESP32_S3_ZERO

#define BOARD_NAME "Waveshare ESP32-S3 Zero"

#define PIN_EPD_BUSY (int8_t)14
#define PIN_EPD_RST (int8_t)15
#define PIN_EPD_DC (int8_t)18
#define PIN_EPD_CS (int8_t)4
#define PIN_EPD_SCK (int8_t)5
#define PIN_EPD_MOSI (int8_t)21
#define PIN_EPD_MISO -1
#define PIN_EPD_PWR -1
#define PIN_BAT_ADC 1

#define PIN_BME_SDA (int8_t)SDA // 8
#define PIN_BME_SCL (int8_t)SCL // 9
#define PIN_BME_PWR -1

// Wakeup pin
#define PIN_WAKEUP GPIO_NUM_2

#define USE_EXT1_WAKEUP

// Wakeup on HIGH: the button goes between PIN_WAKEUP and 3V3, a 10kΩ
// pull-down resistor from PIN_WAKEUP to GND is recommended. The internal
// pull-down is also enabled during deep sleep.
#define WAKEUP_PIN_MODE ESP_EXT1_WAKEUP_ANY_HIGH

// BME I2C Address
// 0x76 if SDO -> GND; 0x77 if SDO -> VCC
#define BME_ADDRESS 0x76

// Built-in LED
// #define HAS_BUILTIN_LED

#define USE_HTTPS_NO_CERT_VERIF

// Display type
// #define DISP_7C_F
#define DISP_BW_V2

// Display driver
#define DRIVER_DESPI_C02
// #define DRIVER_WAVESHARE

// HSPI for EPD
// #define USE_HSPI_FOR_EPD

// Temperature sensor
#define SENSOR_BME280
// #define SENSOR_BME680

// Battery monitoring
#define BATTERY_RESISTOR_DIVIDER 0.2527075812

// R1 = 680kΩ, R2 = 330kΩ (Vout = Vin * R2 / (R1 + R2))
// #define BATTERY_RESISTOR_DIVIDER 0.4984

#define LOCALE it_IT
#define FONT_HEADER "fonts/Roboto_Regular.h"
