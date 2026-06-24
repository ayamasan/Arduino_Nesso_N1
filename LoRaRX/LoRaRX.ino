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

// パケット受信フラグ
volatile bool receivedFlag = false;

// モジュールによるパケット受信時に呼ばれる関数
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  receivedFlag = true;
}

M5Canvas canvas(&M5.Display);

void setup() {
  M5.begin();
  Serial.begin(115200);
  M5.Display.setRotation(1);

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
  Serial.print(F("[SX1262] Initializing ... "));
  canvas.println("[SX1262] Initializing ... ");
  canvas.pushSprite(0, 0);

  int state = radio.begin(868.0, 125.0f, 12, 5, 0x34, 22, 20, 3.0, true);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  radio.setPacketReceivedAction(setFlag);

  Serial.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
}

void loop() {
  if (receivedFlag) {
    receivedFlag = false;

    String str;
    int state = radio.readData(str);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("[SX1262] Received packet!"));
      Serial.print(F("[SX1262] Data:\t\t"));
      Serial.println(str);

      canvas.print(F("[SX1262] Data:\t\t"));
      canvas.println(str);

      Serial.print(F("[SX1262] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      canvas.print(F("[SX1262] RSSI:\t\t"));
      canvas.print(radio.getRSSI());
      canvas.println(F(" dBm"));

      Serial.print(F("[SX1262] SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));

      canvas.print(F("[SX1262] SNR:\t\t"));
      canvas.print(radio.getSNR());
      canvas.println(F(" dB"));
      canvas.pushSprite(0, 0);

      Serial.print(F("[SX1262] Frequency error:\t"));
      Serial.print(radio.getFrequencyError());
      Serial.println(F(" Hz"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println(F("CRC error!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
  }
} 
