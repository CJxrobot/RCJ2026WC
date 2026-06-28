#include "main_core.h"


// --- Sensor Data ---
// main_core.cpp
USData usData = {
  .pos_x_f          = 0.0f,
  .pos_y_f          = 0.0f,
  .dist_cm          = { US_INVALID_DIST, US_INVALID_DIST, US_INVALID_DIST, US_INVALID_DIST },
  .invalid_count    = {0},
  .coord_x          = US_INVALID_DIST,
  .coord_y          = US_INVALID_DIST,
  .current          = US_COUNT - 1,
  .last_trigger_time = 0,
  .last_display_time = 0,
};
TopMaixPosData topmaixPosData;
RobotMonitor robotMonitor;
RobotMonitor teammateMonitor;

// main_core.cpp — 這裡才是唯一定義
volatile uint32_t echo_start[US_COUNT]    = {0};
volatile uint32_t echo_duration[US_COUNT] = {0};
volatile bool     echo_done[US_COUNT]     = {false};

// --- OLED OBJECT ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void main_core_init() {
    // Initialize all Hardware Serials
    Serial.begin(115200);
    Serial2.begin(115200); 
    Serial3.begin(115200);
    Serial4.begin(115200); 
    Serial5.begin(921600);
    Serial6.begin(115200); 
    Serial7.begin(115200);
    Serial8.begin(921600);

    Wire.begin();
    Wire.setClock(400000); // Fast I2C for OLED

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        for(;;); // Lock if OLED fails
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // Pin Setups
    pinMode(Charge_Pin, OUTPUT);
    pinMode(Kicker_Pin, OUTPUT);
    
    // Ensure kicker starts safe
    digitalWrite(Charge_Pin, LOW); // Start charging
    digitalWrite(Kicker_Pin, LOW);

    // Button
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_ENTER, INPUT_PULLUP);
    pinMode(BTN_ESC, INPUT_PULLUP);

    pinMode(COM_O1_PIN, INPUT);
    pinMode(COM_O2_PIN, INPUT);

    for (uint8_t i = 0; i < US_COUNT; i++) {
        pinMode(trigPins[i], OUTPUT);
        pinMode(echoPins[i], INPUT);
        digitalWrite(trigPins[i], LOW);
    }

    attachInterrupt(digitalPinToInterrupt(ECHO_F), echoISR<US_FRONT>, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ECHO_R), echoISR<US_RIGHT>, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ECHO_B), echoISR<US_BACK>,  CHANGE);
    attachInterrupt(digitalPinToInterrupt(ECHO_L), echoISR<US_LEFT>,  CHANGE);

/*    pinMode(TRIG_F, OUTPUT);
    pinMode(TRIG_R, OUTPUT);
    pinMode(TRIG_B, OUTPUT);
    pinMode(TRIG_L, OUTPUT);
    pinMode(ECHO_F, INPUT);
    pinMode(ECHO_R, INPUT);
    pinMode(ECHO_B, INPUT);
    pinMode(ECHO_L, INPUT);*/
}

void drawMessage(const char* msg) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println(msg);
    display.display();
}
void ballsensor() {
  uint8_t buffer[6];
  Serial6.write(0xDD);
  while(!Serial6.available());
  Serial6.readBytes(buffer,6);
  if(buffer[0] != 0xCC) return;
  if (buffer[5] != 0xEE) return;
  uint8_t found = buffer[1];
  uint16_t angle = (uint16_t)buffer[2] | ((uint16_t)buffer[3] << 8);
  uint8_t dist = buffer[4];
  if (!found) {
    robotMonitor.ball_valid = false;
    robotMonitor.ball_angle = 65535; // Invalid angle
    robotMonitor.ball_dist = 255; // Max distance
    return;
  }
  robotMonitor.ball_valid = true;
  robotMonitor.ball_angle = angle;
  robotMonitor.ball_dist = dist;
}
void kicker_control(bool kick) {
    static uint32_t charge_start_time = 0;
    static bool is_charged = false;
    static bool is_charging = false;

    const uint32_t CHARGE_MS = 5000; 
    uint32_t now = millis();

    // 1. Kick Logic (Priority)
    if (kick && is_charged) {
        digitalWrite(Kicker_Pin, HIGH);
        delay(15); // Solenoid pulse
        digitalWrite(Kicker_Pin, LOW);
        
        is_charged = false; 
        is_charging = false;
        Serial.println("KICKED");
        return; // Exit to prevent immediate re-charge trigger in same frame
    }

    // 2. Charging State Machine
    if (!is_charged && !is_charging) {
        // Start a new charge cycle
        charge_start_time = now;
        is_charging = true;
        digitalWrite(Charge_Pin, HIGH);
        Serial.println("CHARGING...");
    }

    if (is_charging) {
        if (now - charge_start_time >= CHARGE_MS) {
            // Charge complete
            digitalWrite(Charge_Pin, LOW);
            is_charging = false;
            is_charged = true;
            Serial.println("READY TO KICK");
        }
    }
}

