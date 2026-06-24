#include <main_core.h>

void sendOwnData() {
    uint8_t angle_byte = robotMonitor.ball_valid ? (uint8_t)(robotMonitor.ball_angle / 10) : 0xFF;
    /*
    // 先算好再印，印的跟送的一致
    Serial.printf("[TX] valid=%d angle_raw=%d angle_byte=%d dist=%d\n",
        robotMonitor.ball_valid,
        robotMonitor.ball_angle,   // 原始值 (int)
        angle_byte,                // 實際送出去的 byte
        robotMonitor.ball_dist
    );*/

    Serial6.write(0x2E);
    Serial6.write(robotMonitor.ball_dist);
    Serial6.write(angle_byte);
    Serial6.write(robotMonitor.pos_x);
    Serial6.write(robotMonitor.pos_y);
    Serial6.write(robotMonitor.strategy);
    Serial6.write(robotMonitor.role);
    Serial6.write(0xEE);
}
// ─── ESP → TEENSY: read incoming frame ────────────────────────────────────────
void readFromESP() {
    Serial6.write(0x24);

    uint8_t buf[8];
    uint8_t idx = 0;

    unsigned long t = millis();
    while (idx < 8) {
        if (Serial6.available()) {
            buf[idx++] = Serial6.read();
        }
        if (millis() - t > 50) {
            return;
        }
    }

    if (buf[0] != 0xCC || buf[7] != 0xEE || buf[6] == 0xFF) {
        teammateMonitor.role = 0;
        return;
    }
    teammateMonitor.ball_dist  = buf[1];
    teammateMonitor.ball_angle = (buf[2] == 0xFF) ? 0xFFFF : buf[2] * 10;
    teammateMonitor.pos_x      = buf[3];
    teammateMonitor.pos_y      = buf[4];
    teammateMonitor.strategy   = buf[5];
    teammateMonitor.role       = buf[6];
}

// ─── SETUP / LOOP ─────────────────────────────────────────────────────────────
void setup() {
    delay(3000);
    main_core_init();
}

void loop() {
    static unsigned long lastSend    = 0;
    static unsigned long lastRequest = 0;
    robotMonitor.pos_x = 100; // dummy data
    robotMonitor.pos_y = 120; // dummy data
    robotMonitor.strategy = 1; // dummy data
    robotMonitor.role = 1; // dummy data
    ballsensor();
    Serial.printf("ball data - Valid: %d, Angle: %d, Dist: %d\n", robotMonitor.ball_valid, robotMonitor.ball_angle, robotMonitor.ball_dist);
        
    if (millis() - lastSend >= 200) {
        // send own data every 200ms
        lastSend = millis();
        sendOwnData();
    }

    // request teammate data every 200ms
    if (millis() - lastRequest >= 200) {
        lastRequest = millis();
        readFromESP();
        Serial.printf("[RX] dist=%d angle=%d x=%d y=%d strategy=%d role=%d\n",
        teammateMonitor.ball_dist, teammateMonitor.ball_angle,
        teammateMonitor.pos_x, teammateMonitor.pos_y,
        teammateMonitor.strategy, teammateMonitor.role
        );
    } 
}