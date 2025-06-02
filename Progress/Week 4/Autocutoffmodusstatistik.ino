const int pinRelay = 13;
const int interval = 20; // 100ms
const int durasiPemulihan = 10000; // 10 detik = 10000ms
const int jumlahSampel = durasiPemulihan / interval; // 100 sampel

const float batas_undervoltage = 180.0;
const float batas_overvoltage = 250.0;
const float batas_fluktuasi = 20.0;

int vmax = 0;
float vrms = 0;
float vrms_sebelumnya = 0;

unsigned long startMillis;
bool proteksiAktif = false;

// Buffer status selama 10 detik
bool statusBaik[jumlahSampel];
int indeksBuffer = 0;
bool dalamPemulihan = false;
unsigned long waktuMulaiPemulihan = 0;

void setup() {
  Serial.begin(9600);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW); // Relay ON (awal)
  startMillis = millis();
}

void loop() {
  int val = analogRead(A1);
  if (val > vmax) vmax = val;

  if (millis() - startMillis >= interval) {
    vrms = (0.9743012 * vmax) - 499.81;

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
      indeksBuffer = 0; // Reset buffer
    }

    // Jika sedang dalam masa pemulihan (10 detik)
    if (dalamPemulihan) {
      statusBaik[indeksBuffer] = (vrms >= batas_undervoltage);
      indeksBuffer++;

      if (indeksBuffer >= jumlahSampel) {
        // Cek berapa kali tegangan baik
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
          // Ulangi pemulihan lagi
          indeksBuffer = 0;
        }
      }
    }

    // Reset untuk siklus berikutnya
    vrms_sebelumnya = vrms;
    vmax = 0;
    startMillis = millis();
  }
}
