//#include <Arduino_Nesso_N1.h>
#include <M5GFX.h>
#include <M5Unified.h>
#include <time.h>

#include <RadioLib.h>

#define LORA_MOSI_PIN 21
#define LORA_MISO_PIN 22
#define LORA_SCK_PIN 20
#define LORA_IRQ_PIN 15
#define LORA_CS_PIN 23
#define LORA_BUSY_PIN 19

// センサー番号（1～5）
#define sensor 1

// LoRa通信周期（ms）
#define lorainterval 20000   // 20sec

// SX1262: NSS, DIO1, NRST, BUSY
SX1262 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, RADIOLIB_NC, LORA_BUSY_PIN);

// 送信状態を保持
int transmissionState = RADIOLIB_ERR_NONE;

// パケット送信完了のフラグ
volatile bool transmittedFlag = false;

// 送信データ構造（17バイト）
// IOT11,XXXX,YYYY,Z
//  (1)  (2)  (3) (4)
// (1) ヘッダーASCII5バイト、固定5種、センサー番号
// (2) 温度ASCII4バイト、10倍の値（0243->24.3℃）
// (3) 湿度ASCII4バイト、10倍の値（0451->45.1％）
// (4) 人検知ASCII1バイト、0=無検知、1=検知中

// ヘッダー定義
const char *header[5] = {
    "IOT11",
    "IOT12",
    "IOT13",
    "IOT14",
    "IOT15"
};
// 送信データ
char senddata[20];
// 土壌湿度データ％（10倍値）
int tempdata = 0;
// 土壌湿度設定閾値％（10倍値）
int hygrodata = 0;
// 土壌湿度乾燥（0=正常、1=乾燥）
int mandet = 0;
// センサ番号（0～4）
int sensnum = 0;

char str[30];
int setlvl = 0;   // 土壌湿度設定閾値（0～100％）
int hpos = 0;     // タッチ位置
int nowpos = -1;  // バー内ならパーセント（0～100）放された時に処理

int lastlvl = 0;
int lastpar = 0;

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

int ana = 0;   // 土壌湿度センサーアナログ値
int digi = 0;  // 土壌湿度センサーデジタル値
int par = 0;   // 土壌湿度×10（0～1000％）


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
    
    // Gloveコネクタのピン設定
    pinMode(5, INPUT);      // Set pin 5 to input mode. (Digital)
    pinMode(4, INPUT);      // Set pin 4 to input mode. (Analog)
    delay(500);
    
    // The analogRead() function will return a value between 0 and 4095, 
    // corresponding to an input voltage range of 0 V to 3.3 V.
    // Arduino Nesso N1のADCは、0～3.3V入力で0～4095が出力される
    // Unit EarthのVRは右で水分小、左で水分大
    ana = analogRead(4);               // ADC値読み出し
    digi = digitalRead(5);             // VR設定値との比較結果
    
    // テキストのプロパティ設定
    display.fillScreen(BLACK);
    
    // LCD表示
    SensDisp(sensnum);
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
}


