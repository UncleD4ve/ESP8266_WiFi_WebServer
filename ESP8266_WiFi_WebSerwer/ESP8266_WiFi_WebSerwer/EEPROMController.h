#pragma once

#ifndef EEPROMController_h
#define EEPROMController_h
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "debug.h"

class EEPROMController {
private:
	EEPROMController() { EEPROM.begin(EEPROM_SIZE); delay(10); };
	EEPROMController(EEPROMController const&) {};
	void operator=(EEPROMController const&) {};

public:

	static EEPROMController* getInstance() {
		static EEPROMController eeprom;
		return &eeprom;
	};

	template<typename T>
	bool saveObject(const int address, const T& t) {
		EEPROM.put(address, t);
		return EEPROM.commit();
	}

	template<typename T>
	T & getObject(const int address, T& t) {
		return EEPROM.get(address, t);
	}

};
#endif