#include <main_core.h>

unsigned long last_1000hz_tick = 0;
unsigned long last_50hz_tick = 0;

//#define DEFAULT_ROLE 1 // 1: offense, 2: defense
#define DEFAULT_ROLE 2 // 1: offense, 2: defense

void setup() {
    main_core_init();
    while(1) {
        drawMessage("Waiting for SubCore...");
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
}


// 宣告微秒等級的計時器
unsigned long last_1hz_tick = 0;
unsigned long last_ball_request_tick = 0; // 新增：非阻塞球資訊請求計時器

void loop() {
    stopMotors();
    while(!digitalRead(COM_O1_PIN));
    while (1) {
        ;
    }
}