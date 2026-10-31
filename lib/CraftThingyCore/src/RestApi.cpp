#include "RestApi.h"
#include "ConfigStore.h"
#include "WiFiManager.h"
#include "GitHubUpdater.h"
#include "ModuleDetector.h"
#include "CraftThingyCrypto.h"
#include "CraftThingyCore.h"
#include "CaptivePortalHtml.h"
#include "PinManager.h"

ApiServerType RestApi::_server(CRAFT_WEB_PORT);

void RestApi::init() {
#if defined(ESP8266)
    _server.collectHeaders("X-Signature", "X-Timestamp", "X-Nonce", "X-Sequence", "Authorization", "X-Master-Key");
#elif defined(ESP32)
    const char* headerKeys[] = {"X-Signature", "X-Timestamp", "X-Nonce", "X-Sequence", "Authorization", "X-Master-Key"};
    _server.collectHeaders(headerKeys, 6);
#endif

    setupRoutes();
    _server.begin();
}

void RestApi::loop() {
    _server.handleClient();
}

void RestApi::sendStandardResponse(int httpCode, const String& dataJson) {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Private-Network", "true");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Signature, X-Timestamp, X-Nonce, X-Sequence, Authorization, X-Master-Key");

    String envelope = "{";
    envelope += "\"status\":\"success\",";
    envelope += "\"code\":" + String(httpCode) + ",";
    envelope += "\"uptime\":" + String(millis() / 1000) + ",";
    envelope += "\"data\":" + dataJson;
    envelope += "}";

    _server.send(httpCode, "application/json", envelope);
}

void RestApi::sendStandardError(int httpCode, const String& message, const String& reason) {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Private-Network", "true");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Signature, X-Timestamp, X-Nonce, X-Sequence, Authorization, X-Master-Key");

    String envelope = "{";
    envelope += "\"status\":\"error\",";
    envelope += "\"code\":" + String(httpCode) + ",";
    envelope += "\"uptime\":" + String(millis() / 1000) + ",";
    envelope += "\"error\":{";
    envelope += "\"message\":\"" + message + "\"";
    if (reason.length() > 0) {
        envelope += ",\"reason\":\"" + reason + "\"";
    }
    envelope += "}}";

    _server.send(httpCode, "application/json", envelope);
}

void RestApi::sendStandardSuccess(const String& message) {
    sendStandardResponse(200, "{\"message\":\"" + message + "\"}");
}

bool RestApi::isRequestAuthorized(ApiServerType& server, String& outMethod) {
    DeviceConfig& cfg = ConfigStore::get();

    String sig = server.header("X-Signature");
    String timestamp = server.header("X-Timestamp");
    String nonce = server.header("X-Nonce");
    String seqStr = server.header("X-Sequence");
    String authHeader = server.header("Authorization");

    // Tier 1: HMAC-SHA256 Rolling Key Verification (Highest Security)
    if (sig.length() == 64 && nonce.length() > 0) {
        uint32_t seq = seqStr.toInt();

        // Anti-replay check
        if (!CraftThingyCrypto::checkAndRecordNonce(nonce, seq)) {
            outMethod = "rejected_replay";
            return false;
        }

        String method = server.method() == HTTP_POST ? "POST" : 
                       (server.method() == HTTP_GET ? "GET" : "PUT");
        String path = server.uri();
        String body = server.arg("plain");

        // Verify with current rolling key
        if (CraftThingyCrypto::verifyRequestSignature(cfg.currentRollingKey, method, path, timestamp, nonce, body, sig)) {
            outMethod = "hmac_rolling_key";
            return true;
        }

        // Fallback: Verify with master secret key
        if (CraftThingyCrypto::verifyRequestSignature(cfg.masterSecretKey, method, path, timestamp, nonce, body, sig)) {
            outMethod = "hmac_master_key";
            return true;
        }

        outMethod = "invalid_signature";
        return false;
    }

    // Tier 2: Bearer Token Authentication
    if (authHeader.startsWith("Bearer ") || authHeader.startsWith("bearer ")) {
        String token = authHeader.substring(7);
        token.trim();
        if (CraftThingyCrypto::constantTimeEquals(token, String(cfg.currentRollingKey)) ||
            CraftThingyCrypto::constantTimeEquals(token, String(cfg.masterSecretKey))) {
            outMethod = "bearer_token";
            return true;
        }
    }

    // Tier 2b: X-Master-Key Header Authentication
    String masterKey = server.header("X-Master-Key");
    if (masterKey.length() > 0) {
        masterKey.trim();
        if (CraftThingyCrypto::constantTimeEquals(masterKey, String(cfg.masterSecretKey)) ||
            CraftThingyCrypto::constantTimeEquals(masterKey, String(cfg.currentRollingKey))) {
            outMethod = "master_key_header";
            return true;
        }
    }

    // Tier 2c: Query Parameter Token (?key= or ?token=)
    if (server.hasArg("key") || server.hasArg("token")) {
        String queryToken = server.hasArg("key") ? server.arg("key") : server.arg("token");
        queryToken.trim();
        if (queryToken.length() > 0 &&
            (CraftThingyCrypto::constantTimeEquals(queryToken, String(cfg.masterSecretKey)) ||
             CraftThingyCrypto::constantTimeEquals(queryToken, String(cfg.currentRollingKey)))) {
            outMethod = "query_token";
            return true;
        }
    }

    // Tier 3: Admin Basic Auth
    if (server.authenticate("admin", cfg.adminPasswordHash)) {
        outMethod = "basic_auth";
        return true;
    }

    // Tier 4: Read-Only Dashboard Telemetry (Permits local web browser dashboard monitoring)
    String uri = server.uri();
    if (server.method() == HTTP_GET) {
        if (uri == "/" || uri == "/setup" || 
            uri == "/api/v1/system/status" || uri == "/api/v1/system/name" ||
            uri == "/api/v1/device/status" || uri == "/api/v1/pump/status" ||
            uri == "/api/v1/wifi/scan" || uri == "/api/v1/hardware/pins" ||
            uri == "/api/status" || uri == "/api/controller/status") {
            outMethod = "dashboard_telemetry_get";
            return true;
        }
    }

    // Tier 5: SoftAP Setup Mode (Permits mobile phone captive portal without HMAC keys)
    if (WiFiManager::isAPMode()) {
        if (uri == "/" || uri == "/setup" || uri.startsWith("/api/v1/wifi/") || 
            uri == "/api/v1/system/status" || uri == "/api/v1/system/name" || uri == "/api/v1/device/name" ||
            uri == "/api/v1/device/status" || uri == "/api/v1/pump/status" ||
            uri == "/api/v1/device/command" || uri == "/api/v1/pump/command" ||
            uri == "/api/v1/hardware/pins" || uri == "/api/hardware/pins") {
            outMethod = "ap_portal_local";
            return true;
        }
    }

    outMethod = "unauthorized";
    return false;
}