void readTopMaix() {
  uint8_t buffer[10];
  Serial3.write(0xDD);
  //while(!Serial3.available()){Serial.println("cam");};
  Serial3.readBytes(buffer,10);
  topmaixPosData.valid = 0;

  if(buffer[0] == 0xCC && buffer[9] == 0xEE){
    uint8_t checksum = (buffer[1] + buffer[2] + buffer[3] + buffer[4]+ buffer[5]+ buffer[6]+ buffer[7]) & 0xFF;
    if(checksum == buffer[8]){
      //Serial.printf("duration: %ld\n", micros() - start);
      topmaixPosData.valid = true;
      topmaixPosData.x = (int8_t)(uint8_t)buffer[1];
      topmaixPosData.y = (int8_t)(uint8_t)buffer[2];
      topmaixPosData.status = buffer[3];

      topmaixPosData.ball_found = buffer[4];
      topmaixPosData.ball_angle = (uint16_t)buffer[5] | ((uint16_t)buffer[6] << 8);
      topmaixPosData.ball_dist = buffer[7];
    }
  }
  else{
    topmaixPosData.valid = false;  // checksum error
  }
}

void roleSwitchControl() {
    if (robotMonitor.role == 1) {
        // Defense-specific logic
    } else if (robotMonitor.role == 2) {
        // Offense-specific logic
    }
}

void update_all_sensor(){
    ballsensor();
    //readTopMaix();
    updateUS();
}

void sensor_fusion() {
    // Ball Sensor Fusion
    /*
    if (topmaixPosData.valid && topmaixPosData.ball_found) {
        robotMonitor.ball_valid = true;
        robotMonitor.ball_angle = topmaixPosData.ball_angle;
        robotMonitor.ball_dist = topmaixPosData.ball_dist;
    } else {
        robotMonitor.ball_valid = false;
        robotMonitor.ball_angle = 65535; // Invalid angle
        robotMonitor.ball_dist = 255; // Max distance
    }*/

    // Position Sensor Fusion(Ultrasonic + TopMaix)
    robotMonitor.pos_x = (int8_t)round(usData.coord_x);
    robotMonitor.pos_y = (int8_t)round(usData.coord_y);
    
    Serial.printf("US distances: L=%.2f, R=%.2f, F=%.2f, B=%.2f\n", usData.dist_cm[US_LEFT], usData.dist_cm[US_RIGHT], usData.dist_cm[US_FRONT], usData.dist_cm[US_BACK]);
    Serial.printf("US coord_x: %.2f, coord_y: %.2f\n", usData.coord_x, usData.coord_y);
    Serial.printf("TopMaix x: %d, y: %d\n", topmaixPosData.x, topmaixPosData.y);
    Serial.printf("readpos_x: %d, readpos_y: %d\n", topmaixPosData.x, topmaixPosData.y);
    
}

void sendMotor(float vx, float vy, float rot_v, int target_heading) {
    uint8_t data[6];
    data[0] = PROTOCAL_HEADER;
    data[1] = (int8_t)vx;
    data[2] = (int8_t)vy;
    // Scale up by 100 so we don't lose decimals (e.g., 0.5 rad/s becomes 50)
    data[3] = (int8_t)(rot_v * 100.0f);
    // Scale down by 10 to fit 0-360 into a single byte (0-36)
    data[4] = (int8_t)(target_heading / 10);
    data[5] = PROTOCAL_END;
    Serial8.write(data, sizeof(data));
}

