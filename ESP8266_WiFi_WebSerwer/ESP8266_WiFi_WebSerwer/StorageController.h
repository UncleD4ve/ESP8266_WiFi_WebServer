#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <map>
#include "EEPROMController.h"


typedef struct project_t {
	char    name[33];
	char    version[33];
} project_t;

typedef struct wifi_st_settings_t {
	char    ssid[33];
	char    password[65];
	uint8_t ip[4];
	uint8_t subnet[4];
	uint8_t gateway[4];
	bool    static_ip;
} wifi_st_settings_t;

typedef struct wifi_ap_settings_t {
	char    ssid[33];
	char    password[65];
	uint8_t ip[4];
	bool    hidden;
} wifi_ap_settings_t;

typedef struct storage_t {
	uint32_t                magic_num;
	project_t               project;
	wifi_st_settings_t      wifi_st;
	wifi_ap_settings_t		wifi_ap;
	int8_t					wifi_mode;
} storage_t;

namespace storage {

	void load();
	bool save(bool force = false);
	
	static std::map<const char *, uint16_t> _varLocation;
	static uint16_t _eepromPointer = 0;

	void setOnResetFunction (std::function<void()>);

	void reset();
	void print();
	void initialPrint();
	void serialBegin();
	void resetByButton();

	const storage_t& getAllSettings();
	const project_t& getProject();
	const wifi_st_settings_t& getWifiStSettings();
	const wifi_ap_settings_t& getWifiApSettings();
	const uint8_t getWiFiMode();

	void setAllSettings(storage_t& );
	void setWifiStSettings(const wifi_st_settings_t& );
	void setWifiApSettings(const wifi_ap_settings_t& );
	void setProjectSettings(const project_t& );
	void setWiFiModeSettings(const uint8_t );
	void setWifiStAllStaticIpSettings();
	void setWifiStStaticSettings(bool);
	void setWifiStStaticIpSettings(IPAddress&);

	template<typename T>
	void addVar(const char * name, const T& t) {
		_varLocation[name] = _eepromPointer;
		debugf(PSTR("Var: %s add at adress: %d size: %d\n"), name, _eepromPointer, sizeof(t));
		_eepromPointer += sizeof(t);

	}

	template<typename T>
	bool saveVar(const char * name, T& t) {
		auto e = _varLocation.find(name);

		if (e != _varLocation.end()) {
			debugf(PSTR("Var: %s save at adress: %d size: %d\n"), name, e->second, sizeof(t));
			EEPROMController *eeprom = EEPROMController::getInstance();
			return eeprom->saveObject(e->second, t);
		}
		else
			return false;
	}

	template<typename T>
	T & getVar(const char * name, T& t) {
		auto e = _varLocation.find(name);

		if (e != _varLocation.end()) {
			debugf(PSTR("Var: %s read from adress: %d size: %d\n"), name, e->second, sizeof(t));
			EEPROMController *eeprom = EEPROMController::getInstance();
			return eeprom->getObject(e->second, t);
		}
		else
		{
			debugf(PSTR("Var: %s doesn't exist! Value unchanged.\n"), name);
			return t;
		}
	}

}