#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <DNSServer.h>

// Wi-Fi credentials
Preferences preferences;
const char* defaultSSID = "a";
const char* defaultPassword = "a";
const char* apSSID = "InnoVatika";
const char* apPassword = "12345678";

// Google Sheets script URL
const String API_KEY = "13ccf99210e38d8a3aa6710f";
const String API_KEY_g = "sWs3PQl051D7WtKBYSzpdQV591YZEErV";
const String DEVICE_ID = "device_1";
const char* googleScriptUrl = "https://script.google.com/macros/s/AKfycbzoO_SOCkgTWcRVDM7_ThDG_eycGDlhuo1HPiPf3dfIbadwagZb8D8ltpmMWCrAXpwH7g/exec";
const char* apiURL = "https://us-central1-grooth-1.cloudfunctions.net/api/api";

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

// DNS Server for captive portal
const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer webServer(80);

// AP mode flag
bool apMode = false;

void setup() {
  Serial.begin(115200);
  pinMode(moistureSensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);
  delay(1000);
  digitalWrite(buzzerPin, LOW);

  // Initialize DHT sensor
  dht.begin();

  // Initialize Preferences
  preferences.begin("WiFiCreds", false);

  // Load Wi-Fi credentials
  String savedSSID = preferences.getString("ssid", defaultSSID);
  String savedPassword = preferences.getString("password", defaultPassword);

  // Try to connect to Wi-Fi
  if (!connectToWiFi(savedSSID.c_str(), savedPassword.c_str())) {
    // If failed, start AP mode
    startAPMode();
  } else {
    // Initialize NTP Client if connected
    timeClient.begin();
    timeClient.setTimeOffset(19800); // GMT+5:30 for IST
  }
}

void loop() {
  if (apMode) {
    // Handle DNS and web server in AP mode
    dnsServer.processNextRequest();
    webServer.handleClient();
  } else if (WiFi.status() == WL_CONNECTED) {
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
      Serial.print(moistureValue);
      Serial.println("");

      // Log data to Google Sheets
      sendDataToGoogleScript(moistureValue, temperature, humidity);
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }
    delay(10000); // 10 seconds between readings
  } else {
    // If WiFi disconnected, try to reconnect or go to AP mode
    String savedSSID = preferences.getString("ssid", defaultSSID);
    String savedPassword = preferences.getString("password", defaultPassword);
    
    if (!connectToWiFi(savedSSID.c_str(), savedPassword.c_str())) {
      startAPMode();
    }
  }
}

bool connectToWiFi(const char* ssid, const char* password) {
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 5) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi.");
    triggerBuzzer(2);
    return true;
  } else {
    Serial.println("\nFailed to connect to Wi-Fi.");
    return false;
  }
}

// Function to get signal strength description
String getSignalStrength(int rssi) {
  if (rssi > -50) return "Excellent";
  if (rssi > -60) return "Good";
  if (rssi > -70) return "Fair";
  if (rssi > -80) return "Weak";
  return "Poor";
}

void startAPMode() {
  Serial.println("Starting AP Mode");
  apMode = true;
  
  // Set up Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // Configure DNS server to redirect all domains to the ESP's IP
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  
  // Set up web server routes
  webServer.on("/", handleRoot);
  webServer.on("/scan", handleScan);
  webServer.on("/connect", handleConnect);
  webServer.onNotFound(handleRoot); // Captive portal
  
  // Start web server
  webServer.begin();
  
  // Notify user with buzzer
  triggerBuzzer(3);
}

