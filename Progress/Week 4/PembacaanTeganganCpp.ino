int vmax = 0;
unsigned long startMillis;
const unsigned long interval = 100; // dari 1000 ms -> 100 ms (0.1 detik)

void setup() {
  Serial.begin(9600);
  startMillis = millis();
}

void loop() {
  int val = analogRead(A0);
  if (val > vmax) {
    vmax = val;
  }

  // Jika waktu interval sudah lewat, hitung dan tampilkan
  if (millis() - startMillis >= interval) {
    float vrms = (0.9741 * vmax) - 499.81;
    Serial.print("vrms = ");
    Serial.println(vrms);

    // Reset untuk siklus berikutnya
    vmax = 0;
    startMillis = millis();
  }
}