void RestApi::setupRoutes() {
    // -----------------------------------------------------------------
    // Captive Portal & Mobile Browser Setup Routes
    // -----------------------------------------------------------------
    _server.on("/", HTTP_GET, handleRootPortal);
    _server.on("/setup", HTTP_GET, handleRootPortal);
    _server.on("/setup", HTTP_POST, handleSetupPost);
    _server.on("/generate_204", HTTP_GET, handleRootPortal);
    _server.on("/gen_204", HTTP_GET, handleRootPortal);
    _server.on("/hotspot-detect.html", HTTP_GET, handleRootPortal);
    _server.on("/canonical.html", HTTP_GET, handleRootPortal);
    _server.on("/ncsi.txt", HTTP_GET, handleRootPortal);
    _server.on("/connecttest.txt", HTTP_GET, handleRootPortal);

    // -----------------------------------------------------------------
    // Standardized v1 API Endpoints (All Authenticated for Safety)
    // -----------------------------------------------------------------
    // Device Controller (Pump / Appliance)
    _server.on("/api/v1/device/status", HTTP_GET, handleControllerStatus);
    _server.on("/api/v1/device/command", HTTP_POST, handleControllerCommand);
    _server.on("/api/v1/pump/status", HTTP_GET, handleControllerStatus);
    _server.on("/api/v1/pump/command", HTTP_POST, handleControllerCommand);

    // Hardware Pin Mapping (Global, Board-Agnostic, and Persistent)
    _server.on("/api/v1/hardware/pins", HTTP_GET, handleGetHardwarePins);
    _server.on("/api/v1/hardware/pins", HTTP_POST, handleSetHardwarePins);

    // System Telemetry & Maintenance
    _server.on("/api/v1/system/status", HTTP_GET, handleStatus);
    _server.on("/api/v1/system/reboot", HTTP_POST, handleReboot);
    _server.on("/api/v1/system/name", HTTP_GET, handleGetDeviceName);
    _server.on("/api/v1/system/name", HTTP_POST, handleSetDeviceName);
    _server.on("/api/v1/device/name", HTTP_GET, handleGetDeviceName);
    _server.on("/api/v1/device/name", HTTP_POST, handleSetDeviceName);
    _server.on("/api/v1/ota/check", HTTP_POST, handleTriggerUpdate);

    // WiFi Networking & Provisioning
    _server.on("/api/v1/wifi/scan", HTTP_GET, handleWifiScan);
    _server.on("/api/v1/wifi/config", HTTP_POST, handleWifiConfig);
    _server.on("/api/v1/network/ip", HTTP_POST, handleIpConfig);
    _server.on("/api/v1/wifi/ip", HTTP_POST, handleIpConfig);
    _server.on("/api/v1/wifi/ap", HTTP_GET, handleGetAPConfig);
    _server.on("/api/v1/wifi/ap", HTTP_POST, handleSetAPConfig);
    _server.on("/api/v1/wifi/ap-config", HTTP_POST, handleSetAPConfig);
    _server.on("/api/v1/github/config", HTTP_POST, handleGitHubConfig);

    // Security & Key Ratchet
    _server.on("/api/v1/security/rotate-key", HTTP_POST, handleRotateKey);
    _server.on("/api/v1/security/update-token", HTTP_POST, handleUpdateToken);
    _server.on("/api/v1/security/update-master-key", HTTP_POST, handleUpdateToken);
    _server.on("/api/v1/security/seed-key", HTTP_POST, handleSeedKey);

#if !defined(ESP8266)
    // -----------------------------------------------------------------
    // Backwards-Compatible Aliases (ESP32 only)
    // -----------------------------------------------------------------
    _server.on("/api/status", HTTP_GET, handleStatus);
    _server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);
    _server.on("/api/wifi", HTTP_POST, handleWifiConfig);
    _server.on("/api/wifi/ap", HTTP_GET, handleGetAPConfig);
    _server.on("/api/wifi/ap", HTTP_POST, handleSetAPConfig);
    _server.on("/api/name", HTTP_GET, handleGetDeviceName);
    _server.on("/api/name", HTTP_POST, handleSetDeviceName);
    _server.on("/api/github", HTTP_POST, handleGitHubConfig);
    _server.on("/api/update", HTTP_POST, handleTriggerUpdate);
    _server.on("/api/key/rotate", HTTP_POST, handleRotateKey);
    _server.on("/api/reboot", HTTP_POST, handleReboot);
    _server.on("/api/controller/status", HTTP_GET, handleControllerStatus);
    _server.on("/api/controller/command", HTTP_POST, handleControllerCommand);
    _server.on("/api/hardware/pins", HTTP_GET, handleGetHardwarePins);
    _server.on("/api/hardware/pins", HTTP_POST, handleSetHardwarePins);
