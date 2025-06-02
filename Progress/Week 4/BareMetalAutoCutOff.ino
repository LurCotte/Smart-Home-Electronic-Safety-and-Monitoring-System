#include <avr/io.h>
#include <avr/interrupt.h>

const int pinRelay = 8;
const int interval = 20; // 20ms sampling interval
const int durasiPemulihan = 10000;
const int jumlahSampel = durasiPemulihan / interval;

const float batas_undervoltage = 180.0;
const float batas_overvoltage = 250.0;
const float batas_fluktuasi = 20.0;

volatile int vmax = 0;
float vrms = 0;
float vrms_sebelumnya = 0;

unsigned long startMillis;
bool proteksiAktif = false;

bool statusBaik[jumlahSampel];
int indeksBuffer = 0;
bool dalamPemulihan = false;
unsigned long waktuMulaiPemulihan = 0;

void setupADC() {
  ADMUX = (1 << REFS0);          // AVCC sebagai referensi, input ADC0
  ADMUX |= (0 << ADLAR);         // Hasil kanan (10-bit)
  
  ADCSRA = (1 << ADEN) |         // Aktifkan ADC
           (1 << ADIE) |         // Aktifkan interrupt
           (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128 → 125kHz
           
  sei();                         // Aktifkan global interrupt

  ADCSRA |= (1 << ADSC);         // Mulai konversi pertama
}

void setup() {
  Serial.begin(9600);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW); // Relay ON (awal)
  startMillis = millis();
  setupADC(); // Inisialisasi ADC interrupt
}

void loop() {
  if (millis() - startMillis >= interval) {
    vrms = (0.982 * vmax) - 499.81;

    Serial.print("VRMS = ");
    Serial.println(vrms);

    bool kondisiBerbahaya = false;

    if (vrms > batas_overvoltage) {
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
      waktuMulaiPemulihan = millis();
      indeksBuffer = 0;
    }

    if (dalamPemulihan) {
      statusBaik[indeksBuffer] = (vrms >= batas_undervoltage);
      indeksBuffer++;

      if (indeksBuffer >= jumlahSampel) {
        int jumlahBaik = 0;
        for (int i = 0; i < jumlahSampel; i++) {
          if (statusBaik[i]) jumlahBaik++;
        }

        float persentaseBaik = (jumlahBaik * 100.0) / jumlahSampel;
        Serial.print("Tegangan Stabil ");
        Serial.print(persentaseBaik);
        Serial.println(" %");

        if (persentaseBaik > 70.0) {
          digitalWrite(pinRelay, LOW); // Relay ON
          Serial.println("Relay ON (Tegangan Stabil Kembali)");
          proteksiAktif = false;
          dalamPemulihan = false;
        } else {
          Serial.println("Tegangan Belum Stabil. Tetap Proteksi.");
          indeksBuffer = 0;
        }
      }
    }

    vrms_sebelumnya = vrms;
    vmax = 0;
    startMillis = millis();
  }
}

// ISR untuk ADC
ISR(ADC_vect) {
  int adcVal = ADC; // baca 10-bit hasil
  if (adcVal > vmax) vmax = adcVal;
  ADCSRA |= (1 << ADSC); // mulai konversi berikutnya
}
