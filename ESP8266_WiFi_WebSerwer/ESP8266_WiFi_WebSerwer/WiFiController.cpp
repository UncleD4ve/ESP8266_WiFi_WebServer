#include "WiFiController.h"

WiFiController::WiFiController() {}

bool WiFiController::begin(int8_t mode, bool wake)
{
	if (mode != WIFI_AP_OR_STA)
		storage::setWiFiModeSettings(mode);

	bool status(false);

	if (wake)
	{
		WiFi.forceSleepWake();
		yield();
		status = connect();
	}

	return status;
}

void WiFiController::forceWifiRegister()
{
	if (WiFi.getMode() == WIFI_AP)
		WiFi.softAPdisconnect();
	yield();
	WiFi.disconnect(true);
	dnsServer.stop();
	delay(100);

	WiFiRegister WiFiReg;
	WiFiReg.begin();
}

bool WiFiController::connect()
{
	yield();
	switch (storage::getWiFiMode())
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
			restartESP();
		}
		default:
		{
			storage::reset();
		}
	}
	return false;
}

bool WiFiController::changeMode(int8_t mode, bool save)
{
	if (mode == WIFI_AP_OR_STA) {
		if (WiFi.getMode() == WIFI_AP)
		{
			WiFi.softAPdisconnect();
			dnsServer.stop();
			storage::setWiFiModeSettings(WIFI_STA_MODE);
		}
		else
			storage::setWiFiModeSettings(WIFI_AP_MODE);
	}
	else
		storage::setWiFiModeSettings(mode);

	if (save) storage::save();
	return connect();
}

bool WiFiController::modeSTA()
{
	if (strlen(storage::getWifiStSettings().ssid) == 0 && (storage::getWiFiMode() == WIFI_STA_MODE || storage::getWiFiMode() == WIFI_STA_AP_MODE))
		forceWifiRegister();

	uint32_t time1 = system_get_time(), time2;
	WiFi.persistent(false);
	WiFi.disconnect();
	yield();
	WiFi.mode(WIFI_STA);
	yield();

	if (storage::getWifiStSettings().static_ip)
		if (WiFi.config(storage::getWifiStSettings().ip, storage::getWifiStSettings().gateway, storage::getWifiStSettings().subnet))
		{
			yield();
			debuglnF("Static IP!");
			debugf(PSTR("IP:        %s\n"), IPAddress(storage::getWifiStSettings().ip).toString().c_str());
			debugf(PSTR("Gateway:   %s\n"), IPAddress(storage::getWifiStSettings().gateway).toString().c_str());
			debugf(PSTR("Subnet:    %s\n"), IPAddress(storage::getWifiStSettings().subnet).toString().c_str());
		}

	debugF("STA mode: ");
	WiFi.begin(storage::getWifiStSettings().ssid, storage::getWifiStSettings().password);
	while (WiFi.status() != WL_CONNECTED) {
		yield();
		time2 = system_get_time();
		if (((time2 - time1) / 1e6) > 60)
		{
			debuglnF("false");
			return false;
		}
	}

	debugF("true, Connected in: ");
	debugln((system_get_time() - time1) / 1e6);
	debugf(PSTR("IP address: %s"),WiFi.localIP().toString().c_str());
	return true;
}

bool WiFiController::modeAP()
{
	WiFi.disconnect();
	yield();
	WiFi.mode(WIFI_AP);
	yield();
	WiFi.softAPConfig(storage::getWifiApSettings().ip, storage::getWifiApSettings().ip, IPAddress(255, 255, 255, 0));
	yield();

	dnsServer.setTTL(AP_DNS_TTL);
	dnsServer.start(AP_DNS_PORT, AP_DNS_DOMAIN, storage::getWifiApSettings().ip);

	if (WiFi.softAP(storage::getWifiApSettings().ssid)) {
		debugF("AP mode: true, ");
		Serial.println(storage::getWifiApSettings().ssid);
		return true;
	}
	else
		debuglnF("AP mode: false");
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

void WiFiController::restartESP()
{
	debuglnF("Device is restarting!");
	if (WiFi.getMode() == WIFI_AP)
		WiFi.softAPdisconnect();
	yield();
	WiFi.disconnect(true);
	delay(100);
	ESP.restart();
}
