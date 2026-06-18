#include "main_core.h"

void setup() {
    main_core_init();
}

void loop() {
    parseESP32(); // Handle incoming ESP32 data asynchronously
    triggerBallRequest(); // Non-blocking ball request
    triggerTeammateRequest(); // Non-blocking teammate request
}
