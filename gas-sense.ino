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

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>

#define SERIAL_MONITOR 115200

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Definisi Pin 
const int PIN_LED_RED    = 11;    // LED merah
const int PIN_LED_YELLOW = 10;    // LED kuning
const int PIN_LED_GREEN  =  9;    // LED hijau
const int PIN_BUZZER     =  8;    // buzzer aktif (tone/noTone)
const int PIN_BUTTON     =  2;    // push button (INT0)
const int PIN_MQ135      = A0;    // output analog MQ-2

// define task function
void TaskSensor(void *pvParameters);
void TaskDisplay(void *pvParameters);
void TaskBuzzer(void *pvParameters);
void TaskButton(void *pvParameters);

enum GasStatus {
  STATUS_SAFE,     // aman
  STATUS_WARNING,  // waspada
  STATUS_DANGER    // bahaya
};

void setup(){

    Serial.begin(SERIAL_MONITOR);

    pinMode(PIN_LED_GREEN,OUTPUT);
    pinMode(PIN_LED_RED,OUTPUT);
    pinMode(PIN_LED_YELLOW,OUTPUT);
    pinMode(PIN_BUZZER,OUTPUT);
    pinMode(PIN_BUZZER,INPUT_PULLUP);
    
    xTaskCreate(TaskSensor,"MQ135",1024,NULL,1,NULL);
    xTaskCreate(TaskDisplay,"LCD Display",256,NULL,1,NULL);
    xTaskCreate(TaskBuzzer,"Buzzer",1024,NULL,1,NULL);
    xTaskCreate(TaskButton,"Push button",1024,NULL,1,NULL);

    // Tampilan splash saat pertama kali menyala 
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print(F("LPG Gas Detector"));
    lcd.setCursor(0, 1);
    lcd.print(F("Initializing..."));
}

void loop(){}

void TaskSensor(void *pvParameters){
    for(;;){
        int sensorValue = analogRead(PIN_MQ135); 
        Serial.print("sensor mq135 val:");
        Serial.println(sensorValue);
    }
}

void TaskDisplay(void *pvParameters){
    for(;;){
        lcd.setCursor(0, 0);
        lcd.print("Hello World!");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void TaskBuzzer(void *pvParameters){
    for(;;){
        
    }
}

void TaskButton(void *pvParameters){
    for(;;){
        
    }
}