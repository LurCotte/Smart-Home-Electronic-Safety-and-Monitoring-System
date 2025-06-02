#include <avr/io.h>
#include <avr/interrupt.h>
#include <EEPROM.h>
#include <stdlib.h>
#include <util/delay.h>

const int pinRelay = 8;
const int interval = 20; // 20ms sampling interval
const int durasiPemulihan = 10000;
const int jumlahSampel = durasiPemulihan / interval;

const float batas_undervoltage = 180.0;
const float batas_overvoltage = 250.0;
const float batas_fluktuasi = 20.0;
const float batas_arus_short = 10.0;
const float tarif_per_kwh = 1352.0;

volatile int vmax = 0;
volatile int imax = 0;
volatile byte channel = 0;

float vrms = 0;
float vrms_sebelumnya = 0;
float irms = 0;
float daya = 0;
float energi_kwh = 0;
float biaya_rupiah = 0;
float energi_kwh_terakhir = 0;

volatile unsigned long waktuMillis = 0;
unsigned long startMillis = 0;

bool proteksiAktif = false;
bool statusBaik[jumlahSampel];
int indeksBuffer = 0;
bool dalamPemulihan = false;

// Fungsi pengganti millis()
unsigned long millis_custom() {
  unsigned long m;
  cli();
  m = waktuMillis;
  sei();
  return m;
}

// Timer1 setup untuk interrupt setiap 1ms
void setupTimer1_millis() {
  TCCR1A = 0;
  TCCR1B = (1 << WGM12);         // CTC mode
  OCR1A = 249;                   // 16MHz / (64 * 1000) - 1 = 249
  TIMSK1 = (1 << OCIE1A);        // enable compare match A
  TCCR1B |= (1 << CS11) | (1 << CS10); // prescaler 64
}

// Timer1 ISR setiap 1ms
ISR(TIMER1_COMPA_vect) {
  waktuMillis++;
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
  ADMUX = (1 << REFS0) | channel;
  ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  ADCSRA |= (1 << ADSC); // start first conversion
}

void setup() {
  Serial.begin(9600);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW);

  muatEEPROM();
  energi_kwh_terakhir = energi_kwh;

  setupADC();
  setupTimer1_millis();
  sei(); // enable global interrupt

  startMillis = millis_custom();
}

void loop() {
  if (millis_custom() - startMillis >= interval) {
    vrms = (0.9743012 * vmax) - 499.81;
    irms = ((0.0211 * imax) - 10.92);
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
      energi_kwh += daya * (interval / 3600000.0); // Wh to kWh
      biaya_rupiah = energi_kwh * tarif_per_kwh;

      if (abs(energi_kwh - energi_kwh_terakhir) >= 0.01) {
        simpanEEPROM();
        energi_kwh_terakhir = energi_kwh;
      }
    }

    vrms_sebelumnya = vrms;
    vmax = 0;
    imax = 0;
    startMillis = millis_custom();
  }
}

// ADC interrupt
ISR(ADC_vect) {
  int adcVal = ADC;
  if (channel == 0) {
    if (adcVal > vmax) vmax = adcVal;
    channel = 1;
  } else {
    if (adcVal > imax) imax = adcVal;
    channel = 0;
  }
  ADMUX = (ADMUX & 0xF0) | channel;
  ADCSRA |= (1 << ADSC);
}
