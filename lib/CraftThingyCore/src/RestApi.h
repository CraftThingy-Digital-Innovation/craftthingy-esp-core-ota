#pragma once

#include <Arduino.h>
#include "CraftThingyConfig.h"

#if defined(ESP8266)
    #include <ESP8266WebServer.h>
    typedef ESP8266WebServer ApiServerType;
#elif defined(ESP32)
    #include <WebServer.h>
    typedef WebServer ApiServerType;
#endif

class RestApi {
public:
    static void init();
    static void loop();

    // Verify HMAC-SHA256 rolling key signature or Bearer/Basic Auth
    static bool isRequestAuthorized(ApiServerType& server, String& outMethod);

    // Standardized REST Response Helpers (Envelopes)
    static void sendStandardResponse(int httpCode, const String& dataJson);
    static void sendStandardError(int httpCode, const String& message, const String& reason = "");
    static void sendStandardSuccess(const String& message);

private:
    static ApiServerType _server;
    static void setupRoutes();

    // Standardized Endpoints
    static void handleStatus();
    static void handleWifiScan();
    static void handleWifiConfig();
    static void handleGitHubConfig();
    static void handleTriggerUpdate();
    static void handleRotateKey();
    static void handleReboot();
    static void handleControllerStatus();
    static void handleControllerCommand();
    static void handleNotFound();
};
