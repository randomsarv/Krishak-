#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wi-Fi credentials
const char* ssid = "Sarvshrest's iPhone";      // Replace with your Wi-Fi SSID
const char* password = "yellow12";  // Replace with your Wi-Fi password

// Google Sheets script URL
const String API_KEY = "sWs3PQl051D7WtKBYSzpdQV591YZEErV";
const String DEVICE_ID = "device123";
const char* googleScriptUrl = "https://script.google.com/macros/s/AKfycbzoO_SOCkgTWcRVDM7_ThDG_eycGDlhuo1HPiPf3dfIbadwagZb8D8ltpmMWCrAXpwH7g/exec";

// NTP Client for timekeeping
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800); // Adjust the time offset for your timezone (19800 = IST)

// DHT sensor setup
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Soil moisture sensor pin
const int moistureSensorPin = 32;

// Setup function
void setup() {
    Serial.begin(115200);
    dht.begin();

    // Connect to Wi-Fi
    connectToWiFi();

    // Initialize NTP Client
    timeClient.begin();
    timeClient.update(); // Make an initial NTP request to sync time
}

// Loop function
void loop() {
    // Update NTP time
    timeClient.update();

    // Read sensor values
    int moistureValue = analogRead(moistureSensorPin);
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Get current time as a string
    String currentTime = timeClient.getFormattedTime();

    // Check if DHT readings are valid
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
    } else {
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print(" %\t");
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" *C");
    }

    // Print the current time
    Serial.print("Current Time: ");
    Serial.println(currentTime);

    // Send data to Google Sheets
    sendDataToGoogleScript(moistureValue, temperature, humidity, currentTime);

    // Wait 10 seconds before sending data again
    delay(10000);
}

// Connect to Wi-Fi
void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi.");
}

// Send data to Google Sheets
void sendDataToGoogleScript(int moistureValue, float temperature, float humidity, String currentTime) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(googleScriptUrl) + "?key=" + API_KEY + "&id=" + DEVICE_ID +
                     "&moisture=" + String(moistureValue) +
                     "&temperature=" + String(temperature) +
                     "&humidity=" + String(humidity) +
                     "&time=" + currentTime; // Include the timestamp

        http.begin(url);

        int httpResponseCode = http.GET();
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.print("Google Sheets response: ");
            Serial.println(response);
        } else {
            Serial.print("Error sending data to Google Sheets: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("Wi-Fi not connected. Retrying...");
        connectToWiFi();
    }
}
