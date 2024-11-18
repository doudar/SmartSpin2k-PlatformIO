/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "settings.h"

#define HTTP_SERVER_LOG_TAG "HTTP_Server"

class HTTPSettings {
public:
    static void processSettings(WebServer& server);
    
private:
    // Network settings
    static void processNetworkSettings(WebServer& server);
    static void processDeviceSettings(WebServer& server);
    static void processStepperSettings(WebServer& server);
    static void processPowerSettings(WebServer& server);
    static void processERGSettings(WebServer& server);
    static void processFeatureSettings(WebServer& server);
    static bool processBluetoothSettings(WebServer& server);  // Changed return type to bool
    static void processPWCSettings(WebServer& server);
    
    // Helper functions
    static bool processCheckbox(WebServer& server, const char* name, bool defaultValue = false);
    static float processFloatValue(WebServer& server, const char* name, float min, float max);
    static int processIntValue(WebServer& server, const char* name, int min, int max);
    static String processStringValue(WebServer& server, const char* name);
    
    // Response handling
    static void sendSettingsResponse(WebServer& server, bool wasBTUpdate, bool wasSettingsUpdate, bool requiresReboot = false);
    static String buildRedirectResponse(const String& message, const String& page, int delay = 1000);
};
