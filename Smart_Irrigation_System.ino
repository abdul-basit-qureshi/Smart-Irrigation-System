// IoT-Based Smart Gardening System with Rain Sensor Integration

#define BLYNK_TEMPLATE_ID "TMPL6exV2DA4_"
#define BLYNK_TEMPLATE_NAME "Smart Gardening"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#define BLYNK_PRINT Serial // Enables Serial Monitor for Debugging

// WiFi and Blynk Credentials
char auth[] = "GpxGyn2u8d-9idQqLA7G4ckEAeUOoxU4";   // Blynk Auth Token
char ssid[] = "SSID";               // WiFi SSID
char pass[] = "WIFI_PASSWORD";            // WiFi Password

// OpenWeatherMap Credentials
const char* apiKey = "b2e12d427e644195fabee196a1d1dd92";  // OpenWeatherMap API Key
const char* cityID = "1176615";                 // City ID for Weather Data

// Sensor and Component Pins
#define RELAY_PIN 26        // Motor Relay
#define SOIL_SENSOR 34      // Soil Moisture Sensor (Analog Input Pin)
#define DHTPIN 27           // DHT11 Temperature and Humidity Sensor Pin
#define DHTTYPE DHT11       // DHT Sensor Type
#define RAIN_SENSOR_PIN 25  // Rain Sensor Digital Output Pin

// Global Variables
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
float temp, humidity, soilMoisture;
float owmTemp, owmHumidity;
int relayState = HIGH;  // Relay State (Motor Control)
bool manualControl = false;  // Manual Motor Control Flag
bool rainDetected = false;   // Rain Detection Flag

// Function Prototypes
void fetchWeatherData();
void readSensors();
void controlMotor();
void checkRainSensor();
void printAllData();
BLYNK_WRITE(V12); // Motor Control from Blynk App

void setup() {
  Serial.begin(115200); // Initialize Serial Monitor
  Blynk.begin(auth, ssid, pass);
  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  digitalWrite(RELAY_PIN, relayState);

  // Set interval to 1 minute (60000ms)
  timer.setInterval(60000L, readSensors);      // Read sensors every 1 minute
  timer.setInterval(60000L, fetchWeatherData); // Fetch weather data every 1 minute
  timer.setInterval(60000L, controlMotor);     // Check motor control every 1 minute
  timer.setInterval(60000L, checkRainSensor);  // Check rain sensor every 1 minute

  Serial.println("Smart Gardening System Initialized...");
}

void loop() {
  Blynk.run();
  timer.run();

  // Print Data Periodically (every 1 minute)
  static unsigned long lastPrintTime = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - lastPrintTime > 60000) { // Print every 1 minute
    printAllData();
    lastPrintTime = currentMillis;
  }
}

// Function to Read Sensors
void readSensors() {
  // Simulate DHT11 Data (Randomly between 12-13 °C)
  temp = random(120, 131) / 10.0;  // Random temperature between 12.0°C to 13.0°C
  humidity = random(40, 60);       // Random humidity between 40% to 60%

  // Read Soil Moisture Sensor
  soilMoisture = analogRead(SOIL_SENSOR);
  soilMoisture = map(soilMoisture, 0, 4095, 0, 100); // Adjusted for ESP32 12-bit ADC
  soilMoisture = (100 - soilMoisture);  // Invert for Consistency

  // Send Data to Blynk
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V3, soilMoisture);
}

// Function to Check Rain Sensor
void checkRainSensor() {
  rainDetected = digitalRead(RAIN_SENSOR_PIN) == LOW; // LOW means rain detected
  Blynk.virtualWrite(V7, rainDetected); // Send rain status to Blynk
}

// Function to Fetch Weather Data from OpenWeatherMap
void fetchWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?id=" + String(cityID) + "&appid=" + String(apiKey) + "&units=metric";

    http.begin(serverPath);

    int httpResponseCode = http.GET();  // Send HTTP GET request

    if (httpResponseCode > 0) {
      String response = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);

      owmTemp = doc["main"]["temp"];
      owmHumidity = doc["main"]["humidity"];

      Blynk.virtualWrite(V5, owmTemp);
      Blynk.virtualWrite(V6, owmHumidity);
    } else {
      Serial.print("Error Fetching Weather Data: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected! Check Network.");
  }
}

// Function to Print All Data to Serial Monitor
void printAllData() {
  Serial.println("=== Smart Gardening System Data ===");

  // Sensor Temperature and Humidity
  Serial.print("Temperature (Sensor): "); Serial.print(temp); Serial.println(" *C");
  Serial.print("Humidity (Sensor): "); Serial.print(humidity); Serial.println(" %");

  // OpenWeatherMap Temperature and Humidity
  Serial.print("Temperature (OpenWeatherMap): "); Serial.print(owmTemp); Serial.println(" *C");
  Serial.print("Humidity (OpenWeatherMap): "); Serial.print(owmHumidity); Serial.println(" %");

  // Soil Moisture
  Serial.print("Soil Moisture: "); Serial.print(soilMoisture); Serial.println(" %");

  // Rain Status
  Serial.print("Rain Status: ");
  if (rainDetected) {
    Serial.println("Rain Detected");
  } else {
    Serial.println("No Rain");
  }

  Serial.println("====================================");
}

// Function to Control Motor Based on Conditions
void controlMotor() {
  if (manualControl) {
    digitalWrite(RELAY_PIN, relayState); // Maintain Manual Control
    return;
  }

  bool conditionMet = false;

  if (!rainDetected) { // Only control motor if no rain
    if (temp > 30 && humidity < 40 && soilMoisture < 40) conditionMet = true;
    if (temp > 30 && humidity > 40 && soilMoisture < 30) conditionMet = true;
    if (temp >= 20 && temp <= 30 && humidity < 40 && soilMoisture < 35) conditionMet = true;
    if (temp >= 20 && temp <= 30 && humidity > 40 && soilMoisture < 30) conditionMet = true;
    if (temp < 20 && humidity < 40 && soilMoisture < 30) conditionMet = true;
  }

  if (conditionMet) {
    relayState = HIGH;
    Serial.println("Motor ON: Conditions Met");
  } else {
    relayState = LOW;
    Serial.println("Motor OFF: Conditions Not Met");
  }

  digitalWrite(RELAY_PIN, relayState);
  Blynk.virtualWrite(V12, relayState); // Update Motor State on Blynk App
}

// Function to Handle Motor Button on Blynk
BLYNK_WRITE(V12) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    manualControl = true;
    relayState = LOW;
    Serial.println("Motor ON: Manual Control");
  } else {
    manualControl = false;
    relayState = HIGH;
    Serial.println("Motor OFF: Manual Control");
  }
  digitalWrite(RELAY_PIN, relayState);
}