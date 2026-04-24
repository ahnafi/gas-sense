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

// Konfigurasi OLED 
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1          // tidak pakai pin reset terpisah
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Definisi Pin 
const int PIN_LED_RED    = 11;    // LED merah
const int PIN_LED_YELLOW = 10;    // LED kuning
const int PIN_LED_GREEN  =  9;    // LED hijau
const int PIN_BUZZER     =  8;    // buzzer aktif (tone/noTone)
const int PIN_BUTTON     =  2;    // push button (INT0)
const int PIN_MQ2        = A0;    // output analog MQ-2

// Ambang Batas ADC (hasil kalibrasi Wokwi) 
// Wokwi membaca ADC MQ-2 pada rentang 200–1010
const int THRESHOLD_DANGER  = 908; // >= bahaya
const int THRESHOLD_WARNING = 886; // >= waspada (& < bahaya)
// di bawah 886 = aman

// Konstanta Sirine (dua nada bergantian) 
const int SIREN_FREQ_HIGH = 1000;  // Hz nada tinggi
const int SIREN_FREQ_LOW  =  500;  // Hz nada rendah
const int SIREN_INTERVAL  =  300;  // ms pergantian nada

// Konstanta Kedip LED Waspada / Bahaya 
const int BLINK_INTERVAL = 400;    // ms periode kedip

// Enum Status Kondisi 
enum GasStatus {
  STATUS_SAFE,     // aman
  STATUS_WARNING,  // waspada
  STATUS_DANGER    // bahaya
};

// Variabel Global 
GasStatus currentStatus   = STATUS_SAFE;
GasStatus previousStatus  = STATUS_SAFE;

// Alarm tetap aktif sampai tombol ditekan (meskipun kondisi sudah aman/waspada)
volatile bool alarmActive = false;

// Waktu terakhir untuk kontrol kedip dan sirine tanpa delay()
unsigned long lastBlinkTime  = 0;
unsigned long lastSirenTime  = 0;

bool     ledBlinkState  = false;  // status nyala/mati saat kedip
bool     sirenHighPhase = true;   // fase nada tinggi/rendah sirine

// Nilai ADC dan PPM terakhir yang dibaca
int      lastAdcValue = 0;
// float    lastPpmValue = 0.0;

// Prototype Fungsi 
float    convertAdcToPpm(int adcValue);
GasStatus evaluateStatus(int adcValue);
void     updateLeds(GasStatus status, unsigned long now);
void     updateBuzzer(unsigned long now);
void     updateOled(GasStatus status, float ppm);
void     handleButtonPress();         // ISR interrupt tombol

// SETUP
void setup() {
  // Inisialisasi Serial (untuk debug opsional) 
  Serial.begin(9600);

  // Konfigurasi pin output 
  pinMode(PIN_LED_RED,    OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_GREEN,  OUTPUT);
  pinMode(PIN_BUZZER,     OUTPUT);

  // Konfigurasi pin input tombol (pull-up internal) 
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // Daftarkan fungsi interrupt pada pin 2 (INT0) 
  // Dipicu saat tombol ditekan (FALLING = HIGH ke LOW)
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), handleButtonPress, FALLING);

  // Inisialisasi OLED 
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    // Jika OLED gagal diinisialisasi, berhenti di sini
    for (;;);
  }
  display.clearDisplay();
  display.display();

  // Tampilan splash saat pertama kali menyala 
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println(F("LPG Gas Detector"));
  display.setCursor(20, 36);
  display.println(F("Initializing..."));
  display.display();
  delay(1500);
}

// LOOP
void loop() {
  unsigned long now = millis();

  // Baca ADC dan konversi ke PPM 
  lastAdcValue = analogRead(PIN_MQ2);
  // lastPpmValue = convertAdcToPpm(lastAdcValue);

  // Evaluasi kondisi berdasarkan nilai ADC 
  previousStatus = currentStatus;
  currentStatus  = evaluateStatus(lastAdcValue);

  // Aktifkan alarm jika baru masuk kondisi bahaya 
  if (currentStatus == STATUS_DANGER) {
    alarmActive = true;
  }

  // Perbarui LED, Buzzer, dan OLED 
  updateLeds(currentStatus, now);
  updateBuzzer(now);
  updateOled(currentStatus);

  // Debug ke Serial Monitor 
  Serial.print(F("ADC: "));
  Serial.print(lastAdcValue);
  Serial.print(F("  Status: "));
  if (currentStatus == STATUS_SAFE)    Serial.println(F("AMAN"));
  else if (currentStatus == STATUS_WARNING) Serial.println(F("WASPADA"));
  else                                  Serial.println(F("BAHAYA"));

  delay(200); // sampling setiap 200ms agar tidak terlalu cepat
}

