#include <avr/io.h>
#include <util/delay.h>

unsigned int adc0 = 0; // Sensor CT
unsigned int adc1 = 0; // ZMPT101B
int imax = 0;
int vmax = 0;
float irms = 0;
float vrms = 0;

const unsigned int total_sampel = 100;
const unsigned long interval_us = 200;
unsigned long lastMicros = 0;
unsigned int sampel_count = 0;

void setup() {
  // ADC setup
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
  ADCSRA |= (1 << ADEN); // Enable ADC

  Serial.begin(9600);
}

void loop() {
  unsigned long sekarang = micros();

  if (sampel_count == 0) {
    imax = 0;
    vmax = 0;
    lastMicros = sekarang;
  }

  if (sampel_count < total_sampel && (sekarang - lastMicros >= interval_us)) {
    lastMicros = sekarang;

    // --- Baca Sensor Arus di A0 (CT) ---
    ADMUX = (1 << REFS0) | 0; // AVCC + channel 0 (A0)
    ADCSRA |= (1 << ADSC);
    while (!(ADCSRA & (1 << ADIF)));
    ADCSRA |= (1 << ADIF);
    adc0 = ADC;
    if (adc0 > imax) imax = adc0;

    // --- Baca Sensor Tegangan di A1 (ZMPT101B) ---
    ADMUX = (1 << REFS0) | 1; // AVCC + channel 1 (A1)
    ADCSRA |= (1 << ADSC);
    while (!(ADCSRA & (1 << ADIF)));
    ADCSRA |= (1 << ADIF);
    adc1 = ADC;
    if (adc1 > vmax) vmax = adc1;

    sampel_count++;
  }

  if (sampel_count >= total_sampel) {
    // --- Hitung Irms ---
    int offset_current = 512;
    int delta_current = abs(imax - offset_current);
    irms = (0.0211 * delta_current) - 10.92;

    // --- Hitung Vrms ---
    int offset_voltage = 512;
    int delta_voltage = abs(vmax - offset_voltage);
    vrms = (0.231 * delta_voltage) - 499.81;  // Sesuaikan dengan kalibrasi ZMPT101B kamu

    // --- Tampilkan hasil ---
    if (irms > 0) {
      Serial.print("Irms: ");
      Serial.print(delta_current);
      Serial.print(" A\t");
    }

    if (vrms > 0) {
      Serial.print("Vrms: ");
      Serial.print(vrms);
      Serial.println(" V");
    }

    sampel_count = 0;
  }
}
