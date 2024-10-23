#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ThingSpeak.h>

// Wi-Fi credentials
const char* ssid = "your_ssid";
const char* password = "your_password";

// ThingSpeak settings (optional, for logging data)
unsigned long myChannelNumber = 123456;  // Replace with your ThingSpeak Channel ID
const char* myWriteAPIKey = "your_write_api_key";  // Replace with your ThingSpeak Write API Key

// NTP Client settings
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);  // GMT+5:30 offset for IST

// Soil moisture sensor pin (Analog pin)
const int moistureSensorPin = 32;  // Use ADC pin 32 for soil moisture sensor

// Relay pin for watering valve (Digital output pin)
const int relayPin = 5;  // GPIO pin 5 for controlling the watering relay

// Moisture thresholds for calibration based on your sensor readings
const int moistureLowThreshold = 2000;  // Start watering above this value
const int moistureHighThreshold = 3000; // Stop watering below this value

// Watering control variables
bool isWatering = false;
unsigned long wateringStartTime = 0;
const unsigned long maxWateringTime = 7000;  // Max watering time in milliseconds (7 seconds)

// Time variables
unsigned long previousMillis = 0;
const long interval = 15000;  // Interval to check and send data (15 seconds)

WiFiClient client;  // For ThingSpeak

void setup() {
  Serial.begin(115200);  // Start the serial communication

  // Initialize pins
  pinMode(moistureSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // Ensure relay is off initially (assuming active-low relay)

  // Connect to Wi-Fi
  connectToWiFi();

  // Initialize NTP Client
  timeClient.begin();

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  // Update time
  timeClient.update();

  // Reconnect Wi-Fi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  // Read moisture sensor value
  int moistureValue = analogRead(moistureSensorPin);
  Serial.print("Soil Moisture Value: ");
  Serial.println(moistureValue);

  // Control watering based on moisture levels
  controlWatering(moistureValue);

  // Send data to ThingSpeak
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendDataToThingSpeak(moistureValue);
  }

  // Check watering timeout
  if (isWatering && (millis() - wateringStartTime > maxWateringTime)) {
    stopWatering();
    Serial.println("Watering Timeout");
  }

  delay(1000);  // Delay for 1 second to avoid flooding the serial output
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
  wateringStartTime = millis();
  digitalWrite(relayPin, LOW);  // Turn on the relay to start watering (assuming active-low relay)
  Serial.println("Watering started");
}

// Function to stop watering
void stopWatering() {
  isWatering = false;
  digitalWrite(relayPin, HIGH);  // Turn off the relay to stop watering (assuming active-low relay)
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