#endif

    // Pre-flight CORS and fallback 404
    _server.onNotFound(handleNotFound);
}

void RestApi::handleStatus() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    DeviceConfig& cfg = ConfigStore::get();
    const HardwareFingerprint& fp = ModuleDetector::getFingerprint();

    String data = "{";
    data += "\"system\":{";
    data += "\"status\":\"online\",";
    data += "\"name\":\"" + String(cfg.deviceName) + "\",";
    data += "\"coreVersion\":\"" + String(CRAFT_CORE_VERSION) + "\",";
#ifdef APP_VERSION
    data += "\"appVersion\":\"" + String(APP_VERSION) + "\",";
#endif
    data += "\"boardTarget\":\"" + String(fp.archTag) + "\",";
    data += "\"chip\":\"" + String(fp.chipName) + "\",";
    data += "\"flashBytes\":" + String(fp.flashSizeBytes) + ",";
    data += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    data += "\"mac\":\"" + fp.macAddress + "\"";
    data += "},";

    String staIpStr = WiFiManager::getIPAddress();
    String apIpStr = WiFiManager::getAPIPAddress();
    bool connected = WiFiManager::isConnected();
    String mainIp = connected ? staIpStr : (apIpStr != "--" ? apIpStr : "192.168.4.1");

    data += "\"network\":{";
    data += "\"state\":\"" + String(connected ? "connected" : (WiFiManager::getState() == WIFI_STATE_CONNECTING ? "connecting" : "ap_mode")) + "\",";
    data += "\"ip\":\"" + mainIp + "\",";
    data += "\"staIp\":\"" + staIpStr + "\",";
    data += "\"apIp\":\"" + apIpStr + "\",";
    data += "\"gateway\":\"" + WiFiManager::getGatewayIP() + "\",";
    data += "\"subnet\":\"" + WiFiManager::getSubnetMask() + "\",";
    data += "\"dns\":\"" + WiFiManager::getDnsIP() + "\",";
    data += "\"useStatic\":" + String(cfg.wifiUseStaticIp ? "true" : "false") + ",";
    data += "\"staticIp\":\"" + String(cfg.wifiStaticIp) + "\",";
    data += "\"ssid\":\"" + String(cfg.wifiSsid) + "\",";
    data += "\"apSsid\":\"" + WiFiManager::getApSSID() + "\",";
    data += "\"apSecured\":" + String(WiFiManager::isAPPasswordSet() ? "true" : "false");
    data += "},";

    data += "\"ota\":{";
    data += "\"status\":\"" + GitHubUpdater::getStatusString() + "\",";
    data += "\"org\":\"" + String(cfg.githubOrg) + "\",";
    data += "\"coreRepo\":\"" + String(cfg.coreRepo) + "\",";
    data += "\"controllerRepo\":\"" + String(cfg.controllerRepo) + "\",";
    data += "\"activeRole\":\"" + String(cfg.activeRole) + "\"";
    data += "},";

    data += "\"security\":{";
    data += "\"rollingSequence\":" + String(cfg.rollingSequence) + ",";
    data += "\"currentRollingKeyPrefix\":\"" + String(cfg.currentRollingKey).substring(0, 8) + "...\"";
    data += "}";

    IDeviceController* ctrl = CraftThingyCore.getController();
    if (ctrl != nullptr) {
        data += ",\"device\":" + ctrl->getStatusJson();
    }

    data += "}";

    sendStandardResponse(200, data);
}

void RestApi::handleControllerStatus() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    IDeviceController* ctrl = CraftThingyCore.getController();
    if (ctrl != nullptr) {
        sendStandardResponse(200, ctrl->getStatusJson());
    } else {
        sendStandardError(404, "No active hardware controller attached");
    }
}

void RestApi::handleControllerCommand() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    IDeviceController* ctrl = CraftThingyCore.getController();
    if (ctrl == nullptr) {
        sendStandardError(404, "No active hardware controller attached");
        return;
    }

    String cmd = _server.hasArg("cmd") ? _server.arg("cmd") : "";
    String payload = _server.hasArg("payload") ? _server.arg("payload") : "";

    if (cmd.length() == 0 && _server.hasArg("plain")) {
        String body = _server.arg("plain");
        int idx = body.indexOf("\"cmd\"");
        if (idx < 0) idx = body.indexOf("\"command\"");
        if (idx >= 0) {
            int start = body.indexOf(':', idx) + 1;
            int q1 = body.indexOf('"', start);
            int q2 = body.indexOf('"', q1 + 1);
            if (q1 >= 0 && q2 > q1) cmd = body.substring(q1 + 1, q2);
        }
        int payloadIdx = body.indexOf("\"payload\"");
        if (payloadIdx >= 0) {
            int start = body.indexOf(':', payloadIdx) + 1;
            int q1 = body.indexOf('"', start);
            int q2 = body.indexOf('"', q1 + 1);
            if (q1 >= 0 && q2 > q1) payload = body.substring(q1 + 1, q2);
        }
    }

    cmd.toLowerCase();
    String response;
    if (ctrl->handleCommand(cmd, payload, response)) {
        sendStandardSuccess(response);
    } else {
        sendStandardError(400, "Unknown command or execution failed", cmd);
    }
}

