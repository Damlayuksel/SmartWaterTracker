#ifndef WEIGHT_SENSOR_H
#define WEIGHT_SENSOR_H

#include <Arduino.h>
#include <HX711.h>
#include <Preferences.h>

class WeightSensor {
public:
    WeightSensor(int doutPin, int sckPin);
    void begin();
    void calibrate(float scaleFactor); // Manual set
    void calibrateWithKnownWeight(float knownWeight); // Interactive
    void tare();
    float getWeight(byte readings = 5);
    bool isDetected();

    float getCalibrationFactor(); // getter

private:
    HX711 scale;
    Preferences prefs;
    int _doutPin;
    int _sckPin;
    float _currentFactor;
};

#endif
