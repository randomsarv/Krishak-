// this version also includes the provision for the esp32 to sleep for 30 mintues as to prevent heating or other power related issues 

#include <WiFi.h>
#include <HTTPClient.h>
#include <ThingSpeak.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Wi-Fi credentials
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// ThingSpeak settings
const char* myChannelNumber = "YOUR_CHANNEL_ID"; 
const char* myWriteAPIKey = "YOUR_API_KEY";

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
  
  // Log the data to ThingSpeak (optional)
  sendDataToThingSpeak(moistureValue);

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
