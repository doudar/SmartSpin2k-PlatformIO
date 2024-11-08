/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <BLE_Common.h>
#include <Constants.h>
#include "BLE_Steering_Service.h"
#include "SS2KLog.h"
bool flip = false;

void BLE_SteeringCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
  std::string rxValue = pCharacteristic->getValue();
  if (rxValue.length() > 0) {
    const int kLogBufMaxLength = 50;
    char logBuf[kLogBufMaxLength];
    int logBufLength = ss2k_log_hex_to_buffer(pCharacteristic->getValue(), pCharacteristic->getValue().length(), logBuf, 0, kLogBufMaxLength);
    logBufLength += snprintf(logBuf + logBufLength, kLogBufMaxLength - logBufLength, " <- Steering Auth");
    // Check if it's the authentication packet
    if (!flip) {
      uint8_t response[] = {0x03, 0x10, 0xff, 0xff};  // Authentication Challenge
      steeringService->getTxCharacteristic()->setValue(response, 4);
      steeringService->getTxCharacteristic()->notify();
    }
    if (flip) {                // Authentication packet is 4 bytes
      uint8_t response[] = {0x03, 0x11, 0xff};  // Authentication response
      steeringService->getTxCharacteristic()->setValue(response, 3);
      steeringService->getTxCharacteristic()->notify();
      steeringService->setAuthenticated(true);
    }
    flip = !flip;
      SS2K_LOG(BLE_SERVER_LOG_TAG, "%s", logBuf);
  }
}

void BLE_SteeringCallbacks::onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
  String str = "Client ID: ";
  str += desc->conn_handle;
  str += " Address: ";
  str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();

  if (subValue == 0) {
    str += " Unsubscribed to ";
  } else if (subValue == 1) {
    str += " Subscribed to notifications for ";
  } else if (subValue == 2) {
    str += " Subscribed to indications for ";
  }
  str += std::string(pCharacteristic->getUUID()).c_str();

  SS2K_LOG(BLE_SERVER_LOG_TAG, "%s", str.c_str());
}

void BLE_SteeringService::setupService(NimBLEServer* pServer) {
  callbacks = new BLE_SteeringCallbacks(this);

  // Create the steering service
  pSteeringService = pServer->createService(STEERING_SERVICE_UUID);

  // Create the steering angle characteristic
  pSteeringAngleCharacteristic = pSteeringService->createCharacteristic(STEERING_ANGLE_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  // Create RX Characteristic (Write)
  pSteeringRxCharacteristic = pSteeringService->createCharacteristic(STEERING_RX_CHAR_UUID, NIMBLE_PROPERTY::WRITE);
  pSteeringRxCharacteristic->setCallbacks(callbacks);

  // Create TX Characteristic (Indicate)
  pSteeringTxCharacteristic = pSteeringService->createCharacteristic(STEERING_TX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);

  // Start the service
  pSteeringService->start();
}

void BLE_SteeringService::update() {
  //if (!authenticated || !pSteeringAngleCharacteristic->getSubscribedCount()) {
  //  return;
  //}

  // Get current steering angle and convert to fixed point
  int16_t angle = rtConfig->getSteeringAngle() * 10;

  // Convert to bytes
  uint8_t angleBytes[2];
  angleBytes[0] = (uint8_t)(angle & 0xff);
  angleBytes[1] = (uint8_t)(angle >> 8);

  // Update characteristic value
  pSteeringAngleCharacteristic->setValue(angleBytes, 2);
  pSteeringAngleCharacteristic->notify();
}