void sendMaincoreData() {
    uint8_t data[10];
    data[0] = PROTOCAL_HEADER;
    data[1] = robotMonitor.pos_x;
    data[2] = robotMonitor.pos_y;
    data[3] = robotMonitor.ball_valid ? 1 : 0;
    data[4] = robotMonitor.ball_angle;
    data[5] = robotMonitor.ball_angle>>8; // High byte
    data[6] = robotMonitor.ball_dist;
    data[7] = robotMonitor.ball_dist>>8; // High byte
    data[8] = robotMonitor.role; // Reserved
    data[9] = PROTOCAL_END;
    Serial8.write(data, sizeof(data));
}

void stopMotors() {
    sendMotor(0.0f, 0.0f, 0.0f, 90); // Send zero velocities to stop
}


bool UI_Interface(){
    update_all_sensor();
    static uint32_t lastDisplayTime = 0;
    switch (robotMonitor.currentState) {
        case RobotState::STATE_READY:
            if (digitalRead(BTN_ENTER) == LOW) {
                Serial8.write(LS_CAL_START); // Command to Sensor Board
                drawMessage("SCANNING");
                delay(500);
                robotMonitor.currentState = RobotState::STATE_CALIBRATING;
                break;
            }

            if (millis() - lastDisplayTime > 100) { // 每 0.1 秒更新一次螢幕
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(0, 0);
                display.printf("ball dist: %d\n", robotMonitor.ball_dist);
                display.printf("ball angle: %d\n",robotMonitor.ball_angle);
                display.display();
                lastDisplayTime = millis();
            }
            if(digitalRead(BTN_UP) == LOW){
                display.clearDisplay();
                display.display();
                return false;
            }
            //offense

            //defense

            break;
        case RobotState::STATE_CALIBRATING:
            if (digitalRead(BTN_ESC) == LOW) {
                Serial8.write(LS_CAL_END); // Command to Save
                drawMessage("SAVING...");
                delay(500);
                robotMonitor.currentState = RobotState::STATE_SAVING;
            }
            break;
        case RobotState::STATE_SAVING:
            if(Serial8.available()){
                uint8_t c = Serial8.read();
                if(c == LS_CAL_END){
                    drawMessage("SAVED!");
                    delay(1000); // 讓 SAVED 停一下
                }
                // 回到初始狀態
                drawMessage("READY");
                delay(1000);
                display.clearDisplay();
                robotMonitor.currentState = RobotState::STATE_READY;
            }
            break;
    }
    return true;
}

void triggerUS(uint8_t i) {
  digitalWrite(trigPins[i], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[i], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[i], LOW);
}


// 合併 updateFilteredUS + markInvalidUS
void updateReading(uint8_t i, uint32_t duration) {
  if (duration > 100 && duration < 15000) {
    float raw = duration * 0.0343f / 2.0f;
    usData.dist_cm[i] = isValidUS(usData.dist_cm[i])
                      ? usData.dist_cm[i] * (1.0f - US_FILTER_ALPHA) + raw * US_FILTER_ALPHA
                      : raw;
    usData.invalid_count[i] = 0;
  } else if (++usData.invalid_count[i] >= US_INVALID_LIMIT) {
    usData.dist_cm[i] = US_INVALID_DIST;
  }
}

