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

// センサー番号（1～5）
#define sensor 1

// LoRa通信周期（ms）@@@
#define lorainterval 20000   // 20sec
//#define lorainterval 2000  // 2sec

// 温湿度センサー
SHT3X sht3x;
QMP6988 qmp;

// SX1262: NSS, DIO1, NRST, BUSY
SX1262 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, RADIOLIB_NC, LORA_BUSY_PIN);

// 送信状態を保持
int transmissionState = RADIOLIB_ERR_NONE;

// パケット送信完了のフラグ
volatile bool transmittedFlag = false;

// 送信データ構造（17バイト）
// IOT01,XXXX,YYYY,Z
//  (1)  (2)  (3) (4)
// (1) ヘッダーASCII5バイト、固定5種、センサー番号
// (2) 温度ASCII4バイト、10倍の値（0243->24.3℃）
// (3) 湿度ASCII4バイト、10倍の値（0451->45.1％）
// (4) 人検知ASCII1バイト、0=無検知、1=検知中

// ヘッダー定義
const char *header[5] = {
    "IOT01",
    "IOT02",
    "IOT03",
    "IOT04",
    "IOT05"
};
// 送信データ
char senddata[20];
// 温度データ（10倍値）
int tempdata = 0;
// 湿度データ（10倍値）
int hygrodata = 0;
// 人検知（0=無、1=有）
int mandet = 0;
// センサ番号（0～4）
int sensnum = 0;

// モジュールによるパケット送信完了時に呼ばれる関数
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif

void setFlag(void)
{
    // 送信完了フラグを設定
    transmittedFlag = true;
}

M5GFX display;
unsigned long tms = 0;  // システム経過時間（msec）
unsigned long tmspir = 0;  // システム経過時間（msec）1秒計測用


void setup()
{
    M5.begin();
    Serial.begin(115200);
    display = M5.Display;
    
    tms = millis();
    tmspir = tms;
    
    sensnum = sensor - 1;
    if(sensnum < 0 || sensnum > 4){
        Serial.println("Sensor Num Eror");
        ErrorDisp("Sensor Num Error");
        while(1) delay(1);
    }
    
    // センサ番号表示
    SensNumDisp(sensnum);
    
    // 人検知ボタン表示
    ManDetDisp(mandet);
    
    delay(500);
    
    
    // LoRa
    // LED_BUILTIN at E1.P7
    auto& ioe = M5.getIOExpander(0);
    
    //LORA_RESET
    ioe.digitalWrite(7, false);
    delay(100);
    ioe.digitalWrite(7, true);
    delay(100);
    
    ioe.digitalWrite(5, true);  //LORA_LNA_ENABLE
    ioe.digitalWrite(6, true);  //LORA_ANTENNA_SWITCH
    
    // 周波数、帯域幅、拡散率、符号化率、同期ワード、送信パワー、前置長、TCXO基準電圧、useRegulatorLDO
    int state = radio.begin(868.0, 125.0f, 12, 5, 0x34, 22, 20, 3.0, true);
    if(state == RADIOLIB_ERR_NONE){
        Serial.println(F("success!"));
    }
    else{
        Serial.print(F("failed, code "));
        Serial.println(state);
        // エラー表示
        ErrorDisp("LoRa Error!");
        
        while(true){
          delay(10);
        }
    }
    
    // パケット送信完了時のコールバックを設定
    radio.setPacketSentAction(setFlag);
    
    // 最初のパケット送信を開始
    // データを送信可能
    sprintf(senddata, "%s,%04d,%04d,%d", header[sensnum], tempdata, hygrodata, mandet);
    Serial.printf("send [%s]\n", senddata);
    transmissionState = radio.startTransmit(senddata);
    
    
    // 温湿度センサー
    if(!qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, 6, 7, 400000U)){
        Serial.println("Couldn't find QMP6988");
        ErrorDisp("Couldn't find QMP6988");
        while(1) delay(1);
    }
    Serial.println("Start QMP6988 OK!");
    delay(100);
    
    if(!sht3x.begin(&Wire, SHT3X_I2C_ADDR, 6, 7, 400000U)){
        Serial.println("Couldn't find SHT3X");
        ErrorDisp("Couldn't find SHT3X");
        while(1) delay(1);
    }
    Serial.println("Start SHT3X OK!");
    SensorRead();
    delay(100);
    
    // PIR人感センサー
    pinMode(4, INPUT);      // Set pin 4 to input mode.
    delay(100);
    
}


// センサ番号表示
void SensNumDisp(int num)
{
    char str[2];
    
    str[0] = num + '1';
    str[1] = 0;
    display.fillScreen(DARKGREY);
    display.setTextColor(GREEN, DARKGREY);
    display.setTextScroll(false);
    display.setFont(&fonts::lgfxJapanGothic_40);
    display.setTextSize(4); 
    Serial.printf("Sensor = %d\n", num+1);
    display.setCursor(display.width()/2-display.textWidth(str)/2, 20);
    display.print(str);
}


