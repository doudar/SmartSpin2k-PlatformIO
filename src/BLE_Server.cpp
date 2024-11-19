/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Main.h"
#include "SS2KLog.h"
#include "BLE_Common.h"
#include "BLE_Cycling_Speed_Cadence.h"
#include "BLE_Cycling_Power_Service.h"
#include "BLE_Heart_Service.h"
#include "BLE_Fitness_Machine_Service.h"
#include "BLE_Custom_Characteristic.h"
#include "BLE_Device_Information_Service.h"
#include "BLE_Wattbike_Service.h"

#include <ArduinoJson.h>
#include <Constants.h>
#include <NimBLEDevice.h>
#include <cmath>
#include <limits>

// BLE Server Settings
SpinBLEServer spinBLEServer;

static MyCallbacks chrCallbacks;

BLE_Cycling_Speed_Cadence cyclingSpeedCadenceService;
BLE_Cycling_Power_Service cyclingPowerService;
BLE_Heart_Service heartService;
BLE_Fitness_Machine_Service fitnessMachineService;
BLE_ss2kCustomCharacteristic ss2kCustomCharacteristic;
BLE_Device_Information_Service deviceInformationService;
BLE_Wattbike_Service wattbikeService;

void startBLEServer() {
  // Server Setup
  SS2K_LOG(BLE_SERVER_LOG_TAG, "Starting BLE Server");
  spinBLEServer.pServer = BLEDevice::createServer();
  spinBLEServer.pServer->setCallbacks(new MyServerCallbacks());

  // start services
  cyclingSpeedCadenceService.setupService(spinBLEServer.pServer, &chrCallbacks);
  cyclingPowerService.setupService(spinBLEServer.pServer, &chrCallbacks);
  heartService.setupService(spinBLEServer.pServer, &chrCallbacks);
  fitnessMachineService.setupService(spinBLEServer.pServer, &chrCallbacks);
  ss2kCustomCharacteristic.setupService(spinBLEServer.pServer);
  deviceInformationService.setupService(spinBLEServer.pServer);
  wattbikeService.setupService(spinBLEServer.pServer);  // No callback needed

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  // const std::string fitnessData = {0b00000001, 0b00100000, 0b00000000};
  // pAdvertising->setServiceData(FITNESSMACHINESERVICE_UUID, fitnessData);

  pAdvertising->addServiceUUID(FITNESSMACHINESERVICE_UUID);
  pAdvertising->addServiceUUID(CYCLINGPOWERSERVICE_UUID);
  pAdvertising->addServiceUUID(CSCSERVICE_UUID);
  pAdvertising->addServiceUUID(HEARTSERVICE_UUID);
  pAdvertising->addServiceUUID(SMARTSPIN2K_SERVICE_UUID);
  pAdvertising->addServiceUUID(WATTBIKE_SERVICE_UUID);
  pAdvertising->setMaxInterval(250);
  pAdvertising->setMinInterval(160);
  pAdvertising->setScanResponse(true);

  BLEFirmwareSetup();
  BLEDevice::startAdvertising();

  SS2K_LOG(BLE_SERVER_LOG_TAG, "Bluetooth Characteristic defined!");
}

void SpinBLEServer::update() {
  // Wheel and crank is used in multiple characteristics. Update first.
  spinBLEServer.updateWheelAndCrankRev();
  // update the BLE information on the server
  heartService.update();
  cyclingPowerService.update();
  cyclingSpeedCadenceService.update();
  fitnessMachineService.update();
  wattbikeService.parseNemit();  // Changed from update() to parseNemit()
}

double SpinBLEServer::calculateSpeed() {
  // Constants for the formula: adjusted for calibration
  const double dragCoefficient   = 1.95;
  const double frontalArea       = 0.9;    // m^2
  const double airDensity        = 1.225;  // kg/m^3
  const double rollingResistance = 0.004;
  const double combinedConstant  = 0.5 * airDensity * dragCoefficient * frontalArea + rollingResistance;
  double power                   = rtConfig->watts.getValue();           // Power in watts
  double speedInMetersPerSecond  = std::cbrt(power / combinedConstant);  // Speed in m/s

  // Convert speed from m/s to km/h
  double speedKmH = speedInMetersPerSecond * 3.6;

  // Apply a calibration factor based on empirical data to adjust the speed into a realistic range
  double calibrationFactor = 1;  // This is an example value; adjust based on calibration
  speedKmH *= calibrationFactor;

  return speedKmH;
}

