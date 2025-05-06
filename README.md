# 🔌 Smart Home Electronic Safety and Monitoring System

![Flyer]([https://github.com/user-attachments/assets/68314629-e209-4303-87ca-301ea59e8e5e](https://github.com/LurCotte/Smart-Home-Electronic-Safety-and-Monitoring-System/blob/main/Progress/Week%203/Mikrokontroller.png))

## Team Member

![Team Member]([[https://github.com/user-attachments/assets/68314629-e209-4303-87ca-301ea59e8e5e](https://github.com/LurCotte/Smart-Home-Electronic-Safety-and-Monitoring-System/blob/main/Progress/Week%203/Mikrokontroller.png)](https://github.com/LurCotte/Smart-Home-Electronic-Safety-and-Monitoring-System/blob/main/Progress/Week%203/Profile.png))

## 📘 Deskripsi Proyek

Proyek ini bertujuan untuk mengembangkan **sistem automasi rumah pintar (Smart Home Automation System)** yang dapat memantau dan mengontrol peralatan elektronik secara real-time. Sistem ini juga dilengkapi dengan fitur **auto cut-off** untuk menjaga peralatan rumah tangga dari fluktuasi tegangan yang berasal dari jaringan listrik PLN.

## 🎯 Tujuan

- Mengontrol peralatan elektronik rumah secara dari satu titik pusat.
- Memantau konsumsi energi listrik dan menghitung biaya secara akurat.
- Melindungi perangkat dari overvoltage atau undervoltage melalui fitur auto cut-off.

## 🧠 Komponen Utama

| Komponen       | Fungsi                                                                 |
|----------------|------------------------------------------------------------------------|
| Arduino Mega   | Mikrokontroler utama yang mengatur komunikasi dan pengolahan data.     |
| ACS712         | Sensor arus untuk mengukur konsumsi arus listrik.                      |
| ZMPT101B       | Sensor tegangan AC untuk memantau tegangan dari sumber PLN.            |
| Relay SPDT     | Saklar untuk mengontrol perangkat listrik seperti lampu.      |
| MCB 1 Phase    | Proteksi arus lebih dan hubung singkat memutus aliran saat overload.  |
| Sensor Tambahan| Sensor-sensor lain untuk mendukung fungsi Smart Home tambahan.         |


## 🔗 Komunikasi Sistem

Sistem menggunakan **komunikasi UART TTL** untuk mentransmisikan data antar perangkat dan sensor secara real-time. Arduino Mega memproses input dari sensor, lalu mengambil tindakan sesuai logika program.

## ⚡ Fitur Utama

- **Auto Cut-Off:** Memutus sambungan perangkat jika terjadi fluktuasi tegangan di luar ambang batas aman.
- **Monitoring Energi:** Penggunaan sensor ACS712 dan ZMPT101B untuk memantau konsumsi arus dan tegangan secara real-time.
- **Kontrol Perangkat Secara Terpusat:** Semua perangkat bisa dikontrol dari satu titik pusat melalui Komputer/laptop via UART.

## 🧩 Penjelasan Alur Sistem

- **Kabel fasa dan netral dari MCB PLN** terlebih dahulu masuk ke **input sensor ZMPT101B**. Sensor ini digunakan untuk membaca nilai tegangan dalam bentuk ADC (Analog to Digital Converter) yang kemudian akan diumpankan ke **pin analog mikrokontroler AVR**.

- **Mikrokontroler AVR** berkomunikasi dengan **laptop melalui komunikasi serial UART**, yang berfungsi sebagai antarmuka dan pusat kontrol untuk mengaktifkan atau menonaktifkan perangkat elektronik dari satu tempat.

- Pada **relay 5V**, pin **pole (common)** dihubungkan ke kabel fasa dari MCB PLN, pin **NO (Normally Open)** dihubungkan ke input MCB 1 phase, dan pin **coil** dihubungkan ke **pin output dari mikrokontroler AVR**.

- Jika **mikrokontroler AVR menerima perintah aktivasi dari laptop**, maka ia akan mengaktifkan pin output yang telah ditentukan, sehingga mengaktifkan relay dan menyebabkan perangkat elektronik menyala. Namun, hal ini hanya akan terjadi **jika tidak terdeteksi adanya fluktuasi tegangan atau hubung singkat**.

- **Perintah pengaktifan perangkat elektronik tidak akan diproses jika terjadi fluktuasi tegangan**, karena fungsi pendeteksi fluktuasi memiliki prioritas lebih tinggi. 

- Selain itu, jika terjadi **hubung singkat**, maka aktivasi perangkat tidak dapat dilakukan karena **MCB secara otomatis akan melakukan pemutusan arus (OFF)** untuk proteksi sistem.

- **Sensor ACS712** digunakan untuk membaca arus listrik yang kemudian akan digunakan untuk **perhitungan daya listrik (Watt)** secara real-time.


## 📊 Mind Map, Flowchart dan Diagram Blok

*Mind Map.*

![Mind Map](https://github.com/user-attachments/assets/68314629-e209-4303-87ca-301ea59e8e5e)


*Flowchart.*

![Flowchart Logika Kerja](https://github.com/user-attachments/assets/52e0e276-9046-4dce-b638-a6a6c75a07c6)

*Diagram Blok Sistem.*

![Diagram Hardware](https://github.com/user-attachments/assets/68ff35f3-2189-4610-88c4-a1e232965c26)



## 🚀 Pengembangan Selanjutnya

- Integrasi dengan koneksi WiFi (ESP8266/ESP32) untuk kontrol via aplikasi mobile.
- Dashboard berbasis web untuk pemantauan konsumsi daya secara grafis.
- Otomatisasi berdasarkan waktu, cuaca, atau sensor gerak.

## 📁 Struktur Direktori (Opsional)
```bash
SmartHome-Automation/
│
├── src/                      # Kode program Arduino
├── images/                   # Diagram dan flowchart sistem
│   ├── block_diagram.png
│   └── flowchart_program.png
├── README.md                 # Dokumentasi proyek
└── docs/                     # Panduan penggunaan & pengembangan
