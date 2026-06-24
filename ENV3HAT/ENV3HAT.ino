/**
 * @file Hat_ENVIII_M5StickC.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief 
 * @version 0.1
 * @date 2024-01-30
 *
 *
 * @Hardwares: M5StickC + Hat_ENVIII
 * @Platform Version: Arduino M5Stack Board Manager v2.1.0
 * @Dependent Library:
 * M5UnitENV: https://github.com/m5stack/M5Unit-ENV
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#include <Arduino_Nesso_N1.h>

#include <M5Unified.h>
#include "M5UnitENV.h"

SHT3X sht3x;
QMP6988 qmp;

M5Canvas canvas(&M5.Display);

void setup() {
    M5.begin();
    Serial.begin(115200);
    
    canvas.createSprite(M5.Display.width(), M5.Display.height());
    canvas.setTextColor(GREEN);
    canvas.setTextScroll(true);
    canvas.setTextSize(2); 
    
    if (!qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, 6, 7, 400000U)) {
        Serial.println("Couldn't find QMP6988");
        while (1) delay(1);
    }
    Serial.println("Start QMP6988 OK!");

    if (!sht3x.begin(&Wire, SHT3X_I2C_ADDR, 6, 7, 400000U)) {
        Serial.println("Couldn't find SHT3X");
        while (1) delay(1);
    }
    Serial.println("Start SHT3X OK!");
}

void loop() {
    if (sht3x.update()) {
        Serial.println("-----SHT3X-----");
        Serial.print("Temperature: ");
        Serial.print(sht3x.cTemp);
        Serial.println(" degrees C");
        Serial.print("Humidity: ");
        Serial.print(sht3x.humidity);
        Serial.println("% rH");
        Serial.println("-------------\r\n");
        
        canvas.println("--SHT3X--");
        canvas.println(String(sht3x.cTemp) + "C");
        canvas.pushSprite(0, 0);
        canvas.println(String(sht3x.humidity) + "%");
        canvas.pushSprite(0, 0);
    }

    if (qmp.update()) {
        Serial.println("-----QMP6988-----");
        Serial.print(F("Temperature: "));
        Serial.print(qmp.cTemp);
        Serial.println(" *C");
        Serial.print(F("Pressure: "));
        Serial.print(qmp.pressure);
        Serial.println(" Pa");
        Serial.print(F("Approx altitude: "));
        Serial.print(qmp.altitude);
        Serial.println(" m");
        Serial.println("-------------\r\n");
        
        canvas.println("--QMP6988--");
        canvas.println(String(qmp.cTemp));
        canvas.pushSprite(0, 0);
        canvas.println(String(qmp.pressure) + "Pa");
        canvas.pushSprite(0, 0);
    }
    
    canvas.println("");
    canvas.pushSprite(0, 0);
    delay(1000);
}
