// for Arduino Nesso N1
// LoRa受信->シリアル通信送信
#include <Arduino_Nesso_N1.h>
#include <M5GFX.h>
#include <M5Unified.h>
#include <time.h>
#include "M5UnitENV.h"

#include <RadioLib.h>

#define LORA_MOSI_PIN 21
#define LORA_MISO_PIN 22
#define LORA_SCK_PIN 20
#define LORA_IRQ_PIN 15
#define LORA_CS_PIN 23
#define LORA_BUSY_PIN 19

// LoRa通信ライムアウト（ms） @@@ 
#define loratout 300000  // 5min
//#define loratout 5000  // 5sec

// LoRa受信データ構造（17バイト）
// IOT01,XXXX,YYYY,Z
//  (1)  (2)  (3) (4)
// (1) ヘッダーASCII5バイト、固定5種、センサー番号
// (2) 温度ASCII4バイト、10倍の値（0243->24.3℃）
// (3) 湿度ASCII4バイト、10倍の値（0451->45.1％）
// (4) 人検知ASCII1バイト、0=無検知、1=検知中

// シリアル出力は、[CR(0D)+LF(0A)]付加して送る（19バイト）
// IOT01,XXXX,YYYY,Z(0D)(0A)

// ヘッダー定義
const char *header[5] = {
    "IOT01",
    "IOT02",
    "IOT03",
    "IOT04",
    "IOT05"
};

// SX1262: NSS, DIO1, NRST, BUSY
SX1262 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, RADIOLIB_NC, LORA_BUSY_PIN);

// パケット受信フラグ
volatile bool receivedFlag = false;

// モジュールによるパケット受信時に呼ばれる関数
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  receivedFlag = true;
}

M5GFX display;
unsigned long tms[5] = {0,0,0,0,0};  // システム経過時間（msec）1秒計測用
int man[5] = {0,0,0,0,0};

int temp[5] = {0,0,0,0,0};

void setup()
{
    int i;
    char str[10];
    
    M5.begin();
    Serial.begin(115200);
    display = M5.Display;
    
    display.fillScreen(DARKGREY);
    
    Serial2.begin(115200, SERIAL_8N1, 4, 5);
    delay(100);
    
    if(!Serial2.available()){
        display.fillScreen(RED);
        display.setTextColor(BLACK, RED);
        display.setTextSize(2); 
        display.setRotation(1);
        display.setCursor(10, 40);
        display.print("SERIAL ERROR !");
        while(true){delay(10);}
    }
    
    tms[0] = millis();
    for(i=0; i<4; i++){
        tms[i+1] = tms[i];
    }
    
    display.setFont(&fonts::lgfxJapanGothic_24);
    display.setTextColor(ORANGE);
    display.setTextSize(1); 
    for(i=0; i<5; i++){
        sprintf(str, "%d", i+1);
        display.setCursor(20, (24+10)*i+40);
        display.print(str);
        draw(i, 0, man[i], temp[i]);
    }
    
    auto& ioe = M5.getIOExpander(0);
    
    //LORA_RESET
    ioe.digitalWrite(7, false);
    delay(100);
    ioe.digitalWrite(7, true);
    delay(100);
    
    ioe.digitalWrite(5, true);  //LORA_LNA_ENABLE
    ioe.digitalWrite(6, true);  //LORA_ANTENNA_SWITCH
    
    Serial.print(F("[SX1262] Initializing ... "));
    
    int state = radio.begin(868.0, 125.0f, 12, 5, 0x34, 22, 20, 3.0, true);
    if(state == RADIOLIB_ERR_NONE){
        Serial.println(F("success!"));
    }
    else{
        Serial.print(F("failed, code "));
        Serial.println(state);
        display.fillScreen(RED);
        display.setTextColor(BLACK, RED);
        display.setTextSize(2); 
        display.setRotation(1);
        display.setCursor(10, 40);
        display.print("LoRa ERROR !");
        while(true){delay(10);}
    }
    
    radio.setPacketReceivedAction(setFlag);
    
    Serial.print(F("[SX1262] Starting to listen ... "));
    state = radio.startReceive();
    if(state == RADIOLIB_ERR_NONE){
        Serial.println(F("success!"));
    }
    else{
        Serial.print(F("failed, code "));
        Serial.println(state);
        display.fillScreen(RED);
        display.setTextColor(BLACK, RED);
        display.setTextSize(2); 
        display.setRotation(1);
        display.setCursor(10, 40);
        display.print("LoRa ERROR !");
        while(true){delay(10);}
    }
}


