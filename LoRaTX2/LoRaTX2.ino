#include <Arduino_Nesso_N1.h>

#include "M5Unified.h"
#include <RadioLib.h>

#define LORA_MOSI_PIN 21
#define LORA_MISO_PIN 22
#define LORA_SCK_PIN 20
#define LORA_IRQ_PIN 15
#define LORA_CS_PIN 23
#define LORA_BUSY_PIN 19

// SX1262: NSS, DIO1, NRST, BUSY
SX1262 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, RADIOLIB_NC, LORA_BUSY_PIN);

// 送信状態を保持
int transmissionState = RADIOLIB_ERR_NONE;

// パケット送信完了のフラグ
volatile bool transmittedFlag = false;

// モジュールによるパケット送信完了時に呼ばれる関数
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // 送信完了フラグを設定
  transmittedFlag = true;
}

M5Canvas canvas(&M5.Display);

void setup() {
  M5.begin();
  Serial.begin(115200);
  M5.Display.setRotation(1);

  // LED_BUILTIN at E1.P7
  auto& ioe = M5.getIOExpander(0);

  //LORA_RESET
  ioe.digitalWrite(7, false);
  delay(100);
  ioe.digitalWrite(7, true);
  delay(100);

  ioe.digitalWrite(5, true);  //LORA_LNA_ENABLE
  ioe.digitalWrite(6, true);  //LORA_ANTENNA_SWITCH

  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextColor(GREEN);
  canvas.setTextScroll(true);
  // SX1262 初期化
  Serial.print(F("[SX1262] Initializing ... "));
  canvas.println("[SX1262] Initializing ... ");
  canvas.pushSprite(0, 0);

  // 周波数、帯域幅、拡散率、符号化率、同期ワード、送信パワー、前置長、TCXO基準電圧、useRegulatorLDO
  int state = radio.begin(868.0, 125.0f, 12, 5, 0x34, 22, 20, 3.0, true);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) {
      delay(10);
    }
  }

  // パケット送信完了時のコールバックを設定
  radio.setPacketSentAction(setFlag);

  // 最初のパケット送信を開始
  Serial.print(F("[SX1262] Sending first packet ... "));

  // 最大256文字の文字列を送信可能
  transmissionState = radio.startTransmit("Hello world from M5Stack!");
}

// 送信パケットカウント
int count = 0;


void loop() {
  M5.update();
  if (M5.BtnA.wasReleased()) {
    Serial.println("A Btn Released");
    
    // 前回の送信が完了したか確認
    if (transmittedFlag) {
      // フラグリセット
      transmittedFlag = false;
  
      if (transmissionState == RADIOLIB_ERR_NONE) {
        Serial.println(F("transmission finished!"));
        canvas.println("OK!");
        canvas.pushSprite(0, 0);
      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);
      }

      // 送信完了後の後処理を実行
      radio.finishTransmit();

      // 再送信まで 1 秒待機
      delay(1000);

      // 新しいパケットを送信
      Serial.print(F("[SX1262] Sending another packet ... "));
      String str = "Hello Arduino Nesso N1 #" + String(count++);
      transmissionState = radio.startTransmit(str);
      canvas.println("Send:" + str);
      canvas.pushSprite(0, 0);
    }
    delay(100);
  }
}
