/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "BLE_Wahoo_Gear_Service.h"
#include "Constants.h"
#include "SS2KLog.h"
#include "Main.h"

static const char* TAG = "BLEWahooGearService";

void BLE_Wahoo_Gear_Service::setupService(NimBLEServer* server) {
    _server = server;
    _service = _server->createService(WAHOO_GEAR_SERVICE_UUID);
    
    _gearCharacteristic = _service->createCharacteristic(
        WAHOO_GEAR_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

        _buttonCharacteristic = _service->createCharacteristic(
        WAHOO_BUTTON_STATUS_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    _service->start();
    SS2K_LOGI(TAG, "Wahoo Gear Service started");
}

void BLE_Wahoo_Gear_Service::update() {
    uint8_t shifterPosition = rtConfig->getShifterPosition();
    
    // Calculate front and rear gear positions
    uint8_t frontGear = 0; // 0-based index (1st gear)
    uint8_t rearGear = 0;  // 0-based index (1st gear)
    
    if (shifterPosition >= FRONT_GEAR_SHIFT_POSITION) {
        // When position is 13 or higher, we're in the second front gear
        frontGear = 2;
        // Rear gear position continues from where we left off
        rearGear = shifterPosition - FRONT_GEAR_SHIFT_POSITION;
    } else {
        // For positions 1-12, we're in the first front gear
        frontGear = 1;
        // Rear gear position is shifterPosition - 1 (to convert to 0-based index)
        rearGear = shifterPosition;
    }

    // Format based on the Wahoo gear indicator protocol
    // [0] = 0 (reserved)
    // [1] = 0 (reserved)
    // [2] = Front gear position (0-based index)
    // [3] = Rear gear position (0-based index)
    // [4] = Number of front gears
    // [5] = Number of rear gears
    uint8_t gearData[6] = {0};
    gearData[2] = frontGear;
    gearData[3] = rearGear;
    gearData[4] = NUM_FRONT_GEARS;
    gearData[5] = NUM_REAR_GEARS;

    _gearCharacteristic->setValue(gearData, sizeof(gearData));
    _buttonCharacteristic->setValue(gearData, sizeof(gearData)); //this can go when done testing
    _gearCharacteristic->notify();
    
    SS2K_LOGI(TAG, "Updated gear position - Front: %d, Rear: %d (Shifter pos: %d)", 
              frontGear + 1, rearGear + 1, shifterPosition);
}
