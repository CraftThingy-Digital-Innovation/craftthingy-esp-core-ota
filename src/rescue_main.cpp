#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <Updater.h>
#include <EEPROM.h>

// ============================================================================
// CraftThingy Ultra-Lean Universal Rescue Stepping-Stone
// Designed for strict sub-180KB footprint to fit within BearSSL TLS limits
// 100% EEPROM compatible with CraftThingy ConfigStore layout
// ============================================================================

#define EEPROM_SIZE 1024
#define EEPROM_MAGIC 0x43524654 // 'CRFT'
#define CRAFT_MAX_PIN_ROLES 8

struct PinRoleEntry {
    char   role[12];
    int8_t gpio;
    uint8_t flags;
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
    int8_t gpioRelay;
    int8_t gpioSensor;
    PinRoleEntry pinRoles[CRAFT_MAX_PIN_ROLES];
};

struct EepromContainer {
    uint32_t magic;
    uint32_t version;
    DeviceConfig data;
    uint32_t crc;
};

static EepromContainer eepromData;
static WiFiServer server(80);
static String otaError = "";

static bool loadConfig() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, eepromData);
    return (eepromData.magic == EEPROM_MAGIC);
}

static bool checkAuthHeader(const String& headers) {
    DeviceConfig& cfg = eepromData.data;
    if (strlen(cfg.masterSecretKey) == 0 && strlen(cfg.currentRollingKey) == 0) return true;
    if (strlen(cfg.masterSecretKey) > 0 && headers.indexOf(cfg.masterSecretKey) >= 0) return true;
    if (strlen(cfg.currentRollingKey) > 0 && headers.indexOf(cfg.currentRollingKey) >= 0) return true;
    return false;
}

