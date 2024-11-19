/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <NimBLEDevice.h>
#include "BLE_Common.h"

class BLE_Fitness_Machine_Service {
 public:
  BLE_Fitness_Machine_Service();
  void setupService(NimBLEServer *pServer, MyCallbacks *chrCallbacks);
  void update();
  bool spinDown(uint8_t response);

 private:
  BLEService *pFitnessMachineService;
  BLECharacteristic *fitnessMachineFeature;
  BLECharacteristic *fitnessMachineIndoorBikeData;
  BLECharacteristic *fitnessMachineStatusCharacteristic;
  BLECharacteristic *fitnessMachineControlPoint;
  BLECharacteristic *fitnessMachineResistanceLevelRange;
  BLECharacteristic *fitnessMachinePowerRange;
  BLECharacteristic *fitnessMachineInclinationRange;
  BLECharacteristic *fitnessMachineTrainingStatus;
  uint8_t ftmsIndoorBikeData[11] = {0};
  void processFTMSWrite();
};

extern BLE_Fitness_Machine_Service fitnessMachineService;