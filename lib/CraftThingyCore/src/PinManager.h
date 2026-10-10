#pragma once

#include <Arduino.h>
#include "CraftThingyConfig.h"
#include "ConfigStore.h"
#include "ModuleDetector.h"

enum PinCapability : uint8_t {
    PIN_CAP_NONE        = 0x00,
    PIN_CAP_INPUT       = 0x01,  // Digital input capable
    PIN_CAP_OUTPUT      = 0x02,  // Digital output capable
    PIN_CAP_STRAPPING   = 0x04,  // Boot strapping pin (warning in UI)
    PIN_CAP_INPUT_ONLY  = 0x08,  // Input only, cannot be output (e.g. ESP32 34-39)
    PIN_CAP_UART        = 0x10,  // Hardware Serial pin (disables console UART if used)
    PIN_CAP_ANALOG      = 0x20,  // ADC capable pin
    PIN_CAP_I2C         = 0x40   // Preferred I2C pin
};

struct BoardPinDesc {
    int8_t gpio;
    const char* name;
    uint8_t capabilities;
    const char* warning;
};

struct PinRoleDefinition {
    char role[12];
    char label[32];
    int8_t defaultGpio;
    uint8_t requiredCaps;
};

class PinManager {
public:
    static void init();

    // Register a required role from a device controller (e.g. "relay", "Water Pump Relay", 0, PIN_CAP_OUTPUT)
    static bool registerRole(const char* role, const char* label, int8_t defaultGpio, uint8_t requiredCaps = (PIN_CAP_INPUT | PIN_CAP_OUTPUT));

    // Get the configured pin for a registered role (reads from ConfigStore, falls back to registered default)
    static int8_t getPin(const char* role);

    // Set and persist pin for a role (with capability check)
    static bool setPin(const char* role, int8_t gpio, String& outError);

    // Board hardware queries
    static size_t getAvailablePins(const BoardPinDesc*& outPins);
    static const BoardPinDesc* findPinDesc(int8_t gpio);
    static bool isPinValidForRole(int8_t gpio, const char* role, String& outError);

    // JSON serialization for GET /api/v1/hardware/pins
    static String getHardwarePinsJson();

    // Apply JSON configuration from POST /api/v1/hardware/pins
    static bool applyPinsFromJson(const String& jsonBody, String& outError);

    // Get count and definitions of registered roles
    static uint8_t getRoleCount();
    static const PinRoleDefinition* getRoleDef(uint8_t index);

private:
    static PinRoleDefinition _registeredRoles[CRAFT_MAX_PIN_ROLES];
    static uint8_t _registeredRoleCount;
};
