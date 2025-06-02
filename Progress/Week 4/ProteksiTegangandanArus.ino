#include <avr/io.h>
#include <avr/interrupt.h>

const int pinRelay = 8;
const int interval = 20; // 20ms sampling interval
const int durasiPemulihan = 10000;
const int jumlahSampel = durasiPemulihan / interval;

const float batas_undervoltage = 180.0;
const float batas_overvoltage = 250.0;
const float batas_fluktuasi = 20.0;
const float batas_arus_short = 5.0; // misalnya 5A untuk short

volatile int vmax = 0;
volatile int imax = 0;
volatile byte channel = 0; // 0 = tegangan (ZMPT), 1 = arus (ACS)

float vrms = 0;
float vrms_sebelumnya = 0;
float irms = 0;

unsigned long startMillis;
bool proteksiAktif = false;
bool statusBaik[jumlahSampel];
int indeksBuffer = 0;
bool dalamPemulihan = false;
unsigned long waktuMulaiPemulihan = 0;

void setupADC() {
  ADMUX = (1 << REFS0);          // AVCC sebagai referensi
  ADMUX |= channel;              // Channel awal (0)
  ADCSRA = (1 << ADEN) | (1 << ADIE) | 
           (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128 → 125kHz
  sei(); // Enable global interrupt
  ADCSRA |= (1 << ADSC); // Mulai konversi pertama
}

void setup() {
  Serial.begin(9600);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW); // Relay ON awal
  startMillis = millis();
  setupADC();
}

void loop() {
  if (millis() - startMillis >= interval) {
    // Konversi hasil pembacaan
    vrms = (0.9743012 * vmax) - 499.81;
    irms = ((0.02109 * imax) - 10.92);

    Serial.print("VRMS = ");
    Serial.print(vrms);
    Serial.print(" | IRMS = ");
    Serial.println(irms);

    bool kondisiBerbahaya = false;

    // PRIORITAS: Proteksi Short Circuit
    if (irms > batas_arus_short) {
      Serial.println("Proteksi: SHORT CIRCUIT!");
      kondisiBerbahaya = true;
    }
    else if (vrms > batas_overvoltage) {
      Serial.println("Proteksi: OVERVOLTAGE!");
      kondisiBerbahaya = true;
    }
    else if (vrms < batas_undervoltage) {
      Serial.println("Proteksi: UNDERVOLTAGE!");
      kondisiBerbahaya = true;
    }
    else if (abs(vrms - vrms_sebelumnya) > batas_fluktuasi) {
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

    // Reset siklus
    vrms_sebelumnya = vrms;
    vmax = 0;
    imax = 0;
    startMillis = millis();
  }
}

// ISR ADC
ISR(ADC_vect) {
  int adcVal = ADC;

  if (channel == 0) {
    if (adcVal > vmax) vmax = adcVal;
    channel = 1;
  } else {
    if (adcVal > imax) imax = adcVal;
    channel = 0;
  }

  // Pindah channel ADC
  ADMUX = (ADMUX & 0xF0) | channel;
  ADCSRA |= (1 << ADSC); // mulai konversi berikutnya
}
