/*
 * Copyright (C) 2020  Anthony Doud & Joel Baranick
 * All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Arduino.h>

#ifdef USE_TELEGRAM
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

class TelegramManager {
public:
    static void initialize(WiFiClientSecure& client);
    static void sendMessage(const String& message);
    static void update();
    static void stop();
    
private:
    static TaskHandle_t telegramTask;
    static bool messageWaiting;
    static String pendingMessage;
    static UniversalTelegramBot* bot;
    static int failureCount;
    
    // Task management
    static void telegramUpdateTask(void* pvParameters);
    static void resetFailureCount();
    
    // Message handling
    static void processPendingMessage();
    static bool isMessageRateLimited();
    static void clearPendingMessage();
    
    // Constants
    static const int MAX_FAILURES = 3;
    static const int MAX_MESSAGES = 5;
    static const unsigned long MESSAGE_TIMEOUT = 120000; // 2 minutes
};

#define SEND_TO_TELEGRAM(message) TelegramManager::sendMessage(message)

#else
#define SEND_TO_TELEGRAM(message) (void)message
#endif // USE_TELEGRAM
