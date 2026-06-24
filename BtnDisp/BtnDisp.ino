#include <Arduino_Nesso_N1.h>

#include <M5Unified.h>

void setup() {
  M5.begin();
  M5.Display.setRotation(1);          // 横画面モードに設定
  M5.Display.setTextDatum(MC_DATUM);  // テキスト配置の中央基準
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);  // 白文字、黒背景
  M5.Display.setTextSize(2);
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    // ボタン押し
    M5.Display.drawString("A Button !", M5.Display.width() / 2, M5.Display.height() / 2);
  }
  if (M5.BtnA.wasReleased()) {
    // ボタン放し
    M5.Display.fillScreen(TFT_BLACK);
  }
} 
