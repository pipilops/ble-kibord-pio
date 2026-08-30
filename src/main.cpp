#include <Arduino.h>
#include "EspUsbHostKeybord.h"
#include <BleKeyboard.h>
#include <BLEDevice.h>
#include <WiFi.h>
#include <usb/usb_host.h>
#include "esp_sleep.h"

#define RETRY_DELAY 10
#define SLEEP_TIMEOUT 300000  // 5 minutes in milliseconds

BleKeyboard bleKeyboard("BLE Kibord");
uint8_t pressedKeys[6] = {0};

unsigned long lastKeyTime = 0;
bool sleeping = false;

bool sendReportWithRetry(KeyReport *rpt) {
  for (int i = 0; i < 5; i++) {
    if (bleKeyboard.isConnected()) {
      bleKeyboard.sendReport(rpt);
      return true;
    }
    delay(RETRY_DELAY);
  }
  return false;
}

void sendFullReport(uint8_t modifiers) {
  KeyReport rpt = { modifiers, 0, {0} };
  memcpy(rpt.keys, pressedKeys, sizeof(pressedKeys));
  sendReportWithRetry(&rpt);
}

class MyHost : public EspUsbHostKeybord {
  void onKey(usb_transfer_t *t) override {
    if (sleeping) {
      Serial.println("Woke up from sleep — restarting to reinit USB + BLE...");
      delay(100);
      esp_restart();  // Restart ESP32 only when waking
    }

    uint8_t mod = t->data_buffer[0];
    uint8_t keys[6];
    memcpy(keys, &t->data_buffer[2], 6);

    lastKeyTime = millis();  // Reset idle timer

    // Remove released keys
    for (int i = 0; i < 6; i++) {
      bool found = false;
      for (int j = 0; j < 6; j++) {
        if (pressedKeys[i] == keys[j]) {
          found = true;
          break;
        }
      }
      if (!found) pressedKeys[i] = 0;
    }

    // Add new keys
    for (int i = 0; i < 6; i++) {
      uint8_t k = keys[i];
      if (k == 0) continue;
      bool exists = false;
      for (int j = 0; j < 6; j++) {
        if (pressedKeys[j] == k) {
          exists = true;
          break;
        }
      }
      if (!exists) {
        for (int j = 0; j < 6; j++) {
          if (pressedKeys[j] == 0) {
            pressedKeys[j] = k;
            break;
          }
        }
      }
    }

    sendFullReport(mod);
  }
};

MyHost usbHost;

void setup() {
  setCpuFrequencyMhz(80);  // 240 -> 80 MHz: much less heat/power, plenty for USB HID + BLE HID
  WiFi.mode(WIFI_OFF);     // guarantee the WiFi radio is never powered up - only BLE is used
  Serial.begin(115200);
  bleKeyboard.setDelay(RETRY_DELAY);
  bleKeyboard.begin();
  BLEDevice::setPower(ESP_PWR_LVL_N0);  // lower BLE TX power (~0dBm); raise if range suffers
  usbHost.begin();
  lastKeyTime = millis();
}

void loop() {
  usbHost.task();

  if (!sleeping && millis() - lastKeyTime > SLEEP_TIMEOUT) {
    Serial.println("Inactivity for 5 minutes. Entering light sleep...");
    bleKeyboard.end();      // Disconnect BLE
    sleeping = true;

    // NOTE: only sleep ONCE per idle period here, not repeatedly. The
    // ESP32-S3's USB OTG host peripheral does not reliably survive
    // repeated light-sleep cycles while active - it can leave the chip
    // hung, requiring a physical reset. After this single 1s nap we stay
    // awake (at the reduced 80MHz clock) until a keypress triggers
    // esp_restart() below, which cleanly reinitializes USB + BLE.
    esp_sleep_enable_timer_wakeup(1000000);  // Wake after 1 second
    esp_light_sleep_start();                 // Sleep once, then stay awake

    // Execution resumes here after light sleep, but wait for key to restart
  }
}
