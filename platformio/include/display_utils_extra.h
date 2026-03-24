#ifndef __DISPLAY_UTILS_EXTRA_H__
#define __DISPLAY_UTILS_EXTRA_H__

/**
 * Start pulsing the built-in LED. This is used to indicate that the device is
 * awake and running, and can be used for debugging purposes. The LED will pulse
 * on and off at regular intervals until the stopLEDPulsing function is called.
 */
void startLEDPulsing();

/**
 * Stops pulsing the built-in LED. This will stop the LED from pulsing and turn it off. This is typically called before the device enters deep sleep mode to
 * conserve power.  If the built-in LED is not defined or available, this function will have no effect.
 */
void stopLEDPulsing();

/**
 * Enables the built-in LED. This function will turn on the built-in LED and keep it on until it is disabled. This can be used to indicate that the device is awake and running, or for debugging purposes. If the built-in LED is not defined or available, this function will have no effect.
 */
void enableBuiltinLED();

#endif // __DISPLAY_UTILS_EXTRA_H__