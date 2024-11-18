/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <NimBLEServer.h>
#include "BLE_Common.h"

class BLE_Wahoo_Gear_Service {
public:
    void setupService(NimBLEServer* server);
    void update();

private:
    NimBLEServer* _server;
    NimBLEService* _service;
    NimBLECharacteristic* _gearCharacteristic;
    NimBLECharacteristic* _buttonCharacteristic;
    
    static const uint8_t NUM_FRONT_GEARS = 2;
    static const uint8_t NUM_REAR_GEARS = 15;
    static const uint8_t FRONT_GEAR_SHIFT_POSITION = 13; // Position where front gear changes to 2
};
