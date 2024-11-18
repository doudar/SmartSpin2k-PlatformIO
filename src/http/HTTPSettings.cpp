/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "http/HTTPSettings.h"
#include "Main.h"
#include "SS2KLog.h"

void HTTPSettings::processSettings(WebServer& server) {
  bool wasBTUpdate       = false;
  bool wasSettingsUpdate = false;
  bool reboot            = false;

  // Process Network Settings
  processNetworkSettings(server);
  processDeviceSettings(server);
  wasSettingsUpdate = processStepperSettings(server);
  processPowerSettings(server);
  processERGSettings(server);
  if (wasSettingsUpdate) {
    processFeatureSettings(server);
  }
  // Process Bluetooth settings and capture the result
  wasBTUpdate = processBluetoothSettings(server);

  processPWCSettings(server);

  SS2K_LOG(HTTP_SERVER_LOG_TAG, "Config Updated From Web");
  ss2k->saveFlag = true;

  if (reboot) {
    sendSettingsResponse(server, wasBTUpdate, wasSettingsUpdate, true);
    ss2k->rebootFlag = true;
  } else {
    sendSettingsResponse(server, wasBTUpdate, wasSettingsUpdate, false);
  }
}

void HTTPSettings::processNetworkSettings(WebServer& server) {
  if (!server.arg("ssid").isEmpty()) {
    String ssid = server.arg("ssid");
    ssid.trim();
    userConfig->setSsid(ssid);
  }

  if (!server.arg("password").isEmpty()) {
    String password = server.arg("password");
    password.trim();
    userConfig->setPassword(password);
  }
}

void HTTPSettings::processDeviceSettings(WebServer& server) {
  if (!server.arg("deviceName").isEmpty()) {
    String deviceName = server.arg("deviceName");
    deviceName.trim();
    userConfig->setDeviceName(deviceName);
  }
}

bool HTTPSettings::processStepperSettings(WebServer& server) {
  bool wasSettingsUpdate = false;
  if (!server.arg("shiftStep").isEmpty()) {
    wasSettingsUpdate  = true;  // shiftStep is our signal that it was a settings update.
    uint64_t shiftStep = server.arg("shiftStep").toInt();
    if (shiftStep >= MIN_SHIFT_STEP && shiftStep <= MAX_SHIFT_STEP) {
      userConfig->setShiftStep(shiftStep);
    }
  }

  if (!server.arg("stepperPower").isEmpty()) {
    uint64_t stepperPower = server.arg("stepperPower").toInt();
    if (stepperPower >= MIN_STEPPER_POWER && stepperPower <= MAX_STEPPER_POWER) {
      userConfig->setStepperPower(stepperPower);
      ss2k->updateStepperPower();
    }
  }

  if (!server.arg("stepperDir").isEmpty()) {
    userConfig->setStepperDir(true);
  } else if (wasSettingsUpdate) {
    userConfig->setStepperDir(false);
  }

  if (!server.arg("stealthChop").isEmpty()) {
    userConfig->setStealthChop(true);
    ss2k->updateStealthChop();
  } else if (wasSettingsUpdate) {
    userConfig->setStealthChop(false);
    ss2k->updateStealthChop();
  }
  return wasSettingsUpdate;
}

void HTTPSettings::processPowerSettings(WebServer& server) {
  if (!server.arg("maxWatts").isEmpty()) {
    uint64_t maxWatts = server.arg("maxWatts").toInt();
    if (maxWatts >= 0 && maxWatts <= DEFAULT_MAX_WATTS) {
      userConfig->setMaxWatts(maxWatts);
    }
  }

  if (!server.arg("minWatts").isEmpty()) {
    uint64_t minWatts = server.arg("minWatts").toInt();
    if (minWatts >= 0 && minWatts <= DEFAULT_MIN_WATTS) {
      userConfig->setMinWatts(minWatts);
    }
  }

  if (!server.arg("powerCorrectionFactor").isEmpty()) {
    float powerCorrectionFactor = server.arg("powerCorrectionFactor").toFloat();
    if (powerCorrectionFactor >= MIN_PCF && powerCorrectionFactor <= MAX_PCF) {
      userConfig->setPowerCorrectionFactor(powerCorrectionFactor);
    }
  }
}

void HTTPSettings::processERGSettings(WebServer& server) {
  if (!server.arg("ERGSensitivity").isEmpty()) {
    float ERGSensitivity = server.arg("ERGSensitivity").toFloat();
    if (ERGSensitivity >= 0.1 && ERGSensitivity <= 20) {
      userConfig->setERGSensitivity(ERGSensitivity);
    }
  }

  if (!server.arg("inclineMultiplier").isEmpty()) {
    float inclineMultiplier = server.arg("inclineMultiplier").toFloat();
    if (inclineMultiplier >= 0 && inclineMultiplier <= 10) {
      userConfig->setInclineMultiplier(inclineMultiplier);
    }
  }
}

