#pragma once

#include <Arduino.h>
#include "CraftThingyConfig.h"

#define CRAFT_MAX_PIN_ROLES 8

struct PinRoleEntry {
    char   role[12];     // Logical name: e.g. "relay", "sensor", "btn1", "i2c_sda"
    int8_t gpio;         // Configured GPIO pin (-1 = unassigned / use default)
    uint8_t flags;       // Bitmask: 0x01 = inverted (active-low), 0x02 = input-pullup
};

struct DeviceConfig {
    char wifiSsid[33];
    char wifiPassword[65];
    char adminPasswordHash[65];
    char masterSecretKey[65];
    char currentRollingKey[65];
    uint32_t rollingSequence;
    char githubOrg[64];
    char coreRepo[64];
    char controllerRepo[64];
    char activeRole[32];
    char githubToken[64];
    uint32_t otaIntervalSec;
    bool configured;
    char deviceName[32];
    char apSsid[33];
    char apPassword[65];
    bool wifiUseStaticIp;
    char wifiStaticIp[16];
    char wifiGateway[16];
    char wifiSubnet[16];
    char wifiDns1[16];
    char wifiDns2[16];
    int8_t gpioRelay;   // Backwards-compatible legacy relay pin (-1 = board default)
    int8_t gpioSensor;  // Backwards-compatible legacy sensor pin (-1 = board default)
    PinRoleEntry pinRoles[CRAFT_MAX_PIN_ROLES]; // Global persistent pin roles across all modules
};

class ConfigStore {
public:
    static bool init();
    static bool load();
    static bool save();
    static void resetToDefaults();

    static DeviceConfig& get();

    // Advance the rolling key sequence and update persistent storage
    static bool rotateRollingKey();

    // Update Master Secret Key / Static Bearer Token
    static bool setMasterSecretKey(const String& newKey);

    // Re-seed the rolling key ratchet with a new base seed & starting sequence
    static bool setRollingKeySeed(const String& newSeed, uint32_t startingSequence = 1, bool syncMaster = true);

    // Update GitHub target configuration
    static bool setGitHubConfig(const String& org, const String& coreRepo, 
                                const String& controllerRepo, const String& token);

    // Update WiFi configuration (supports DHCP or Static IP)
    static bool setWiFiConfig(const String& ssid, const String& pass,
                              bool useStatic = false,
                              const String& staticIp = "",
                              const String& gateway = "",
                              const String& subnet = "255.255.255.0",
                              const String& dns1 = "",
                              const String& dns2 = "");

    // Update IP Addressing configuration (DHCP vs Static IP, Subnet, Gateway, DNS) independently of WiFi credentials
    static bool setIpConfig(bool useStatic,
                            const String& staticIp = "",
                            const String& gateway = "",
                            const String& subnet = "255.255.255.0",
                            const String& dns1 = "",
                            const String& dns2 = "");

    // Global Hardware GPIO pin role mapping (persisted across OTA updates)
    static int8_t getPin(const char* role, int8_t defaultGpio = -1);
    static bool setPin(const char* role, int8_t gpio, uint8_t flags = 0);
    static const PinRoleEntry* getPinRoles();

    // Backwards-compatible pin setter
    static bool setPinConfig(int8_t relayPin, int8_t sensorPin);

    // Update Active Role
    static bool setActiveRole(const String& role);

    // Update Device Name (Internal identifier, does NOT alter WiFi AP name)
    static bool setDeviceName(const String& name);

    // Update SoftAP Fallback WiFi Configuration (SSID & WPA2 Password)
    static bool setAPConfig(const String& ssid, const String& pass);

    // Check if device is configured
    static bool isConfigured();

private:
    static DeviceConfig _config;
    static void populateDefaults();
};
