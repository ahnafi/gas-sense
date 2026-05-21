/*
 * ============================================================
 * Sistem Deteksi Dini Kebocoran Gas LPG
 * Platform  : Arduino Uno (simulasi Wokwi)
 * Sensor    : MQ-2 (pin analog A0)
 * Display   : I2C 16X2 (0X27) - SDA=A4, SCL=A5
 * LED       : Merah=11, Kuning=10, Hijau=9
 * Buzzer    : pin 8 (tone/noTone)
 * Button    : pin 2 (interrupt eksternal INT0)
 * ============================================================
 */

#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define SERIAL_MONITOR 115200

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Definisi Pin
const int PIN_LED_RED = 11;    // LED merah
const int PIN_LED_YELLOW = 10; // LED kuning
const int PIN_LED_GREEN = 9;   // LED hijau
const int PIN_BUZZER = 8;      // buzzer aktif (tone/noTone)
const int PIN_BUTTON = 2;      // push button (INT0)
const int PIN_MQ135 = A0;      // output analog MQ-2

// define task function
void TaskSensor(void *pvParameters);
void TaskDisplay(void *pvParameters);
void TaskBuzzer(void *pvParameters);
void TaskButton(void *pvParameters);

enum GasStatus { STATUS_CLEAN, STATUS_POLUTED, STATUS_HAZARDOUS };

struct SensorData {
  GasStatus status;
  int ppm;
  bool alarmActive;
};

QueueHandle_t dataQueue;

void setup() {

  Serial.begin(SERIAL_MONITOR);

  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  dataQueue = xQueueCreate(1, sizeof(SensorData));
  if (dataQueue != NULL) {
    SensorData initialData = {STATUS_CLEAN, 0, false};
    xQueueOverwrite(dataQueue, &initialData);
  }

  xTaskCreate(TaskSensor, "MQ135", 128, NULL, 1, NULL);
  xTaskCreate(TaskDisplay, "LCD", 128, NULL, 1, NULL);
  xTaskCreate(TaskBuzzer, "Buzzer", 128, NULL, 1, NULL);
  xTaskCreate(TaskButton, "Button", 128, NULL, 1, NULL);

  // Tampilan splash saat pertama kali menyala
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("LPG Gas Detector"));
  lcd.setCursor(0, 1);
  lcd.print(F("Initializing..."));
  Serial.println(F("Setup finished. System Initialized."));
}

void loop() {}

void TaskSensor(void *pvParameters) {
  const float RL = 10.0;
  const float R0 = 10.0;
  const float m = -0.47;
  const float b = 1.31;

  for (;;) {
    int adc = analogRead(PIN_MQ135);
    float vOut = (adc * 5.0) / 1023.0;
    int currentPPM = 0;

    if (vOut > 0.0) {
      float rS = RL * ((5.0 - vOut) / vOut);
      float ratio = rS / R0;
      currentPPM = pow(10, ((log10(ratio) - b) / m));
    }

    SensorData data;
    if (dataQueue != NULL) {
      xQueuePeek(dataQueue, &data, 0); // Ambil state alarm terakhir
      data.ppm = currentPPM;

      if (currentPPM <= 100) {
        data.status = STATUS_CLEAN;
      } else if (currentPPM > 100 && currentPPM <= 300) {
        data.status = STATUS_POLUTED;
        data.alarmActive = true;
      } else {
        data.status = STATUS_HAZARDOUS;
        data.alarmActive = true;
      }

      xQueueOverwrite(dataQueue, &data);

      Serial.print(F("TaskSensor - ADC: "));
      Serial.print(adc);
      Serial.print(F(" | PPM: "));
      Serial.print(data.ppm);
      Serial.print(F(" | Status: "));
      Serial.println(data.status);
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void TaskDisplay(void *pvParameters) {
  bool redBlink = false;

  for (;;) {
    SensorData data;
    if (dataQueue != NULL && xQueuePeek(dataQueue, &data, 0) == pdTRUE) {
      lcd.setCursor(0, 0);
      if (data.status == STATUS_CLEAN) {
        lcd.print(F("Status: BERSIH  "));
        digitalWrite(PIN_LED_GREEN, HIGH);
        digitalWrite(PIN_LED_YELLOW, LOW);
        digitalWrite(PIN_LED_RED, LOW);
      } else if (data.status == STATUS_POLUTED) {
        lcd.print(F("Status: KOTOR   "));
        digitalWrite(PIN_LED_GREEN, LOW);
        digitalWrite(PIN_LED_YELLOW, HIGH);
        digitalWrite(PIN_LED_RED, LOW);
      } else {
        lcd.print(F("Status: BAHAYA! "));
        digitalWrite(PIN_LED_GREEN, LOW);
        digitalWrite(PIN_LED_YELLOW, LOW);
        redBlink = !redBlink;
        digitalWrite(PIN_LED_RED, redBlink ? HIGH : LOW);
      }

      int progress = data.ppm > 300 ? 100 : (data.ppm * 100) / 300;
      lcd.setCursor(0, 1);
      lcd.print(progress);
      lcd.print(F("% ("));
      lcd.print(data.ppm);
      lcd.print(F(" ppm)      "));

      Serial.print(F("TaskDisplay - Update LCD | Progress: "));
      Serial.print(progress);
      Serial.println(F("%"));
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void TaskBuzzer(void *pvParameters) {
  for (;;) {
    SensorData data;
    if (dataQueue != NULL && xQueuePeek(dataQueue, &data, 0) == pdTRUE) {
      if (data.alarmActive) {
        Serial.print(F("TaskBuzzer - Alarm ACTIVE! Status: "));
        Serial.println(data.status);
        if (data.status == STATUS_HAZARDOUS) {
          tone(PIN_BUZZER, 800);
          vTaskDelay(pdMS_TO_TICKS(300));
          tone(PIN_BUZZER, 1200);
          vTaskDelay(pdMS_TO_TICKS(300));
        } else {
          tone(PIN_BUZZER, 1000);
          vTaskDelay(pdMS_TO_TICKS(500));
          noTone(PIN_BUZZER);
          
          for (int i = 0; i < 30; i++) {
            SensorData tempData;
            if (xQueuePeek(dataQueue, &tempData, 0) == pdTRUE) {
               if (tempData.status != data.status) break; // Keluar dari delay jika status berubah
            }
            vTaskDelay(pdMS_TO_TICKS(100));
          }
        }
      } else {
        noTone(PIN_BUZZER);
        vTaskDelay(pdMS_TO_TICKS(100));
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void TaskButton(void *pvParameters) {
  for (;;) {
    if (digitalRead(PIN_BUTTON) == LOW) {
      Serial.println(F("TaskButton - Button Pressed!"));
      SensorData data;
      if (dataQueue != NULL && xQueuePeek(dataQueue, &data, 0) == pdTRUE) {
        if (data.status == STATUS_CLEAN) {
          Serial.println(F("TaskButton - Alarm Reset."));
          data.alarmActive = false;
          xQueueOverwrite(dataQueue, &data);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(200));
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
