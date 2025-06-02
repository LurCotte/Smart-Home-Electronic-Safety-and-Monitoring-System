#include <avr/io.h>
#include <EEPROM.h>

const int pinRelay = 13;
const int interval = 20; // Interval sampling 20ms
const int durasiPemulihan = 10000; // Waktu pemulihan 10 detik
const int jumlahSampel = durasiPemulihan / interval;

const float batas_undervoltage = 150.0;
const float batas_overvoltage = 250.0;
const float batas_fluktuasi = 100.0;
const float batas_arus_short = 3.0;
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
  while (ADCSRA & (1 << ADSC)); // Tunggu konversi selesai
  return ADC;
}

void setup() {
  Serial.begin(9600);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW); // Relay ON awal

  muatEEPROM();
  energi_kwh_terakhir = energi_kwh;

  startMillis = millis();
  setupADC();
}

void loop() {
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

    vrms = (0.9743012 * vmax) - 499.81;
    irms = ((0.0111 * imax) - 5.673);
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
          indeksBuffer = 0; // Reset setelah sukses
        } else {
          Serial.println("Belum Stabil. Lanjut Monitoring...");
          // Jangan reset indeksBuffer di sini
        }
      }
    }

    if (!proteksiAktif) {
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
