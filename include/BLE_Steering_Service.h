/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <NimBLEDevice.h>

class BLE_SteeringService;  // Forward declaration

class BLE_SteeringCallbacks : public NimBLECharacteristicCallbacks {
 public:
  BLE_SteeringCallbacks(BLE_SteeringService* service) : steeringService(service) {}
  void onWrite(BLECharacteristic* pCharacteristic);
  void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue);

 private:
  BLE_SteeringService* steeringService;
};

class BLE_SteeringService {
 public:
  void setupService(NimBLEServer* pServer);
  void update();
  void setAuthenticated(bool auth) { authenticated = auth; } 
  BLECharacteristic* getTxCharacteristic() { return pSteeringTxCharacteristic; }

 private:
  BLEService* pSteeringService;
  BLECharacteristic* pSteeringAngleCharacteristic;
  BLECharacteristic* pSteeringRxCharacteristic;
  BLECharacteristic* pSteeringTxCharacteristic;
  BLE_SteeringCallbacks* callbacks;
  bool authenticated = false;
};
