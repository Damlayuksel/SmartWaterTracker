#include "WebDashboard.h"
#include <WiFi.h>

WebDashboard::WebDashboard(WaterLogic* logic) : server(80) {
    _logic = logic;
}

void WebDashboard::begin(const char* ssid, const char* password) {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("WiFi connected. IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi Failed! continuing offline...");
    }

    // Routes
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/refill", HTTP_GET, [this]() { 
        _logic->refillBottle(); 
        server.send(200, "text/plain", "Refilled"); 
    });
    server.on("/setup", HTTP_GET, [this]() { 
        _logic->setFullBottle(); 
        server.send(200, "text/plain", "Setup Done"); 
    });
    server.on("/tare", HTTP_GET, [this]() { handleTare(); });

    server.begin();
    Serial.println("Web Server Started.");
}

void WebDashboard::handle() {
    server.handleClient();
}

void WebDashboard::handleRoot() {
    server.send(200, "text/html", getHTML());
}

void WebDashboard::handleStatus() {
    // Return JSON
    String json = "{";
    json += "\"dailyIntake\":" + String(_logic->getDailyIntake()) + ",";
    json += "\"dailyGoal\":" + String(_logic->getDailyGoal()) + ",";
    json += "\"currentWeight\":" + String(_logic->getCurrentWeight()) + ",";
    json += "\"bottleCapacity\":" + String(_logic->getBottleCapacity()) + ",";
    json += "\"state\":\"" + _logic->getStateName() + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

void WebDashboard::handleTare() {
    server.send(200, "text/plain", "Tare not implemented via Web for safety.");
}

String WebDashboard::getHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Water Tracker</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&display=swap" rel="stylesheet">
    <style>
        body { font-family: 'Inter', sans-serif; background-color: #0f172a; color: #f1f5f9; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }
        .dashboard { width: 100%; max-width: 400px; padding: 20px; box-sizing: border-box; }
        h1 { text-align: center; color: #38bdf8; margin-bottom: 20px; font-weight: 800; letter-spacing: -0.05rem; }
        
        /* Stats Grid */
        .stats-grid { display: grid; gap: 15px; grid-template-columns: 1fr 1fr; }
        .card { background: #1e293b; padding: 20px; border-radius: 16px; box-shadow: 0 10px 15px -3px rgba(0,0,0,0.3); text-align: center; }
        .card.full-width { grid-column: span 2; background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%); border: 1px solid #334155; }
        
        .label { font-size: 0.85rem; color: #94a3b8; text-transform: uppercase; letter-spacing: 0.05rem; margin-bottom: 8px; font-weight: 600; }
        .value { font-size: 1.5rem; font-weight: 700; color: #fff; }
        .value.highlight { color: #38bdf8; font-size: 2.5rem; }
        .unit { font-size: 0.9rem; color: #64748b; font-weight: 400; margin-left: 2px; }
        
        .status-badge { display: inline-block; background: #334155; color: #cbd5e1; padding: 6px 14px; border-radius: 99px; font-size: 0.8rem; margin-top: 10px; border: 1px solid #475569; }
        .status-badge.active { background: #166534; color: #86efac; border-color: #22c55e; }
        
        .btn-group { display: flex; gap: 10px; margin-top: 20px; }
        .btn { flex: 1; padding: 12px; border: none; border-radius: 8px; font-weight: 600; cursor: pointer; transition: opacity 0.2s; }
        .btn:active { opacity: 0.8; }
        .btn-primary { background: #3b82f6; color: white; }
        .btn-secondary { background: #334155; color: #e2e8f0; }
        
    </style>
</head>
<body>
    <div class="dashboard">
        <h1>Hydration Tracker</h1>

        <div class="card full-width">
            <div class="label">Total Consumed</div>
            <div class="value highlight"><span id="intake">0</span><span class="unit">g</span></div>
            <div id="goal-text" style="color: #64748b; font-size: 0.9rem; margin-top: 5px;">Goal: 2000 g</div>
            <div class="status-badge" id="state-badge">IDLE</div>
        </div>

        <div class="stats-grid" style="margin-top: 20px; grid-template-columns: 1fr;">
            <div class="card">
                <div class="label">Current Weight</div>
                <div class="value"><span id="current">0</span><span class="unit">g</span></div>
            </div>
        </div>

        <div class="btn-group">
            <button class="btn btn-secondary" onclick="doAction('/setup')">Set Full Bottle</button>
            <button class="btn btn-primary" onclick="doAction('/refill')">Refill</button>
        </div>
    </div>

    <script>
        function update() {
            fetch('/status')
                .then(r => r.json())
                .then(data => {
                    // Update Elements
                    document.getElementById('intake').innerText = Math.round(data.dailyIntake);
                    document.getElementById('goal-text').innerText = 'Goal: ' + Math.round(data.dailyGoal) + ' g';
                    document.getElementById('current').innerText = Math.round(data.currentWeight);
                    
                    const stateBadge = document.getElementById('state-badge');
                    stateBadge.innerText = data.state;
                    
                    if (data.state === 'STABILIZING') {
                        stateBadge.classList.add('active');
                        stateBadge.style.background = '#eab308'; // Yellow
                    } else if (data.state === 'REMOVED' || data.state === 'SETUP_NEEDED') {
                        stateBadge.classList.add('active');
                        stateBadge.style.background = '#ef4444'; // Red
                    } else {
                        stateBadge.classList.remove('active');
                        stateBadge.style.background = '#334155';
                    }
                });
        }
        
        function doAction(url) {
            if(confirm('Are you sure?')) {
                fetch(url).then(r => alert('Done!'));
            }
        }

        setInterval(update, 1000);
        update();
    </script>
</body>
</html>
)rawliteral";
    return html;
}
