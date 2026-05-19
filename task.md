# Tugas Implementasi Sistem Deteksi Dini Kebocoran Gas LPG

Dokumen ini berisi panduan dan daftar tugas yang harus diimplementasikan pada file `gas-sense.ino`. Sistem ini menggunakan Arduino dengan FreeRTOS.

## 1. Task Sensor (`TaskSensor`)
Tugas ini bertanggung jawab membaca nilai dari sensor MQ-2 (gas LPG) dan mengonversinya menjadi satuan PPM, lalu menentukan status keamanan lingkungan.

**Langkah Implementasi:**
1. Baca nilai ADC dari sensor analog `PIN_MQ135`.
2. Lakukan kalibrasi matematika untuk mengubah nilai ADC menjadi PPM. Gunakan rumus berikut:
   - **Tahap 1 (Konversi Tegangan):** 
     $$V_{out} = \frac{ADC \times 5.0}{1023}$$
   - **Tahap 2 (Resistansi Sensor):**
     $$R_{S} = R_{L} \times \frac{5.0 - V_{out}}{V_{out}}$$
     *(Catatan: $R_{L}$ adalah nilai resistansi beban pada modul sensor)*
   - **Tahap 3 (Rasio Resistansi):**
     $$Ratio = \frac{R_{S}}{R_{0}}$$
     *(Catatan: $R_{0}$ adalah nilai resistansi sensor pada kondisi udara bersih)*
   - **Tahap 4 (Kalkulasi PPM):**
     $$PPM = 10^{\frac{\log_{10}(Ratio) - b}{m}}$$
     *(Catatan: Nilai $m$ dan $b$ didapat dari grafik logaritmik datasheet sensor gas LPG)*
3. Tentukan `GasStatus` berdasarkan nilai PPM:
   - **Aman (`STATUS_SAFE`)**: Di bawah 300 PPM.
   - **Waspada (`STATUS_WARNING`)**: Antara 300 hingga 1000 PPM.
   - **Bahaya (`STATUS_DANGER`)**: Di atas 1000 PPM.

## 2. Manajemen Data Antar Task (Inter-Task Communication)
Data status gas (dari `enum GasStatus`) dan nilai PPM yang didapat dari `TaskSensor` harus bisa dibaca oleh task lainnya (Display, Buzzer, dan Button).
- **Implementasi**: Gunakan Global Variable (dengan perlindungan Mutex jika diperlukan) atau FreeRTOS Queue sehingga semua Task dapat merespons perubahan status secara bersamaan.

## 3. Task Display (`TaskDisplay`)
Tugas ini bertugas untuk mengatur antarmuka LCD I2C 16x2 dan mengatur lampu indikator LED.

**Logika Display I2C 16x2:**
- **Baris Atas (Baris 0):** Menampilkan teks kondisi saat ini (contoh: `"Status: AMAN"`, `"Status: WARNING"`, atau `"Status: BAHAYA!"`).
- **Baris Bawah (Baris 1):** Menampilkan progress bar (persentase `%`) yang merepresentasikan konsentrasi udara/gas LPG berdasarkan nilai PPM.

**Logika Kontrol LED:**
- **Aman (`STATUS_SAFE`)**: Lampu Hijau menyala terus, Merah dan Kuning mati.
- **Waspada (`STATUS_WARNING`)**: Lampu Kuning menyala terus, Hijau dan Merah mati.
- **Bahaya (`STATUS_DANGER`)**: Lampu Merah berkedip/menyala seperti pola lampu ambulans darurat (menyala bergantian atau berkedip cepat), Kuning dan Hijau mati.

## 4. Task Buzzer (`TaskBuzzer`)
Tugas ini mengatur alarm peringatan suara.

**Logika Buzzer:**
- Hanya aktif saat status gas menjadi **Waspada (Warning)** atau **Bahaya (Danger)**.
- **Warning**: Buzzer menyala dengan jeda intermiten setiap 3 hingga 6 detik.
- **Danger**: Buzzer menyala terus menerus dengan nada sirine (frekuensi melengking bergantian).
- **Syarat Penting**: Jika sistem sudah pernah memasuki status Warning atau Danger dan buzzer menyala, **buzzer akan tetap menyala** meskipun kondisi gas sudah kembali **Aman**, hingga tombol ditekan untuk mematikannya.

## 5. Task Button (`TaskButton`)
Tugas ini menangani input dari push button untuk mematikan alarm.

**Logika Button:**
- Tombol hanya berfungsi untuk **mematikan alarm buzzer**.
- **Syarat**: Buzzer HANYA BISA dimatikan jika status gas saat ini sudah **Aman (`STATUS_SAFE`)**.
- Jika tombol ditekan tetapi kondisi masih `STATUS_WARNING` atau `STATUS_DANGER`, abaikan input (buzzer tidak boleh mati).
