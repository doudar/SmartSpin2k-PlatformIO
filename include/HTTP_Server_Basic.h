/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Arduino.h>
#include "http/HTTPCore.h"
#include "http/HTTPRoutes.h"
#include "http/HTTPFileSystem.h"
#include "http/HTTPSettings.h"
#include "http/HTTPFirmware.h"

// For backward compatibility, use HTTPCore as HTTP_Server
using HTTP_Server = HTTPCore;

extern HTTP_Server httpServer;
