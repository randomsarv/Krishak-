#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>

// DNS Server for captive portal
DNSServer dnsServer;
WebServer webServer(80);
Preferences preferences;

// Constants
const byte DNS_PORT = 53;
const char* AP_SSID = "ESP32_Setup";
const char* AP_PASSWORD = "12345678";  // Optional: you can remove this for an open network
const char* PREF_NAMESPACE = "wifi_creds";

// Variables to store WiFi credentials
String storedSSID = "";
String storedPassword = "";

// Function prototypes
void setupAP();
void handleRoot();
void handleWiFiSave();
void handleNotFound();
String getWiFiScanHTML();
void saveWiFiCredentials(const char* ssid, const char* password);
bool loadWiFiCredentials();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  
  // Try to load credentials and connect
  if (loadWiFiCredentials()) {
    WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
  }
  
  // If not connected, start AP mode
  if (WiFi.status() != WL_CONNECTED) {
    setupAP();
  } else {
    Serial.println("\nConnected to WiFi");
    Serial.println("IP: " + WiFi.localIP().toString());
  }
}

void loop() {
  // Handle DNS and Web Server if in AP mode
  if (WiFi.status() != WL_CONNECTED) {
    dnsServer.processNextRequest();
    webServer.handleClient();
  }
}

void setupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);  // Remove AP_PASSWORD for open network
  
  // Configure DNS Server
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  
  // Configure Web Server
  webServer.on("/", handleRoot);
  webServer.on("/save", handleWiFiSave);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
  
  Serial.println("AP Mode Started");
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
}

void handleRoot() {
  webServer.send(200, "text/html", getWiFiScanHTML());
}

void handleWiFiSave() {
  String ssid = webServer.arg("ssid");
  String password = webServer.arg("password");
  
  if (ssid.length() > 0) {
    saveWiFiCredentials(ssid.c_str(), password.c_str());
    
    String response = "<!DOCTYPE html><html><body>";
    response += "<h1>Credentials Saved</h1>";
    response += "<p>The ESP32 will now restart and attempt to connect to the WiFi network.</p>";
    response += "</body></html>";
    
    webServer.send(200, "text/html", response);
    delay(2000);
    ESP.restart();
  } else {
    webServer.send(400, "text/plain", "Invalid SSID");
  }
}

void handleNotFound() {
  // Redirect all requests to the captive portal
  webServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  webServer.send(302, "text/plain", "");
}

String getWiFiScanHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial; margin: 20px; }";
  html += "select, input { width: 100%; padding: 8px; margin: 8px 0; }";
  html += "button { background-color: #4CAF50; color: white; padding: 10px; border: none; border-radius: 4px; cursor: pointer; width: 100%; }";
  html += "</style></head><body>";
  html += "<h1>ESP32 WiFi Setup</h1>";
  html += "<form action='/save' method='POST'>";
  html += "<label>Select Network:</label>";
  html += "<select name='ssid' id='ssid'>";
  
  // Scan for networks
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + "dBm)</option>";
  }
  
  html += "</select><br>";
  html += "<label>Password:</label><br>";
  html += "<input type='password' name='password'><br><br>";
  html += "<button type='submit'>Connect</button>";
  html += "</form></body></html>";
  
  return html;
}

void saveWiFiCredentials(const char* ssid, const char* password) {
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}

bool loadWiFiCredentials() {
  preferences.begin(PREF_NAMESPACE, true);
  storedSSID = preferences.getString("ssid", "");
  storedPassword = preferences.getString("password", "");
  preferences.end();
  
  return storedSSID.length() > 0;
}