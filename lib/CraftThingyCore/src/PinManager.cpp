#include "PinManager.h"

PinRoleDefinition PinManager::_registeredRoles[CRAFT_MAX_PIN_ROLES];
uint8_t PinManager::_registeredRoleCount = 0;

// =================================================================
// Board Pinout Definition Tables
// =================================================================

// 1. ESP-01 / ESP-01S (4 usable GPIOs)
static const BoardPinDesc PINS_ESP01[] = {
    {0, "GPIO 0", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING, "Strapping pin (Must be HIGH at boot to run flash)"},
    {2, "GPIO 2", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING, "Strapping pin (Must be HIGH at boot; Onboard Blue LED on 01S)"},
    {1, "GPIO 1 (TX)", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Serial TX pin (Disables debug logging if used for GPIO)"},
    {3, "GPIO 3 (RX)", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Serial RX pin (Disables serial console if used for GPIO)"}
};
static const size_t PINS_ESP01_COUNT = sizeof(PINS_ESP01) / sizeof(PINS_ESP01[0]);

// 2. Generic ESP8266 (ESP-12E/F, NodeMCU, D1 Mini)
static const BoardPinDesc PINS_ESP8266_GENERIC[] = {
    {16, "GPIO 16 (D0)", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "No internal pull-up / Deep sleep wake connection"},
    {5,  "GPIO 5 (D1)",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_I2C, "Default I2C SCL"},
    {4,  "GPIO 4 (D2)",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_I2C, "Default I2C SDA"},
    {0,  "GPIO 0 (D3)",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING, "Flash button (Must be HIGH at boot)"},
    {2,  "GPIO 2 (D4)",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING, "Internal LED (Must be HIGH at boot)"},
    {14, "GPIO 14 (D5)", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "SPI SCLK"},
    {12, "GPIO 12 (D6)", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "SPI MISO"},
    {13, "GPIO 13 (D7)", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "SPI MOSI"},
    {15, "GPIO 15 (D8)", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING, "Must be LOW at boot (needs 10k pulldown)"},
    {1,  "GPIO 1 (TX)",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Serial TX"},
    {3,  "GPIO 3 (RX)",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Serial RX"},
    {17, "A0 (ADC0)",    PIN_CAP_ANALOG | PIN_CAP_INPUT_ONLY, "Analog Input (0-1.0V max)"}
};
static const size_t PINS_ESP8266_GENERIC_COUNT = sizeof(PINS_ESP8266_GENERIC) / sizeof(PINS_ESP8266_GENERIC[0]);

// 3. Standard ESP32 (WROOM / WROVER / DevKit)
static const BoardPinDesc PINS_ESP32_GENERIC[] = {
    {4,  "GPIO 4",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, ""},
    {5,  "GPIO 5",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING, "Strapping pin (VSPI CS)"},
    {13, "GPIO 13", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, ""},
    {14, "GPIO 14", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, ""},
    {16, "GPIO 16", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "UART2 RX (Check if PSRAM used)"},
    {17, "GPIO 17", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "UART2 TX (Check if PSRAM used)"},
    {18, "GPIO 18", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "Default SPI SCK"},
    {19, "GPIO 19", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "Default SPI MISO"},
    {21, "GPIO 21", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_I2C, "Default I2C SDA"},
    {22, "GPIO 22", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_I2C, "Default I2C SCL"},
    {23, "GPIO 23", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "Default SPI MOSI"},
    {25, "GPIO 25", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "DAC1 output"},
    {26, "GPIO 26", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "DAC2 output"},
    {27, "GPIO 27", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, ""},
    {32, "GPIO 32", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch4"},
    {33, "GPIO 33", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch5"},
    {34, "GPIO 34", PIN_CAP_INPUT | PIN_CAP_INPUT_ONLY | PIN_CAP_ANALOG, "Input Only (No internal pull-up/down)"},
    {35, "GPIO 35", PIN_CAP_INPUT | PIN_CAP_INPUT_ONLY | PIN_CAP_ANALOG, "Input Only (No internal pull-up/down)"},
    {36, "GPIO 36 (VP)", PIN_CAP_INPUT | PIN_CAP_INPUT_ONLY | PIN_CAP_ANALOG, "Input Only (Sens VP)"},
    {39, "GPIO 39 (VN)", PIN_CAP_INPUT | PIN_CAP_INPUT_ONLY | PIN_CAP_ANALOG, "Input Only (Sens VN)"},
    {0,  "GPIO 0",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING, "Strapping pin (Must be HIGH to boot normally)"},
    {2,  "GPIO 2",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING | PIN_CAP_ANALOG, "Strapping pin (Must be LOW for download)"},
    {12, "GPIO 12", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING | PIN_CAP_ANALOG, "Strapping pin (Flash voltage selector)"},
    {15, "GPIO 15", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING | PIN_CAP_ANALOG, "Strapping pin (MTDO)"},
    {1,  "GPIO 1 (TX)", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Serial TX"},
    {3,  "GPIO 3 (RX)", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Serial RX"}
};
static const size_t PINS_ESP32_GENERIC_COUNT = sizeof(PINS_ESP32_GENERIC) / sizeof(PINS_ESP32_GENERIC[0]);

// 4. ESP32-S3 (DevKitC / Native USB)
static const BoardPinDesc PINS_ESP32_S3[] = {
    {1,  "GPIO 1",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch0"},
    {2,  "GPIO 2",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch1"},
    {4,  "GPIO 4",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch3"},
    {5,  "GPIO 5",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch4"},
    {6,  "GPIO 6",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch5"},
    {7,  "GPIO 7",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch6"},
    {8,  "GPIO 8",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_I2C, "Default I2C SDA"},
    {9,  "GPIO 9",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_I2C, "Default I2C SCL"},
    {10, "GPIO 10", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {11, "GPIO 11", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {12, "GPIO 12", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {13, "GPIO 13", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {14, "GPIO 14", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {15, "GPIO 15", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {16, "GPIO 16", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {17, "GPIO 17", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {18, "GPIO 18", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {19, "GPIO 19 (USB D-)", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Native USB D-"},
    {20, "GPIO 20 (USB D+)", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Native USB D+"},
    {21, "GPIO 21", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {38, "GPIO 38", PIN_CAP_INPUT | PIN_CAP_OUTPUT, "RGB LED on DevKit"},
    {39, "GPIO 39", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {40, "GPIO 40", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {41, "GPIO 41", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {42, "GPIO 42", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {0,  "GPIO 0",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING, "Strapping pin (Boot button)"},
    {45, "GPIO 45", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING, "Strapping pin (VDD_SPI)"},
    {46, "GPIO 46", PIN_CAP_INPUT | PIN_CAP_INPUT_ONLY | PIN_CAP_STRAPPING, "Strapping pin (ROM messages)"}
};
static const size_t PINS_ESP32_S3_COUNT = sizeof(PINS_ESP32_S3) / sizeof(PINS_ESP32_S3[0]);

// 5. ESP32-C3 (RISC-V)
static const BoardPinDesc PINS_ESP32_C3[] = {
    {0,  "GPIO 0",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch0"},
    {1,  "GPIO 1",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch1"},
    {2,  "GPIO 2",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING | PIN_CAP_ANALOG, "Strapping pin"},
    {3,  "GPIO 3",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch3"},
    {4,  "GPIO 4",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch4"},
    {5,  "GPIO 5",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_ANALOG, "ADC1 Ch5"},
    {6,  "GPIO 6",  PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {7,  "GPIO 7",  PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {8,  "GPIO 8",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING | PIN_CAP_I2C, "Default I2C SDA"},
    {9,  "GPIO 9",  PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_STRAPPING | PIN_CAP_I2C, "Default I2C SCL (Boot button)"},
    {10, "GPIO 10", PIN_CAP_INPUT | PIN_CAP_OUTPUT, ""},
    {18, "GPIO 18 (USB D-)", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Native USB D-"},
    {19, "GPIO 19 (USB D+)", PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "Native USB D+"},
    {20, "GPIO 20 (RX)",     PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "UART RX"},
    {21, "GPIO 21 (TX)",     PIN_CAP_INPUT | PIN_CAP_OUTPUT | PIN_CAP_UART, "UART TX"}
};
static const size_t PINS_ESP32_C3_COUNT = sizeof(PINS_ESP32_C3) / sizeof(PINS_ESP32_C3[0]);

// =================================================================
// PinManager Implementation
// =================================================================

void PinManager::init() {
    _registeredRoleCount = 0;
}

bool PinManager::registerRole(const char* role, const char* label, int8_t defaultGpio, uint8_t requiredCaps) {
    if (!role || strlen(role) == 0 || _registeredRoleCount >= CRAFT_MAX_PIN_ROLES) {
        return false;
    }

    // Check if role is already registered; update if so
    for (uint8_t i = 0; i < _registeredRoleCount; ++i) {
        if (strcmp(_registeredRoles[i].role, role) == 0) {
            strncpy(_registeredRoles[i].label, label ? label : role, sizeof(_registeredRoles[i].label) - 1);
            _registeredRoles[i].label[sizeof(_registeredRoles[i].label) - 1] = '\0';
            _registeredRoles[i].defaultGpio = defaultGpio;
            _registeredRoles[i].requiredCaps = requiredCaps;
            return true;
        }
    }

    // Add new role definition
    uint8_t idx = _registeredRoleCount++;
    strncpy(_registeredRoles[idx].role, role, sizeof(_registeredRoles[idx].role) - 1);
    _registeredRoles[idx].role[sizeof(_registeredRoles[idx].role) - 1] = '\0';
    strncpy(_registeredRoles[idx].label, label ? label : role, sizeof(_registeredRoles[idx].label) - 1);
    _registeredRoles[idx].label[sizeof(_registeredRoles[idx].label) - 1] = '\0';
    _registeredRoles[idx].defaultGpio = defaultGpio;
    _registeredRoles[idx].requiredCaps = requiredCaps;
    return true;
}

int8_t PinManager::getPin(const char* role) {
    if (!role || strlen(role) == 0) return -1;

    // Find registered default fallback
    int8_t defPin = -1;
    for (uint8_t i = 0; i < _registeredRoleCount; ++i) {
        if (strcmp(_registeredRoles[i].role, role) == 0) {
            defPin = _registeredRoles[i].defaultGpio;
            break;
        }
    }

    // Query ConfigStore (persisted in EEPROM/NVS across updates)
    return ConfigStore::getPin(role, defPin);
}

bool PinManager::setPin(const char* role, int8_t gpio, String& outError) {
    if (!role || strlen(role) == 0) {
        outError = "Role name is required";
        return false;
    }

    if (!isPinValidForRole(gpio, role, outError)) {
        return false;
    }

    if (!ConfigStore::setPin(role, gpio)) {
        outError = "Failed to save pin to persistent EEPROM storage";
        return false;
    }

    return true;
}

size_t PinManager::getAvailablePins(const BoardPinDesc*& outPins) {
    const HardwareFingerprint& fp = ModuleDetector::getFingerprint();

    switch (fp.chipType) {
        case CHIP_ESP8266_ESP01:
        case CHIP_ESP8266_ESP01S:
            outPins = PINS_ESP01;
            return PINS_ESP01_COUNT;

        case CHIP_ESP8266_GENERIC:
            outPins = PINS_ESP8266_GENERIC;
            return PINS_ESP8266_GENERIC_COUNT;

        case CHIP_ESP32_S3:
            outPins = PINS_ESP32_S3;
            return PINS_ESP32_S3_COUNT;

        case CHIP_ESP32_C3:
        case CHIP_ESP32_C6:
            outPins = PINS_ESP32_C3;
            return PINS_ESP32_C3_COUNT;

        case CHIP_ESP32_ORIGINAL:
        case CHIP_ESP32_S2:
        default:
            outPins = PINS_ESP32_GENERIC;
            return PINS_ESP32_GENERIC_COUNT;
    }
}

const BoardPinDesc* PinManager::findPinDesc(int8_t gpio) {
    const BoardPinDesc* pins = nullptr;
    size_t count = getAvailablePins(pins);

    for (size_t i = 0; i < count; ++i) {
        if (pins[i].gpio == gpio) {
            return &pins[i];
        }
    }
    return nullptr;
}

bool PinManager::isPinValidForRole(int8_t gpio, const char* role, String& outError) {
    if (gpio < 0) {
        outError = "Invalid GPIO pin index";
        return false;
    }

    const BoardPinDesc* desc = findPinDesc(gpio);
    if (!desc) {
        outError = "GPIO " + String(gpio) + " is not physically available or safe on this board";
        return false;
    }

    // Check role capabilities requirements
    for (uint8_t i = 0; i < _registeredRoleCount; ++i) {
        if (strcmp(_registeredRoles[i].role, role) == 0) {
            uint8_t req = _registeredRoles[i].requiredCaps;
            if ((req & PIN_CAP_OUTPUT) && (desc->capabilities & PIN_CAP_INPUT_ONLY)) {
                outError = "GPIO " + String(gpio) + " is Input-Only and cannot drive an output actuator/relay";
                return false;
            }
            break;
        }
    }

    return true;
}

String PinManager::getHardwarePinsJson() {
    const HardwareFingerprint& fp = ModuleDetector::getFingerprint();
    const BoardPinDesc* pins = nullptr;
    size_t pinCount = getAvailablePins(pins);

    String json = "{";
    json += "\"boardTarget\":\"" + String(fp.archTag) + "\",";
    json += "\"chip\":\"" + String(fp.chipName) + "\",";

    // 1. Available physical pins for this detected board
    json += "\"availablePins\":[";
    for (size_t i = 0; i < pinCount; ++i) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"gpio\":" + String(pins[i].gpio) + ",";
        json += "\"name\":\"" + String(pins[i].name) + "\",";
        json += "\"capabilities\":" + String(pins[i].capabilities) + ",";
        json += "\"warning\":\"" + String(pins[i].warning) + "\"";
        json += "}";
    }
    json += "],";

    // 2. Currently registered functional roles for the active controller
    json += "\"roles\":[";
    for (uint8_t i = 0; i < _registeredRoleCount; ++i) {
        if (i > 0) json += ",";
        int8_t currentPin = getPin(_registeredRoles[i].role);
        json += "{";
        json += "\"role\":\"" + String(_registeredRoles[i].role) + "\",";
        json += "\"label\":\"" + String(_registeredRoles[i].label) + "\",";
        json += "\"gpio\":" + String(currentPin) + ",";
        json += "\"defaultGpio\":" + String(_registeredRoles[i].defaultGpio) + ",";
        json += "\"requiredCaps\":" + String(_registeredRoles[i].requiredCaps);
        json += "}";
    }
    json += "]";

    json += "}";
    return json;
}

bool PinManager::applyPinsFromJson(const String& jsonBody, String& outError) {
    if (jsonBody.length() == 0) {
        outError = "Empty JSON payload";
        return false;
    }

    bool matchedAny = false;

    // Scan for each registered role in the JSON payload
    for (uint8_t i = 0; i < _registeredRoleCount; ++i) {
        const char* role = _registeredRoles[i].role;
        int8_t gpio = -1;
        bool found = false;

        // Check format 1: {"roles": [{"role": "relay", "gpio": 0}, ...]}
        String roleSearch = "\"role\":\"" + String(role) + "\"";
        int rolePos = jsonBody.indexOf(roleSearch);
        if (rolePos < 0) {
            roleSearch = "\"role\": \"" + String(role) + "\"";
            rolePos = jsonBody.indexOf(roleSearch);
        }

        if (rolePos >= 0) {
            int gpioPos = jsonBody.indexOf("\"gpio\":", rolePos);
            if (gpioPos < 0) gpioPos = jsonBody.indexOf("\"gpio\": ", rolePos);
            if (gpioPos < 0) gpioPos = jsonBody.indexOf("\"pin\":", rolePos);
            if (gpioPos < 0) gpioPos = jsonBody.indexOf("\"pin\": ", rolePos);

            if (gpioPos >= 0 && (gpioPos - rolePos) < 120) {
                int colon = jsonBody.indexOf(':', gpioPos);
                int valStart = colon + 1;
                while (valStart < (int)jsonBody.length() && (jsonBody[valStart] == ' ' || jsonBody[valStart] == '"')) valStart++;
                int valEnd = valStart;
                while (valEnd < (int)jsonBody.length() && (isDigit(jsonBody[valEnd]) || jsonBody[valEnd] == '-')) valEnd++;
                if (valEnd > valStart) {
                    gpio = (int8_t)jsonBody.substring(valStart, valEnd).toInt();
                    found = true;
                }
            }
        }

        // Check format 2: {"relay": 0, "sensor": 2}
        if (!found) {
            String keySearch = "\"" + String(role) + "\"";
            int keyPos = jsonBody.indexOf(keySearch);
            if (keyPos >= 0) {
                int colon = jsonBody.indexOf(':', keyPos);
                if (colon >= 0) {
                    int valStart = colon + 1;
                    while (valStart < (int)jsonBody.length() && (jsonBody[valStart] == ' ' || jsonBody[valStart] == '"')) valStart++;
                    int valEnd = valStart;
                    while (valEnd < (int)jsonBody.length() && (isDigit(jsonBody[valEnd]) || jsonBody[valEnd] == '-')) valEnd++;
                    if (valEnd > valStart) {
                        gpio = (int8_t)jsonBody.substring(valStart, valEnd).toInt();
                        found = true;
                    }
                }
            }
        }

        if (found) {
            matchedAny = true;
            if (!setPin(role, gpio, outError)) {
                return false;
            }
        }
    }

    if (!matchedAny) {
        outError = "No known pin roles matched in JSON payload";
        return false;
    }

    return true;
}

uint8_t PinManager::getRoleCount() {
    return _registeredRoleCount;
}

const PinRoleDefinition* PinManager::getRoleDef(uint8_t index) {
    if (index >= _registeredRoleCount) return nullptr;
    return &_registeredRoles[index];
}