void updateUSCoordinate() {
    // ==================== X 軸邏輯 (右正, 左負) ====================
    if (isValidUS(usData.dist_cm[US_LEFT]) && usData.dist_cm[US_LEFT] > 5.0f && usData.dist_cm[US_LEFT] < 30.0f && 
        (usData.dist_cm[US_LEFT] < usData.dist_cm[US_RIGHT] || !isValidUS(usData.dist_cm[US_RIGHT]))) {
        // 左邊在 5~30 且比右邊小 -> 採信左邊
        usData.coord_x = -(91.0f - usData.dist_cm[US_LEFT]); 
    } 
    else if (isValidUS(usData.dist_cm[US_RIGHT]) && usData.dist_cm[US_RIGHT] > 5.0f && usData.dist_cm[US_RIGHT] < 30.0f && 
            (usData.dist_cm[US_RIGHT] < usData.dist_cm[US_LEFT] || !isValidUS(usData.dist_cm[US_LEFT]))) {
        // 右邊在 5~30 且比左邊小 -> 採信右邊
        usData.coord_x = 91.0f - usData.dist_cm[US_RIGHT]; 
    } 
    else {
        // 兩邊都大於 30，走原本的相減
        usData.coord_x = (isValidUS(usData.dist_cm[US_LEFT]) && isValidUS(usData.dist_cm[US_RIGHT]))
                        ? (usData.dist_cm[US_LEFT] - usData.dist_cm[US_RIGHT]) / 2.0f : US_INVALID_DIST;
    }
    // ==================== Y 軸邏輯 (前後總長 243, 半長 121.5) ====================
    if (isValidUS(usData.dist_cm[US_BACK]) && usData.dist_cm[US_BACK] > 5.0f && usData.dist_cm[US_BACK] < 30.0f && 
        (!isValidUS(usData.dist_cm[US_FRONT]) || usData.dist_cm[US_FRONT] > 150.0f)) {
        // 靠近後方，前方很大 -> 車在後半場 (負半場)
        usData.coord_y = -(121.5f - usData.dist_cm[US_BACK]);
    } 
    else if (isValidUS(usData.dist_cm[US_FRONT]) && usData.dist_cm[US_FRONT] > 5.0f && usData.dist_cm[US_FRONT] < 30.0f && 
            (!isValidUS(usData.dist_cm[US_BACK]) || usData.dist_cm[US_BACK] > 150.0f)) {
        // 靠近前方，後方很大 -> 車在前半場 (正半場)
        usData.coord_y = 121.5f - usData.dist_cm[US_FRONT]; 
    } 
    else {
        // 兩邊都正常，維持原來的相減
        usData.coord_y = (isValidUS(usData.dist_cm[US_BACK]) && isValidUS(usData.dist_cm[US_FRONT]))
                        ? (usData.dist_cm[US_BACK] - usData.dist_cm[US_FRONT]) / 2.0f : US_INVALID_DIST;
    }

    // ==================== 球門厚度補償 ====================
    if (usData.coord_x < 30.0f && usData.coord_x > -30.0f) {
        if (usData.coord_y > 0.0f) {
            usData.coord_y -= 6.0f;  // y > 0 時，往 0 修正 (減 6)
        } 
        else if (usData.coord_y < 0.0f) {
            usData.coord_y += 6.0f;  // y < 0 時，往 0 修正 (加 6)
        }
    }
}

void updateUS() {
    static uint8_t  current_us        = US_COUNT - 1;
    static uint32_t last_trigger_time = 0;
    if (millis() - last_trigger_time >= 50) {
        last_trigger_time = millis();
        current_us = (current_us + 1) % US_COUNT;
        echo_done[current_us] = false;
        triggerUS(current_us);
    }

    for (uint8_t i = 0; i < US_COUNT; i++) {
        if (!echo_done[i]) continue;

        noInterrupts();
        uint32_t duration = echo_duration[i];
        echo_done[i] = false;
        interrupts();

        updateReading(i, duration);
        updateUSCoordinate();
    }
}

/*
void updateUS() {
  static uint8_t  current_us        = US_COUNT - 1;
  static uint32_t last_trigger_time = 0;

  if (millis() - last_trigger_time >= 20) {
    last_trigger_time = millis();
    current_us = (current_us + 1) % US_COUNT;

    digitalWrite(trigPins[current_us], LOW);
    delayMicroseconds(2);
    digitalWrite(trigPins[current_us], HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPins[current_us], LOW);

    uint32_t duration = pulseIn(echoPins[current_us], HIGH, 30000);

    updateReading(current_us, duration);
    updateUSCoordinate();
  }
}*/
