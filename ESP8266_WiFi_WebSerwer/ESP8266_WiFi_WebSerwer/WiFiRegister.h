#pragma once

#ifndef WiFiRegister_h
#define WiFiRegister_h
#include "Arduino.h"
#include "WiFiRegister.h"
#include "StorageController.h"
#include "debug.h"

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

enum action {
	MODE_DEFAULT = 0,
	MODE_CONNECTION= 1,
	MODE_RESTART = 2,
};

class WiFiRegister {
public:
	WiFiRegister(const char*);
	WiFiRegister();
	void begin();
private:
	AsyncWebServer _server;
	DNSServer dnsServer;
	WiFiClient _client;
	char _ssid[33];
	char _pass[65];
	const char * _apName;
	char _status[13];

	uint8_t action = 0;

	uint8_t encryptionTypeStr(uint8_t);
	uint8_t encryptionPowerStr(int8_t);
	const  char * encryptionColorStr(int8_t);
	String constructHTMLpage();

	void ssidFromWeb();
	void restart();
};
#endif