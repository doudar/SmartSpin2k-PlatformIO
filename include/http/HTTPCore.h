/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Update.h>
#include "http/HTTPRoutes.h"
#include "http/HTTPFileSystem.h"
#include "http/HTTPSettings.h"
#include "http/HTTPFirmware.h"
#include "wifi/WiFiManager.h"

#define HTTP_SERVER_LOG_TAG "HTTP_Server"

class HTTPCore {
public:
    HTTPCore();
    void start();
    void stop();
    void update();
    bool hasInternetConnection() const;
    void setInternetConnection(bool connected);
    WebServer& getServer();

private:
    bool internetConnection;
    WebServer server;
    unsigned long lastUpdateTime;
    unsigned long lastMDNSUpdate;
    
    void setupRoutes();
    void updateMDNS();
    static const unsigned long MDNS_UPDATE_INTERVAL = 30000; // 30 seconds
};

extern HTTPCore httpServer;
