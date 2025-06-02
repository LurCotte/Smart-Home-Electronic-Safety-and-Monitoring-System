int ipuncak;
void setup() {
Serial.begin(9600);
}

void loop() {
ipuncak=analogRead(A1);
  Serial.println(ipuncak);
  delay(100);
}

