int vmax = 0;
unsigned long startMillis;
const unsigned long interval = 20; // 0.1 detik
float vrms = 0;
float vrms_sebelumnya = 0;

const int pinRelay = 8; // Pin relay
const unsigned long delayProteksi = 3000; // Delay 5 detik (5000 ms)

// Ambang batas proteksi
const float batas_overvoltage = 250.0;
const float batas_undervoltage = 180.0;
const float batas_fluktuasi = 30.0;

// Status proteksi
bool proteksiAktif = false;
unsigned long waktuProteksiAktif = 0;

void setup() {
  Serial.begin(9600);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW); // LOW = relay ON (normal)
  startMillis = millis();
}

void loop() {
  int val = analogRead(A0);
  if (val > vmax) {
    vmax = val;
  }

  if (millis() - startMillis >= interval) {
    vrms = (0.975 * vmax) - 499.81;

    Serial.print("vrms = ");
    Serial.println(vrms);

    bool kondisiBerbahaya = false;

    // 1. Overvoltage
    if (vrms > batas_overvoltage) {
      Serial.println("Proteksi: OVERVOLTAGE!");
      kondisiBerbahaya = true;
    }

    // 2. Undervoltage
    else if (vrms < batas_undervoltage) {
      Serial.println("Proteksi: UNDERVOLTAGE!");
      kondisiBerbahaya = true;
    }

    // 3. Fluktuasi tegangan mendadak
    else if (abs(vrms - vrms_sebelumnya) > batas_fluktuasi) {
      Serial.println("Proteksi: FLUKTUASI TEGANGAN!");
      kondisiBerbahaya = true;
    }

    // Aktifkan proteksi jika kondisi berbahaya
    if (kondisiBerbahaya) {
      if (!proteksiAktif) {
        proteksiAktif = true;
        waktuProteksiAktif = millis();
        digitalWrite(pinRelay, HIGH); // Relay OFF
        Serial.println("Relay OFF (Proteksi Aktif)");
      } else {
        Serial.println("Proteksi Masih Aktif");
      }
    }

    // Jika tidak berbahaya dan sudah lewat 5 detik, hidupkan kembali relay
    if (!kondisiBerbahaya && proteksiAktif) {
      if (millis() - waktuProteksiAktif >= delayProteksi) {
        proteksiAktif = false;
        digitalWrite(pinRelay, LOW); // Relay ON
        Serial.println("Relay ON (Tegangan Stabil Kembali)");
      } else {
        Serial.println("Menunggu Tegangan Stabil (Delay Proteksi)...");
      }
    }

    vrms_sebelumnya = vrms;
    vmax = 0;
    startMillis = millis();
  }
}
