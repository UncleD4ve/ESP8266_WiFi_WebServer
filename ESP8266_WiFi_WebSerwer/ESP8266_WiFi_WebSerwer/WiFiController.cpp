#include "WiFiController.h"

WiFiController::WiFiController() {}

bool WiFiController::begin(const char* SSID, int8_t mode, bool wake)
{
	_apName = SSID;
	_mode = mode != WIFI_AP_OR_STA ? mode : eeprom.getWifiMode();
	bool status(false);

	pinMode(14, INPUT);
	if (digitalRead(14) == LOW) {
		pinMode(BUILTIN_LED, OUTPUT);
		digitalWrite(BUILTIN_LED, LOW);
		while (digitalRead(14) == LOW) yield();
		digitalWrite(BUILTIN_LED, HIGH);
		eeprom.resetConfig();
	}

	eeprom.readWifi(_ssid, _pass);

	if (wake)
	{
		WiFi.forceSleepWake();
		yield();
		status = connect();
	}

	return status;
}

void WiFiController::forceWifiERegister()
{
	WiFiRegister WiFiReg;
	WiFiReg.begin();
}

bool WiFiController::connect()
{
	yield();
	switch (_mode)
	{
	case WIFI_STA_AP_MODE:
	{
		if (WiFi.status() == WL_CONNECTED)
			return true;

		if (WiFi.getMode() == WIFI_AP)
			WiFi.softAPdisconnect();
		yield();

		bool status(true);
		if (!modeSTA())
			status = modeAP();

		return status;
	}
	case WIFI_STA_MODE:
	{
		if (WiFi.status() != WL_CONNECTED)
			while (!modeSTA())
				delay(1000);
		return true;
	}
	case WIFI_AP_MODE:
	{
		for (uint i(0); i < 3; ++i)
			if (modeAP()) return true;
		resetESP();
	}
	default:
	{
		eeprom.resetConfig();
	}
	}
	return false;
}

bool WiFiController::changeMode(int8_t mode, bool save)
{
	if (mode == WIFI_AP_OR_STA)
		if (WiFi.getMode() == WIFI_AP)
		{
			WiFi.softAPdisconnect();
			dnsServer.stop();
			_mode = WIFI_STA_MODE;
		}
		else
			_mode = WIFI_AP_MODE;
	else
		_mode = mode;

	if (save) eeprom.setWifiMode(_mode);
	return connect();
}

bool WiFiController::modeSTA()
{
	if (!eeprom.getConfig() && (_mode == WIFI_STA_MODE || _mode == WIFI_STA_AP_MODE))
	{
		WiFiRegister WiFiReg(_apName);
		WiFiReg.begin();
	}

	uint32_t time1 = system_get_time(), time2;
	WiFi.persistent(false);
	WiFi.disconnect();
	yield();
	WiFi.mode(WIFI_STA);
	yield();

	if (eeprom.isStaticAddres())
		if (WiFi.config(eeprom.getIp(), eeprom.getGateway(), eeprom.getSubnet()))
		{
			yield();
			Serial.println(F("Static IP!"));
			Serial.println(eeprom.getIp());
			Serial.println(eeprom.getGateway());
			Serial.println(eeprom.getSubnet());
		}

	Serial.print(F("STA mode "));
	WiFi.begin((const char*)_ssid.c_str(), (const char*)_pass.c_str());
	while (WiFi.status() != WL_CONNECTED) {
		yield();
		time2 = system_get_time();
		if (((time2 - time1) / 1e6) > 60)
		{
			Serial.println(F("false"));
			return false;
		}
	}

	Serial.print(F("true, Connected in: "));
	Serial.println((system_get_time() - time1) / 1e6);
	Serial.print(PSTR("IP address: "));
	Serial.println(WiFi.localIP());
	return true;
}

bool WiFiController::modeAP()
{
	IPAddress apIP(5, 5, 5, 5);
	WiFi.disconnect();
	yield();
	WiFi.mode(WIFI_AP);
	yield();
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	yield();

	dnsServer.setTTL(1);
	dnsServer.start(53, "*", apIP);

	if (WiFi.softAP(_apName)) {
		Serial.print(F("AP mode: true, "));
		Serial.println(_apName);
		return true;
	}
	else
		Serial.println(F("AP mode: false"));
	return false;
}

bool WiFiController::checkInternet()
{
	yield();
	return _client.connect(F("httpbin.org"), 80);
}

void WiFiController::dnsLoop()
{
	yield();
	dnsServer.processNextRequest();
}

void WiFiController::resetESP()
{
	Serial.println(F("Device is restarting!"));
	if (WiFi.getMode() == WIFI_AP)
		WiFi.softAPdisconnect();
	yield();
	WiFi.disconnect(true);
	delay(100);
	ESP.restart();
}
