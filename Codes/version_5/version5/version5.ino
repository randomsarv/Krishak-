#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wi-Fi credentials
Preferences preferences;
const char* defaultSSID = "A";
const char* defaultPassword = "ayushgoswami7";

// Google Sheets script URL
const String API_KEY = "sWs3PQl051D7WtKBYSzpdQV591YZEErV";
const String DEVICE_ID = "device_1";
const char* googleScriptUrl = "https://script.google.com/macros/s/AKfycbzoO_SOCkgTWcRVDM7_ThDG_eycGDlhuo1HPiPf3dfIbadwagZb8D8ltpmMWCrAXpwH7g/exec";

// Soil moisture sensor pin
const int moistureSensorPin = 32;

// NTP Client for timekeeping
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// DHT sensor
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Buzzer pin
const int buzzerPin = 15;

void setup() {
  Serial.begin(115200);
  pinMode(moistureSensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // Initialize DHT sensor
  dht.begin();

  // Initialize Preferences
  preferences.begin("WiFiCreds", false);

  // Load Wi-Fi credentials
  String savedSSID = preferences.getString("ssid", defaultSSID);
  String savedPassword = preferences.getString("password", defaultPassword);

  // Connect to Wi-Fi
  connectToWiFi(savedSSID.c_str(), savedPassword.c_str());

  // Initialize NTP Client
  timeClient.begin();
  timeClient.setTimeOffset(19800); // GMT+5:30 for IST
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Take sensor readings
    int moistureValue = analogRead(moistureSensorPin);
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Check if DHT readings are valid
    if (!isnan(humidity) && !isnan(temperature)) {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" *C");

      // Log data to Google Sheets
      sendDataToGoogleScript(moistureValue, temperature, humidity);
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }
  }
}

void connectToWiFi(const char* ssid, const char* password) {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi.");
    triggerBuzzer();
  } else {
    Serial.println("\nFailed to connect to Wi-Fi.");
  }
}

void triggerBuzzer() {
  digitalWrite(buzzerPin, HIGH);
  delay(500);
  digitalWrite(buzzerPin, LOW);
}

void sendDataToGoogleScript(int moistureValue, float temperature, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(googleScriptUrl) + 
                 "?api_key=" + API_KEY + 
                 "&device_id=" + DEVICE_ID + 
                 "&moisture=" + String(moistureValue) + 
                 "&temperature=" + String(temperature) + 
                 "&humidity=" + String(humidity);
    http.begin(url);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.print("Google Script response: ");
      Serial.println(http.getString());
    } else {
      Serial.print("Error sending data to Google Sheets: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Wi-Fi not connected");
  }
}
