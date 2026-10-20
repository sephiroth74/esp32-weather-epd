#ifndef __BOARD_UTILS_H__
#define __BOARD_UTILS_H__

void printWakeupReason();

bool usbHostAttached(unsigned long timeoutMs);

void waitForSerialMonitor(unsigned long timeoutMs);

void enterDeepSleep();

#endif // __BOARD_UTILS_H__