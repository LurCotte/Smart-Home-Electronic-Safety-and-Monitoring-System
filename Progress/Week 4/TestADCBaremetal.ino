#include <avr/io.h>

int adc0 = 0;  // Ubah ke signed int agar sesuai dengan offset
//int offset = 510;

void setup() {
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
  ADCSRA |= (1 << ADEN);                                // Enable ADC
  Serial.begin(9600);
}

void loop() {
  // Baca channel A0
  ADMUX = (1 << REFS0) | 0;
  ADCSRA |= (1 << ADSC);
  while (!(ADCSRA & (1 << ADIF)));
  ADCSRA |= (1 << ADIF);
  adc0 = ADC;

  //int selisih = abs(adc0 - offset);

  Serial.println(adc0);

  //delay(100); // agar Serial Monitor tidak terlalu cepat
}