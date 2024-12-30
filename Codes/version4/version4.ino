#include <WiFi.h>
#include <HTTPClient.h>
#include <ThingSpeak.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Wi-Fi credentials
const char* ssid = "Dlink";
const char* password = "Cidi@1234";


// Google Sheets script URL
const String API_KEY = "sWs3PQl051D7WtKBYSzpdQV591YZEErV";
const String DEVICE_ID = "device123";
const char* googleScriptUrl = "https://script.google.com/macros/s/AKfycbzoO_SOCkgTWcRVDM7_ThDG_eycGDlhuo1HPiPf3dfIbadwagZb8D8ltpmMWCrAXpwH7g/exec";

// Soil moisture sensor pin (Analog pin)
const int moistureSensorPin = 32;  // ADC pin for soil moisture sensor

// Relay pin for watering valve (Digital output pin)
const int relayPin = 5;  // GPIO pin for controlling the watering relay

// Moisture thresholds for calibration based on your sensor readings
const int moistureLowThreshold = 3000;  // Start watering below this value
const int moistureHighThreshold = 2000; // Stop watering above this value

// Watering control variables
const unsigned long maxWateringTime = 7000;  // Max watering time in milliseconds (7 seconds)
bool isWatering = false;

// Deep sleep interval (30 minutes = 1800 seconds)
const unsigned long sleepTimeInSeconds = 1800;

// NTP Client for timekeeping (optional)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(moistureSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // Ensure relay is off initially (active-low relay)

  // Connect to Wi-Fi
  connectToWiFi();

  // Initialize ThingSpeak (optional for logging data)
  ThingSpeak.begin(WiFi);
  
  // Initialize NTP Client (optional)
  timeClient.begin();
  timeClient.setTimeOffset(19800);  // Set time offset for IST (GMT+5:30)

  // Take first soil moisture reading and control watering
  int moistureValue = analogRead(moistureSensorPin);
  Serial.print("Soil Moisture Value: ");
  Serial.println(moistureValue);

  controlWatering(moistureValue);
  
  // Log the data to ThingSpeak
  sendDataToThingSpeak(moistureValue);

  // Log the data to Google Sheets
  sendDataToGoogleScript(moistureValue);

  // Go to deep sleep for the set interval
  Serial.println("Going to sleep now for 30 minutes...");
  esp_sleep_enable_timer_wakeup(sleepTimeInSeconds * 1000000);  // Convert seconds to microseconds
  esp_deep_sleep_start();
}

void loop() {
  // The code will not reach this point due to deep sleep
}

// Function to control watering based on moisture level
void controlWatering(int moistureValue) {
  if (moistureValue > moistureLowThreshold && !isWatering) {
    startWatering();
  } else if (moistureValue <= moistureHighThreshold && isWatering) {
    stopWatering();
  }
}

// Function to start watering
void startWatering() {
  isWatering = true;
  digitalWrite(relayPin, LOW);  // Turn on the relay to start watering (active-low relay)
  Serial.println("Watering started");

  delay(maxWateringTime);  // Run watering for the set amount of time (7 seconds)

  stopWatering();  // Automatically stop watering after the time elapses
}

// Function to stop watering
void stopWatering() {
  isWatering = false;
  digitalWrite(relayPin, HIGH);  // Turn off the relay (active-low relay)
  Serial.println("Watering stopped");
}

// Function to send data to ThingSpeak (optional)
void sendDataToThingSpeak(int moistureValue) {
  ThingSpeak.setField(1, moistureValue);
  int response = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (response == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
  } else {
    Serial.print("Problem sending data. HTTP error code: ");
    Serial.println(response);
  }
}

// Function to send data to Google Sheets through Google Script
void sendDataToGoogleScript(int moistureValue) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(googleScriptUrl) + "&moisture=" + String(moistureValue);
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

// Function to connect to Wi-Fi
void connectToWiFi() {
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
  } else {
    Serial.println("\nFailed to connect to Wi-Fi.");
  }
}