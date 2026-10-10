#pragma once

#include <Arduino.h>

/**
 * @brief Abstract interface for application-specific controllers (e.g. Water Pump, Washing Machine).
 * 
 * Separates the core OTA & security infrastructure from the actual physical hardware automation.
 */
class IDeviceController {
public:
    virtual ~IDeviceController() {}

    // Initialize physical pins, sensors, relays, timers
    virtual void setup() = 0;

    // Periodic non-blocking operational loop
    virtual void loop() = 0;

    // Identifying role name (e.g. "water_pump", "washing_machine")
    virtual const char* getRoleName() const = 0;

    // Current controller application version
    virtual const char* getAppVersion() const = 0;

    // Handle incoming command from local API or remote trigger
    virtual bool handleCommand(const String& cmd, const String& payload, String& response) = 0;

    // Return human-readable status for web portal or status API
    virtual String getStatusJson() = 0;

    // Safety Interlock for OTA Firmware Updates:
    // Returns true if the controller is in an idle, safe state to update and reboot.
    // Returns false if a critical operation (e.g. pump actively running, spin cycle) is underway.
    virtual bool isSafeToUpdate() const { return true; }

    // Gracefully shutdown actuators / place hardware in fail-safe state prior to flash reboot.
    virtual void prepareForUpdate() {}
};
