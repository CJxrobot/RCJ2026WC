#include <Servo.h>
#include <Arduino.h>

Servo myESC;
const int escPin = 41;

void setup() {
    Serial.begin(9600);
    delay(500); // 讓系統穩定
    
    Serial.println("====== 開始保險解鎖流程 ======");
    
    // 步驟 1：先綁定腳位，但此時先不送高油門，防止意外
    myESC.attach(escPin);
    
    // 步驟 2：送出絕對安全值 0（有些電調需要先看到完全沒訊號或極低值）
    myESC.writeMicroseconds(1000); 
    Serial.println("1. 送出最低油門基準線 (1000us)...");
    delay(1500); 

    // 步驟 3：強制刷新最低點（保險機制）
    // BLHeli 聽到連續、穩定的 1000us 超過 2 秒，就一定會發出「嗶-嗶-嗶」隨後「嗶——」的長音
    Serial.println("2. 保持最低油門，等待電調唱完解鎖音樂...");
    for(int i = 0; i < 20; i++) {
        myESC.writeMicroseconds(1000);
        delay(100); // 用小循環連續發送，比單純 delay(2000) 更能確保訊號連續不中斷
    }
    
    Serial.println("====== 解鎖流程結束，馬達即將旋轉 ======");
}

void loop() {
    // 步驟 4：解鎖成功後，用「漸進式」微調油門，不要直接跳到高速度，這樣最保險
    // 1000 是靜止，1050~1100 通常是 BLHeli_S 的啟動馬達臨界點 (Min Throttle)
    myESC.writeMicroseconds(1150); 
    delay(100);
}
