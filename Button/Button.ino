#include <Arduino_Nesso_N1.h>

#include "M5Unified.h"

void setup()
{
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);
}

void loop()
{
    M5.update();
    if (M5.BtnA.wasPressed()) {
        Serial.println("A Btn Pressed");
    }
    if (M5.BtnA.wasReleased()) {
        Serial.println("A Btn Released");
    }
    if (M5.BtnB.wasPressed()) {
        Serial.println("B Btn Pressed");
    }
    if (M5.BtnB.wasReleased()) {
        Serial.println("B Btn Released");
    }
} 