void SpinBLEServer::updateWheelAndCrankRev() {
  float wheelSize     = 2.127;  // 700cX28 circumference, typical in meters
  float wheelSpeedMps = 0.0;
  if (rtConfig->getSimulatedSpeed() > 5) {
    wheelSpeedMps = rtConfig->getSimulatedSpeed() / 3.6;
  } else {
    wheelSpeedMps = this->calculateSpeed() / 3.6;  // covert km/h to m/s
  }

  // Calculate wheel revolutions per minute
  float wheelRpm        = (wheelSpeedMps / wheelSize) * 60;
  double wheelRevPeriod = (60 * 1024) / wheelRpm;
  if (wheelRpm > 0) {
    spinBLEClient.cscCumulativeWheelRev++;                // Increment cumulative wheel revolutions
    spinBLEClient.cscLastWheelEvtTime += wheelRevPeriod;  // Convert RPM to time, ensuring no division by zero
  }

  float cadence = rtConfig->cad.getValue();
  if (cadence > 0) {
    float crankRevPeriod = (60 * 1024) / cadence;
    spinBLEClient.cscCumulativeCrankRev++;
    spinBLEClient.cscLastCrankEvtTime += crankRevPeriod;
  }
}

// Creating Server Connection Callbacks
void MyServerCallbacks::onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) {
  SS2K_LOG(BLE_SERVER_LOG_TAG, "Bluetooth Remote Client Connected: %s Connected Clients: %d", NimBLEAddress(desc->peer_ota_addr).toString().c_str(), pServer->getConnectedCount());

  if (pServer->getConnectedCount() < CONFIG_BT_NIMBLE_MAX_CONNECTIONS - NUM_BLE_DEVICES) {
    BLEDevice::startAdvertising();
  } else {
    SS2K_LOG(BLE_SERVER_LOG_TAG, "Max Remote Client Connections Reached");
    BLEDevice::stopAdvertising();
  }
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer) {
  SS2K_LOG(BLE_SERVER_LOG_TAG, "Bluetooth Remote Client Disconnected. Remaining Clients: %d", pServer->getConnectedCount());
  BLEDevice::startAdvertising();
  // client disconnected while trying to write fw - reboot to clear the faulty upload.
  if (ss2k->isUpdating) {
    SS2K_LOG(BLE_SERVER_LOG_TAG, "Rebooting because of update interruption.", pServer->getConnectedCount());
    ss2k->rebootFlag = true;
  }
}

bool MyServerCallbacks::onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) {
  SS2K_LOG(BLE_SERVER_LOG_TAG, "Updated Server Connection Parameters for %s", pClient->getPeerAddress().toString().c_str());
  return true;
};

// END SERVER CALLBACKS

void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
  if (pCharacteristic->getUUID() == FITNESSMACHINECONTROLPOINT_UUID) {
    spinBLEServer.writeCache.push(pCharacteristic->getValue());
  } else {
    SS2K_LOG(BLE_SERVER_LOG_TAG, "Write to %s is not supported", pCharacteristic->getUUID().toString());
  }
}

void MyCallbacks::onSubscribe(NimBLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue) {
  String str       = "Client ID: ";
  NimBLEUUID pUUID = pCharacteristic->getUUID();
  str += desc->conn_handle;
  str += " Address: ";
  str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
  if (subValue == 0) {
    str += " Unsubscribed to ";
    spinBLEServer.setClientSubscribed(pUUID, false);
  } else if (subValue == 1) {
    str += " Subscribed to notifications for ";
    spinBLEServer.setClientSubscribed(pUUID, true);
  } else if (subValue == 2) {
    str += " Subscribed to indications for ";
    spinBLEServer.setClientSubscribed(pUUID, true);
  } else if (subValue == 3) {
    str += " Subscribed to notifications and indications for ";
    spinBLEServer.setClientSubscribed(pUUID, true);
  }
  str += std::string(pCharacteristic->getUUID()).c_str();

  SS2K_LOG(BLE_SERVER_LOG_TAG, "%s", str.c_str());
}