void RestApi::handleWifiScan() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    String scanResult = WiFiManager::scanNetworksJson();
    sendStandardResponse(200, scanResult);
}

void RestApi::handleWifiConfig() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    String ssid = _server.hasArg("ssid") ? _server.arg("ssid") : "";
    String pass = _server.hasArg("pass") ? _server.arg("pass") : "";
    bool useStatic = false;
    String staticIp = "";
    String gateway = "";
    String subnet = "255.255.255.0";
    String dns1 = "";
    String dns2 = "";

    if (_server.hasArg("use_static")) {
        useStatic = (_server.arg("use_static") == "true" || _server.arg("use_static") == "1");
    } else if (_server.hasArg("useStatic")) {
        useStatic = (_server.arg("useStatic") == "true" || _server.arg("useStatic") == "1");
    }
    if (_server.hasArg("static_ip")) staticIp = _server.arg("static_ip");
    else if (_server.hasArg("staticIp")) staticIp = _server.arg("staticIp");
    if (_server.hasArg("gateway")) gateway = _server.arg("gateway");
    if (_server.hasArg("subnet")) subnet = _server.arg("subnet");
    if (_server.hasArg("dns1")) dns1 = _server.arg("dns1");
    if (_server.hasArg("dns2")) dns2 = _server.arg("dns2");

    if (_server.hasArg("plain")) {
        String body = _server.arg("plain");
        auto extractJson = [&](const String& key) -> String {
            int idx = body.indexOf("\"" + key + "\"");
            if (idx < 0) return "";
            int start = body.indexOf(':', idx) + 1;
            int q1 = body.indexOf('"', start);
            int q2 = body.indexOf('"', q1 + 1);
            if (q1 >= 0 && q2 > q1) return body.substring(q1 + 1, q2);
            return "";
        };

        if (ssid.length() == 0) ssid = extractJson("ssid");
        if (pass.length() == 0) pass = extractJson("pass");
        if (!useStatic && (body.indexOf("\"useStatic\":true") >= 0 || body.indexOf("\"use_static\":true") >= 0)) {
            useStatic = true;
        }
        if (staticIp.length() == 0) staticIp = extractJson("staticIp");
        if (staticIp.length() == 0) staticIp = extractJson("static_ip");
        if (gateway.length() == 0) gateway = extractJson("gateway");
        if (subnet.length() == 0 || subnet == "255.255.255.0") {
            String sub = extractJson("subnet");
            if (sub.length() > 0) subnet = sub;
        }
        if (dns1.length() == 0) dns1 = extractJson("dns1");
        if (dns2.length() == 0) dns2 = extractJson("dns2");
    }

    ssid.trim();
    DeviceConfig& cfg = ConfigStore::get();

    // If SSID was not passed in request, reuse existing saved network credentials
    if (ssid.length() == 0 && strlen(cfg.wifiSsid) > 0) {
        ssid = String(cfg.wifiSsid);
    }
    if (pass.length() == 0 && strlen(cfg.wifiPassword) > 0) {
        pass = String(cfg.wifiPassword);
    }

    if (ssid.length() == 0) {
        sendStandardError(400, "Missing required 'ssid' parameter (no saved Wi-Fi network found)");
        return;
    }

    staticIp.trim();
    gateway.trim();
    subnet.trim();
    dns1.trim();
    dns2.trim();

    ConfigStore::setWiFiConfig(ssid, pass, useStatic, staticIp, gateway, subnet, dns1, dns2);
    sendStandardSuccess("WiFi credentials saved. Connecting in background...");

    WiFiManager::connectSTA(ssid.c_str(), pass.c_str(), CRAFT_WIFI_MAX_RETRIES);
}

void RestApi::handleIpConfig() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    bool useStatic = false;
    String staticIp = "";
    String gateway = "";
    String subnet = "255.255.255.0";
    String dns1 = "";
    String dns2 = "";

    if (_server.hasArg("use_static")) {
        useStatic = (_server.arg("use_static") == "true" || _server.arg("use_static") == "1");
    } else if (_server.hasArg("useStatic")) {
        useStatic = (_server.arg("useStatic") == "true" || _server.arg("useStatic") == "1");
    }
    if (_server.hasArg("static_ip")) staticIp = _server.arg("static_ip");
    else if (_server.hasArg("staticIp")) staticIp = _server.arg("staticIp");
    if (_server.hasArg("gateway")) gateway = _server.arg("gateway");
    if (_server.hasArg("subnet")) subnet = _server.arg("subnet");
    if (_server.hasArg("dns1")) dns1 = _server.arg("dns1");
    if (_server.hasArg("dns2")) dns2 = _server.arg("dns2");

    if (_server.hasArg("plain")) {
        String body = _server.arg("plain");
        auto extractJson = [&](const String& key) -> String {
            int idx = body.indexOf("\"" + key + "\"");
            if (idx < 0) return "";
            int start = body.indexOf(':', idx) + 1;
            int q1 = body.indexOf('"', start);
            int q2 = body.indexOf('"', q1 + 1);
            if (q1 >= 0 && q2 > q1) return body.substring(q1 + 1, q2);
            return "";
        };

        if (!useStatic && (body.indexOf("\"useStatic\":true") >= 0 || body.indexOf("\"use_static\":true") >= 0)) {
            useStatic = true;
        }
        if (staticIp.length() == 0) staticIp = extractJson("staticIp");
        if (staticIp.length() == 0) staticIp = extractJson("static_ip");
        if (gateway.length() == 0) gateway = extractJson("gateway");
        if (subnet.length() == 0 || subnet == "255.255.255.0") {
            String sub = extractJson("subnet");
            if (sub.length() > 0) subnet = sub;
        }
        if (dns1.length() == 0) dns1 = extractJson("dns1");
        if (dns2.length() == 0) dns2 = extractJson("dns2");
    }

    staticIp.trim();
    gateway.trim();
    subnet.trim();
    dns1.trim();
    dns2.trim();

    if (useStatic && staticIp.length() == 0) {
        sendStandardError(400, "Missing required 'staticIp' parameter for static IP configuration");
        return;
    }

    ConfigStore::setIpConfig(useStatic, staticIp, gateway, subnet, dns1, dns2);
    sendStandardSuccess("IP configuration updated. Reapplying network settings in background...");

    WiFiManager::applyIpConfig();
}

