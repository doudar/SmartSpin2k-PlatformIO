/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "http/HTTPFileSystem.h"
#include "SS2KLog.h"

// Initialize static members
File HTTPFileSystem::fsUploadFile;
bool HTTPFileSystem::uploadInProgress = false;
size_t HTTPFileSystem::uploadProgress = 0;
size_t HTTPFileSystem::uploadTotal = 0;

// Initialize content type constants
const char* const HTTPFileSystem::TEXT_PLAIN = "text/plain";
const char* const HTTPFileSystem::TEXT_HTML = "text/html";
const char* const HTTPFileSystem::TEXT_CSS = "text/css";
const char* const HTTPFileSystem::TEXT_JAVASCRIPT = "application/javascript";
const char* const HTTPFileSystem::TEXT_JSON = "application/json";
const char* const HTTPFileSystem::IMAGE_PNG = "image/png";
const char* const HTTPFileSystem::IMAGE_JPG = "image/jpeg";
const char* const HTTPFileSystem::IMAGE_GIF = "image/gif";
const char* const HTTPFileSystem::IMAGE_ICO = "image/x-icon";
const char* const HTTPFileSystem::APPLICATION_OCTET_STREAM = "application/octet-stream";

bool HTTPFileSystem::initialize() {
    if (!LittleFS.begin()) {
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "LittleFS Mount Failed");
        return false;
    }
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "LittleFS Mounted Successfully");
    return true;
}

void HTTPFileSystem::handleFileRead(WebServer& server, const String& path, const String& contentType) {
    String finalPath = path;
    if (finalPath.endsWith("/")) {
        finalPath += "index.html";
    }
    
    String mimeType = contentType;
    if (mimeType.isEmpty()) {
        mimeType = getContentType(finalPath);
    }
    
    if (exists(finalPath)) {
        File file = openFile(finalPath, "r");
        if (file) {
            server.streamFile(file, mimeType);
            file.close();
            SS2K_LOG(HTTP_SERVER_LOG_TAG, "Served %s", finalPath.c_str());
            return;
        }
    }
    
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "File Not Found: %s", finalPath.c_str());
    server.send(404, TEXT_PLAIN, "File Not Found");
}

void HTTPFileSystem::handleFileUpload(WebServer& server) {
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        uploadInProgress = true;
        uploadProgress = 0;
        uploadTotal = 0;
        
        String filename = upload.filename;
        if (!filename.startsWith("/")) {
            filename = "/" + filename;
        }
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "handleFileUpload Name: %s", filename.c_str());
        beginFileUpload(filename);
        
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        uploadProgress += upload.currentSize;
        uploadTotal = upload.totalSize;
        
        if (fsUploadFile) {
            continueFileUpload(upload.buf, upload.currentSize);
        }
        
    } else if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile) {
            endFileUpload();
            SS2K_LOG(HTTP_SERVER_LOG_TAG, "handleFileUpload Size: %zu", upload.totalSize);
        }
        uploadInProgress = false;
    }
}

void HTTPFileSystem::handleFileDelete(WebServer& server) {
    if (!server.hasArg("path")) {
        server.send(400, TEXT_PLAIN, "Path Argument Missing");
        return;
    }
    
    String path = server.arg("path");
    if (!exists(path)) {
        server.send(404, TEXT_PLAIN, "File Not Found");
        return;
    }
    
    if (LittleFS.remove(path)) {
        server.send(200, TEXT_PLAIN, "File Deleted");
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "File Deleted: %s", path.c_str());
    } else {
        server.send(500, TEXT_PLAIN, "Delete Failed");
        SS2K_LOG(HTTP_SERVER_LOG_TAG, "Delete Failed: %s", path.c_str());
    }
}

void HTTPFileSystem::handleFileList(WebServer& server) {
    String path = server.hasArg("dir") ? server.arg("dir") : "/";
    
    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) {
        server.send(400, TEXT_PLAIN, "Directory Not Found");
        return;
    }
    
    String output = "[";
    File entry = dir.openNextFile();
    while (entry) {
        if (output != "[") {
            output += ',';
        }
        output += "{\"name\":\"" + String(entry.name()) + "\",";
        output += "\"size\":" + String(entry.size()) + ",";
        output += "\"isDir\":" + String(entry.isDirectory() ? "true" : "false") + "}";
        entry = dir.openNextFile();
    }
    output += "]";
    
    server.send(200, TEXT_JSON, output);
}

String HTTPFileSystem::getContentType(const String& filename) {
    if (filename.endsWith(".html")) return TEXT_HTML;
    else if (filename.endsWith(".css")) return TEXT_CSS;
    else if (filename.endsWith(".js")) return TEXT_JAVASCRIPT;
    else if (filename.endsWith(".json")) return TEXT_JSON;
    else if (filename.endsWith(".png")) return IMAGE_PNG;
    else if (filename.endsWith(".jpg")) return IMAGE_JPG;
    else if (filename.endsWith(".gif")) return IMAGE_GIF;
    else if (filename.endsWith(".ico")) return IMAGE_ICO;
    else if (filename.endsWith(".gz")) {
        String baseFilename = filename.substring(0, filename.length() - 3);
        return getContentType(baseFilename);
    }
    return TEXT_PLAIN;
}

bool HTTPFileSystem::exists(const String& path) {
    return LittleFS.exists(path);
}

bool HTTPFileSystem::isDirectory(const String& path) {
    File file = LittleFS.open(path);
    bool isDir = file && file.isDirectory();
    file.close();
    return isDir;
}

File HTTPFileSystem::openFile(const String& path, const char* mode) {
    return LittleFS.open(path, mode);
}

void HTTPFileSystem::beginFileUpload(const String& filename) {
    if (fsUploadFile) {
        fsUploadFile.close();
    }
    fsUploadFile = LittleFS.open(filename, "w");
}

void HTTPFileSystem::continueFileUpload(uint8_t* data, size_t len) {
    if (fsUploadFile && data && len > 0) {
        fsUploadFile.write(data, len);
    }
}

void HTTPFileSystem::endFileUpload() {
    if (fsUploadFile) {
        fsUploadFile.close();
    }
}