// This might be worth depreciating. With multiple clients connected (SS2k App, + Training App), it at least needs to be an int, not a bool.
void SpinBLEServer::setClientSubscribed(NimBLEUUID pUUID, bool subscribe) {
  if (pUUID == HEARTCHARACTERISTIC_UUID) {
    spinBLEServer.clientSubscribed.Heartrate = subscribe;
  } else if (pUUID == CYCLINGPOWERMEASUREMENT_UUID) {
    spinBLEServer.clientSubscribed.CyclingPowerMeasurement = subscribe;
  } else if (pUUID == FITNESSMACHINEINDOORBIKEDATA_UUID) {
    spinBLEServer.clientSubscribed.IndoorBikeData = subscribe;
  } else if (pUUID == CSCMEASUREMENT_UUID) {
    spinBLEServer.clientSubscribed.CyclingSpeedCadence = subscribe;
  }
}

// Return number of clients connected to our server.
int connectedClientCount() {
  if (BLEDevice::getServer()) {
    return BLEDevice::getServer()->getConnectedCount();
  } else {
    return 0;
  }
}

void calculateInstPwrFromHR() {
  static int oldHR    = rtConfig->hr.getValue();
  static int newHR    = rtConfig->hr.getValue();
  static double delta = 0;
  oldHR               = newHR;  // Copying HR from Last loop
  newHR               = rtConfig->hr.getValue();

  delta = (newHR - oldHR) / ((BLE_CLIENT_DELAY / 1000) + 1);

  // userConfig->setSimulatedWatts((s1Pwr*s2HR)-(s2Pwr*S1HR))/(S2HR-s1HR)+(userConfig->getSimulatedHr(*((s1Pwr-s2Pwr)/(s1HR-s2HR)));
  int avgP = ((userPWC->session1Pwr * userPWC->session2HR) - (userPWC->session2Pwr * userPWC->session1HR)) / (userPWC->session2HR - userPWC->session1HR) +
             (newHR * ((userPWC->session1Pwr - userPWC->session2Pwr) / (userPWC->session1HR - userPWC->session2HR)));

  if (avgP < DEFAULT_MIN_WATTS) {
    avgP = DEFAULT_MIN_WATTS;
  }

  if (delta < 0) {
    // magic math here for inst power
  }

  if (delta > 0) {
    // magic math here for inst power
  }

#ifndef DEBUG_HR_TO_PWR
  rtConfig->watts.setValue(avgP);
  rtConfig->cad.setValue(NORMAL_CAD);
#endif  // DEBUG_HR_TO_PWR

  SS2K_LOG(BLE_SERVER_LOG_TAG, "Power From HR: %d", avgP);
}

void logCharacteristic(char *buffer, const size_t bufferCapacity, const byte *data, const size_t dataLength, const NimBLEUUID serviceUUID, const NimBLEUUID charUUID,
                       const char *format, ...) {
  int bufferLength = ss2k_log_hex_to_buffer(data, dataLength, buffer, 0, bufferCapacity);
  bufferLength += snprintf(buffer + bufferLength, bufferCapacity - bufferLength, "-> %s | %s | ", serviceUUID.toString().c_str(), charUUID.toString().c_str());
  va_list args;
  va_start(args, format);
  bufferLength += vsnprintf(buffer + bufferLength, bufferCapacity - bufferLength, format, args);
  va_end(args);

  SS2K_LOG(BLE_SERVER_LOG_TAG, "%s", buffer);
#ifdef USE_TELEGRAM
  SEND_TO_TELEGRAM(String(buffer));
#endif
}