void RestApi::handleGitHubConfig() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    String org = _server.hasArg("org") ? _server.arg("org") : "";
    String coreRepo = _server.hasArg("core_repo") ? _server.arg("core_repo") : "";
    String ctrlRepo = _server.hasArg("controller_repo") ? _server.arg("controller_repo") : "";
    String role = _server.hasArg("role") ? _server.arg("role") : "";
    String token = _server.hasArg("token") ? _server.arg("token") : "";

    if (_server.hasArg("plain")) {
        String body = _server.arg("plain");
        auto extractJson = [&](const String& key) -> String {
            int idx = body.indexOf("\"" + key + "\"");
            if (idx < 0) return "";
            int start = body.indexOf(':', idx) + 1;
            int q1 = body.indexOf('"', start);
            int q2 = body.indexOf('"', q1 + 1);
            if (q1 >= 0 && q2 > q1) return body.substring(q1 + 1, q2);
            return "";
        };

        if (org.length() == 0) org = extractJson("org");
        if (coreRepo.length() == 0) coreRepo = extractJson("core_repo");
        if (ctrlRepo.length() == 0) ctrlRepo = extractJson("controller_repo");
        if (role.length() == 0) role = extractJson("role");
        if (token.length() == 0) token = extractJson("token");
    }

    ConfigStore::setGitHubConfig(org, coreRepo, ctrlRepo, token);
    if (role.length() > 0) {
        ConfigStore::setActiveRole(role);
    }

    // Automatically advance rolling key ratchet for forward secrecy
    ConfigStore::rotateRollingKey();

    DeviceConfig& cfg = ConfigStore::get();
    String data = "{";
    data += "\"message\":\"GitHub target updated and rolling key ratcheted\",";
    data += "\"newRollingSequence\":" + String(cfg.rollingSequence) + ",";
    data += "\"newRollingKey\":\"" + String(cfg.currentRollingKey) + "\"";
    data += "}";

    sendStandardResponse(200, data);
}

void RestApi::handleTriggerUpdate() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    GitHubUpdater::triggerCheck();
    sendStandardSuccess("GitHub OTA update check triggered");
}

void RestApi::handleRotateKey() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    if (ConfigStore::rotateRollingKey()) {
        DeviceConfig& cfg = ConfigStore::get();
        String data = "{";
        data += "\"message\":\"Rolling key advanced to next sequence\",";
        data += "\"newRollingSequence\":" + String(cfg.rollingSequence) + ",";
        data += "\"newRollingKey\":\"" + String(cfg.currentRollingKey) + "\"";
        data += "}";
        sendStandardResponse(200, data);
    } else {
        sendStandardError(500, "Failed to rotate rolling key");
    }
}

