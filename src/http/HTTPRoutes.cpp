/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "http/HTTPRoutes.h"
#include "Main.h"
#include "SS2KLog.h"
#include "Builtin_Pages.h"
#include "HTTPUpdateServer.h"
#include <LittleFS.h>
#include <Update.h>

// Initialize static members
WebServer* HTTPRoutes::currentServer = nullptr;
File HTTPRoutes::fsUploadFile;

// Initialize static handler functions
HTTPRoutes::HandlerFunction HTTPRoutes::handleIndexFile         = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleBTScanner         = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleLittleFSFile      = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleConfigJSON        = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleRuntimeConfigJSON = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handlePWCJSON           = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleShift             = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleHRSlider          = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleWattsSlider       = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleCadSlider         = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleERGMode           = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleTargetWattsSlider = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleLogin             = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleOTAUpdate         = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleFileUpload        = nullptr;
HTTPRoutes::HandlerFunction HTTPRoutes::handleSendSettings      = nullptr;

void HTTPRoutes::initialize(WebServer& server) {
  currentServer = &server;

  // Initialize handler functions using lambda functions to capture the static methods
  handleIndexFile         = []() { _handleIndexFile(); };
  handleBTScanner         = []() { _handleBTScanner(); };
  handleLittleFSFile      = []() { _handleLittleFSFile(); };
  handleConfigJSON        = []() { _handleConfigJSON(); };
  handleRuntimeConfigJSON = []() { _handleRuntimeConfigJSON(); };
  handlePWCJSON           = []() { _handlePWCJSON(); };
  handleShift             = []() { _handleShift(); };
  handleHRSlider          = []() { _handleHRSlider(); };
  handleWattsSlider       = []() { _handleWattsSlider(); };
  handleCadSlider         = []() { _handleCadSlider(); };
  handleERGMode           = []() { _handleERGMode(); };
  handleTargetWattsSlider = []() { _handleTargetWattsSlider(); };
  handleLogin             = []() { _handleLogin(); };
  handleOTAUpdate         = []() { _handleOTAUpdate(); };
  handleFileUpload        = []() { _handleFileUpload(); };
  handleSendSettings      = []() { _handleSendSettings(); };
}

void HTTPRoutes::_handleIndexFile() {
  String filename = "/index.html";
  if (LittleFS.exists(filename)) {
    File file = LittleFS.open(filename, FILE_READ);
    currentServer->streamFile(file, "text/html");
    file.close();
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Served %s", filename.c_str());
  } else {
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "%s not found. Sending builtin Index.html", filename.c_str());
    currentServer->send(200, "text/html", noIndexHTML);
  }
}

void HTTPRoutes::_handleBTScanner() {
  SS2K_LOG(HTTP_SERVER_LOG_TAG, "Scanning from web request");
  spinBLEClient.dontBlockScan = true;
  spinBLEClient.doScan        = true;
  _handleLittleFSFile();
}

void HTTPRoutes::_handleLittleFSFile() {
  String filename = currentServer->uri();
  int dotPosition = filename.lastIndexOf(".");
  String fileType = filename.substring((dotPosition + 1), filename.length());

  if (LittleFS.exists(filename)) {
    File file = LittleFS.open(filename, FILE_READ);
    if (fileType == "gz") {
      fileType = "html";
    }
    currentServer->streamFile(file, "text/" + fileType);
    file.close();
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Served %s", filename.c_str());
  } else if (!LittleFS.exists("/index.html")) {
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "%s not found and no filesystem. Sending builtin index.html", filename.c_str());
    _handleIndexFile();
  } else {
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "%s not found. Sending 404.", filename.c_str());
    String outputhtml = "<html><body><h1>ERROR 404 <br> FILE NOT FOUND!" + filename + "</h1></body></html>";
    currentServer->send(404, "text/html", outputhtml);
  }
}

void HTTPRoutes::_handleConfigJSON() {
  String tString = userConfig->returnJSON();
  currentServer->send(200, "text/plain", tString);
}

void HTTPRoutes::_handleRuntimeConfigJSON() {
  String tString = rtConfig->returnJSON();
  currentServer->send(200, "text/plain", tString);
}

void HTTPRoutes::_handlePWCJSON() {
  String tString = userPWC->returnJSON();
  currentServer->send(200, "text/plain", tString);
}

void HTTPRoutes::_handleShift() {
  int value = currentServer->arg("value").toInt();
  if ((value > -10) && (value < 10)) {
    rtConfig->setShifterPosition(rtConfig->getShifterPosition() + value);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Shift From HTML");
  } else {
    rtConfig->setShifterPosition(value);
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Invalid HTML Shift");
    currentServer->send(200, "text/plain", "OK");
  }
}

void HTTPRoutes::_handleHRSlider() {
  String value = currentServer->arg("value");
  if (value == "enable") {
    rtConfig->hr.setSimulate(true);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "HR Simulator turned on");
  } else if (value == "disable") {
    rtConfig->hr.setSimulate(false);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "HR Simulator turned off");
  } else {
    rtConfig->hr.setValue(value.toInt());
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "HR is now: %d", rtConfig->hr.getValue());
    currentServer->send(200, "text/plain", "OK");
  }
}