// Evaluasi kondisi gas berdasarkan nilai ADC hasil kalibrasi Wokwi
GasStatus evaluateStatus(int adcValue) {
  if (adcValue >= THRESHOLD_DANGER) {
    return STATUS_DANGER;
  } else if (adcValue >= THRESHOLD_WARNING) {
    return STATUS_WARNING;
  } else {
    return STATUS_SAFE;
  }
}


// Perbarui kondisi LED sesuai status (tanpa blocking delay)
void updateLeds(GasStatus status, unsigned long now) {
  switch (status) {

    // Kondisi AMAN: hijau nyala, kuning & merah mati 
    case STATUS_SAFE:
      digitalWrite(PIN_LED_GREEN,  HIGH);
      digitalWrite(PIN_LED_YELLOW, LOW);
      digitalWrite(PIN_LED_RED,    LOW);  // merah mati jika sebelumnya bahaya
      break;

    // Kondisi WASPADA: kuning kedip, hijau & merah mati 
    case STATUS_WARNING:
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_RED,   LOW);   // merah mati saat waspada
      // Kedip kuning tanpa delay menggunakan millis()
      if (now - lastBlinkTime >= BLINK_INTERVAL) {
        lastBlinkTime = now;
        ledBlinkState = !ledBlinkState;
        digitalWrite(PIN_LED_YELLOW, ledBlinkState ? HIGH : LOW);
      }
      break;

    // Kondisi BAHAYA: merah kedip, hijau & kuning mati 
    case STATUS_DANGER:
      digitalWrite(PIN_LED_GREEN,  LOW);
      digitalWrite(PIN_LED_YELLOW, LOW);
      // Kedip merah tanpa delay menggunakan millis()
      if (now - lastBlinkTime >= BLINK_INTERVAL) {
        lastBlinkTime = now;
        ledBlinkState = !ledBlinkState;
        digitalWrite(PIN_LED_RED, ledBlinkState ? HIGH : LOW);
      }
      break;
  }
}

// Perbarui buzzer: sirine aktif jika alarmActive = true
// Alarm tetap berbunyi meskipun kondisi sudah berubah ke aman/waspada
// Hanya bisa dimatikan lewat tombol interrupt
void updateBuzzer(unsigned long now) {
  if (!alarmActive) {
    // Matikan buzzer jika alarm tidak aktif
    noTone(PIN_BUZZER);
    return;
  }

  // Sirine dua nada bergantian (mirip ambulan) tanpa blocking delay
  if (now - lastSirenTime >= SIREN_INTERVAL) {
    lastSirenTime = now;
    sirenHighPhase = !sirenHighPhase;
    if (sirenHighPhase) {
      tone(PIN_BUZZER, SIREN_FREQ_HIGH);
    } else {
      tone(PIN_BUZZER, SIREN_FREQ_LOW);
    }
  }
}

// Perbarui tampilan OLED dengan status dan grafik bar level gas
void updateOled(GasStatus status) {
  display.clearDisplay();

  // Judul / Header 
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 0);
  display.println(F("LPG GAS DETECTOR"));

  // Garis pemisah
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Label STATUS 
  display.setTextSize(1);
  display.setCursor(0, 15);
  display.println(F("STATUS:"));

  // Teks status besar 
  display.setTextSize(2);
  display.setCursor(0, 26);
  switch (status) {
    case STATUS_SAFE:
      display.println(F("AMAN"));
      break;
    case STATUS_WARNING:
      display.println(F("WASPADA"));
      break;
    case STATUS_DANGER:
      display.println(F("BAHAYA!"));
      break;
  }

  // Garis pemisah bawah
  display.drawLine(0, 46, 127, 46, SSD1306_WHITE);

  // Grafik bar horizontal dari kiri ke kanan 
  // Pemetaan ADC (200-1010) ke lebar bar (0-120 pixel)
  int barWidth = map(lastAdcValue, 200, 1010, 0, 100);
  barWidth = constrain(barWidth, 0, 100);

  // Gambar kotak luar bar (border) di posisi y=50
  display.drawRect(2, 50, 100, 10, SSD1306_WHITE);

  // Isi bar sesuai status
  display.fillRect(3, 51, barWidth - 2, 8, SSD1306_WHITE);

  // Indikator alarm aktif di pojok kanan bawah 
  if (alarmActive) {
    display.setTextSize(1);
    display.setCursor(104, 50);
    display.print(F("ALRM"));
  }

  display.display();
}

// ISR (Interrupt Service Routine) - dipanggil saat tombol ditekan
// Fungsi: mematikan alarm buzzer secara manual
void handleButtonPress() {
  // Matikan alarm
  alarmActive = false;
  // Hentikan tone langsung dari ISR (aman untuk Wokwi)
  noTone(PIN_BUZZER);
}