void HTTPSettings::processFeatureSettings(WebServer& server) {
  if (!server.arg("autoUpdate").isEmpty()) {
    userConfig->setAutoUpdate(true);
  } else {
    userConfig->setAutoUpdate(false);
  }

  if (!server.arg("shifterDir").isEmpty()) {
    userConfig->setShifterDir(true);
  } else {
    userConfig->setShifterDir(false);
  }

  if (!server.arg("udpLogEnabled").isEmpty()) {
    userConfig->setUdpLogEnabled(true);
  } else {
    userConfig->setUdpLogEnabled(false);
  }
}

bool HTTPSettings::processBluetoothSettings(WebServer& server) {
  bool wasBTUpdate = false;

  if (!server.arg("blePMDropdown").isEmpty()) {
    wasBTUpdate = true;
    if (server.arg("blePMDropdown")) {
      String powerMeter = server.arg("blePMDropdown");
      if (powerMeter != userConfig->getConnectedPowerMeter()) {
        userConfig->setConnectedPowerMeter(powerMeter);
        spinBLEClient.reconnectAllDevices();
      }
    } else {
      userConfig->setConnectedPowerMeter("any");
    }
  }

  if (!server.arg("bleHRDropdown").isEmpty()) {
    wasBTUpdate = true;
    if (server.arg("bleHRDropdown")) {
      String heartMonitor = server.arg("bleHRDropdown");
      if (heartMonitor != userConfig->getConnectedHeartMonitor()) {
        spinBLEClient.reconnectAllDevices();
      }
      userConfig->setConnectedHeartMonitor(heartMonitor);
    } else {
      userConfig->setConnectedHeartMonitor("any");
    }
  }

  if (!server.arg("bleRemoteDropdown").isEmpty()) {
    wasBTUpdate = true;
    if (server.arg("bleRemoteDropdown")) {
      String remote = server.arg("bleRemoteDropdown");
      if (remote != userConfig->getConnectedRemote()) {
        spinBLEClient.reconnectAllDevices();
      }
      userConfig->setConnectedRemote(remote);
    } else {
      userConfig->setConnectedRemote("any");
    }
  }

  return wasBTUpdate;
}

void HTTPSettings::processPWCSettings(WebServer& server) {
  if (!server.arg("session1HR").isEmpty()) {
    userPWC->session1HR = server.arg("session1HR").toInt();
  } else {  // wasn't pwc settings page
    return;
  }
  if (!server.arg("session1Pwr").isEmpty()) {
    userPWC->session1Pwr = server.arg("session1Pwr").toInt();
  }
  if (!server.arg("session2HR").isEmpty()) {
    userPWC->session2HR = server.arg("session2HR").toInt();
  }
  if (!server.arg("session2Pwr").isEmpty()) {
    userPWC->session2Pwr = server.arg("session2Pwr").toInt();
  }
  if (!server.arg("hr2Pwr").isEmpty()) {
    userPWC->hr2Pwr = true;
  } else {
    userPWC->hr2Pwr = false;
  }
}

void HTTPSettings::sendSettingsResponse(WebServer& server, bool wasBTUpdate, bool wasSettingsUpdate, bool requiresReboot) {
  String response;
  if (wasBTUpdate) {
    response = buildRedirectResponse("Selections Saved!", "/bluetoothscanner.html");
  } else if (wasSettingsUpdate || requiresReboot) {
    response = buildRedirectResponse(
        "Network settings will be applied at next reboot.<br>"
        "Everything else is available immediately.",
        "/settings.html");
  } else {
    response = buildRedirectResponse(
        "Network settings will be applied at next reboot.<br>"
        "Everything else is available immediately.",
        "/index.html");
  }

  if (requiresReboot) {
    response = buildRedirectResponse("Please wait while your settings are saved and SmartSpin2k reboots.", "/bluetoothscanner.html", 5000);
  }

  server.send(200, "text/html", response);
}

String HTTPSettings::buildRedirectResponse(const String& message, const String& page, int delay) {
  return "<!DOCTYPE html><html><body><h2>" + message + "</h2></body><script>setTimeout(\"location.href = '" + page + "';\", " + String(delay) + ");</script></html>";
}

bool HTTPSettings::processCheckbox(WebServer& server, const char* name, bool defaultValue) {
  if (!server.hasArg(name)) {
    return defaultValue;
  }
  return server.arg(name) == "true" || server.arg(name) == "1";
}

float HTTPSettings::processFloatValue(WebServer& server, const char* name, float min, float max) {
  if (!server.hasArg(name)) {
    return min;
  }
  float value = server.arg(name).toFloat();
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

int HTTPSettings::processIntValue(WebServer& server, const char* name, int min, int max) {
  if (!server.hasArg(name)) {
    return min;
  }
  int value = server.arg(name).toInt();
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

String HTTPSettings::processStringValue(WebServer& server, const char* name) {
  if (!server.hasArg(name)) {
    return "";
  }
  return server.arg(name);
}