static void handleClientRequest(WiFiClient& client) {
    uint32_t startMs = millis();
    while (!client.available() && (millis() - startMs < 2000)) {
        delay(5);
        yield();
    }
    if (!client.available()) {
        client.stop();
        return;
    }

    String reqLine = client.readStringUntil('\r');
    if (client.peek() == '\n') client.read();

    // Read remaining headers until blank line
    String headers = "";
    int contentLength = -1;
    bool isMultipart = false;
    String boundary = "";

    while (client.connected()) {
        String line = client.readStringUntil('\r');
        if (client.peek() == '\n') client.read();
        if (line.length() == 0) break;

        headers += line + "\n";
        String lineLower = line;
        lineLower.toLowerCase();
        if (lineLower.startsWith("content-length:")) {
            contentLength = line.substring(15).toInt();
        } else if (lineLower.indexOf("multipart/form-data") >= 0) {
            isMultipart = true;
            int bIdx = line.indexOf("boundary=");
            if (bIdx >= 0) {
                boundary = "--" + line.substring(bIdx + 9);
                boundary.trim();
            }
        }
    }

    // 1. GET / or GET /api/v1/system/status
    if (reqLine.startsWith("GET /") || reqLine.startsWith("GET /api/v1/system/status")) {
        DeviceConfig& cfg = eepromData.data;
        String json = "{\"status\":\"success\",\"code\":200,";
        json += "\"role\":\"rescue_bridge\",";
        json += "\"targetRole\":\"" + String(cfg.activeRole) + "\",";
        json += "\"name\":\"CraftThingy-Rescue-Bridge\",";
        json += "\"coreVersion\":\"1.1.7-rescue\",";
        json += "\"appVersion\":\"rescue-stepping-stone\",";
        json += "\"uptime\":" + String(millis() / 1000) + ",";
        json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"freeSketchSpace\":" + String(ESP.getFreeSketchSpace()) + ",";
        json += "\"flashBytes\":" + String(ESP.getFlashChipRealSize()) + ",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"mac\":\"" + WiFi.macAddress() + "\",";
        json += "\"configured\":" + String(eepromData.magic == EEPROM_MAGIC ? "true" : "false") + ",";
        json += "\"otaError\":\"" + otaError + "\"}";

        client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + String(json.length()) + "\r\n\r\n" + json);
        delay(10);
        client.stop();
        return;
    }

    // 2. POST /api/v1/system/reboot
    if (reqLine.startsWith("POST /api/v1/system/reboot")) {
        if (!checkAuthHeader(headers)) {
            client.print("HTTP/1.1 401 Unauthorized\r\nConnection: close\r\nContent-Length: 26\r\n\r\n{\"error\":\"Unauthorized\"}");
            delay(10);
            client.stop();
            return;
        }
        client.print("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 27\r\n\r\n{\"message\":\"Rebooting...\"}");
        delay(100);
        client.stop();
        delay(500);
        ESP.restart();
        return;
    }

    // 3. POST /update or POST /api/v1/ota/flash (OTA Flash Streaming)
    if (reqLine.startsWith("POST /update") || reqLine.startsWith("POST /api/v1/ota/flash")) {
        if (!checkAuthHeader(headers) && reqLine.indexOf("key=") < 0) {
            client.print("HTTP/1.1 401 Unauthorized\r\nConnection: close\r\nContent-Length: 26\r\n\r\n{\"error\":\"Unauthorized\"}");
            delay(10);
            client.stop();
            return;
        }

        uint32_t maxSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        size_t otaSize = (contentLength > 0 && (size_t)contentLength < maxSpace && !isMultipart) ? contentLength : maxSpace;

        if (!Update.begin(otaSize, U_FLASH)) {
            otaError = "Update.begin failed (" + String(Update.getError()) + ")";
            client.print("HTTP/1.1 500 Internal Error\r\nConnection: close\r\nContent-Length: 35\r\n\r\n{\"error\":\"Update.begin failed\"}");
            delay(10);
            client.stop();
            return;
        }

        // Stream binary directly to flash
        uint8_t buf[1024];
        size_t written = 0;
        uint32_t lastData = millis();
        bool isDone = false;

        while ((client.connected() || client.available() > 0) && !isDone) {
            size_t avail = client.available();
            if (avail > 0) {
                size_t toRead = avail < sizeof(buf) ? avail : sizeof(buf);
                if (contentLength > 0 && !isMultipart && (written + toRead > (size_t)contentLength)) {
                    toRead = contentLength - written;
                }
                size_t bytesRead = client.readBytes(buf, toRead);
                if (bytesRead > 0) {
                    if (Update.write(buf, bytesRead) != bytesRead) {
                        otaError = "Flash write error";
                        Update.end(false);
                        client.print("HTTP/1.1 500 Internal Error\r\nConnection: close\r\nContent-Length: 30\r\n\r\n{\"error\":\"Flash write err\"}");
                        delay(10);
                        client.stop();
                        return;
                    }
                    written += bytesRead;
                    lastData = millis();
                    if (contentLength > 0 && !isMultipart && written >= (size_t)contentLength) {
                        isDone = true;
                    }
                }
            } else {
                if (millis() - lastData > 6000) {
                    break;
                }
                delay(2);
                yield();
            }
        }

        if (Update.end(true)) {
            client.print("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 29\r\n\r\n{\"message\":\"Update success\"}");
            delay(100);
            client.stop();
            delay(500);
            ESP.restart();
            return;
        } else {
            otaError = "Update.end failed (" + String(Update.getError()) + ")";
            client.print("HTTP/1.1 500 Internal Error\r\nConnection: close\r\nContent-Length: 33\r\n\r\n{\"error\":\"Update verification\"}");
            delay(10);
            client.stop();
            return;
        }
    }

    // Default 404
    client.print("HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 22\r\n\r\n{\"error\":\"Not found\"}");
    delay(10);
    client.stop();
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println(F("\n[Rescue Stepping-Stone] Booting..."));

    bool hasCfg = loadConfig();
    DeviceConfig& cfg = eepromData.data;

    WiFi.persistent(false);
    WiFi.disconnect(false);
    WiFi.mode(WIFI_STA);

    if (hasCfg && cfg.wifiUseStaticIp && strlen(cfg.wifiStaticIp) > 0) {
        IPAddress ip, gw, sub, dns1, dns2;
        ip.fromString(cfg.wifiStaticIp);
        gw.fromString(cfg.wifiGateway);
        sub.fromString(cfg.wifiSubnet);
        dns1.fromString(cfg.wifiDns1);
        dns2.fromString(cfg.wifiDns2);
        WiFi.config(ip, gw, sub, dns1, dns2);
        Serial.printf("[Rescue] Static IP: %s\n", cfg.wifiStaticIp);
    }

    if (hasCfg && strlen(cfg.wifiSsid) > 0) {
        Serial.printf("[Rescue] Connecting to %s...\n", cfg.wifiSsid);
        WiFi.begin(cfg.wifiSsid, cfg.wifiPassword);
        uint32_t t = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - t < 12000)) {
            delay(250);
            yield();
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[Rescue] Online! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println(F("[Rescue] WiFi failed. Starting Rescue SoftAP..."));
        WiFi.mode(WIFI_AP);
        WiFi.softAP(F("CraftThingy-Rescue-AP"), F("12345678"));
    }

    server.begin();
    Serial.println(F("[Rescue] Server listening on port 80."));
}

void loop() {
    WiFiClient client = server.accept();
    if (client) {
        handleClientRequest(client);
    }
    yield();
}
