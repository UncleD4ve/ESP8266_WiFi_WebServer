#ifndef WiFiController_h
#define WiFiController_h
#include "WiFiRegister.h"
#include "EEPROMController.h"
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>

enum WiFiControllerMode {
	WIFI_STA_AP_MODE = 0,
	WIFI_STA_MODE = 1,
	WIFI_AP_MODE = 2,
	WIFI_AP_OR_STA = 4
};

class WiFiController {
public:
	WiFiController();
	bool begin(const char* SSID, int8_t mode = WIFI_AP_OR_STA, bool wake = true);
	void forceWifiERegister();
	bool connect();
	bool checkInternet();
	bool changeMode(int8_t mode = WIFI_AP_OR_STA, bool save = false);
	void dnsLoop();
	void resetESP();

private:
	EEPROMController eeprom;
	WiFiClient _client;
	DNSServer dnsServer;

	String _ssid;
	String _pass;
	const char* _apName;
	int8_t _mode;

	bool modeSTA();
	bool modeAP();
};
#endif