#ifndef WATER_LOGIC_H
#define WATER_LOGIC_H

#include <Arduino.h>
#include "WeightSensor.h"

enum TrackingState {
    STATE_SETUP, // [NEW] Waiting for user to register bottle
    STATE_IDLE,
    STATE_REMOVED,
    STATE_STABILIZING
};

class WaterLogic {
public:
    WaterLogic(WeightSensor* sensor);
    void begin();
    void update();
    void setFullBottle(); 
    void refillBottle(); // [NEW] Manual Refill trigger 
    
    // Getters
    float getDailyIntake();
    float getDailyGoal();
    float getCurrentWeight(); // [NEW] Result of last read
    float getBottleCapacity(); // [NEW] What we calibrated as "Full"
    
    void setDailyGoal(float goal);
    String getStateName();

private:
    WeightSensor* _sensor;
    TrackingState _currentState;
    
    float _weightBeforeLift;
    float _currentWeight;
    float _dailyIntake;
    float _dailyGoal;
    float _bottleFullWeight; // [NEW]
    
    unsigned long _stabilizeStartTime;
    const unsigned long STABILIZATION_DELAY = 2000; // 2 seconds
    const float LIFT_THRESHOLD = 50.0; // Grams. If weight drops by this or result is below this, consider lifted.
                                       // Better logic: if weight < (last_stable - threshold)
    
    void changeState(TrackingState newState);
    void handleIdle();
    void handleRemoved();
    void handleStabilizing();
};

#endif
