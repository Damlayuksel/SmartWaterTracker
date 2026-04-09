#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>
#include <WebServer.h>
#include "WaterLogic.h"

class WebDashboard {
public:
    WebDashboard(WaterLogic* logic);
    void begin(const char* ssid, const char* password);
    void handle();

private:
    WebServer server;
    WaterLogic* _logic;

    void handleRoot();
    void handleStatus();
    void handleTare();
    
    // HTML Template
    String getHTML();
};

#endif
