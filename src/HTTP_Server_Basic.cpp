/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "HTTP_Server_Basic.h"
#include "Main.h"
#include "SS2KLog.h"
#include "wifi/WiFiManager.h"

// Initialize the global instance (already declared in HTTPCore.cpp)
// extern HTTP_Server httpServer;

// The original HTTP_Server_Basic.cpp functionality has been refactored into:
// - HTTPCore: Core server functionality
// - HTTPRoutes: Route handlers
// - HTTPFileSystem: File system operations
// - HTTPSettings: Settings processing
// - HTTPFirmware: Firmware updates
// - WiFiManager: WiFi connection management

// This file remains for backward compatibility and initialization
