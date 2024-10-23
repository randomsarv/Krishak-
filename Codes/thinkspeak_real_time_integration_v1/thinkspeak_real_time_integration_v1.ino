#include <WiFi.h>
#include <ThingSpeak.h>

// Soil moisture sensor connected to GPIO32 (Analog Input)
#define SOIL_MOISTURE_PIN 32

const char* ssid = "Lappy"; // Your WiFi SSID
const char* password = "ayushgoswami7"; // Your WiFi Password

// ThingSpeak settings
unsigned long myChannelNumber = 2683244; // Replace with your channel number
const char * myWriteAPIKey = ""; // Replace with your Write API Key
WiFiClient client;

// Variables for sensor data
int soilMoistureValue = 0;

void setup() {
  Serial.begin(115200);
  pinMode(SOIL_MOISTURE_PIN, INPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  // Read soil moisture sensor value
  soilMoistureValue = analogRead(SOIL_MOISTURE_PIN); // Read analog value

  // Map the sensor value to a percentage (optional)
  int soilMoisturePercentage = map(soilMoistureValue, 0, 4095, 0, 100); // Adjust 4095 based on calibration
  
  // Debug: Print the raw value and percentage
  Serial.print("Soil Moisture Value (Raw): ");
  Serial.println(soilMoistureValue); 
  Serial.print("Soil Moisture Percentage: ");
  Serial.println(soilMoisturePercentage);

  // Send data to ThingSpeak
  int x = ThingSpeak.writeField(myChannelNumber, 1, soilMoisturePercentage, myWriteAPIKey);
  
  if (x == 200) {
    Serial.println("Data successfully sent to ThingSpeak");
  } else {
    Serial.println("Problem sending data. HTTP error code: " + String(x));
  }

  delay(15000); // ThingSpeak allows an update every 15 seconds
}
