/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <functional>
#include <LittleFS.h>
#include "Builtin_Pages.h"

class HTTPRoutes {
public:
    using HandlerFunction = std::function<void()>;
    
    // Static handler functions that can be used directly with WebServer
    static HandlerFunction handleIndexFile;
    static HandlerFunction handleBTScanner;
    static HandlerFunction handleLittleFSFile;
    static HandlerFunction handleConfigJSON;
    static HandlerFunction handleRuntimeConfigJSON;
    static HandlerFunction handlePWCJSON;
    static HandlerFunction handleShift;
    static HandlerFunction handleHRSlider;
    static HandlerFunction handleWattsSlider;
    static HandlerFunction handleCadSlider;
    static HandlerFunction handleERGMode;
    static HandlerFunction handleTargetWattsSlider;
    static HandlerFunction handleLogin;
    static HandlerFunction handleOTAUpdate;
    static HandlerFunction handleFileUpload;
    
    // Setup function to register all routes
    static void setupRoutes(WebServer& server);
    
    // Initialize handlers with server reference
    static void initialize(WebServer& server);

private:
    static void setupDefaultRoutes(WebServer& server);
    static void setupFileRoutes(WebServer& server);
    static void setupControlRoutes(WebServer& server);
    static void setupUpdateRoutes(WebServer& server);
    
    // Store WebServer reference for handlers
    static WebServer* currentServer;
    static File fsUploadFile;
    
    // Actual handler implementations
    static void _handleIndexFile();
    static void _handleBTScanner();
    static void _handleLittleFSFile();
    static void _handleConfigJSON();
    static void _handleRuntimeConfigJSON();
    static void _handlePWCJSON();
    static void _handleShift();
    static void _handleHRSlider();
    static void _handleWattsSlider();
    static void _handleCadSlider();
    static void _handleERGMode();
    static void _handleTargetWattsSlider();
    static void _handleLogin();
    static void _handleOTAUpdate();
    static void _handleFileUpload();
};
