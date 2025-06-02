#include <avr/io.h>
#include <EEPROM.h>
#include <stdlib.h>
#include <stdio.h>
#include <arduino.h>

const int pinRelay = 13;
const int interval = 20; // 20ms sampling interval
const int durasiPemulihan = 10000;
const int jumlahSampel = durasiPemulihan / interval;

const float batas_undervoltage = 150.0;
const float batas_overvoltage = 250.0;
const float batas_fluktuasi = 100.0;
const float batas_arus_short = 2.0;
const float tarif_per_kwh = 1352.0;

float vrms = 0;
float vrms_sebelumnya = 0;
float irms = 0;
float daya = 0;
float energi_kwh = 0;
float biaya_rupiah = 0;
float energi_kwh_terakhir = 0;

unsigned long startMillis;
bool proteksiAktif = false;

struct StatusPemulihan {
  bool teganganBaik;
  bool arusBaik;
};

StatusPemulihan statusBaik[jumlahSampel];
int indeksBuffer = 0;
bool dalamPemulihan = false;

// UART BARE-METAL
void UART_init() {
  UBRR0L = 103; // Baudrate 9600 @ 16MHz
  UCSR0B = (1 << TXEN0) | (1 << RXEN0); // Enable TX dan RX
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data
}

void UART_transmit(char data) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = data;
}

void UART_print(const char* str) {
  while (*str) UART_transmit(*str++);
}

void UART_print_float(const char* label, float value, uint8_t precision) {
  char buffer[64];
  dtostrf(value, 0, precision, buffer);
  UART_print(label);
  UART_print(buffer);
  UART_print("\r\n");
}

void simpanEEPROM() {
  EEPROM.put(0, energi_kwh);
  EEPROM.put(sizeof(float), biaya_rupiah);
}

void muatEEPROM() {
  EEPROM.get(0, energi_kwh);
  EEPROM.get(sizeof(float), biaya_rupiah);
}

void setupADC() {
  ADMUX = (1 << REFS0); // AVCC sebagai referensi
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, prescaler 128
}

int bacaADC(byte channel) {
  ADMUX = (ADMUX & 0xF0) | (channel & 0x07);
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADC;
}

void setup() {
  Serial.begin(9600);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW); // Relay ON awal

  muatEEPROM(); // Load dari EEPROM
  energi_kwh_terakhir = energi_kwh;

  startMillis = millis();
  setupADC();
}

void loop() {
  if (Serial.available()) {
    String perintah = Serial.readStringUntil('\n');
    perintah.trim();

    if (perintah.equalsIgnoreCase("RESET")) {
      energi_kwh = 0;
      biaya_rupiah = 0;
      simpanEEPROM();
      energi_kwh_terakhir = 0;
      Serial.println("Energi dan biaya telah direset ke 0.");
    }
  }

  if (millis() - startMillis >= interval) {
    int vmax = 0;
    int imax = 0;

    for (int i = 0; i < 50; i++) {
      int vSample = bacaADC(1); // A1 = tegangan
      int iSample = bacaADC(0); // A0 = arus

      if (vSample > vmax) vmax = vSample;
      if (iSample > imax) imax = iSample;

      delayMicroseconds(300);
    }

    Serial.print("ADC Arus (ADC0) Max = ");
    Serial.println(imax);

    vrms = (0.978150 * vmax) - 499.81;
    irms = ((0.01345 * imax) - 6.9207);
    daya = vrms * irms;

    Serial.print("VRMS = "); Serial.print(vrms);
    Serial.print(" | IRMS = "); Serial.print(irms);
    Serial.print(" | Daya = "); Serial.print(daya);
    Serial.print(" | kWh = "); Serial.print(energi_kwh, 4);
    Serial.print(" | Rp = "); Serial.println(biaya_rupiah, 2);

    bool kondisiBerbahaya = false;

    if (irms > batas_arus_short) {
      Serial.println("Proteksi: SHORT CIRCUIT!");
      kondisiBerbahaya = true;
    } else if (vrms > batas_overvoltage) {
      Serial.println("Proteksi: OVERVOLTAGE!");
      kondisiBerbahaya = true;
    } else if (vrms < batas_undervoltage) {
      Serial.println("Proteksi: UNDERVOLTAGE!");
      kondisiBerbahaya = true;
    } else if (abs(vrms - vrms_sebelumnya) > batas_fluktuasi) {
      Serial.println("Proteksi: FLUKTUASI TEGANGAN!");
      kondisiBerbahaya = true;
    }

    if (kondisiBerbahaya && !proteksiAktif) {
      proteksiAktif = true;
      digitalWrite(pinRelay, HIGH); // Relay OFF
      Serial.println("Relay OFF (Proteksi Aktif)");
      dalamPemulihan = true;
      indeksBuffer = 0;
    }

    if (dalamPemulihan) {
      statusBaik[indeksBuffer].teganganBaik = (vrms >= batas_undervoltage && vrms <= batas_overvoltage);
      statusBaik[indeksBuffer].arusBaik = (irms <= batas_arus_short);
      indeksBuffer++;

      if (indeksBuffer >= jumlahSampel) {
        int jumlahTeganganBaik = 0;
        int jumlahArusBaik = 0;

        for (int i = 0; i < jumlahSampel; i++) {
          if (statusBaik[i].teganganBaik) jumlahTeganganBaik++;
          if (statusBaik[i].arusBaik) jumlahArusBaik++;
        }

        float persenTeganganBaik = (jumlahTeganganBaik * 100.0) / jumlahSampel;
        float persenArusBaik = (jumlahArusBaik * 100.0) / jumlahSampel;

        Serial.print("Tegangan Stabil: "); Serial.print(persenTeganganBaik); Serial.print("% | ");
        Serial.print("Arus Stabil: "); Serial.print(persenArusBaik); Serial.println("%");

        if (persenTeganganBaik > 70.0 && persenArusBaik > 70.0) {
          digitalWrite(pinRelay, LOW); // Relay ON
          Serial.println("Relay ON (Tegangan dan Arus Stabil)");
          proteksiAktif = false;
          dalamPemulihan = false;
        } else {
          Serial.println("Belum Stabil. Tetap Proteksi.");
          indeksBuffer = 0;
        }
      }
    }

    // ✅ Hanya hitung energi & biaya jika arus cukup (bukan noise)
    if (!proteksiAktif && irms > 0.08) {
      energi_kwh += daya * (interval / 3600000.0); // Wh → kWh
      biaya_rupiah = energi_kwh * tarif_per_kwh;

      if (abs(energi_kwh - energi_kwh_terakhir) >= 0.01) {
        simpanEEPROM();
        energi_kwh_terakhir = energi_kwh;
      }
    }

    vrms_sebelumnya = vrms;
    startMillis = millis();
  }
}