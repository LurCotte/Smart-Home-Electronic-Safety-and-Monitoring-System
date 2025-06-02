#include <avr/io.h>
#include <EEPROM.h>
#include <stdlib.h>
#include <stdio.h>
#include <arduino.h>

const int pinRelay = 13;
const int interval = 20;
const int durasiPemulihan = 1000;
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
bool permintaanManualRelay = false;


struct StatusPemulihan {
  bool teganganBaik;
  bool arusBaik;
};

StatusPemulihan statusBaik[jumlahSampel];
int indeksBuffer = 0;
bool dalamPemulihan = false;

// UART
void UART_init() {
  UBRR0L = 103;
  UCSR0B = (1 << TXEN0) | (1 << RXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
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

void kirimDataPaket() {
  char buffer[64];
  sprintf(buffer, "V:%.2f;I:%.3f;E:%.4f;C:%.2f\r\n", vrms, irms, energi_kwh, biaya_rupiah);
  UART_print(buffer);
}

char UART_receive() {
  while (!(UCSR0A & (1 << RXC0)));
  return UDR0;
}

// EEPROM
void simpanEEPROM() {
  EEPROM.put(0, energi_kwh);
  EEPROM.put(sizeof(float), biaya_rupiah);
}

void muatEEPROM() {
  EEPROM.get(0, energi_kwh);
  EEPROM.get(sizeof(float), biaya_rupiah);
}

// ADC
void setupADC() {
  ADMUX = (1 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

int bacaADC(byte channel) {
  ADMUX = (ADMUX & 0xF0) | (channel & 0x07);
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  return ADC;
}

void setup() {
  UART_init();
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, HIGH);

  muatEEPROM();
  energi_kwh_terakhir = energi_kwh;

  startMillis = millis();
  setupADC();

  UART_print("Sistem siap.\r\n");
  UART_print("Ketik 'N' = ON relay (cek stabilitas), 'F' = OFF manual, 'R' = reset energi.\r\n");
}

void loop() {
  if (UCSR0A & (1 << RXC0)) {
    char cmd = UART_receive();
    if (cmd == 'N' || cmd == 'n') {
      permintaanManualRelay = true;
      UART_print("Permintaan manual: Nyalakan relay (cek stabilitas 10 detik)\r\n");
      indeksBuffer = 0;
    } else if (cmd == 'F' || cmd == 'f') {
      if (!proteksiAktif) {
        digitalWrite(pinRelay, HIGH);
        UART_print("Relay dimatikan manual.\r\n");
      } else {
        UART_print("Tidak bisa dimatikan. Proteksi aktif.\r\n");
      }
    } else if (cmd == 'R' || cmd == 'r') {
      energi_kwh = 0;
      biaya_rupiah = 0;
      energi_kwh_terakhir = 0;
      simpanEEPROM();
     
      UART_print("Energi dan biaya telah di-reset.\r\n");
    }
  }

  if (millis() - startMillis >= interval) {
    int vmax = 0;
    int imax = 0;

    for (int i = 0; i < 50; i++) {
      int vSample = bacaADC(1);
      int iSample = bacaADC(0);
      if (vSample > vmax) vmax = vSample;
      if (iSample > imax) imax = iSample;
      delayMicroseconds(300);
    }

    vrms = (0.978150 * vmax) - 499.81;
    irms = ((0.01345 * imax) - 6.9207);
    daya = vrms * irms;

    UART_print_float("VRMS = ", vrms, 2);
    UART_print_float("kWh  = ", energi_kwh, 4);
    UART_print_float("Rp   = ", biaya_rupiah, 2);

    bool kondisiBerbahaya = false;

    if (irms > batas_arus_short) {
      UART_print("Proteksi: SHORT CIRCUIT!\r\n");
      kondisiBerbahaya = true;
    } else if (vrms > batas_overvoltage) {
      UART_print("Proteksi: OVERVOLTAGE!\r\n");
      kondisiBerbahaya = true;
    } else if (vrms < batas_undervoltage) {
      UART_print("Proteksi: UNDERVOLTAGE!\r\n");
      kondisiBerbahaya = true;
    } else if (abs(vrms - vrms_sebelumnya) > batas_fluktuasi) {
      UART_print("Proteksi: FLUKTUASI TEGANGAN!\r\n");
      kondisiBerbahaya = true;
    }

    if (kondisiBerbahaya && !proteksiAktif) {
      proteksiAktif = true;
      digitalWrite(pinRelay, HIGH);
      UART_print("Relay OFF (Proteksi Aktif)\r\n");
      dalamPemulihan = true;
      indeksBuffer = 0;
    }

    if (dalamPemulihan || permintaanManualRelay) {
      statusBaik[indeksBuffer].teganganBaik = (vrms >= batas_undervoltage && vrms <= batas_overvoltage);
      statusBaik[indeksBuffer].arusBaik = (irms <= batas_arus_short);
      indeksBuffer++;

      if (indeksBuffer >= jumlahSampel) {
        int jumlahTeganganBaik = 0, jumlahArusBaik = 0;
        for (int i = 0; i < jumlahSampel; i++) {
          if (statusBaik[i].teganganBaik) jumlahTeganganBaik++;
          if (statusBaik[i].arusBaik) jumlahArusBaik++;
        }

        float persenTeganganBaik = (jumlahTeganganBaik * 100.0) / jumlahSampel;
        float persenArusBaik = (jumlahArusBaik * 100.0) / jumlahSampel;

        UART_print_float("Tegangan Stabil = ", persenTeganganBaik, 1);
        UART_print_float("Arus Stabil     = ", persenArusBaik, 1);

        if (persenTeganganBaik > 70.0 && persenArusBaik > 70.0) {
          digitalWrite(pinRelay, LOW);
          UART_print("Relay ON (Tegangan dan Arus Stabil)\r\n");
          proteksiAktif = false;
          dalamPemulihan = false;
          permintaanManualRelay = false;
        } else {
          if (dalamPemulihan) {
            UART_print("Belum Stabil. Tetap Proteksi.\r\n");
          } else if (permintaanManualRelay) {
            UART_print("Permintaan manual ditolak: Tegangan/arus tidak stabil.\r\n");
            permintaanManualRelay = false;
          }
          indeksBuffer = 0;
        }
      }
    }

    if (!proteksiAktif  && irms > 0.1) {
      energi_kwh += daya * (interval / 3600000.0);
      biaya_rupiah = energi_kwh * tarif_per_kwh;

      if (abs(energi_kwh - energi_kwh_terakhir) >= 0.01) {
        simpanEEPROM();
        energi_kwh_terakhir = energi_kwh;
      }
    }

    kirimDataPaket();

    vrms_sebelumnya = vrms;
    startMillis = millis();
  }
}