void RestApi::handleUpdateToken() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    DeviceConfig& cfg = ConfigStore::get();

    // -------------------------------------------------------------
    // Master Key Protection Gate:
    // To change the Master Key, caller MUST prove ownership of the
    // CURRENT Master Secret Key! (Prevents a daily rolling key from
    // hijacking the root administrator master key).
    // -------------------------------------------------------------
    String currentMaster = _server.header("X-Master-Key");
    if (currentMaster.length() == 0 && _server.hasArg("current_master_key")) {
        currentMaster = _server.arg("current_master_key");
    }
    if (currentMaster.length() == 0 && _server.hasArg("current_key")) {
        currentMaster = _server.arg("current_key");
    }

    String body = _server.hasArg("plain") ? _server.arg("plain") : "";
    auto extractJson = [&](const String& key) -> String {
        int idx = body.indexOf("\"" + key + "\"");
        if (idx < 0) return "";
        int start = body.indexOf(':', idx) + 1;
        int q1 = body.indexOf('"', start);
        int q2 = body.indexOf('"', q1 + 1);
        if (q1 >= 0 && q2 > q1) return body.substring(q1 + 1, q2);
        return "";
    };

    if (currentMaster.length() == 0 && body.length() > 0) {
        currentMaster = extractJson("current_master_key");
        if (currentMaster.length() == 0) currentMaster = extractJson("currentMasterKey");
        if (currentMaster.length() == 0) currentMaster = extractJson("current_key");
    }

    bool masterVerified = false;
    if (currentMaster.length() > 0 && CraftThingyCrypto::constantTimeEquals(currentMaster, String(cfg.masterSecretKey))) {
        masterVerified = true;
    } else if (authMethod == "hmac_master_key") {
        masterVerified = true;
    } else {
        String authHeader = _server.header("Authorization");
        if (authHeader.startsWith("Bearer ") || authHeader.startsWith("bearer ")) {
            String token = authHeader.substring(7);
            token.trim();
            if (CraftThingyCrypto::constantTimeEquals(token, String(cfg.masterSecretKey))) {
                masterVerified = true;
            }
        }
    }

    if (!masterVerified) {
        sendStandardError(403, "Forbidden: Changing Master Secret Key requires proof of CURRENT Master Secret Key ('current_master_key' or X-Master-Key header)");
        return;
    }

    String newToken = _server.hasArg("token") ? _server.arg("token") : 
                     (_server.hasArg("new_token") ? _server.arg("new_token") : 
                     (_server.hasArg("key") ? _server.arg("key") : 
                     (_server.hasArg("new_master_key") ? _server.arg("new_master_key") : "")));

    if (newToken.length() == 0 && body.length() > 0) {
        newToken = extractJson("token");
        if (newToken.length() == 0) newToken = extractJson("new_token");
        if (newToken.length() == 0) newToken = extractJson("newKey");
        if (newToken.length() == 0) newToken = extractJson("new_master_key");
        if (newToken.length() == 0) newToken = extractJson("key");
    }

    newToken.trim();
    if (newToken.length() < 16) {
        sendStandardError(400, "Token too short: Must be at least 16 characters for security");
        return;
    }

    if (ConfigStore::setMasterSecretKey(newToken)) {
        String data = "{";
        data += "\"message\":\"Master Secret Key updated successfully\",";
        data += "\"tokenPrefix\":\"" + newToken.substring(0, 4) + "...\"";
        data += "}";
        sendStandardResponse(200, data);
    } else {
        sendStandardError(500, "Failed to persist new master token to storage");
    }
}

void RestApi::handleSeedKey() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    DeviceConfig& cfg = ConfigStore::get();
    String newSeed = _server.hasArg("seed") ? _server.arg("seed") : 
                    (_server.hasArg("key") ? _server.arg("key") : "");
    String seqStr = _server.hasArg("sequence") ? _server.arg("sequence") : 
                   (_server.hasArg("seq") ? _server.arg("seq") : "1");
    bool syncMaster = true;
    if (_server.hasArg("sync_master")) {
        syncMaster = (_server.arg("sync_master") == "true" || _server.arg("sync_master") == "1");
    }

    String body = _server.hasArg("plain") ? _server.arg("plain") : "";
    auto extractJson = [&](const String& key) -> String {
        int idx = body.indexOf("\"" + key + "\"");
        if (idx < 0) return "";
        int start = body.indexOf(':', idx) + 1;
        int q1 = body.indexOf('"', start);
        int q2 = body.indexOf('"', q1 + 1);
        if (q1 >= 0 && q2 > q1) return body.substring(q1 + 1, q2);
        return "";
    };

    if (newSeed.length() == 0 && body.length() > 0) {
        newSeed = extractJson("seed");
        if (newSeed.length() == 0) newSeed = extractJson("key");
        String s = extractJson("sequence");
        if (s.length() > 0) seqStr = s;
        if (body.indexOf("\"sync_master\":false") >= 0 || body.indexOf("\"syncMaster\":false") >= 0) {
            syncMaster = false;
        }
    }

    // Guard Master key sync: Only allow syncing master if current master key is verified
    if (syncMaster) {
        String currentMaster = _server.header("X-Master-Key");
        if (currentMaster.length() == 0 && _server.hasArg("current_master_key")) {
            currentMaster = _server.arg("current_master_key");
        }
        if (currentMaster.length() == 0 && body.length() > 0) {
            currentMaster = extractJson("current_master_key");
            if (currentMaster.length() == 0) currentMaster = extractJson("currentMasterKey");
        }
        bool masterVerified = false;
        if (currentMaster.length() > 0 && CraftThingyCrypto::constantTimeEquals(currentMaster, String(cfg.masterSecretKey))) {
            masterVerified = true;
        } else if (authMethod == "hmac_master_key") {
            masterVerified = true;
        } else {
            String authHeader = _server.header("Authorization");
            if (authHeader.startsWith("Bearer ") || authHeader.startsWith("bearer ")) {
                String token = authHeader.substring(7);
                token.trim();
                if (CraftThingyCrypto::constantTimeEquals(token, String(cfg.masterSecretKey))) {
                    masterVerified = true;
                }
            }
        }
        if (!masterVerified) {
            syncMaster = false; // Gracefully downgrade syncMaster so rolling ratchet re-seeds safely
        }
    }

    newSeed.trim();
    if (newSeed.length() < 16) {
        sendStandardError(400, "Seed too short: Must be at least 16 characters (recommended 64-hex characters)");
        return;
    }

    uint32_t startingSeq = seqStr.toInt();
    if (startingSeq == 0) startingSeq = 1;

    if (ConfigStore::setRollingKeySeed(newSeed, startingSeq, syncMaster)) {
        String data = "{";
        data += "\"message\":\"Rolling key ratchet re-seeded successfully\",";
        data += "\"startingSequence\":" + String(startingSeq) + ",";
        data += "\"syncedMaster\":" + String(syncMaster ? "true" : "false") + ",";
        data += "\"keyPrefix\":\"" + newSeed.substring(0, 8) + "...\"";
        data += "}";
        sendStandardResponse(200, data);
    } else {
        sendStandardError(500, "Failed to persist new rolling seed to flash storage");
    }
}

