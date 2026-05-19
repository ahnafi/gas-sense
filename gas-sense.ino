/*
 * ============================================================
 * Sistem Deteksi Dini Kebocoran Gas LPG
 * Platform  : Arduino Uno (simulasi Wokwi)
 * Sensor    : MQ-2 (pin analog A0)
 * Display   : OLED 128x64 I2C (0x3C) - SDA=A4, SCL=A5
 * LED       : Merah=11, Kuning=10, Hijau=9
 * Buzzer    : pin 8 (tone/noTone)
 * Button    : pin 2 (interrupt eksternal INT0)
 * ============================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_FreeRTOS.h>

// Konfigurasi OLED 
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1          // tidak pakai pin reset terpisah
#define OLED_ADDRESS 0x3C

#define SERIAL_MONITOR 115200

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Definisi Pin 
const int PIN_LED_RED    = 11;    // LED merah
const int PIN_LED_YELLOW = 10;    // LED kuning
const int PIN_LED_GREEN  =  9;    // LED hijau
const int PIN_BUZZER     =  8;    // buzzer aktif (tone/noTone)
const int PIN_BUTTON     =  2;    // push button (INT0)
const int PIN_MQ135        = A0;    // output analog MQ-2

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
    xTaskCreate(TaskDisplay,"Oled Display",6144,NULL,1,NULL);
    xTaskCreate(TaskBuzzer,"Buzzer",1024,NULL,1,NULL);
    xTaskCreate(TaskButton,"Push button",1024,NULL,1,NULL);

    // Tampilan splash saat pertama kali menyala 
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println(F("LPG Gas Detector"));
    display.setCursor(20, 36);
    display.println(F("Initializing..."));
    display.display();
}

void loop(){}

void TaskSensor(void *pvParameters){
    for(;;){
        int sensorValue = analogRead(sensorPin); 
        Serial.print("sensor mq135 val:");
        Serial.println(sensorValue);
    }
}

void TaskDisplay(void *pvParameters){
    for(;;){
        
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