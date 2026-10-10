#include "WiFiManager.h"
#include "ConfigStore.h"

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
#endif

CraftWiFiState WiFiManager::_state = WIFI_STATE_IDLE;
DNSServer WiFiManager::_dnsServer;
String WiFiManager::_apSSID = "CraftThingy-Setup";
uint32_t WiFiManager::_lastReconnectAttempt = 0;
uint32_t WiFiManager::_connectingStartMs = 0;
uint8_t WiFiManager::_retryCount = 0;
bool WiFiManager::_dnsActive = false;
bool WiFiManager::_mdnsActive = false;

void WiFiManager::init() {
    DeviceConfig& cfg = ConfigStore::get();
    if (strlen(cfg.apSsid) > 0) {
        _apSSID = String(cfg.apSsid);
    } else {
        String mac = getMacAddress();
        mac.replace(":", "");
        String suffix = mac.substring(mac.length() - 4);
        suffix.toUpperCase();
        _apSSID = "CraftThingy-" + suffix;
    }

    WiFi.persistent(false);
    WiFi.disconnect(true);
    _state = WIFI_STATE_IDLE;
}

bool WiFiManager::connectSTA(const char* ssid, const char* password, uint8_t maxRetries) {
    if (!ssid || strlen(ssid) == 0) {
        startAPPortal();
        return false;
    }

    _state = WIFI_STATE_CONNECTING;
    _connectingStartMs = millis();
    _retryCount = 0;

    // Use WIFI_AP_STA so the softAP connection stays active!
    // This allows in-flight HTTP requests to complete and avoids dropping the user's connection.
    WiFi.mode(WIFI_AP_STA);

    DeviceConfig& cfg = ConfigStore::get();
    String hostName = (strlen(cfg.deviceName) > 0) ? String(cfg.deviceName) : _apSSID;
    hostName.replace(" ", "-");
#if defined(ESP8266)
    WiFi.hostname(hostName);
#elif defined(ESP32)
    WiFi.setHostname(hostName.c_str());
#endif

    // Static IP or DHCP configuration
    if (cfg.wifiUseStaticIp && strlen(cfg.wifiStaticIp) > 0) {
        IPAddress staticIP, gateway, subnet, dns1, dns2;
        if (staticIP.fromString(cfg.wifiStaticIp)) {
            if (!gateway.fromString(cfg.wifiGateway)) {
                gateway = IPAddress(0, 0, 0, 0);
            }
            if (!subnet.fromString(cfg.wifiSubnet)) {
                subnet = IPAddress(255, 255, 255, 0);
            }
            if (!dns1.fromString(cfg.wifiDns1)) {
                dns1 = gateway;
            }
            dns2.fromString(cfg.wifiDns2);
            WiFi.config(staticIP, gateway, subnet, dns1, dns2);
        }
    } else {
        // Reset to DHCP (Dynamic IP)
#if defined(ESP8266)
        WiFi.config(0u, 0u, 0u);
#elif defined(ESP32)
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
#endif
    }

    WiFi.begin(ssid, password);

    // Give a brief window (up to 1.5s) to connect immediately without stalling HTTP responses
    uint32_t startMs = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 1500) {
        delay(50);
        yield();
    }

    if (WiFi.status() == WL_CONNECTED) {
        _state = WIFI_STATE_CONNECTED;
        if (_dnsActive) {
            _dnsServer.stop();
            _dnsActive = false;
        }
        initMDNS();
        return true;
    }

    // Connecting in background; loop() will monitor progress
    return true;
}

void WiFiManager::startAPPortal() {
    _state = WIFI_STATE_AP_PORTAL;

    WiFi.mode(WIFI_AP_STA);
    IPAddress apIP(192, 168, 4, 1);
    IPAddress netMsk(255, 255, 255, 0);

    WiFi.softAPConfig(apIP, apIP, netMsk);
    DeviceConfig& cfg = ConfigStore::get();
    const char* apPass = (strlen(cfg.apPassword) >= 8) ? cfg.apPassword : nullptr;
    WiFi.softAP(_apSSID.c_str(), apPass, CRAFT_AP_CHANNEL);

    if (!_dnsActive) {
        _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        _dnsServer.start(CRAFT_DNS_PORT, "*", apIP);
        _dnsActive = true;
    }
}

bool WiFiManager::retryStation() {
    DeviceConfig& cfg = ConfigStore::get();
    if (strlen(cfg.wifiSsid) > 0) {
        return connectSTA(cfg.wifiSsid, cfg.wifiPassword, 3);
    }
    return false;
}

void WiFiManager::initMDNS() {
    DeviceConfig& cfg = ConfigStore::get();
    String mdnsName = (strlen(cfg.deviceName) > 0) ? String(cfg.deviceName) : _apSSID;
    mdnsName.replace(" ", "-");
    mdnsName.toLowerCase();

#if defined(ESP8266)
    if (MDNS.begin(mdnsName.c_str())) {
        MDNS.addService("http", "tcp", 80);
        _mdnsActive = true;
    }
#elif defined(ESP32)
    if (MDNS.begin(mdnsName.c_str())) {
        MDNS.addService("http", "tcp", 80);
        _mdnsActive = true;
    }
#endif
}

