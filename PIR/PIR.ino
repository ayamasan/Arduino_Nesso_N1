/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
 * @Hardwares: M5Core + Unit PIR
 * @Platform Version: Arduino M5Stack Board Manager v2.1.3
 * @Dependent Library:
 * M5Stack@^0.4.6: https://github.com/m5stack/M5Stack
 */

#include <Arduino_Nesso_N1.h>
#include <M5Unified.h>

M5Canvas canvas(&M5.Display);

void setup()
{
    M5.begin();
    Serial.begin(115200);
    
    pinMode(4, INPUT);      // Set pin 4 to input mode.
    
    canvas.createSprite(M5.Display.width(), M5.Display.height());
    canvas.setTextColor(GREEN);
    canvas.setTextScroll(true);
    canvas.setTextSize(2); 
}


void loop()
{
    if (digitalRead(4) == 1) {  // If pin 4 reads a value of 1.
        Serial.println("FIND!");
        canvas.println("FIND!");
        canvas.pushSprite(0, 0);
    } else {
        Serial.println("NONE");
        canvas.println("NONE");
        canvas.pushSprite(0, 0);
    }
    delay(500);
}

