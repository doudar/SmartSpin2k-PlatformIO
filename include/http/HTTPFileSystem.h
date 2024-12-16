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

#define HTTP_SERVER_LOG_TAG "HTTP_Server"

class HTTPFileSystem {
public:
    static bool initialize();
    static void handleFileRead(WebServer& server, const String& path, const String& contentType = "");
    static void handleFileUpload(WebServer& server);
    static void handleFileDelete(WebServer& server);
    static void handleFileList(WebServer& server);
    
    // File upload status tracking
    static bool isUploadInProgress() { return uploadInProgress; }
    static size_t getUploadProgress() { return uploadProgress; }
    static size_t getUploadTotal() { return uploadTotal; }
    
private:
    static String getContentType(const String& filename);
    static bool exists(const String& path);
    static bool isDirectory(const String& path);
    static File openFile(const String& path, const char* mode);
    
    // File upload handling
    static File fsUploadFile;
    static bool uploadInProgress;
    static size_t uploadProgress;
    static size_t uploadTotal;
    
    static void beginFileUpload(const String& filename);
    static void continueFileUpload(uint8_t* data, size_t len);
    static void endFileUpload();
    
    // Constants
    static const char* const TEXT_PLAIN;
    static const char* const TEXT_HTML;
    static const char* const TEXT_CSS;
    static const char* const TEXT_JAVASCRIPT;
    static const char* const TEXT_JSON;
    static const char* const IMAGE_PNG;
    static const char* const IMAGE_JPG;
    static const char* const IMAGE_GIF;
    static const char* const IMAGE_ICO;
    static const char* const APPLICATION_OCTET_STREAM;
};
