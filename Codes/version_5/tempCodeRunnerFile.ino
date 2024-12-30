#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>

// Wi-Fi credentials storage
Preferences preferences;
const char* defaultSSID = "ESP32_Hotspot";
const char* defaultPassword = "12345678";

// Google Sheets script URL
const String API_KEY = "sWs3PQl051D7WtKBYSzpdQV591YZEErV";
const String DEVICE_ID = "device123";
const char* googleScriptUrl = "https://script.google.com/macros/s/AKfycbzoO_SOCkgTWcRVDM7_ThDG_eycGDlhuo1HPiPf3dfIbadwagZb8D8ltpmMWCrAXpwH7g/exec";

// Soil moisture sensor pin (Analog pin)
const int moistureSensorPin = 32;

// NTP Client for timekeeping (optional)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// DHT sensor
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Buzzer pin
const int buzzerPin = 15;

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  pinMode(moistureSensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // Initialize DHT sensor
  dht.begin();

  // Initialize Preferences
  preferences.begin("WiFiCreds", false);

  // Load stored Wi-Fi credentials
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");

  if (savedSSID.length() > 0 && savedPassword.length() > 0) {
    connectToWiFi(savedSSID.c_str(), savedPassword.c_str());
  } else {
    startAccessPoint();
  }

  // Initialize OTA
  setupOTA();

  // Initialize NTP Client (optional)
  timeClient.begin();
  timeClient.setTimeOffset(19800);
}

void loop() {
  ArduinoOTA.handle();

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

void startAccessPoint() {
  WiFi.softAP(defaultSSID, defaultPassword);
  Serial.println("Access Point Started");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<form action='/connect' method='POST'>"
                  "<label>SSID:</label><input type='text' name='ssid'><br>"
                  "<label>Password:</label><input type='password' name='password'><br>"
                  "<button type='submit'>Connect</button></form>";
    request->send(200, "text/html", html);
  });

  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);

    request->send(200, "text/plain", "Saved Wi-Fi credentials. Rebooting...");
    delay(2000);
    ESP.restart();
  });

  server.begin();
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
    startAccessPoint();
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
    String url = String(googleScriptUrl) + "&moisture=" + String(moistureValue) + "&temperature=" + String(temperature) + "&humidity=" + String(humidity);
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

void setupOTA() {
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}
