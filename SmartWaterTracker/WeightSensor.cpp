#include "WeightSensor.h"

WeightSensor::WeightSensor(int doutPin, int sckPin) {
    _doutPin = doutPin;
    _sckPin = sckPin;
}

void WeightSensor::begin() {
    scale.begin(_doutPin, _sckPin);
    
    // Load calibration from NVS
    prefs.begin("water_tracker", false); // Namespace
    _currentFactor = prefs.getFloat("cal_factor", 420.0); // Default 420.0
    
    Serial.print("Loaded Calibration Factor: ");
    Serial.println(_currentFactor);
    
    scale.set_scale(_currentFactor);
    prefs.end();
}

void WeightSensor::calibrate(float scaleFactor) {
    _currentFactor = scaleFactor;
    scale.set_scale(_currentFactor);
}

void WeightSensor::calibrateWithKnownWeight(float knownWeight) {
    // 1. Get raw reading (minus tare offset, so we need to valid TARE first!)
    // Assuming user just Tared empty scale.
    Serial.println("Measuring known weight...");
    
    // Get raw value
    double raw = scale.get_value(20); 
    
    // Calc factor: raw = weight * factor  --> factor = raw / weight
    // HX711 library logic: get_units = (raw - offset) / scale
    // So scale = (raw - offset) / known_weight
    // scale.get_value() returns (raw - offset) already if tared.
    
    if (knownWeight != 0) {
        _currentFactor = raw / knownWeight;
    }
    
    scale.set_scale(_currentFactor);
    
    // Save to NVS
    prefs.begin("water_tracker", false);
    prefs.putFloat("cal_factor", _currentFactor);
    prefs.end();
    
    Serial.print("New Factor Calculated & Saved: ");
    Serial.println(_currentFactor);
}

// Getter
float WeightSensor::getCalibrationFactor() {
    return _currentFactor;
}

void WeightSensor::tare() {
    Serial.print("Taring...");
    if (scale.wait_ready_timeout(2000)) {
        scale.tare();
        Serial.println(" OK.");
    } else {
        Serial.println(" FAILED. Sensor not ready (Check Wiring).");
    }
}

float WeightSensor::getWeight(byte readings) {
    // Returns weight in units defined by calibration (usually grams or kg)
    // Wait up to 100ms for sensor to be ready, otherwise return error.
    if (scale.wait_ready_timeout(100)) {
        return scale.get_units(readings);
    }
    // Try one more time with longer timeout if failing?
    // Actually, if update() calls this loop, we don't want to block too long.
    // But setFullBottle calls this from an event.
    return -999.0; // Error value
}

bool WeightSensor::isDetected() {
    return scale.wait_ready_timeout(100);
}
