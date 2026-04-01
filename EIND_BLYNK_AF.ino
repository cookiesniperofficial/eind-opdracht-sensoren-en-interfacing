#define BLYNK_TEMPLATE_ID "TMPL56sJVSzYA"
#define BLYNK_TEMPLATE_NAME "esp32 monitoring and gyro"
#define BLYNK_AUTH_TOKEN "h8o706h4z2EAAE3769kfC7SeaBekrk-L"

#include <Wire.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

#define SOIL_PIN 1
#define SHOCK_PIN 2
#define BUZZER_PIN 3
#define LED_R 8
#define LED_G 9
#define LED_B 6
#define SDA_PIN 4
#define SCL_PIN 5

Adafruit_BMP280 bmp;

char ssid[] = "embed";
char pass[] = "weareincontrol";

bool bmp_ok = false;

// instelbare temperatuurgrens
float tempLimit = 35;

// Blynk slider op V20
BLYNK_WRITE(V20) {
  tempLimit = param.asFloat();
  Serial.print("Nieuwe temperatuurgrens ingesteld: ");
  Serial.println(tempLimit);
}

void setup() {
  Serial.begin(115200);

  pinMode(SOIL_PIN, INPUT);
  pinMode(SHOCK_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);

  // --- BMP280 zoeken (max 5 pogingen) ---
  int attempts = 0;
  while (!bmp_ok && attempts < 5) {
    Serial.print("Zoeken naar BMP280... poging ");
    Serial.println(attempts + 1);

    if (bmp.begin(0x76)) {
      bmp_ok = true;
      Serial.println("BMP280 gevonden op 0x76!");
    }
    else if (bmp.begin(0x77)) {
      bmp_ok = true;
      Serial.println("BMP280 gevonden op 0x77!");
    }
    else {
      Serial.println("BMP280 niet gevonden...");
      attempts++;
      delay(1000);
    }
  }

  if (!bmp_ok) {
    Serial.println(">>> SENSOR FOUT: BMP280 niet gevonden na 5 pogingen!");
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // stuur standaard temperatuurgrens naar Blynk
  Blynk.virtualWrite(V20, tempLimit);

  Serial.println("\n--- SYSTEM STARTED ---\n");
}

void loop() {
  Blynk.run();

  float temperature = bmp_ok ? bmp.readTemperature() : 0;
  float pressure = bmp_ok ? bmp.readPressure() / 100.0F : 0;

  int soilValue = analogRead(SOIL_PIN);
  int shockValue = digitalRead(SHOCK_PIN);

  Serial.println("====== SENSOR DATA ======");
  Serial.print("Temperatuur: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Luchtdruk:   ");
  Serial.print(pressure);
  Serial.println(" hPa");

  Serial.print("Soil waarde: ");
  Serial.println(soilValue);

  Serial.print("Shock:       ");
  Serial.println(shockValue == 1 ? "JA (ALARM)" : "nee");
  Serial.println("=========================\n");

  // Reset LED's
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  String ledStatus = "GROEN (OK)";
  digitalWrite(LED_G, HIGH);

  // temperatuur alarm via Blynk slider
  if (temperature > tempLimit) {
    Serial.println(">>> ALARM: Temperatuur te hoog!");
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, LOW);
    tone(BUZZER_PIN, 2500, 300);
    ledStatus = "ROOD (HOGE TEMPERATUUR)";
  }

  // Shock alarm (paars)
  if (shockValue == 1) {
    Serial.println(">>> ALARM: Shock gedetecteerd!");
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_B, HIGH);
    digitalWrite(LED_G, LOW);
    tone(BUZZER_PIN, 2000, 300);
    ledStatus = "PAARS (SHOCK)";
  }

  // Droge plant
  if (soilValue > 3000) {
    Serial.println(">>> WAARSCHUWING: Plant is droog!");
    digitalWrite(LED_B, HIGH);
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, LOW);
    tone(BUZZER_PIN, 1500, 200);
    ledStatus = "BLAUW (DROOG)";
  }

  // BMP280 foutstatus
  if (!bmp_ok) {
    Serial.println(">>> FOUT: BMP280 niet verbonden!");
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, LOW);
    ledStatus = "GEEL (SENSOR FOUT)";
  }

  // Blynk updates
  Blynk.virtualWrite(V4, soilValue);
  Blynk.virtualWrite(V5, temperature);
  Blynk.virtualWrite(V7, pressure);
  Blynk.virtualWrite(V8, shockValue);
  Blynk.virtualWrite(V10, ledStatus);

  delay(500);
}
