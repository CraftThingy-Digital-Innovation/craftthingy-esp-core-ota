#include <Arduino.h>
#include "CraftThingyCore.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("=================================================="));
    Serial.println(F("   CraftThingy Digital Innovation - Universal OTA"));
    Serial.println(F("   Pure Base Framework & Bootstrapper Firmware   "));
    Serial.println(F("=================================================="));

    // Hardware detection runs during init
    ModuleDetector::init();
    const HardwareFingerprint& fp = ModuleDetector::getFingerprint();
    Serial.printf("Chip Detected : %s (%s)\n", fp.chipName, fp.archTag);
    Serial.printf("Flash Size    : %u bytes\n", fp.flashSizeBytes);
    Serial.printf("MAC Address   : %s\n", fp.macAddress.c_str());

    // Initialize all networking, security, REST API, PinManager, and GitHub OTA updater
    // Runs in Base Core OTA profile (ready to download any controller application via OTA)
    CraftThingyCore.begin(nullptr);
}

void loop() {
    CraftThingyCore.loop();
}
