/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "http/HTTPFirmware.h"
#include "Main.h"
#include "SS2KLog.h"
#include "cert.h"
#include <ArduinoJson.h>

// Version class implementation
Version::Version(const char* version) : major(0), minor(0), patch(0) {
    parseVersion(version);
}

void Version::parseVersion(const char* version) {
    sscanf(version, "%d.%d.%d", &major, &minor, &patch);
}

bool Version::operator>(const Version& other) const {
    if (major != other.major) return major > other.major;
    if (minor != other.minor) return minor > other.minor;
    return patch > other.patch;
}

void HTTPFirmware::checkForUpdates() {
    HTTPClient http;
    WiFiClientSecure client;
    client.setCACert(getRootCACertificate());
    
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Checking for newer firmware:");
    http.begin(userConfig->getFirmwareUpdateURL() + String(FW_VERSIONFILE), getRootCACertificate());
    
    delay(100);
    int httpCode = http.GET();
    delay(100);
    
    String payload;
    if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        payload.trim();
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "  - Server version: %s", payload.c_str());
        httpServer.setInternetConnection(true);
    } else {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "error downloading %s %d", FW_VERSIONFILE, httpCode);
        httpServer.setInternetConnection(false);
    }
    http.end();

    if (httpCode == HTTP_CODE_OK) {
        bool updateAnyway = false;
        if (!LittleFS.exists("/index.html")) {
            SS2K_LOG(HTTP_SERVER_LOG_TAG, "  -index.html not found.");
        }

        Version availableVer(payload.c_str());
        Version currentVer(FIRMWARE_VERSION);

        if (((availableVer > currentVer) && (userConfig->getAutoUpdate())) || (!LittleFS.exists("/index.html"))) {
            // Update LittleFS first
            updateLittleFS();

            // Then update firmware if needed
            if ((availableVer > currentVer) || updateAnyway) {
                if (userConfig->getAutoUpdate()) {
                    SS2K_LOG(HTTP_SERVER_LOG_TAG, "New firmware detected!");
                    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Upgrading from %s to %s", FIRMWARE_VERSION, payload.c_str());
                    updateFirmware();
                }
            }
        } else {
            SS2K_LOG(HTTP_SERVER_LOG_TAG, "  - Current Version: %s", FIRMWARE_VERSION);
        }
    }
}

void HTTPFirmware::updateLittleFS() {
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Updating FileSystem");
    
    if (!downloadFileList()) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Failed to download file list");
        return;
    }
}

void HTTPFirmware::updateFirmware() {
    WiFiClientSecure client;
    client.setCACert(getRootCACertificate());
    
    t_httpUpdate_return ret = httpUpdate.update(client, userConfig->getFirmwareUpdateURL() + String(FW_BINFILE));
    
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            SS2K_LOG(HTTP_SERVER_LOG_TAG, "HTTP_UPDATE_FAILED Error %d : %s", 
                    httpUpdate.getLastError(), 
                    httpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            SS2K_LOG(HTTP_SERVER_LOG_TAG, "HTTP_UPDATE_NO_UPDATES");
            break;

        case HTTP_UPDATE_OK:
            SS2K_LOG(HTTP_SERVER_LOG_TAG, "HTTP_UPDATE_OK");
            break;
    }
}

bool HTTPFirmware::downloadFileList() {
    HTTPClient http;
    http.begin(DATA_UPDATEURL + String(DATA_FILELIST), getRootCACertificate());
    
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "error downloading %s %d", DATA_FILELIST, httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    payload.trim();
    http.end();

    return processFileList(payload);
}

bool HTTPFirmware::processFileList(const String& fileListContent) {
    StaticJsonDocument<500> doc;
    DeserializationError error = deserializeJson(doc, fileListContent);
    
    if (error) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Failed to parse file list");
        return false;
    }

    JsonArray files = doc.as<JsonArray>();
    bool success = true;

    for (JsonVariant v : files) {
        String fileName = "/" + v.as<String>();
        if (!downloadAndSaveFile(fileName)) {
            success = false;
        }
    }

    return success;
}

bool HTTPFirmware::downloadAndSaveFile(const String& filename) {
    HTTPClient http;
    http.begin(DATA_UPDATEURL + filename, getRootCACertificate());
    
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Error downloading %s %d", filename.c_str(), httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    payload.trim();
    http.end();

    LittleFS.remove(filename);
    File file = LittleFS.open(filename, FILE_WRITE, true);
    if (!file) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Failed to create file, %s", filename.c_str());
        return false;
    }

    if (!file.print(payload)) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Failed to write to file, %s", filename.c_str());
        file.close();
        return false;
    }

    file.close();
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Created: %s", filename.c_str());
    return true;
}

void HTTPFirmware::handleOTAUpdate(WebServer& server) {
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Update: %s", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            SS2K_LOG(HTTP_SERVER_LOG_TAG, "Update Success: %u bytes\nRebooting...", upload.totalSize);
            server.send(200, "text/plain", "Update successful. Rebooting...");
            delay(1000);
            ESP.restart();
        } else {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Update aborted");
    }
}

const char* HTTPFirmware::getRootCACertificate() {
    return rootCACertificate;
}
