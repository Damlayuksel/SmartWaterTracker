#include "WeightSensor.h"
#include "WaterLogic.h"
#include "WebDashboard.h"

// --- PINS ---
// ESP32-C3 Mini Pinout
// User Wiring: DT=5, SCK=4
const int HX711_DOUT_PIN = 5;
const int HX711_SCK_PIN = 4;

// --- WIFI ---
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// --- GLOBALS ---
WeightSensor weightSensor(HX711_DOUT_PIN, HX711_SCK_PIN);
WaterLogic waterLogic(&weightSensor);
WebDashboard dashboard(&waterLogic);

bool isCalibrating = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Water Tracker...");

  // Initialize Sensor
  Serial.println("Initializing Sensor...");
  weightSensor.begin(); // Loads calibration from NVS automatically
  
  Serial.println("Taring Scale...");
  weightSensor.tare(); // Assume scale is empty at start

  // Initialize Logic
  Serial.println("Initializing Logic...");
  waterLogic.begin();

  // Initialize Dashboard
  dashboard.begin(WIFI_SSID, WIFI_PASS);

  Serial.println("System Ready.");
  Serial.println("Commands:");
  Serial.println("  't' -> Tare (Reset scale to 0)");
  Serial.println("  'f' -> Set Full Bottle (Kaydet)");
  Serial.println("  'stat' -> Show Status");
}

void loop() {
  // Update Logic
  waterLogic.update();
  dashboard.handle();

  // Handle Serial Commands for testing/setup
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // Check if we are in calibration mode expecting a number
    if (isCalibrating) {
        float weight = input.toFloat();
        if (weight > 0) {
            Serial.print("Calibrating with known weight: ");
            Serial.print(weight);
            Serial.println(" g");
            weightSensor.calibrateWithKnownWeight(weight);
            isCalibrating = false;
            Serial.println("Calibration Done. System Ready.");
        } else {
            Serial.println("Invalid weight. Please enter a positive number (e.g. 100).");
        }
        return;
    }

    // Normal Commands
    if (input == "t") {
        Serial.println("Taring scale...");
        weightSensor.tare();
        Serial.println("Done.");
    } else if (input == "f") {
        Serial.println("Recording Full Bottle...");
        waterLogic.setFullBottle();
    } else if (input == "r") {
        Serial.println("Manual Refill Triggered.");
        waterLogic.refillBottle();
    } else if (input == "s") {
         Serial.print("Daily Intake: ");
         Serial.println(waterLogic.getDailyIntake());
    } else if (input == "c") {
        Serial.println("ENTERING CALIBRATION MODE");
        Serial.println("1. Place a known weight on the scale (e.g. Your phone or a bottle).");
        Serial.println("2. Type the weight in GRAMS (e.g. 200) and press Enter.");
        isCalibrating = true;
    }
  }

  // Small delay to prevent CPU hogging, though update() needs to be fast enough
  delay(100); 
}
