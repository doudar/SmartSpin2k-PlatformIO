/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "wifi/WiFiManager.h"
#include "Main.h"
#include "SS2KLog.h"
#include <WiFiProv.h>

// Initialize static members
DNSServer WiFiManager::dnsServer;
IPAddress WiFiManager::myIP;

void WiFiManager::startWifi() {
    int attempts = 0;

    // Try Station mode first if SSID is not default
    if (strcmp(userConfig->getSsid(), DEVICE_NAME) != 0) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Connecting to: %s", userConfig->getSsid());
        setupStationMode();
        
        while (WiFi.status() != WL_CONNECTED) {
            vTaskDelay(1000 / portTICK_RATE_MS);
            SS2K_LOG(HTTP_SERVER_LOG_TAG, "Waiting for connection to be established...");
            attempts++;
            if (attempts > WIFI_CONNECT_TIMEOUT) {
                SS2K_LOG(HTTP_SERVER_LOG_TAG, "Couldn't Connect. Switching to AP mode");
                WiFi.disconnect(true, true);
                WiFi.setAutoReconnect(false);
                WiFi.mode(WIFI_MODE_NULL);
                vTaskDelay(1000 / portTICK_RATE_MS);
                break;
            }
        }
    }

    // If connected in STA mode, set IP and return
    if (WiFi.status() == WL_CONNECTED) {
        myIP = WiFi.localIP();
        setupMDNS();
        if (WiFi.getMode() == WIFI_STA) {
            syncClock();
        }
        return;
    }

    // Otherwise set up AP mode
    setupAPMode();
    
    if (strcmp(userConfig->getSsid(), DEVICE_NAME) == 0) {
        // If default SSID is still in use, let the user select a new password
        WiFi.softAP(userConfig->getDeviceName(), userConfig->getPassword());
    } else {
        WiFi.softAP(userConfig->getDeviceName(), DEFAULT_PASSWORD);
    }
    
    vTaskDelay(50);
    myIP = WiFi.softAPIP();
    
    // Setup DNS server for captive portal
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", myIP);
    
    setupMDNS();
    
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Connected to %s IP address: %s", 
             userConfig->getSsid(), myIP.toString().c_str());
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Open http://%s.local/", userConfig->getDeviceName());
    
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
}

void WiFiManager::stopWifi() {
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Closing connection to: %s", userConfig->getSsid());
    WiFi.disconnect();
}

void WiFiManager::setupStationMode() {
    WiFi.setHostname(userConfig->getDeviceName());
    WiFi.mode(WIFI_STA);
    WiFi.begin(userConfig->getSsid(), userConfig->getPassword());
    WiFi.setAutoReconnect(true);
}

void WiFiManager::setupAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.setHostname("reset");  // Fixes a bug when switching Arduino Core Versions
    WiFi.softAPsetHostname("reset");
    WiFi.setHostname(userConfig->getDeviceName());
    WiFi.softAPsetHostname(userConfig->getDeviceName());
    WiFi.enableAP(true);
    vTaskDelay(500);  // Microcontroller requires some time to reset the mode
}

void WiFiManager::setupMDNS() {
    if (!MDNS.begin(userConfig->getDeviceName())) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Error setting up MDNS responder!");
        return;
    }
    MDNS.addService("http", "_tcp", 80);
    MDNS.addServiceTxt("http", "_tcp", "lf", "0");
}

void WiFiManager::syncClock() {
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Syncing clock...");
    configTime(0, 0, "pool.ntp.org");  // get UTC time via NTP
    time_t now = time(nullptr);
    while (now < 10) {  // wait 10 seconds
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Waiting for clock sync...");
        delay(100);
        now = time(nullptr);
    }
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Clock synced to: %.f", difftime(now, (time_t)0));
}

void WiFiManager::processDNS() {
    if (WiFi.getMode() != WIFI_MODE_STA) {
        dnsServer.processNextRequest();
    }
}

IPAddress WiFiManager::getIP() {
    return myIP;
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}