void handleRoot() {
  // Perform WiFi scan for initial load
  int numNetworks = WiFi.scanNetworks();
  
  String html = "<html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }";
  html += "h1 { color: #0066cc; text-align: center; margin-bottom: 20px; }";
  html += ".container { max-width: 500px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += ".btn { background-color: #4CAF50; color: white; padding: 12px 15px; border: none; border-radius: 4px; cursor: pointer; width: 100%; font-size: 16px; margin: 10px 0; }";
  html += ".btn:hover { background-color: #45a049; }";
  html += "select, input { padding: 12px; margin: 10px 0; width: 100%; box-sizing: border-box; border: 1px solid #ddd; border-radius: 4px; font-size: 16px; }";
  html += ".loader { border: 4px solid #f3f3f3; border-top: 4px solid #3498db; border-radius: 50%; width: 30px; height: 30px; animation: spin 2s linear infinite; margin: 20px auto; display: none; }";
  html += "@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }";
  html += ".info { background-color: #e7f3fe; border-left: 6px solid #2196F3; padding: 10px; margin: 15px 0; }";
  html += ".status { text-align: center; margin: 15px 0; font-weight: bold; }";
  html += ".signal-excellent { color: #32CD32; }"; // Light green
  html += ".signal-good { color: #7FFF00; }";      // Green yellow
  html += ".signal-fair { color: #FFD700; }";      // Gold
  html += ".signal-weak { color: #FFA500; }";      // Orange
  html += ".signal-poor { color: #FF4500; }";      // Red orange
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>InnoVatika WiFi Setup</h1>";
  
  html += "<div class='info'>Please select your WiFi network and enter the password to connect your device.</div>";
  
  html += "<div class='status' id='scanStatus'>";
  if (numNetworks == 0) {
    html += "No networks found. Please scan again.";
  } else {
    html += numNetworks + " networks found:";
  }
  html += "</div>";
  
  html += "<div class='loader' id='scanLoader'></div>";
  
  html += "<form id='wifiForm' action='/connect' method='POST'>";
  
  // Create dropdown with networks
  html += "<select id='networkSelect' name='ssid'>";
  html += "<option value=''>Select WiFi Network</option>";
  
  // Add networks to dropdown
  if (numNetworks > 0) {
    for (int i = 0; i < numNetworks; i++) {
      String strength = getSignalStrength(WiFi.RSSI(i));
      String secureStatus = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      String signalClass = "";
      
      // Set signal strength class
      if (strength == "Excellent") signalClass = "signal-excellent";
      else if (strength == "Good") signalClass = "signal-good";
      else if (strength == "Fair") signalClass = "signal-fair";
      else if (strength == "Weak") signalClass = "signal-weak";
      else signalClass = "signal-poor";
      
      html += "<option value='" + WiFi.SSID(i) + "'>" +
              WiFi.SSID(i) + " - <span class='" + signalClass + "'>" + strength + 
              "</span> (" + WiFi.RSSI(i) + "dBm) - " + secureStatus + "</option>";
    }
  }
  html += "</select>";
  
  html += "<div id='manualInput' style='display:none;'>";
  html += "<input type='text' id='ssidInput' name='ssid' placeholder='WiFi Name'>";
  html += "</div>";
  
  // html += "<label><input type='checkbox' id='manualCheckbox' onclick='toggleManualInput()'> Enter network name manually</label>";
  html += "<input type='password' name='password' placeholder='WiFi Password'>";
  html += "<input type='submit' class='btn' value='Connect'>";
  html += "</form>";
  
  html += "<button class='btn' onclick='scanNetworks()' style='background-color: #2196F3;'>Rescan Networks</button>";
  html += "</div>"; // Close container
  
  html += "<script>";
  // Function to toggle between dropdown and manual input
  // html += "function toggleManualInput() {";
  // html += "  var manualCheck = document.getElementById('manualCheckbox');";
  // html += "  var manualInput = document.getElementById('manualInput');";
  // html += "  var networkSelect = document.getElementById('networkSelect');";
  // html += "  var ssidInput = document.getElementById('ssidInput');";
  // html += "  if (manualCheck.checked) {";
  // html += "    manualInput.style.display = 'block';";
  // html += "    networkSelect.style.display = 'none';";
  // html += "    networkSelect.name = '';";
  // html += "    ssidInput.name = 'ssid';";
  // html += "  } else {";
  // html += "    manualInput.style.display = 'none';";
  // html += "    networkSelect.style.display = 'block';";
  // html += "    networkSelect.name = 'ssid';";
  // html += "    ssidInput.name = '';";
  // html += "  }";
  // html += "}";
  
  // Function to scan networks
  html += "function scanNetworks() {";
  html += "  document.getElementById('scanStatus').innerHTML = 'Scanning for networks...';";
  html += "  document.getElementById('scanLoader').style.display = 'block';";
  html += "  document.getElementById('networkSelect').style.display = 'none';";
  html += "  fetch('/scan').then(response => response.text()).then(data => {";
  html += "    var selectEl = document.getElementById('networkSelect');";
  html += "    selectEl.innerHTML = data;";
  html += "    selectEl.style.display = 'block';";
  html += "    document.getElementById('scanLoader').style.display = 'none';";
  html += "    var networkCount = selectEl.options.length - 1;";
  html += "    document.getElementById('scanStatus').innerHTML = networkCount + ' networks found:';";
  html += "  });";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

void handleScan() {
  Serial.println("Scanning WiFi networks...");
  
  int n = WiFi.scanNetworks();
  String options = "<option value=''>Select WiFi Network</option>";
  
  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      String strength = getSignalStrength(WiFi.RSSI(i));
      String secureStatus = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      String signalClass = "";
      
      // Set signal strength class
      if (strength == "Excellent") signalClass = "signal-excellent";
      else if (strength == "Good") signalClass = "signal-good";
      else if (strength == "Fair") signalClass = "signal-fair";
      else if (strength == "Weak") signalClass = "signal-weak";
      else signalClass = "signal-poor";
      
      options += "<option value='" + WiFi.SSID(i) + "'>" +
                WiFi.SSID(i) + " - <span class='" + signalClass + "'>" + strength + 
                "</span> (" + WiFi.RSSI(i) + "dBm) - " + secureStatus + "</option>";
    }
  }
  
  webServer.send(200, "text/html", options);
}

