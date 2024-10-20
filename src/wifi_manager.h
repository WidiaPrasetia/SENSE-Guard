#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>

void setupWiFi();
void handleWebRequests(AsyncWebServer &server);

#endif // WIFI_MANAGER_H