// センサ番号表示
void SensDisp(int num)
{
    display.setFont(&fonts::Font0);
    // 設定閾値表示
    display.setTextColor(GREEN, BLACK);  // 緑文字、黒背景
    display.setTextSize(2);
    sprintf(str, "    %d%%    ", setlvl);
    display.setCursor(display.width()/2-display.textWidth(str)/2, 30/2);
    display.print(str);
    
    // 土壌湿度バーグラフ表示
    int maxh = (display.height()-34);      // LCDのバー表示エリア高
    int maxw = display.width() - 20;       // LCDのバー表示エリア幅
    int level = (ana * maxh) / 4096;       // バーの高さ計算
    //par = ((4095-ana) * 1000) / 4096;  // パーセント
    // バー上部をグレーで塗りつぶし
    display.fillRect(10, 34, maxw, level, DARKGREY);
    if(par < setlvl*10){   // 乾燥（設定値より低い）
        // LCDの下からバー表示
        // ADC値は大で乾燥なので逆の値で描画
        display.fillRect(10, 34+level, maxw, maxh-level, RED);
        display.setTextColor(WHITE, RED);  // 白色文字、赤色背景
    }
    else{
        // ADC値は小で湿潤なので逆の値で描画
        display.fillRect(10, 34+level, maxw, maxh-level, CYAN);
        display.setTextColor(BLACK, CYAN); // 黒色文字、水色背景
    }
    sprintf(str, "%02d.%d%%", par/10, par%10);
    display.setCursor(display.width()/2-display.textWidth(str)/2, 34+maxh-20);
    display.print(str);
    
    // センサー番号表示
    str[0] = num + '1';
    str[1] = 0;
    display.setTextColor(GREEN);
    display.setTextScroll(false);
    display.setFont(&fonts::lgfxJapanGothic_40);
    display.setTextSize(4); 
    //Serial.printf("Sensor = %d\n", num+1);
    display.setCursor(display.width()/2-display.textWidth(str)/2, 40);
    display.print(str);
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
    
    // ボタン状態取得
    lgfx::touch_point_t tp[3];
    int nums = display.getTouchRaw(tp, 3);
    
    // 設定ボタンタッチあり
    int lvl = -1;
    if(nums > 0){
        lvl = checkbutton(nums, tp[0].x, tp[0].y);
    }
    if(lvl >= 0){
        setlvl = lvl;
        hpos = tp[0].y;
        display.fillRect(0, 34, 10, display.height()-34, BLACK);
        // 設定位置描画
        display.drawLine(0, hpos, 10, hpos, GREEN);
        display.drawLine(0, hpos-1, 10, hpos-1, GREEN);
    }
    
    // 土壌湿度センサー読出し
    ana = analogRead(4);               // ADC値読み出し
    digi = digitalRead(5);             // VR設定値との比較結果
    
    par = ((4095-ana) * 1000) / 4096;  // パーセント
    
    // LCD表示（計測±0.5％で再描画）
    if(par >= lastpar+5
    || par <= lastpar-5
    || setlvl != lastlvl){
        SensDisp(sensnum);
        lastpar = par;
        lastlvl = setlvl;
        //Serial.printf("Sensor = %d\n", ana);
    }
    
    // 送信周期をずらして競合を避ける
    switch(sensor){
        case 1  : tshift = 0;   break;
        case 2  : tshift = 700;  break;
        case 3  : tshift = 1300; break;
        case 4  : tshift = 2500; break;
        case 5  : tshift = 3700; break;
        default : tshift = 2300; break;
    }
    
    tmsnow = millis();
    
    // 土壌湿度低下は、1秒間隔で行って、見つかっている間中、LoRa送信し続ける
    if((tmsnow - tmspir) >= 1000 || tmsnow < tmspir){  // 土壌湿度低下チェック1sec間隔
        if(tmsnow < tmspir){
            tmspir = tmsnow;
        }
        else{
            tmspir += 1000;
        }
        
        // 土壌湿度センサーデータを格納
        if(par < setlvl*10){   // 乾燥（設定値より低い）
            if(mandet == 0){
                Serial.println("KANSO");
                mandet = 1;
            }
        }
        else{
            if(mandet != 0){
                Serial.println("OK");
                mandet = 0;
            }
        }
    }
    
    if((tmsnow - tms) >= (lorainterval + tshift) || tmsnow < tms){  // 送信間隔
        if(tmsnow < tms){
            tms = tmsnow;
        }
        else{
            tms += (lorainterval + tshift);
        }
        
        tempdata = par;           // 温度に計測値（％×10）
        hygrodata = setlvl * 10;  // 湿度に閾値（％×10）
        
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
} 


int checkbutton(int nums, int x, int y)
{
    if(x > 10 && x < display.width()-10){
        if(y >= 34 && y <= display.height()){
            // バーエリア内のタッチ
            // タッチの座標は、左上がゼロ
            nowpos = ((display.height()-y) * 100) / (display.height()-34);
        }
        else{
            nowpos = -1;
        }
    }
    else{
        nowpos = -1;
    }
    return(nowpos);
}