void WiFiManager::loop() {
    if (_state == WIFI_STATE_AP_PORTAL) {
        if (_dnsActive) {
            _dnsServer.processNextRequest();
        }

        // Periodic background attempt every 60 seconds if configured
        if (ConfigStore::isConfigured() && (millis() - _lastReconnectAttempt > 60000)) {
            _lastReconnectAttempt = millis();
            DeviceConfig& cfg = ConfigStore::get();
            if (WiFi.status() != WL_CONNECTED) {
                WiFi.begin(cfg.wifiSsid, cfg.wifiPassword);
                uint32_t testStart = millis();
                while (WiFi.status() != WL_CONNECTED && (millis() - testStart) < 3000) {
                    delay(100);
                    yield();
                }
                if (WiFi.status() == WL_CONNECTED) {
                    _state = WIFI_STATE_CONNECTED;
                    if (_dnsActive) {
                        _dnsServer.stop();
                        _dnsActive = false;
                    }
                    initMDNS();
                }
            }
        }
    } else if (_state == WIFI_STATE_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _state = WIFI_STATE_CONNECTED;
            _retryCount = 0;
            if (_dnsActive) {
                _dnsServer.stop();
                _dnsActive = false;
            }
            initMDNS();
        } else if (millis() - _connectingStartMs > 30000) {
            // 30-second connection timeout, fallback to AP portal
            _state = WIFI_STATE_FAILED;
            startAPPortal();
        }
    } else if (_state == WIFI_STATE_CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            _state = WIFI_STATE_CONNECTING;
            _connectingStartMs = millis();
        }
    }

#if defined(ESP8266)
    if (_mdnsActive) {
        MDNS.update();
    }
#endif
}

CraftWiFiState WiFiManager::getState() {
    return _state;
}

bool WiFiManager::isConnected() {
    return (_state == WIFI_STATE_CONNECTED) && (WiFi.status() == WL_CONNECTED);
}

bool WiFiManager::isAPMode() {
    return _state == WIFI_STATE_AP_PORTAL;
}

String WiFiManager::getIPAddress() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String WiFiManager::getAPIPAddress() {
    return WiFi.softAPIP().toString();
}

String WiFiManager::getGatewayIP() {
    if (isConnected()) {
        return WiFi.gatewayIP().toString();
    }
    return "0.0.0.0";
}

String WiFiManager::getSubnetMask() {
    if (isConnected()) {
        return WiFi.subnetMask().toString();
    }
    return "255.255.255.0";
}

String WiFiManager::getDnsIP() {
    if (isConnected()) {
        return WiFi.dnsIP().toString();
    }
    return "0.0.0.0";
}

String WiFiManager::getMacAddress() {
    return WiFi.macAddress();
}

String WiFiManager::getApSSID() {
    return _apSSID;
}

String WiFiManager::scanNetworksJson() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
#if defined(ESP8266)
        json += "\"enc\":" + String(WiFi.encryptionType(i) != ENC_TYPE_NONE ? "true" : "false") + "}";
#elif defined(ESP32)
        json += "\"enc\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
#endif
    }
    json += "]";
    return json;
}

bool WiFiManager::isAPPasswordSet() {
    DeviceConfig& cfg = ConfigStore::get();
    return strlen(cfg.apPassword) >= 8;
}

void WiFiManager::configureAP(const String& ssid, const String& password) {
    if (ssid.length() > 0) {
        _apSSID = ssid;
    } else {
        String mac = getMacAddress();
        mac.replace(":", "");
        String suffix = mac.substring(mac.length() - 4);
        suffix.toUpperCase();
        _apSSID = "CraftThingy-" + suffix;
    }

    DeviceConfig& cfg = ConfigStore::get();
    const char* apPass = (password.length() >= 8) ? password.c_str() : 
                         (strlen(cfg.apPassword) >= 8 ? cfg.apPassword : nullptr);

    // Apply immediately to the live radio beacon if SoftAP is broadcasting or in AP mode
    WiFiMode_t currentMode = WiFi.getMode();
    if ((currentMode & WIFI_AP) || _state == WIFI_STATE_AP_PORTAL || _state == WIFI_STATE_CONNECTING) {
        WiFi.softAP(_apSSID.c_str(), apPass, CRAFT_AP_CHANNEL);
    }
}

void WiFiManager::setDeviceName(const String& name) {
    if (name.length() == 0) return;
    String hostName = name;
    hostName.replace(" ", "-");
#if defined(ESP8266)
    WiFi.hostname(hostName);
#elif defined(ESP32)
    WiFi.setHostname(hostName.c_str());
#endif
    // Intentionally does NOT alter _apSSID or SoftAP hotspot name!
}