int count = 0;

void loop()
{
    int i;
    unsigned long tmsnow = 0;
    int sens = -1;
    String s;
    
    // LoRa受信タイムアウトチェック
    tmsnow = millis();
    for(i=0; i<5; i++){
        if(tmsnow < tms[i]){  // 時計折り返し
            tms[i] = tmsnow;
        }
        else if((tmsnow - tms[i]) >= loratout){  // タイムアウト経過
            man[i] = 0;
            draw(i, 0, man[i], temp[i]);
        }
    }
    
    String sensdata;
    if(receivedFlag){
        receivedFlag = false;
        sens = -1;
        
        int state = radio.readData(sensdata);
        
        if(state == RADIOLIB_ERR_NONE){
            //Serial.println(sensdata);
            if(sensdata.length() != 17){
                Serial.println(F("Data Lenght error!"));
            }
            else{
                if(sensdata.startsWith(header[0])){
                    sens = 0;
                }
                else if(sensdata.startsWith(header[1])){
                    sens = 1;
                }
                else if(sensdata.startsWith(header[2])){
                    sens = 2;
                }
                else if(sensdata.startsWith(header[3])){
                    sens = 3;
                }
                else if(sensdata.startsWith(header[4])){
                    sens = 4;
                }
                else{
                    sens = -1;
                }
                if(sens >= 0){
                    s = sensdata.substring(16, 17);
                    man[sens] = s.toInt();
                }
            }
        }
        else if(state == RADIOLIB_ERR_CRC_MISMATCH){
            Serial.println(F("CRC error!"));
        }
        else{
            Serial.print(F("failed, code "));
            Serial.println(state);
        }
    }
    
    if(sens >= 0){  // LoRa受信
        s = sensdata.substring(6, 10);
        temp[sens] = s.toInt();
        tms[sens] = tmsnow;
        draw(sens, 1, man[sens], temp[sens]);
        
        //Serial.print("Data sent to target : ");
        //Serial.println(sensdata);
        Serial.print(sens);
        
        // シリアル送信
        Serial2.println(sensdata);
        
        count++;
        if(count > 80){
            count = 0;
            Serial.print("\n");
        }
    }
    
    //delay(1000); // 1秒待機
}


void draw(int sens, int onoff, int man, int temp)
{
    int color1, color2;
    char str[20];
    int tt;
    
    switch(sens){
        case 0  : color1 = RED;     color2 = MAROON;    break;
        case 1  : color1 = BLUE;    color2 = NAVY;      break;
        case 2  : color1 = GREEN;   color2 = DARKGREEN; break;
        case 3  : color1 = ORANGE;  color2 = OLIVE;     break;
        case 4  : color1 = MAGENTA; color2 = PURPLE;    break;
        default : color1 = CYAN;    color2 = DARKCYAN;  break;
    }
    if(onoff == 0){
        display.fillRect(40, (24+10)*sens+40, 52, 24, color2);
        display.fillRect(100, (24+10)*sens+40, 20, 24, BLACK);
    }
    else{
        display.fillRect(40, (24+10)*sens+40, 52, 24, color1);
        if(man == 0){
            display.fillRect(100, (24+10)*sens+40, 20, 24, BLACK);
        }
        else{
            display.fillRect(100, (24+10)*sens+40, 20, 24, ORANGE);
        }
    }
    
    display.setFont(&fonts::lgfxJapanGothic_16);
    display.setTextColor(BLACK);
    display.setTextSize(1);
    if(temp < 0) tt = temp * (-1);
    else         tt = temp;
    sprintf(str, "%d.%d", temp/10, temp%10);
    display.setCursor(45, (24+10)*sens+40+4);
    display.print(str);
    display.setFont(&fonts::lgfxJapanGothic_24);
    display.setTextColor(ORANGE);
    
    display.display();
}