void handleConnect() {
  String ssid = webServer.arg("ssid");
  String password = webServer.arg("password");
  
  if (ssid.length() > 0) {
    // Save credentials to preferences
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    
    String html = "<html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }";
    html += "h1 { color: #0066cc; text-align: center; margin-bottom: 20px; }";
    html += ".container { max-width: 500px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += ".progress { height: 20px; background-color: #f3f3f3; border-radius: 10px; margin: 20px 0; position: relative; }";
    html += ".progress-bar { background-color: #4CAF50; height: 100%; border-radius: 10px; animation: progress 10s linear; }";
    html += "@keyframes progress { 0% { width: 0%; } 100% { width: 100%; } }";
    html += "</style>";
    html += "<meta http-equiv='refresh' content='10;url=/'>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>Connecting to WiFi</h1>";
    html += "<p>Attempting to connect to <strong>" + ssid + "</strong>...</p>";
    html += "<div class='progress'><div class='progress-bar'></div></div>";
    html += "<p>The device will restart in normal mode if connection is successful.</p>";
    html += "<p>If connection fails, you'll be redirected back to the setup page in 10 seconds.</p>";
    html += "</div>";
    html += "</body></html>";
    webServer.send(200, "text/html", html);
    
    // Try to connect with new credentials
    Serial.println("Trying to connect with new credentials");
    delay(1000);
    
    if (connectToWiFi(ssid.c_str(), password.c_str())) {
      // If connected successfully, exit AP mode
      apMode = false;
      WiFi.mode(WIFI_STA);
      
      // Initialize NTP Client
      timeClient.begin();
      timeClient.setTimeOffset(19800); // GMT+5:30 for IST
      
      // Stop DNS and web server
      dnsServer.stop();
      webServer.stop();
    }
  } else {
    // Redirect back to config page if SSID is empty
    webServer.sendHeader("Location", "/", true);
    webServer.send(302, "text/plain", "");
  }
}

void triggerBuzzer(int n) {
  for(int i=0; i<n; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    delay(500);
  }
}

void sendDataToGoogleScript(int moistureValue, float temperature, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(apiURL) + 
                 "?apiKey=" + API_KEY + 
                 "&deviceId=" + DEVICE_ID + 
                 "&moisture=" + String(moistureValue) + 
                 "&temperature=" + String(temperature) + 
                 "&humidity=" + String(humidity);
    // Serial.println(url);
    http.begin(url);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      // Serial.print("Google Script response: ");
      Serial.println(http.getString());
    } else {
      // Serial.print("Error sending data to Google Sheets: ");
      Serial.println(httpResponseCode);
    }
    http.end();


    String url2 = String(googleScriptUrl) + 
                 "?apiKey=" + API_KEY_g + 
                 "&deviceId=" + DEVICE_ID + 
                 "&moisture=" + String(moistureValue) + 
                 "&temperature=" + String(temperature) + 
                 "&humidity=" + String(humidity) +
                 "&isFetch=false";
    // Serial.println(url);
    http.begin(url2);

    int httpResponseCode2 = http.GET();
    if (httpResponseCode2 > 0) {
      // Serial.print("Google Script response: ");
      Serial.println(http.getString());
    } else {
      // Serial.print("Error sending data to Google Sheets: ");
      Serial.println(httpResponseCode2);
    }
    http.end();
  } else {
    Serial.println("Wi-Fi not connected");
  }
}