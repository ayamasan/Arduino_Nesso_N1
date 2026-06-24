// for Arduino Nesso N1
// シリアル通信送信
#include <Arduino_Nesso_N1.h>
#include <M5Unified.h>

M5Canvas canvas(&M5.Display);

void setup()
{
    M5.begin();
    Serial.begin(115200);
    
    Serial2.begin(115200, SERIAL_8N1, 4, 5);
    
    canvas.createSprite(M5.Display.width(), M5.Display.height());
    canvas.setTextColor(GREEN);
    canvas.setTextScroll(true);
    canvas.setTextSize(1); 
    canvas.setRotation(1);
}

int count = 0;

void loop()
{
    if (Serial2.available()) {
        Serial.print("IOT sent to target : ");
        Serial.println(count);
        Serial2.print("IOT sent to target : ");
        Serial2.println(count);
        
        canvas.println("IOT sent to target : " + String(count));
        canvas.pushSprite(0, 0);
        
        count++;
    }

    delay(1000); // 1秒待機
}

