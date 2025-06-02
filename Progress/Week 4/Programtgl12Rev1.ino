#include <avr/io.h>
#include <avr/interrupt.h>
#include <EEPROM.h>

const int pinRelay = 13;
const int interval = 20; // 20ms sampling interval
const int durasiPemulihan = 10000;
const int jumlahSampel = durasiPemulihan / interval;

const float batas_undervoltage = 150.0;
const float batas_overvoltage = 250.0;
const float batas_fluktuasi = 100.0;
const float batas_arus_short = 3.0; // misalnya 10A
const float tarif_per_kwh = 1352.0;

volatile int vmax = 0;
volatile int imax = 0;
// >>> Tukar channel awal: 1 = tegangan, 0 = arus
volatile byte channel = 1; // Mulai dari channel 1 (A1) untuk tegangan

float vrms = 0;
float vrms_sebelumnya = 0;
float irms = 0;
float daya = 0;
float energi_kwh = 0;
float biaya_rupiah = 0;
float energi_kwh_terakhir = 0;

unsigned long startMillis;
bool proteksiAktif = false;
bool statusBaik[jumlahSampel];
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
  ADMUX |= channel;     // Mulai dari channel 1 (tegangan)
  ADCSRA = (1 << ADEN) | (1 << ADIE) |
           (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler 128 (125kHz)
  sei(); // enable global interrupt
  ADCSRA |= (1 << ADSC); // mulai konversi pertama
}

void setup() {
  Serial.begin(9600);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW); // Relay ON awal

  muatEEPROM(); // load data energi & biaya dari EEPROM
  energi_kwh_terakhir = energi_kwh;

  startMillis = millis();
  setupADC();
}

void loop() {
  if (millis() - startMillis >= interval) {
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
      statusBaik[indeksBuffer] = (vrms >= batas_undervoltage);
      indeksBuffer++;

      if (indeksBuffer >= jumlahSampel) {
        int jumlahBaik = 0;
        for (int i = 0; i < jumlahSampel; i++) {
          if (statusBaik[i]) jumlahBaik++;
        }

        float persentaseBaik = (jumlahBaik * 100.0) / jumlahSampel;
        Serial.print("Tegangan Stabil "); Serial.print(persentaseBaik); Serial.println(" %");

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

    if (!proteksiAktif) {
      energi_kwh += daya * (interval / 3600000.0); // Wh → kWh
      biaya_rupiah = energi_kwh * tarif_per_kwh;

      if (abs(energi_kwh - energi_kwh_terakhir) >= 0.01) {
        simpanEEPROM();
        energi_kwh_terakhir = energi_kwh;
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
  if (channel == 1) { // channel 1 = A1 = tegangan
    if (adcVal > vmax) vmax = adcVal;
    channel = 0; // selanjutnya baca arus
  } else { // channel 0 = A0 = arus
    if (adcVal > imax) imax = adcVal;
    channel = 1; // selanjutnya baca tegangan
  }
  ADMUX = (ADMUX & 0xF0) | channel;
  ADCSRA |= (1 << ADSC); // mulai konversi berikutnya
}