// 人検知ボタン表示
void ManDetDisp(int det)
{
    if(det == 0){  // 非検知
        display.fillRect(0, display.height()-60, display.width(), 60, CYAN);
        display.setFont(&fonts::lgfxJapanGothic_24);
        display.setTextColor(ORANGE);
        display.setTextSize(1); 
        display.setCursor(display.width()/2-display.textWidth("無検知")/2, display.height()-60/2-24/2);
        display.print("無検知");
    }
    else{  // 検知中
        display.fillRect(0, display.height()-60, display.width(), 60, RED);
        display.setFont(&fonts::lgfxJapanGothic_24);
        display.setTextColor(BLACK);
        display.setTextSize(1); 
        display.setCursor(display.width()/2-display.textWidth("人検知！")/2, display.height()-60/2-24/2);
        display.print("人検知！");
    }
}


// エラー表示
void ErrorDisp(char *str)
{
    display.fillScreen(RED);
    display.setTextColor(WHITE, RED);
    display.setFont(&fonts::lgfxJapanGothic_24);
    display.setTextSize(1); 
    display.setRotation(1);
    display.setCursor(display.width()/2-display.textWidth(str)/2, 120/2-24/2);
    display.print(str);
}


// 送信パケットカウント
int count = 0;


// メインループ
void loop()
{
    unsigned long tmsnow = 0;
    int tshift = 0;
    
    // 送信周期をずらして競合を避ける
    #if 1  // @@@
    switch(sensor){
        case 1  : tshift = 0;   break;
        case 2  : tshift = 700;  break;
        case 3  : tshift = 1300; break;
        case 4  : tshift = 2500; break;
        case 5  : tshift = 3700; break;
        default : tshift = 2300; break;
    }
    #else
    switch(sensor){
        case 1  : tshift = 0; break;
        case 2  : tshift = 100; break;
        case 3  : tshift = 330; break;
        case 4  : tshift = 550; break;
        case 5  : tshift = 770; break;
        default : tshift = 230; break;
    }
    #endif
    
    tmsnow = millis();
    
    // 人検知は、1秒間隔で行って、見つかっている間中、LoRa送信し続ける
    if((tmsnow - tmspir) >= 1000 || tmsnow < tmspir){  // PIRチェック1sec間隔
        if(tmsnow < tmspir){
            tmspir = tmsnow;
        }
        else{
            tmspir += 1000;
        }
        // PIR人感センサー
        if(digitalRead(4) == 1){  // If pin 4 reads a value of 1.
            if(mandet == 0){
                Serial.println("FIND!");
                mandet = 1;
                ManDetDisp(mandet);
            }
        }
        else{
            if(mandet != 0){
                Serial.println("NONE");
                mandet = 0;
                ManDetDisp(mandet);
            }
        }
    }
    
    if((tmsnow - tms) >= (lorainterval + tshift) || tmsnow < tms || mandet != 0){  // 送信間隔
        if(tmsnow < tms){
            tms = tmsnow;
        }
        else{
            tms += (lorainterval + tshift);
        }
        
        // 温湿度センサー
        SensorRead();
        
        
        // 前回の送信が完了したか確認
        if(transmittedFlag){
            // フラグリセット
            transmittedFlag = false;
            
            if(transmissionState == RADIOLIB_ERR_NONE){
                Serial.println(F("transmission finished!"));
            }
            else{
                Serial.print(F("failed, code "));
                Serial.println(transmissionState);
            }
            
            // 送信完了後の後処理を実行
            radio.finishTransmit();
            
            // 再送信まで 1 秒待機
            //delay(1000);
            
            // 新しいパケットを送信
            sprintf(senddata, "%s,%04d,%04d,%d", header[sensnum], tempdata, hygrodata, mandet);
            Serial.printf("send [%s]\n", senddata);
            transmissionState = radio.startTransmit(senddata);
        }
        //mandet = 0;
    }
    
    //delay(1000);
} 



void SensorRead()
{
    // 温湿度センサー
    if(sht3x.update()){
        Serial.print("Temperature: ");
        Serial.print(sht3x.cTemp);
        Serial.printf(", Humidity: ");
        Serial.println(sht3x.humidity);
        tempdata = (int)(sht3x.cTemp * 10);  // 温度はSHT3Xの値を使用する
        hygrodata = (int)(sht3x.humidity * 10);  // 湿度はSHT3Xの値を使用する
    }
    if(qmp.update()){
        Serial.printf("Temperature: ");
        Serial.print(qmp.cTemp);
        Serial.printf(", Pressure: ");
        Serial.println(qmp.pressure);
    }
}
