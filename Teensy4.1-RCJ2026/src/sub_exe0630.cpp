#include "sub_core.h"

#define DtoR_const 0.0174529f

//Line Sensor
#define EMERGENCY_THRESHOLD 90

uint8_t role = 0; // 0: default, 1: defense, 2: offense
void setup() {
    sub_core_init();
    digitalWrite(LED_BUILTIN, HIGH); // Indicate setup incomplete
    while(1){
        Serial.println("Waiting for MainCore...");
        if(Serial8.available()){
            uint8_t temp = Serial8.read();
            Serial.printf("Received handshake: 0x%02X\n", temp);
            if(temp == 0x0A){
                role = 1; // offense
                Serial8.write(PROTOCAL_ACT);
                break;
            }
            else if(temp == 0x0D){
                role = 2; // defense
                Serial8.write(PROTOCAL_ACT);
                break;
            }
            else{
                Serial.println("Invalid handshake, retrying...");
            }
        }
        else{
            Serial.println("Invalid handshake, retrying...");
        }
    }
    digitalWrite(LED_BUILTIN, LOW); // Indicate setup complete
}
float get_line_move_deg(int8_t range, uint32_t line_state, uint8_t center) {
    float x_sum = 0, y_sum = 0;
    int cnt = 0;
    const float deg2rad = M_PI / 180.0f;
    for (int i = -range ; i <= range; i++) {
      uint8_t idx = (center + i + 32) % 32;
        if (!((lineData.state >> idx) & 1)) {
            float deg = idx * 11.25f;
            float rad = deg * deg2rad;
            x_sum += cosf(rad);
            y_sum += sinf(rad);
            cnt++;
        }
    }
    if(cnt > 0){
        float move_deg = atan2(y_sum, x_sum) * 57.2958f;
        if (move_deg < 0) move_deg += 360.0f;
        return move_deg;
    }
    return -1; // No line detected
}
void defense_mode3(){
  uint32_t ball_in_front_time = 0;
  const float deg2rad = M_PI / 180.0f;

  while(1){
    update_gyro_sensor();
    update_line_sensor();
    readMaincoreData();
    float vx = 0, vy = 0;
    int count = 0;
    float x_sum = 0, y_sum = 0;

    for (int i = 0 ; i < 32; i++) {
        if (!((lineData.state >> i) & 1)) {
            float rad = (i * 11.25f) * deg2rad;
            x_sum += cosf(rad);
            y_sum += sinf(rad);
            count++;
        }
    }
    Serial.printf("Ball valid:%d angle:%d dist:%d | Line count:%d\n",subMonitor.ball_valid, subMonitor.ball_angle, subMonitor.ball_dist, count);

    if(count < 3) {
      Serial.printf("subMonitor.pos_x %f subMonitor.pos_y %f\n", subMonitor.pos_x, subMonitor.pos_y);
      float dx = 0.0f   - subMonitor.pos_x;
      float dy = -90.0f - subMonitor.pos_y;
      vx = constrain(dx * 0.15f, -40.0f, 40.0f);
      vy = constrain(dy * 0.40f, -40.0f, 40.0f);
      if (fabsf(vx) < 8.0f) vx = 0.0f;
      if (vy != 0.0f && fabsf(vy) < 12.0f) vy = (vy > 0.0f) ? 12.0f : -12.0f;
    }
    else{
      bool ball_in_front = (subMonitor.ball_valid) && (subMonitor.ball_angle > 80 && subMonitor.ball_angle < 100);
      if(ball_in_front){//ball in front motion
        Serial.printf("ball in front %d\n", millis() - ball_in_front_time);
        if(ball_in_front_time != 0){
          if(millis() - ball_in_front_time > 3000){
            return; // switch to offense motion
          }
        }
        else{
          ball_in_front_time = millis();
        }
      }
      else{//defense motion
        float right_deg = get_line_move_deg(11, lineData.state, 0);
        float top_deg = get_line_move_deg(5, lineData.state, 8);
        float left_deg = get_line_move_deg(11, lineData.state, 16);
        float down_deg = get_line_move_deg(5, lineData.state, 24);
        bool ball_on_right = (subMonitor.ball_valid) && (subMonitor.ball_angle > 270 || subMonitor.ball_angle < 80);
        bool ball_on_left = (subMonitor.ball_valid) && (subMonitor.ball_angle > 100 && subMonitor.ball_angle < 270);
        float move_deg = -1;
        float lock_deg = atan2(y_sum, x_sum) * 57.2958f;
        if (lock_deg < 0) lock_deg += 360.0f;
        float mag = sqrt(y_sum * y_sum + x_sum * x_sum);//index of checking how far the robot have shifted from the line
        if(ball_on_right){
          move_deg = right_deg;
        }
        else if (ball_on_left){
          move_deg = left_deg;   
        }
        if(move_deg != -1){
          float move_rad = move_deg * DtoR_const;
          vx = 40.0f * cosf(move_rad);
          vy = 40.0f * sinf(move_rad);
        }
        else if(lock_deg != -1){
          float lock_rad = lock_deg * DtoR_const;;
          vx = mag * 15 * cosf(lock_rad);
          vy = mag * 15 * sinf(lock_rad);
        }
        if(top_deg != -1 && down_deg != -1 && abs(down_deg - 270) < 45){
          Serial.println("side");
          float escape_deg = get_line_move_deg(9, lineData.state, 8);
          Serial.printf("escape%f\n", escape_deg);
          bool robot_on_right = (escape_deg > 90 && escape_deg < 180);
          bool robot_on_left = (escape_deg < 90 && escape_deg > 0);
          bool robot_at_corner = (escape_deg < 112.5 && escape_deg > 57.5);
          move_deg = -1;
          if(ball_on_right){
            Serial.println("ball right lock");
            if(robot_on_left){
              move_deg = escape_deg;
            }
          }
          else if (ball_on_left){
            Serial.println("ball left2 lock");
            if(robot_on_right){
              move_deg = escape_deg;
            }
          }
          if(robot_at_corner){
            move_deg = escape_deg;
          }
          if(move_deg != -1){
            float move_rad = move_deg * DtoR_const;
            vx = 60.0f * cosf(move_rad);
            vy = 60.0f * sinf(move_rad);
          }
          else{
            vx = 0;
            if(vy < 0){
              vy = 0;
            }
          }
        }
        Serial.printf("move_deg: %f, lock_deg: %f left_deg: %f, right_deg: %f, mag%f\n", move_deg, lock_deg, left_deg, right_deg, mag);
      }
    }
    Serial.printf("vx: %f, vy %f\n", vx, vy);
    if(vx > 90){
      vx = 90;
    }
    else if(vx < -90){
      vx = -90;
    }
    if(vy > 90){
      vy = 90;
    }
    else if(vy < -90){
      vy = -90;
    }
    Vector_Motion(vx, vy, 0);
  }
}
void defense_mode4(){
  update_gyro_sensor();
  update_line_sensor();
  readMaincoreData();

  int count = 0;
  float x_sum = 0, y_sum = 0;
  const float deg2rad = M_PI / 180.0f;
  float vx = 0, vy = 0;
  for (int i = 0 ; i < 32; i++) {
      if (!((lineData.state >> i) & 1)) {
          float rad = (i * 11.25f) * deg2rad;
          x_sum += cosf(rad);
          y_sum += sinf(rad);
          count++;
      }
  }
  Serial.printf("Ball valid:%d angle:%d dist:%d | Line count:%d\n",subMonitor.ball_valid, subMonitor.ball_angle, subMonitor.ball_dist, count);

  if (count < 3) {
    Serial.printf("subMonitor.pos_x %f subMonitor.pos_y %f\n", subMonitor.pos_x, subMonitor.pos_y);
    float dx = 0.0f   - subMonitor.pos_x;
    float dy = -90.0f - subMonitor.pos_y;
    vx = constrain(dx * 0.15f, -40.0f, 40.0f);
    vy = constrain(dy * 0.40f, -40.0f, 40.0f);
    if (fabsf(vx) < 8.0f) vx = 0.0f;
    if (vy != 0.0f && fabsf(vy) < 12.0f) vy = (vy > 0.0f) ? 12.0f : -12.0f;
  }
  else{
    float right_deg = get_line_move_deg(13, lineData.state, 0);
    float top_deg = get_line_move_deg(5, lineData.state, 8);
    float left_deg = get_line_move_deg(13, lineData.state, 16);
    float down_deg = get_line_move_deg(5, lineData.state, 24);
    bool ball_on_right = (subMonitor.ball_valid) && (subMonitor.ball_angle > 270 || subMonitor.ball_angle < 80);
    bool ball_on_left = (subMonitor.ball_valid) && (subMonitor.ball_angle > 100 && subMonitor.ball_angle < 270);
    
    float move_deg = -1;
    float lock_deg = atan2(y_sum, x_sum) * 57.2958f;
    if (lock_deg < 0) lock_deg += 360.0f;
    float mag = sqrt(y_sum * y_sum + x_sum * x_sum);//index of checking how far the robot have shifted from the line
    if(ball_on_right){
      move_deg = right_deg;
    }
    else if (ball_on_left){
      move_deg = left_deg;   
    }
    if(move_deg != -1){
      float move_rad = move_deg * DtoR_const;
      vx = 55.0f * cosf(move_rad);
      vy = 55.0f * sinf(move_rad);
    }
    else if(lock_deg != -1){
      float lock_rad = lock_deg * DtoR_const;;
      vx = mag * 15 * cosf(lock_rad);
      vy = mag * 15 * sinf(lock_rad);
    }
    if(top_deg != -1 && down_deg != -1 && abs(down_deg - 270) < 45){
      Serial.println("side");
      float escape_deg = get_line_move_deg(9, lineData.state, 8);
      Serial.printf("escape%f\n", escape_deg);
      bool robot_on_right = (escape_deg > 90 && escape_deg < 180);
      bool robot_on_left = (escape_deg < 90 && escape_deg > 0);
      bool robot_at_corner = (escape_deg < 112.5 && escape_deg > 57.5);
      move_deg = -1;
      if(ball_on_right){
        Serial.println("ball right lock");
        if(robot_on_left){
          move_deg = escape_deg;
        }
      }
      else if (ball_on_left){
        Serial.println("ball left2 lock");
        if(robot_on_right){
          move_deg = escape_deg;
        }
      }
      if(robot_at_corner){
        move_deg = escape_deg;
      }
      if(move_deg != -1){
        float move_rad = move_deg * DtoR_const;
        vx = 60.0f * cosf(move_rad);
        vy = 60.0f * sinf(move_rad);
      }
      else{
        vx = 0;
        if(vy < 0){
          vy = 0;
        }
      }
    }
    //Serial.printf("move_deg: %f, lock_deg: %f left_deg: %f, right_deg: %f, mag%f\n", move_deg, lock_deg, left_deg, right_deg, mag);
  }
  Serial.printf("vx: %f, vy %f\n", vx, vy);
  Vector_Motion(vx, vy, 0);
}