void HTTPRoutes::_handleWattsSlider() {
  String value = currentServer->arg("value");
  if (value == "enable") {
    rtConfig->watts.setSimulate(true);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Watt Simulator turned on");
  } else if (value == "disable") {
    rtConfig->watts.setSimulate(false);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Watt Simulator turned off");
  } else {
    rtConfig->watts.setValue(value.toInt());
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Watts are now: %d", rtConfig->watts.getValue());
    currentServer->send(200, "text/plain", "OK");
  }
}

void HTTPRoutes::_handleCadSlider() {
  String value = currentServer->arg("value");
  if (value == "enable") {
    rtConfig->cad.setSimulate(true);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "CAD Simulator turned on");
  } else if (value == "disable") {
    rtConfig->cad.setSimulate(false);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "CAD Simulator turned off");
  } else {
    rtConfig->cad.setValue(value.toInt());
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "CAD is now: %d", rtConfig->cad.getValue());
    currentServer->send(200, "text/plain", "OK");
  }
}

void HTTPRoutes::_handleERGMode() {
  String value = currentServer->arg("value");
  if (value == "enable") {
    rtConfig->setFTMSMode(FitnessMachineControlPointProcedure::SetTargetPower);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "ERG Mode turned on");
  } else {
    rtConfig->setFTMSMode(FitnessMachineControlPointProcedure::RequestControl);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "ERG Mode turned off");
  }
}

void HTTPRoutes::_handleTargetWattsSlider() {
  String value = currentServer->arg("value");
  if (value == "enable") {
    rtConfig->setSimTargetWatts(true);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Target Watts Simulator turned on");
  } else if (value == "disable") {
    rtConfig->setSimTargetWatts(false);
    currentServer->send(200, "text/plain", "OK");
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Target Watts Simulator turned off");
  } else {
    rtConfig->watts.setTarget(value.toInt());
    SS2K_LOG(HTTP_SERVER_LOG_TAG, "Target Watts are now: %d", rtConfig->watts.getTarget());
    currentServer->send(200, "text/plain", "OK");
  }
}

void HTTPRoutes::_handleLogin() {
  currentServer->sendHeader("Connection", "close");
  currentServer->send(200, "text/html", OTALoginIndex);
}

void HTTPRoutes::_handleOTAUpdate() {
  ss2k->stopTasks();
  currentServer->sendHeader("Connection", "close");
  currentServer->send(200, "text/html", OTAServerIndex);
}

void HTTPRoutes::_handleFileUpload() {
  HTTPFirmware::handleOTAUpdate(*currentServer);
}

void HTTPRoutes::setupRoutes(WebServer& server) {
  initialize(server);
  setupDefaultRoutes(server);
  setupFileRoutes(server);
  setupControlRoutes(server);
  setupUpdateRoutes(server);
}

void HTTPRoutes::setupDefaultRoutes(WebServer& server) {
  server.on("/", HTTP_GET, handleIndexFile);
  server.on("/index.html", HTTP_GET, handleIndexFile);
  server.on("/generate_204", HTTP_GET, handleIndexFile);         // Android captive portal
  server.on("/fwlink", HTTP_GET, handleIndexFile);               // Microsoft captive portal
  server.on("/hotspot-detect.html", HTTP_GET, handleIndexFile);  // Apple captive portal
}

void HTTPRoutes::setupFileRoutes(WebServer& server) {
  server.on("/style.css", HTTP_GET, handleLittleFSFile);
  server.on("/btsimulator.html", HTTP_GET, handleLittleFSFile);
  server.on("/develop.html", HTTP_GET, handleLittleFSFile);
  server.on("/shift.html", HTTP_GET, handleLittleFSFile);
  server.on("/settings.html", HTTP_GET, handleLittleFSFile);
  server.on("/status.html", HTTP_GET, handleLittleFSFile);
  server.on("/bluetoothscanner.html", HTTP_GET, handleBTScanner);
  server.on("/streamfit.html", HTTP_GET, handleLittleFSFile);
  server.on("/hrtowatts.html", HTTP_GET, handleLittleFSFile);
  server.on("/favicon.ico", HTTP_GET, handleLittleFSFile);
  server.on("/jquery.js.gz", HTTP_GET, handleLittleFSFile);
}

void HTTPRoutes::setupControlRoutes(WebServer& server) {
  server.on("/hrslider", HTTP_GET, handleHRSlider);
  server.on("/wattsslider", HTTP_GET, handleWattsSlider);
  server.on("/cadslider", HTTP_GET, handleCadSlider);
  server.on("/ergmode", HTTP_GET, handleERGMode);
  server.on("/targetwattsslider", HTTP_GET, handleTargetWattsSlider);
  server.on("/shift", HTTP_GET, handleShift);
  server.on("/configJSON", HTTP_GET, handleConfigJSON);
  server.on("/runtimeConfigJSON", HTTP_GET, handleRuntimeConfigJSON);
  server.on("/PWCJSON", HTTP_GET, handlePWCJSON);
  server.on("/BLEScan", HTTP_GET, handleBTScanner);
  server.on("/send_settings", HTTP_GET, handleSendSettings);
}

void HTTPRoutes::setupUpdateRoutes(WebServer& server) {
  server.on("/login", HTTP_GET, handleLogin);
  server.on("/OTAIndex", HTTP_GET, handleOTAUpdate);

  server.on(
      "/update", HTTP_POST,
      []() {
        currentServer->sendHeader("Connection", "close");
        currentServer->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      },
      handleFileUpload);
}

void HTTPRoutes::_handleSendSettings() { HTTPSettings::processSettings(*currentServer); }
