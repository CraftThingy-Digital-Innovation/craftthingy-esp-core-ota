#include <Arduino.h>
#include <CraftThingyCore.h>

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("=================================================="));
    Serial.println(F(" CraftThingy ESP Core OTA Framework              "));
    Serial.println(F("=================================================="));

    // Initialize all core subsystems, hardware detector, WiFi & failover AP, REST API, and OTA
    CraftThingyCore.begin(nullptr);
}

void loop() {
    CraftThingyCore.loop();
}
