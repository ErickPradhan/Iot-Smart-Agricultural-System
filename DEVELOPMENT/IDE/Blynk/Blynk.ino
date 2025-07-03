#define BLYNK_TEMPLATE_ID "TMPL6ZlPcFjMh"
#define BLYNK_TEMPLATE_NAME "IOT based smart agriculture"
#define BLYNK_AUTH_TOKEN "fQiSqZbpdfCxzSQA-y_wo_FjgVK2bKqi"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD settings
#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_ROWS);

// DHT11
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Sensor pins
const int trigPin = 12;
const int echoPin = 13;
const int soilMoisturePin = 34;

// Output pins
const int buzzerPin = 14;
const int relayPin = 32;

// Soil moisture threshold
const int dryThreshold = 30;

// Wi-Fi credentials
char ssid[] = "Ankit's iphone";
char pass[] = "12345679";

BlynkTimer timer;

// Tank height in cm
const float tankHeight = 10.0;
bool waterAlertSent = false;
const int waterLevelThreshold = 20;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Smart Agri System...");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  digitalWrite(buzzerPin, LOW);
  digitalWrite(relayPin, HIGH); // Pump OFF

  dht.begin();
  Wire.begin(21, 22); // ESP32 I2C pins

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Agri Start");
  delay(2000); // Show welcome message
  lcd.clear();

  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");

  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20) {
    delay(500);
    Serial.print(".");
    wifi_attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Blynk.config(BLYNK_AUTH_TOKEN);
    if (Blynk.connect(10000)) {
      Serial.println("Connected to Blynk!");
    } else {
      Serial.println("Blynk connection failed. Continuing without it.");
    }
  } else {
    Serial.println("\nWiFi connection failed. Continuing without Blynk.");
  }

  timer.setInterval(2000L, sendSensorData);
}

void sendSensorData() {
  // Ultrasonic
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  float distanceCm = duration * 0.034 / 2.0;

  int waterLevelPercent = 0;
  if (distanceCm <= tankHeight && distanceCm > 0) {
    waterLevelPercent = map(distanceCm, tankHeight, 0, 0, 100);
    waterLevelPercent = constrain(waterLevelPercent, 0, 100);
  }

  if (waterLevelPercent < waterLevelThreshold && !waterAlertSent) {
    Blynk.logEvent("water_shortage", "Water level is low! Refill the tank.");
    waterAlertSent = true;
  } else if (waterLevelPercent >= waterLevelThreshold) {
    waterAlertSent = false;
  }

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DHT Error");
    return;
  }

  int soilValue = analogRead(soilMoisturePin);
  int soilMoisturePercent = map(soilValue, 4095, 1500, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  if (soilMoisturePercent < dryThreshold) {
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(relayPin, LOW); // Pump ON
  } else {
    digitalWrite(buzzerPin, LOW);
    digitalWrite(relayPin, HIGH); // Pump OFF
  }

  Blynk.virtualWrite(V0, waterLevelPercent);
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);
  Blynk.virtualWrite(V3, soilMoisturePercent);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C H:");
  lcd.print(humidity, 0);

  lcd.setCursor(0, 1);
  lcd.print("S:");
  lcd.print(soilMoisturePercent);
  lcd.print("% W:");
  lcd.print(waterLevelPercent);
  lcd.print("%");

  // Serial Debug
  Serial.print("Water Level: ");
  Serial.print(waterLevelPercent);
  Serial.print("% | Temp: ");
  Serial.print(temperature);
  Serial.print("C | Hum: ");
  Serial.print(humidity);
  Serial.print("% | Soil: ");
  Serial.print(soilMoisturePercent);
  Serial.print("% | Relay: ");
  Serial.println(digitalRead(relayPin) == LOW ? "ON" : "OFF");
}

void loop() {
  Blynk.run();
  timer.run();
}