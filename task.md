# Task: Implementasi FreeRTOS Queue pada `gas-sense.ino`

## Tujuan
Menghilangkan penggunaan variabel global untuk komunikasi antar-task guna mencegah *race condition* dan meningkatkan stabilitas sistem, dengan memanfaatkan fitur **FreeRTOS Queue**.

## Target File
- `gas-sense.ino`

## Instruksi Pengerjaan

### 1. Tambahkan FreeRTOS Queue
- Buat dan inisialisasi FreeRTOS Queue menggunakan `xQueueCreate` di dalam fungsi `setup()`.
- Buat tipe data `struct` baru (misalnya `SystemData` atau `SensorData`) untuk membungkus data-data yang akan dikirim melalui queue.

### 2. Hapus Variabel Global
- Hapus penggunaan variabel global *volatile* berikut dari kode:
  ```cpp
  volatile GasStatus currentStatus = STATUS_CLEAN;
  volatile int currentPPM = 0;
  volatile bool alarmActive = false;
  ```

### 3. Implementasikan Queue pada Semua Task
- Modifikasi fungsi-fungsi task berikut agar mengirim (`xQueueSend`) dan menerima (`xQueueReceive`) data melalui queue sebagai pengganti variabel global:
  - `TaskSensor(void *pvParameters)`: Sebagai *Producer*. Membaca data ADC dari pin sensor MQ-2, menghitung PPM, menentukan statusnya (CLEAN/POLUTED/HAZARDOUS), dan mengirim `struct` data tersebut ke dalam queue.
  - `TaskDisplay(void *pvParameters)`: Sebagai *Consumer*. Menerima data dari queue untuk memperbarui tampilan nilai PPM, *progress*, dan status pada LCD.
  - `TaskBuzzer(void *pvParameters)`: Sebagai *Consumer*. Menerima informasi untuk menentukan apakah alarm peringatan berbunyi sesuai state/status data.
  - `TaskButton(void *pvParameters)`: Menangani interrupt/polling button. Anda mungkin perlu memodifikasi logika reset alarm (`alarmActive = false`) ini menjadi pengiriman *flag* atau pesan ke task lain menggunakan Queue yang sama atau Queue/Semaphore terpisah.

## Catatan
- Pastikan semua task tidak saling *blocking* terlalu lama.
- Hapus semua *referensi* langsung ke variabel `currentStatus`, `currentPPM`, dan `alarmActive` di dalam kode.
