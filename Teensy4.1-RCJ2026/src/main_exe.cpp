<<<<<<< HEAD
#include <main_core.h>

unsigned long last_1000hz_tick = 0;
unsigned long last_50hz_tick = 0;

//#define DEFAULT_ROLE 1 // 1: offense, 2: defense
#define DEFAULT_ROLE 2 // 1: offense, 2: defense

#include <main_core.h>


//#define DEFAULT_ROLE 1 // 1: offense, 2: defense
#define DEFAULT_ROLE 2 // 1: offense, 2: defense

void setup() {
    delay(3000); // Allow time for serial monitor to connect
    main_core_init();
    drawMessage("init");
    delay(500);
    while(1) {
        if(DEFAULT_ROLE == 1) {// 1: offense, 2: defense
            Serial8.write(0x0A);
        } else if(DEFAULT_ROLE == 2) {// 1: offense, 2: defense
            Serial8.write(0x0D);
        }
        // Wait a short moment for the sub-core to respond 
        if(Serial8.available() > 0) {
            if(Serial8.read() == PROTOCAL_ACT) {
                break; // Connection confirmed
            }
        }
    }    
    while(UI_Interface()) {
        ;
    }
    Serial8.read(); // Clear the MOVE_CMD from the buffer
    while(Serial8.read() != PROTOCAL_ACT) {
        Serial8.write(MOVE_CMD); // Tell SubCore to start sending commands
    }

}



// 宣告微秒等級的計時器
unsigned long last_1hz_tick = 0;
unsigned long last_ball_request_tick = 0; // 新增：非阻塞球資訊請求計時器

void loop() {
    update_all_sensor();
    sensor_fusion();
    sendMaincoreData();
    Serial.printf("pos_x: %d, pos_y: %d, ball_dist: %d, ball_angle: %d, ball_valid: %d, has_possession: %d, strategy: %d, role: %d\n",
        robotMonitor.pos_x,
        robotMonitor.pos_y,
        robotMonitor.ball_dist,
        robotMonitor.ball_angle,
        robotMonitor.ball_valid,
        robotMonitor.has_possession,
        robotMonitor.strategy,
        robotMonitor.role
    );
=======
#include <main_core.h>


//#define DEFAULT_ROLE 1 // 1: offense, 2: defense
#define DEFAULT_ROLE 2 // 1: offense, 2: defense

void setup() {
    delay(3000); // Allow time for serial monitor to connect
    main_core_init();
    drawMessage("init");
    delay(500);
    while(1) {
        if(DEFAULT_ROLE == 1) {// 1: offense, 2: defense
            Serial8.write(0x0A);
        } else if(DEFAULT_ROLE == 2) {// 1: offense, 2: defense
            Serial8.write(0x0D);
        }
        // Wait a short moment for the sub-core to respond 
        if(Serial8.available() > 0) {
            if(Serial8.read() == PROTOCAL_ACT) {
                break; // Connection confirmed
            }
        }
    }    
    while(UI_Interface()) {
        ;
    }
    Serial8.read(); // Clear the MOVE_CMD from the buffer
    while(Serial8.read() != PROTOCAL_ACT) {
        Serial8.write(MOVE_CMD); // Tell SubCore to start sending commands
    }

}

void loop() {
    ballsensor();
    if(DEFAULT_ROLE == 1) {
        ;// Handle offense logic
    } 
    else if(DEFAULT_ROLE == 2) {
        Serial.printf("Ball Sensor: Valid: %d, Angle: %d, Dist: %d\n", robotMonitor.ball_valid, robotMonitor.ball_angle, robotMonitor.ball_dist);
        //sendMaincoreData(); // Handle defense logic
        //readMaix(); // Update Maix data for defense
        Serial.print("rooboo Data - X: ");
        Serial.print(robotMonitor.pos_x);
        Serial.print("  Y: ");
        Serial.println(robotMonitor.pos_y);
        //Serial.printf("Sent ball data - Valid: %d, Angle: %d, Dist: %d\n", robotMonitor.ball_valid, robotMonitor.ball_angle, robotMonitor.ball_dist);
    }
>>>>>>> 22e0647b3e3af4a9ac8de029d1931108b12b9514
}