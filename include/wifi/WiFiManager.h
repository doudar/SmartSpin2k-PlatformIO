/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

class WiFiManager {
public:
    static void startWifi();
    static void stopWifi();
    static IPAddress getIP();
    static bool isConnected();
    static void processDNS();
    
private:
    static void setupStationMode();
    static void setupAPMode();
    static void setupMDNS();
    static void syncClock();

    static const byte DNS_PORT = 53;
    static const int WIFI_CONNECT_TIMEOUT = 20; // seconds
    
    static DNSServer dnsServer;
    static IPAddress myIP;
};
