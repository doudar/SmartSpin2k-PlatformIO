/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "http/HTTPCore.h"
#include "Main.h"
#include "SS2KLog.h"
#include <ESPmDNS.h>

// Initialize the global instance
HTTPCore httpServer;

HTTPCore::HTTPCore() 
    : internetConnection(false)
    , server(80)
    , lastUpdateTime(0)
    , lastMDNSUpdate(0) {
}

void HTTPCore::start() {
    server.enableCORS(true);
    
    // Setup all routes through HTTPRoutes
    HTTPRoutes::setupRoutes(server);
    
    server.begin();
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "HTTP server started");
}

void HTTPCore::stop() {
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Stopping Http Server");
    server.stop();
    server.close();
}

void HTTPCore::update() {
    unsigned long currentTime = millis();
    
    // Handle client requests with rate limiting
    if (currentTime - lastUpdateTime > WEBSERVER_DELAY) {
        lastUpdateTime = currentTime;
        server.handleClient();
        
        // Process DNS requests if in AP mode
        WiFiManager::processDNS();
        
        // Update MDNS periodically
        updateMDNS();
    }
}

void HTTPCore::updateMDNS() {
    unsigned long currentTime = millis();
    if (currentTime - lastMDNSUpdate > MDNS_UPDATE_INTERVAL) {
        MDNS.addServiceTxt("http", "_tcp", "lf", String(currentTime));
        lastMDNSUpdate = currentTime;
    }
}

bool HTTPCore::hasInternetConnection() const {
    return internetConnection;
}

void HTTPCore::setInternetConnection(bool connected) {
    internetConnection = connected;
}

WebServer& HTTPCore::getServer() {
    return server;
}
