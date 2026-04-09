#include "WaterLogic.h"

WaterLogic::WaterLogic(WeightSensor* sensor) {
    _sensor = sensor;
    _currentState = STATE_IDLE;
    _dailyIntake = 0;
    _dailyGoal = 2000.0; // Default 2000ml
    _weightBeforeLift = 0;
    _bottleFullWeight = 500.0; // Default assumption until set
}

void WaterLogic::begin() {
    _sensor->begin();
    // User Requirement: Wait for manual setup.
    _currentState = STATE_SETUP;
    Serial.println("System Waiting for Bottle Setup (Press 'f' or use Web).");
}

void WaterLogic::update() {
    _currentWeight = _sensor->getWeight();

    switch (_currentState) {
        case STATE_SETUP:
            // Do nothing, wait for setFullBottle()
            break;
        case STATE_IDLE:
            handleIdle();
            break;
        case STATE_REMOVED:
            handleRemoved();
            break;
        case STATE_STABILIZING:
            handleStabilizing();
            break;
    }
}

void WaterLogic::refillBottle() {
    // User refilled the bottle.
    // We assume the bottle is now ON the scale and FULL (or partially full).
    // We restart the tracking from this new weight.
    float w = _sensor->getWeight(10);
    _weightBeforeLift = w;
    _bottleFullWeight = w; // Optional: update capacity too? User said "Refill button".
    _currentState = STATE_IDLE;
    Serial.print("Refilled! New Baseline: ");
    Serial.println(w);
}

void WaterLogic::changeState(TrackingState newState) {
    _currentState = newState;
    Serial.print("State Changed to: ");
    Serial.println(getStateName());
}

void WaterLogic::handleIdle() {
// Helper to detect stability
    // Ideally we want to prevent updating _weightBeforeLift if the weight is "crashing" down.
    
    if (_currentWeight < LIFT_THRESHOLD) {
        // Bottle Removed!
        Serial.print("Bottle Removed! Last Stable Weight: ");
        Serial.println(_weightBeforeLift);
        changeState(STATE_REMOVED);
    } else {
        // Only update _weightBeforeLift if the weight is STABLE, 
        // i.e., not dropping rapidly.
        // We compare with the previous loop's weight is tricky because of noise.
        // Better: logic -> If current weight is significantly LESS than _weightBeforeLift (e.g. > 20g difference)
        // AND it hasn't stabilized yet, don't update.
        // But simpler: If we are simply sitting there, update.
        // If we are LIFTING, the next sample will likely be < Threshold or significantly lower.
        
        // Let's us a simple "Decay" prevention.
        // If new weight > _weightBeforeLift - 5.0 (grams), then update.
        // If it drops more than 5g in one cycle, assume it's starting to lift and HOLD the old value.
        
        if (_currentWeight > (_weightBeforeLift - 5.0)) {
             _weightBeforeLift = _currentWeight;
        }
        // Else: It dropped > 5g since last loop. Hold the old value!
        // If it was just noise, it will come back up next loop.
        // If it is a lift, it will continue to drop to 0.
    }
}

void WaterLogic::handleRemoved() {
    // Waiting for bottle to be placed back.
    if (_currentWeight > LIFT_THRESHOLD) {
        // Bottle detected!
        Serial.println("Bottle Detected... Stabilizing...");
        _stabilizeStartTime = millis();
        changeState(STATE_STABILIZING);
    }
}

void WaterLogic::handleStabilizing() {
    // Wait for 2 seconds
    if (millis() - _stabilizeStartTime >= STABILIZATION_DELAY) {
        // Time is up.
        float weightAfter = _sensor->getWeight(10); // Take a good average
        Serial.print("Stabilized Weight: ");
        Serial.println(weightAfter);

        float consumed = _weightBeforeLift - weightAfter;
        
        Serial.print("Difference (Consumed): ");
        Serial.println(consumed);

        if (consumed > 10.0) { // arbitrary noise threshold e.g. 10g
            _dailyIntake += consumed;
            Serial.print("Intake Registered! Total Today: ");
            Serial.println(_dailyIntake);
        } else if (consumed < -10.0) {
            Serial.println("Refill detected or negative consumption ignored.");
             // Optional: Handle Refill?
             // If weightAfter > _weightBeforeLift, they probably refilled it.
        } else {
            Serial.println("Difference too small, ignored.");
        }

        changeState(STATE_IDLE);
    }
}

void WaterLogic::setFullBottle() {
    float w = _sensor->getWeight(10);
    // Retry if error
    if (w == -999.0) {
        Serial.println("Sensor busy, retrying...");
        delay(100);
        w = _sensor->getWeight(10);
    }
    
    if (w == -999.0) {
       Serial.println("Error: Could not read weight for Full Bottle setup.");
       return;
    }
    
    Serial.print("Full Bottle Calibrated: ");
    Serial.println(w);
    _bottleFullWeight = w; // Store as capacity
    _weightBeforeLift = w; // Initial weight
    _currentState = STATE_IDLE; // Start Tracking!
}

float WaterLogic::getDailyIntake() {
    return _dailyIntake;
}

float WaterLogic::getDailyGoal() {
    return _dailyGoal;
}

float WaterLogic::getCurrentWeight() {
    return _currentWeight;
}

float WaterLogic::getBottleCapacity() {
    return _bottleFullWeight;
}

void WaterLogic::setDailyGoal(float goal) {
    _dailyGoal = goal;
}

String WaterLogic::getStateName() {
    switch (_currentState) {
        case STATE_SETUP: return "SETUP_NEEDED";
        case STATE_IDLE: return "IDLE";
        case STATE_REMOVED: return "REMOVED";
        case STATE_STABILIZING: return "STABILIZING";
        default: return "UNKNOWN";
    }
}
