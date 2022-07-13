#pragma once

#ifndef WiFiController_h
#define WiFiController_h
#include "WiFiRegister.h"
#include "EEPROMController.h"
#include "EEPROMController.h"
#include "StorageController.h"
#include "debug.h"

#include <ESP8266HTTPClient.h>
#include <DNSServer.h>

enum WiFiControllerMode {
	WIFI_STA_AP_MODE = 0,
	WIFI_STA_MODE = 1,
	WIFI_AP_MODE = 2,
	WIFI_AP_OR_STA = 3
};

class WiFiController {
public:
	WiFiController();
	bool begin(int8_t mode = WIFI_AP_OR_STA, bool wake = true);
	void forceWifiRegister();
	bool connect();
	bool checkInternet();
	bool changeMode(int8_t mode = WIFI_AP_OR_STA, bool save = false);
	void dnsLoop();
	void restartESP();

private:
	WiFiClient _client;
	DNSServer dnsServer;

	bool modeSTA();
	bool modeAP();
};
#endif