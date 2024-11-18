/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <LittleFS.h>
#include "settings.h"
#include "http/HTTPCore.h"

#define HTTP_SERVER_LOG_TAG "HTTP_Server"

// Forward declaration of Version class
class Version {
public:
    Version(const char* version);
    bool operator>(const Version& other) const;
private:
    int major;
    int minor;
    int patch;
    void parseVersion(const char* version);
};

class HTTPFirmware {
public:
    static void checkForUpdates();
    static void handleOTAUpdate(WebServer& server);
    
private:
    // Update components
    static void updateLittleFS();
    static void updateFirmware();
    static bool downloadFile(const String& url, const String& filename);
    
    // Version management
    static bool needsUpdate(const String& currentVersion, const String& availableVersion);
    static bool validateVersion(const String& version);
    
    // Security
    static void setupSecureClient(WiFiClientSecure& client);
    static const char* getRootCACertificate();
    
    // Update handlers
    static void handleFirmwareUpdate(WebServer& server);
    static void handleFileSystemUpdate(WebServer& server);
    static void handleUpdateProgress(size_t progress, size_t total);
    
    // Error handling
    static void handleUpdateError(int error);
    static String getUpdateErrorString(int error);
    
    // File system operations
    static bool downloadFileList();
    static bool processFileList(const String& fileListContent);
    static bool downloadAndSaveFile(const String& filename);
};
