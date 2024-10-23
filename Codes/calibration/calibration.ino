// Soil moisture sensor pin (Analog pin)
const int moistureSensorPin = 32;  // Use ADC pin 32 for soil moisture sensor

// Relay pin for watering valve (Digital output pin)
const int relayPin = 5;  // GPIO pin 5 for controlling the watering relay

// Moisture thresholds for calibration based on your sensor readings
const int moistureLowThreshold = 3000;  // Start watering below this value
const int moistureHighThreshold = 2000; // Stop watering above this value

// Watering control variables
bool isWatering = false;
unsigned long wateringStartTime = 0;
const unsigned long maxWateringTime = 7000;  // Max watering time in milliseconds (7 seconds)

void setup() {
  Serial.begin(115200);  // Start the serial communication

  // Initialize pins
  pinMode(moistureSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // Ensure relay is off initially (assuming active-low relay)
}

void loop() {
  // Read moisture sensor value
  int moistureValue = analogRead(moistureSensorPin);
  Serial.print("Soil Moisture Value: ");
  Serial.println(moistureValue);

  // Control watering based on moisture levels
  controlWatering(moistureValue);

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