void RestApi::handleGetDeviceName() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    DeviceConfig& cfg = ConfigStore::get();
    const HardwareFingerprint& fp = ModuleDetector::getFingerprint();

    String data = "{";
    data += "\"deviceName\":\"" + String(cfg.deviceName) + "\",";
    data += "\"activeRole\":\"" + String(cfg.activeRole) + "\",";
    data += "\"mac\":\"" + fp.macAddress + "\",";
    data += "\"boardTarget\":\"" + String(fp.archTag) + "\"";
    data += "}";

    sendStandardResponse(200, data);
}

void RestApi::handleSetDeviceName() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    String name = _server.hasArg("name") ? _server.arg("name") : "";
    if (_server.hasArg("plain")) {
        String body = _server.arg("plain");
        int idx = body.indexOf("\"name\"");
        if (idx >= 0) {
            int start = body.indexOf(':', idx) + 1;
            int q1 = body.indexOf('"', start);
            int q2 = body.indexOf('"', q1 + 1);
            if (q1 >= 0 && q2 > q1) name = body.substring(q1 + 1, q2);
        }
    }

    name.trim();
    if (name.length() == 0 || name.length() > 31) {
        sendStandardError(400, "Device name must be between 1 and 31 characters");
        return;
    }

    if (ConfigStore::setDeviceName(name)) {
        WiFiManager::setDeviceName(name);
        String data = "{";
        data += "\"message\":\"Device internal identification name updated successfully\",";
        data += "\"deviceName\":\"" + name + "\"";
        data += "}";
        sendStandardResponse(200, data);
    } else {
        sendStandardError(500, "Failed to save device name");
    }
}

void RestApi::handleGetAPConfig() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    DeviceConfig& cfg = ConfigStore::get();
    String data = "{";
    data += "\"apSsid\":\"" + WiFiManager::getApSSID() + "\",";
    data += "\"isCustomSsid\":" + String(strlen(cfg.apSsid) > 0 ? "true" : "false") + ",";
    data += "\"isPasswordProtected\":" + String(WiFiManager::isAPPasswordSet() ? "true" : "false") + ",";
    data += "\"apIP\":\"" + WiFiManager::getAPIPAddress() + "\"";
    data += "}";

    sendStandardResponse(200, data);
}

void RestApi::handleSetAPConfig() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    String ssid = _server.hasArg("ssid") ? _server.arg("ssid") : "";
    String pass = _server.hasArg("pass") ? _server.arg("pass") : 
                 (_server.hasArg("password") ? _server.arg("password") : "");

    if (_server.hasArg("plain")) {
        String body = _server.arg("plain");
        auto extractJson = [&](const String& key) -> String {
            int idx = body.indexOf("\"" + key + "\"");
            if (idx < 0) return "";
            int start = body.indexOf(':', idx) + 1;
            int q1 = body.indexOf('"', start);
            int q2 = body.indexOf('"', q1 + 1);
            if (q1 >= 0 && q2 > q1) return body.substring(q1 + 1, q2);
            return "";
        };

        if (ssid.length() == 0) ssid = extractJson("ssid");
        if (pass.length() == 0) pass = extractJson("pass");
        if (pass.length() == 0) pass = extractJson("password");
    }

    ssid.trim();
    pass.trim();

    if (pass.length() > 0 && pass.length() < 8) {
        sendStandardError(400, "WiFi AP password must be at least 8 characters for WPA2, or empty for open network");
        return;
    }

    if (ConfigStore::setAPConfig(ssid, pass)) {
        WiFiManager::configureAP(ssid, pass);
        String data = "{";
        data += "\"message\":\"WiFi fallback AP hotspot configuration updated successfully\",";
        data += "\"apSsid\":\"" + WiFiManager::getApSSID() + "\",";
        data += "\"isPasswordProtected\":" + String(pass.length() >= 8 ? "true" : "false");
        data += "}";
        sendStandardResponse(200, data);
    } else {
        sendStandardError(500, "Failed to persist AP configuration");
    }
}

void RestApi::handleReboot() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    sendStandardSuccess("Device restarting...");
    delay(500);
    ESP.restart();
}

void RestApi::handleRootPortal() {
    IDeviceController* ctrl = CraftThingyCore.getController();
    if (ctrl != nullptr && ctrl->handleHttpRequest(_server, "/", "GET")) {
        return;
    }

    _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server.sendHeader("Pragma", "no-cache");
    _server.sendHeader("Expires", "-1");
    _server.sendHeader("Content-Encoding", "gzip");
    _server.send_P(200, "text/html", (const char*)CAPTIVE_PORTAL_HTML_GZ, sizeof(CAPTIVE_PORTAL_HTML_GZ));
}