void offense(){
  uint32_t offense_start_time = millis();
  float vx = 0;
  float vy = 0;
  float init_lineDegree = -1;
  float diff = 0;
  bool overhalf = false;
  bool first_detect = false;
  uint32_t speed_timer = 0;
  float linesensorDegreelist[32] = {
      0.00, 11.25, 22.50, 33.75, 45.00, 56.25, 67.50, 78.75, 
      90.00, 101.25, 112.50, 123.75, 135.00, 146.25, 157.50, 168.75, 
      180.00, 191.25, 202.50, 213.75, 225.00, 236.25, 247.50, 258.75, 
      270.00, 281.25, 292.50, 303.75, 315.00, 326.25, 337.50, 348.75
  };
  while(1){
    Serial.println("offense");
    update_gyro_sensor();
    update_line_sensor();
    readMaincoreData();
    float sumX = 0.0f;
    float sumY = 0.0f;
    int count = 0;
    for(int i = 0; i < LS_count; i++){
      if(bitRead(lineData.state, i) == 0){
        float deg = linesensorDegreelist[i];
        sumX += cos(deg * DtoR_const);
        sumY += sin(deg * DtoR_const);
        count++;
      }
    }
    if(count > 1){
      float lineDegree = atan2(sumY, sumX) * 57.2958f;
      if(lineDegree < 0) lineDegree += 360;

      if(!first_detect){
        init_lineDegree = lineDegree;
        first_detect = true;
        speed_timer = millis();
      }

      diff = fabs(lineDegree - init_lineDegree);
      if(diff > 180) diff = 360 - diff;

      float finalDegree;
      if(diff > EMERGENCY_THRESHOLD){
        overhalf = true;
        finalDegree = fmod(init_lineDegree + 180.0f, 360.0f);
      }
      else{
        overhalf = false;
        finalDegree = fmod(lineDegree + 180.0f, 360.0f);
      }
      vx = 70.0f * cos(finalDegree * DtoR_const);
      vy = 70.0f * sin(finalDegree * DtoR_const); 
    }
    else{
      if(millis() - offense_start_time > 3000){
        Serial.println("offense end");
        //delay(1000);
        return;//back to defense mode
      }
      first_detect = false;
      if(subMonitor.ball_valid){
        float moving_degree = subMonitor.ball_angle;
        float offset = 0;
        float ballspeed = constrain(map(subMonitor.ball_dist, 55, 80, 30, 60), 30, 60);
        //float ballspeedVx = constrain(map(subMonitor.ball_dist, 70, 85, 30, 50), 30, 50);
        //float ballspeedVy = constrain(map(subMonitor.ball_dist, 70, 85, 25, 50), 25, 50);

        //Serial.println(subMonitor.ball_dist);
        if(subMonitor.ball_angle >= 83 && subMonitor.ball_angle <= 97){
          //moving_degree = subMonitor.ball_angle;
          moving_degree = 90;
          ballspeed = 50;
        }
        else if(subMonitor.ball_angle > 97 && subMonitor.ball_angle <= 130){
          ballspeed = constrain(map(subMonitor.ball_dist, 55, 80, 30, 50),30,50);
          float offsetRatio = exp(-0.1 * (subMonitor.ball_dist - 65));
          offsetRatio = constrain(offsetRatio, 0.0, 1.0);
          offset = 10 * offsetRatio;
          moving_degree = subMonitor.ball_angle + offset;
        }
        else if(subMonitor.ball_angle > 130 && subMonitor.ball_angle < 155){
          float offsetRatio = exp(-0.1 * (subMonitor.ball_dist - 60));
          offsetRatio = constrain(offsetRatio, 0.0, 1.0);
          offset = 90 * offsetRatio;
          moving_degree = subMonitor.ball_angle + offset;
          digitalWrite(LED_BUILTIN,LOW);
          //float angleError = fabs(ballData.angle - 90);
          //float smoothWeight = constrain(angleError / 25.0f, 0.0f, 1.0f);
        }
        else if(subMonitor.ball_angle >= 155 && subMonitor.ball_angle <= 270){
          float offsetRatio = exp(-0.03 * (subMonitor.ball_dist - 65));
          offsetRatio = constrain(offsetRatio, 0.0, 1.0);
          offset = 100 * offsetRatio;
          //Serial.print(" offset=");Serial.print(offset);
          moving_degree = subMonitor.ball_angle + offset;
          digitalWrite(LED_BUILTIN,LOW);
        }
        else if(subMonitor.ball_angle >= 50 && subMonitor.ball_angle < 83){
          ballspeed =constrain(map(subMonitor.ball_dist, 55, 80, 30, 50),30,50); //30;
          float offsetRatio = exp(-0.1 * (subMonitor.ball_dist - 65));
          offsetRatio = constrain(offsetRatio, 0.0, 1.0);
          offset = 10 * offsetRatio;
          moving_degree = subMonitor.ball_angle - offset;
        }
        else if(subMonitor.ball_angle < 50 && subMonitor.ball_angle >=25){
          float offsetRatio = exp(-0.1 * (subMonitor.ball_dist - 60));
          offsetRatio = constrain(offsetRatio, 0.0, 1.0);
          offset = 90 * offsetRatio;
          moving_degree = subMonitor.ball_angle - offset;
          digitalWrite(LED_BUILTIN,LOW);
        }
        else if(subMonitor.ball_angle < 25 || subMonitor.ball_angle > 270){
          float offsetRatio = exp(-0.03 * (subMonitor.ball_dist - 60));
          offsetRatio = constrain(offsetRatio, 0.0, 1.0);
          offset = 100 * offsetRatio;
          moving_degree = subMonitor.ball_angle - offset;
          digitalWrite(LED_BUILTIN,LOW);
        }
        if(moving_degree < 0) moving_degree += 360;
        if(moving_degree >= 360) moving_degree -= 360;
        vx = (int16_t)round(ballspeed * cos(moving_degree * DtoR_const));
        vy = (int16_t)round(ballspeed * sin(moving_degree * DtoR_const));
        Serial.print(" ball=");Serial.print(subMonitor.ball_angle);
        Serial.print(" balldist=");Serial.print(subMonitor.ball_dist);
        Serial.print(" balldist=");Serial.print(subMonitor.ball_dist);
        Serial.print(" move=");Serial.println(moving_degree);
        Serial.print(" ballspeed=");Serial.println(ballspeed);
      }
      else{
        vx = 0;
        vy = 0;
      }
    }
    Vector_Motion(vx, vy, 0);
  }
  Serial.println("offense end");
}
void loop(){
  while(1){
    update_gyro_sensor();
    update_line_sensor();
    //Serial.printf("Gyro Heading: %f\n", gyroData.heading);
    //for(uint8_t i = 0; i < 32; i++){
      //Serial.printf("%d", (lineData.state >> i) & 1);
    //}
    //Serial.println();
    if (Serial8.available()) {  
      uint8_t cmd = Serial8.read();
      Serial.println(cmd,HEX);
      if(cmd == LS_CAL_START) {
        Serial.println("Starting line sensor calibration...");
          uint16_t max_ls[32], min_ls[32];
          uint16_t front_max = 0, front_min = 4095;
          uint16_t mid_max = 0, mid_min = 4095;
          for (int i = 0; i < 32; i++) { 
            max_ls[i] = 0; 
            min_ls[i] = 4095; 
          }
          int timer = 0;
          while (1) {
            if(micros() - timer > 100000) { // 每 100ms 更新一次
              digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Toggle LED for visual feedback
              timer = micros();
            }
            if(Serial8.available()){
              uint8_t cmd = Serial8.read();
              if(cmd == LS_CAL_END){ // End calibration command
                break;
              }
            }
            for (int i = 0; i < LS_count; i++) {
              int r = readMux(i % 16, (i < 16) ? 1 : 2);
              if (r > max_ls[i]) max_ls[i] = r; 
              if (r < min_ls[i]) min_ls[i] = r;
            }
          }
          for (int i = 0; i < LS_count; i++) avg_ls[i] = (max_ls[i] + min_ls[i]) / 2;
          for (int i = 0; i < LS_count; i++) {
            Serial.printf("Sensor %d: min=%d, max=%d, avg=%d\n", i, min_ls[i], max_ls[i], avg_ls[i]);
          }
          avg_ls[32] = (front_max + front_min) / 2;
          EEPROM.put(0, avg_ls);
          delay(1000); // Ensure EEPROM write completes
          Serial8.write(LS_CAL_ACK); // Send end calibration acknowledgment
      } 
      else if (cmd == MOVE_CMD) {// When BTN_UP is pressed, send a move command to the main core
        Serial8.write(PROTOCAL_ACT);
        break;
      }
    }
  }
  //digitalWrite(LED_BUILTIN, HIGH); // Toggle LED for visual feedback
  while(1){
    //defense_mode4();
    defense_mode3();
    /*offense motion*/
    //move away from line
    while(1){
      update_gyro_sensor();
      update_line_sensor();
      Vector_Motion(0,30,0);
      int count = 0;
      for (int i = 0 ; i < 32; i++) {
        if (!((lineData.state >> i) & 1)) {
          count++;
        }
      }
      if(count < 3) {
        break;
      }
    }
    Serial.println("leave");
    offense();
  }
}   