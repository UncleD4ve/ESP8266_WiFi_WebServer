#pragma once

#ifndef WebServerController_h
#define WebServerController_h

#include "Arduino.h"
#include "WiFiController.h"
#include "StorageController.h"
#include "debug.h"

#include <ArduinoOTA.h>
#include <FS.h>
#include <map>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

enum wsActionEnum {
	SERVER_WS_NULL = 0,
	SERVER_WS_CHANGE_WIFI_MODE = 1,
	SERVER_WS_RESTART = 2,
	SERVER_WS_SAVE_RESTART = 3,
	SERVER_WS_CHANGE_WIFI_CONNECTION = 4,
	SERVER_WS_TURN_OFF = 5,
	SERVER_WS_SET_WIFI_MODE = 6,
};

class WebServerController {
public:
	WebServerController();
	WebServerController& beginOTA(uint8_t minutes, const char* PROGMEM = OTA_SSID, const char* PROGMEM = OTA_PASSWD);
	WebServerController& beginSPIFFS();
	WebServerController& beginWsServer();
	WebServerController& beginServer(bool editor = false);

	void PreventEspStuck();
	void resetConnectionByTime(uint16_t minutes = 5);

	void WebServerLoop(bool PreventEspStuck = true, bool resetConnectionByTime = false);

	void addWsInitial(const char*, std::function<String()>);
	void addWsEvent(const char *, std::function<void(void *, uint8_t *, size_t)>);

	WiFiController WiFiContr;
	AsyncWebSocket ws;
private:
	std::map<const char *, std::function<String()>> _wsInitial;
	struct str_cmp { bool operator()(char const *a, char const *b) const { return strcmp(a, b) < 0; } };
	std::map<const char *, std::function<void(void *, uint8_t *, size_t)>,str_cmp > _wsOnEvent;
	
	AsyncWebServer server;

	File _fsUploadFile;

	uint32_t dieTimer = 0;
	uint32_t wiFiTimer = 0;
	uint32_t _otaTimer = 0;
	uint8_t  dieCounter = 0;

	char buffer[5];

	uint8_t wsAction = 0;
	bool _otaStatus = false;

	void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
	String formatBytes(size_t);
};
#endif