void RestApi::handleSetupPost() {
    String ssid = _server.hasArg("ssid") ? _server.arg("ssid") : "";
    String pass = _server.hasArg("pass") ? _server.arg("pass") : "";
    String name = _server.hasArg("name") ? _server.arg("name") : "";
    String apSsid = _server.hasArg("ap_ssid") ? _server.arg("ap_ssid") : "";
    String apPass = _server.hasArg("ap_pass") ? _server.arg("ap_pass") : "";
    bool useStatic = false;
    String staticIp = "";
    String gateway = "";
    String subnet = "255.255.255.0";
    String dns1 = "";
    String dns2 = "";

    if (_server.hasArg("use_static")) {
        useStatic = (_server.arg("use_static") == "true" || _server.arg("use_static") == "1");
    } else if (_server.hasArg("useStatic")) {
        useStatic = (_server.arg("useStatic") == "true" || _server.arg("useStatic") == "1");
    }
    if (_server.hasArg("static_ip")) staticIp = _server.arg("static_ip");
    else if (_server.hasArg("staticIp")) staticIp = _server.arg("staticIp");
    if (_server.hasArg("gateway")) gateway = _server.arg("gateway");
    if (_server.hasArg("subnet")) subnet = _server.arg("subnet");
    if (_server.hasArg("dns1")) dns1 = _server.arg("dns1");
    if (_server.hasArg("dns2")) dns2 = _server.arg("dns2");

    if (ssid.length() == 0 && _server.hasArg("plain")) {
        String body = _server.arg("plain");
        auto extractJson = [&](const String& key) -> String {
            int idx = body.indexOf("\"" + key + "\"");
            if (idx < 0) return "";
            int start = body.indexOf(':', idx) + 1;
            int q1 = body.indexOf('"', start);
            int q2 = body.indexOf('"', q1 + 1);
            if (q1 >= 0 && q2 > q1) return body.substring(q1 + 1, q2);
            return "";
        };

        if (ssid.length() == 0) ssid = extractJson("ssid");
        if (pass.length() == 0) pass = extractJson("pass");
        if (name.length() == 0) name = extractJson("name");
        if (apSsid.length() == 0) apSsid = extractJson("ap_ssid");
        if (apPass.length() == 0) apPass = extractJson("ap_pass");
        if (!useStatic && (body.indexOf("\"useStatic\":true") >= 0 || body.indexOf("\"use_static\":true") >= 0)) {
            useStatic = true;
        }
        if (staticIp.length() == 0) staticIp = extractJson("staticIp");
        if (staticIp.length() == 0) staticIp = extractJson("static_ip");
        if (gateway.length() == 0) gateway = extractJson("gateway");
        if (subnet.length() == 0 || subnet == "255.255.255.0") {
            String sub = extractJson("subnet");
            if (sub.length() > 0) subnet = sub;
        }
        if (dns1.length() == 0) dns1 = extractJson("dns1");
        if (dns2.length() == 0) dns2 = extractJson("dns2");
    }

    ssid.trim();
    if (ssid.length() == 0) {
        sendStandardError(400, "Missing required 'ssid' parameter");
        return;
    }

    name.trim();
    if (name.length() > 0) {
        ConfigStore::setDeviceName(name);
        WiFiManager::setDeviceName(name);
    }

    apSsid.trim();
    apPass.trim();
    if (apSsid.length() > 0 || apPass.length() > 0) {
        if (apPass.length() > 0 && apPass.length() < 8) {
            sendStandardError(400, "AP password must be at least 8 characters");
            return;
        }
        ConfigStore::setAPConfig(apSsid, apPass);
        WiFiManager::configureAP(apSsid, apPass);
    }

    staticIp.trim();
    gateway.trim();
    subnet.trim();
    dns1.trim();
    dns2.trim();

    ConfigStore::setWiFiConfig(ssid, pass, useStatic, staticIp, gateway, subnet, dns1, dns2);
    sendStandardSuccess("Configuration saved! ESP connecting to: " + ssid);

    WiFiManager::connectSTA(ssid.c_str(), pass.c_str(), CRAFT_WIFI_MAX_RETRIES);
}

void RestApi::handleGetHardwarePins() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    sendStandardResponse(200, PinManager::getHardwarePinsJson());
}

void RestApi::handleSetHardwarePins() {
    String authMethod;
    if (!isRequestAuthorized(_server, authMethod)) {
        sendStandardError(401, "Unauthorized: Valid HMAC signature or Bearer token required", authMethod);
        return;
    }

    String body = _server.arg("plain");
    String err;
    if (!PinManager::applyPinsFromJson(body, err)) {
        sendStandardError(400, "Failed to apply hardware pins: " + err);
        return;
    }

    // Notify active controller to reinitialize pins immediately
    IDeviceController* ctrl = CraftThingyCore.getController();
    if (ctrl != nullptr) {
        ctrl->setup();
    }

    sendStandardResponse(200, "{\"message\":\"Hardware pins successfully updated and persisted across OTA updates\",\"pins\":" + PinManager::getHardwarePinsJson() + "}");
}

void RestApi::handleNotFound() {
    if (_server.method() == HTTP_OPTIONS) {
        _server.sendHeader("Access-Control-Allow-Origin", "*");
        _server.sendHeader("Access-Control-Allow-Private-Network", "true");
        _server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        _server.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Signature, X-Timestamp, X-Nonce, X-Sequence, Authorization, X-Master-Key");
        _server.send(204);
        return;
    }

    IDeviceController* ctrl = CraftThingyCore.getController();
    String methodStr = (_server.method() == HTTP_GET) ? "GET" : ((_server.method() == HTTP_POST) ? "POST" : "OTHER");
    if (ctrl != nullptr && ctrl->handleHttpRequest(_server, _server.uri(), methodStr)) {
        return;
    }

    // In SoftAP mode, intercept unmatched captive portal requests (e.g. Apple / Android / Windows) and redirect to setup portal
    if (WiFiManager::isAPMode()) {
        String uri = _server.uri();
        if (!uri.startsWith("/api/")) {
            _server.sendHeader("Location", "http://192.168.4.1/");
            _server.send(302, "text/plain", "");
            return;
        }
    }

    sendStandardError(404, "Endpoint not found", _server.uri());
}
