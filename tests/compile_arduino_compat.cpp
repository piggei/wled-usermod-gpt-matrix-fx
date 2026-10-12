#include "wled.h"
// Also compile the ESP32-only currentCore() branch without actual FreeRTOS.
#define ARDUINO_ARCH_ESP32 1
inline int xPortGetCoreID() {return 1;}
// Compile-only guard: mock wled.h defines the Arduino sq(x) macro.
#include "../gpt_matrix_fx.cpp"
