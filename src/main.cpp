#include <Arduino.h>
#include "CraftThingyCore.h"
#include "controllers/WaterPumpController.h"
#include "controllers/WashingMachineController.h"

// Instantiate potential controllers
static WaterPumpController waterPump(0, 2);           // ESP-01 / ESP-01S default pins (GPIO0 relay, GPIO2 sensor)
static WashingMachineController washingMachine(12, 13, 14); // ESP32 default pins

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("=================================================="));
    Serial.println(F("   CraftThingy Digital Innovation - Universal OTA"));
    Serial.println(F("=================================================="));

    // Hardware detection runs during init
    ModuleDetector::init();
    const HardwareFingerprint& fp = ModuleDetector::getFingerprint();
    Serial.printf("Chip Detected : %s (%s)\n", fp.chipName, fp.archTag);
    Serial.printf("Flash Size    : %u bytes\n", fp.flashSizeBytes);
    Serial.printf("MAC Address   : %s\n", fp.macAddress.c_str());

    // Select controller based on configured role
    ConfigStore::init();
    DeviceConfig& cfg = ConfigStore::get();
    String activeRole = String(cfg.activeRole);

    IDeviceController* selectedController = nullptr;
    if (activeRole == "water_pump") {
        Serial.println(F("[Core] Activating WaterPumpController profile"));
        selectedController = &waterPump;
    } else if (activeRole == "washing_machine") {
        Serial.println(F("[Core] Activating WashingMachineController profile"));
        selectedController = &washingMachine;
    } else {
        Serial.println(F("[Core] Running Standalone Core OTA profile"));
    }

    // Initialize all networking, security, REST API, and OTA updater
    CraftThingyCore.begin(selectedController);
}

void loop() {
    CraftThingyCore.loop();
}
