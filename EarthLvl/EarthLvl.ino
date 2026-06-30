/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
 * @Hardwares: M5Core + Unit Earth
 * @Platform Version: Arduino M5Stack Board Manager v2.1.3
 * @Dependent Library:
 * M5Stack@^0.4.6: https://github.com/m5stack/M5Stack
 */

#include <arduino.h>
#include <M5GFX.h>
#include <M5Unified.h>

M5GFX display;
char str[30];
int setlvl = 0;

int hpos = 0;     // タッチ位置
int nowpos = -1;  // バー内ならパーセント（0～100）放された時に処理

void setup()
{
    M5.begin();
    display = M5.Display;
    Serial.begin(115200);
    
    // Gloveコネクタのピン設定
    pinMode(5, INPUT);      // Set pin 5 to input mode. (Digital)
    pinMode(4, INPUT);      // Set pin 4 to input mode. (Analog)
    
    // テキストのプロパティ設定
    display.fillScreen(BLACK);
    display.setTextColor(GREEN, BLACK);  // 緑文字、黒背景
    display.setTextSize(2);
    sprintf(str, "    %d%%    ", setlvl);
    display.setCursor(display.width()/2-display.textWidth(str)/2, 30/2);
    display.print(str);
}

void loop()
{
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
    
    // The analogRead() function will return a value between 0 and 4095, 
    // corresponding to an input voltage range of 0 V to 3.3 V.
    // Arduino Nesso N1のADCは、0～3.3V入力で0～4095が出力される
    // Unit EarthのVRは右で水分小、左で水分大
    int ana = analogRead(4);               // ADC値読み出し
    int digi = digitalRead(5);             // VR設定値との比較結果
    
    display.setTextColor(GREEN, BLACK);    // 緑色文字、黒背景
    sprintf(str, "    %d%%    ", setlvl);
    display.setCursor(display.width()/2-display.textWidth(str)/2, 30/2);
    display.print(str);
    
    int maxh = (display.height()-34);      // LCDのバー表示エリア高
    int maxw = display.width() - 20;       // LCDのバー表示エリア幅
    int level = (ana * maxh) / 4096;       // バーの高さ計算
    int par = ((4095-ana) * 1000) / 4096;  // パーセント
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
    display.setCursor(display.width()/2-display.textWidth(str)/2, 34+maxh-30/2);
    display.print(str);
    delay(100);                            // 0.1秒間隔で更新
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
