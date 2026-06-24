#include <main_core.h>

// ---- 顯示相關 ----
void printUSValue(float d) {
  if (!isValidUS(d)) {
    display.print(" ---.-");
    return;
  }
  char buf[8];
  snprintf(buf, sizeof(buf), "%5.1f", d);
  display.print(buf);
}

void printCoordValue(float c) {
  if (isValidUS(c)) display.print((int)round(c));
  else              display.print("---");
}

void showUSDistances() {
    static uint32_t last_display_time = 0;
    if (millis() - last_display_time < 100) return;
    last_display_time = millis();

    static const char* labels[US_COUNT] = { "Front:", "Right:", "Back: ", "Left: " };
    static const uint8_t ys[US_COUNT]    = { 30, 38, 46, 54 };

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 22);
    display.print("X:"); printCoordValue(usData.coord_x);
    display.print(" Y:"); printCoordValue(usData.coord_y);

    for (uint8_t i = 0; i < US_COUNT; i++) {
        display.setCursor(0, ys[i]);
        display.print(labels[i]);
        printUSValue(usData.dist_cm[i]);
        display.print(" cm");
    }

    display.display();
}

void setup() {
    main_core_init();
  }

void loop() {
  updateUS();
  showUSDistances();
}