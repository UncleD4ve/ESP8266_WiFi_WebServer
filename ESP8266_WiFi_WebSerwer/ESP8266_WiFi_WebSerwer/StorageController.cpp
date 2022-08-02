#include "StorageController.h"

#include "config.h"
#include "debug.h"
#include "EEPROMController.h"

namespace storage {

	EEPROMController *eeprom = EEPROMController::getInstance();
	std::function<void()> _onResetFun = nullptr;
	storage_t data;

	bool changed = false;

	void serialBegin() {
		debug_init();
	}

	void resetByButton() {
		if (RESET_BY_BUTTON) {
			pinMode(RESET_BY_BUTTON_GPIO, INPUT);
			if (digitalRead(RESET_BY_BUTTON_GPIO) == LOW) {
				debugF("RESET by button!");
				pinMode(BUILTIN_LED, OUTPUT);
				digitalWrite(BUILTIN_LED, LOW);
				while (digitalRead(RESET_BY_BUTTON_GPIO) == LOW) yield();
				digitalWrite(BUILTIN_LED, HIGH);
				debuglnF(" Done");
				reset();
				save();
				WiFi.disconnect(true);
				delay(200);
				ESP.restart();
			}
		}
	}

	void setOnResetFunction(std::function<void()> fun) {
		_onResetFun = fun;
	}

	void initialPrint() {
		debuglnF("\n---------------Initial---------------");
		debugf(PSTR("MAC:              %s\n"), WiFi.macAddress().c_str());
		debugf(PSTR("BootReason:       %dB\n"), ESP.getResetInfoPtr()->reason);
		debugf(PSTR("BootReason name:  %sB\n"), ESP.getResetInfo().c_str());
		debugf(PSTR("Heap:             %dB\n"), ESP.getFreeHeap());
		debuglnF("---------------Initial---------------\n");
	}


	void load() {
		debugF("Loading storage status: ");

		storage_t newData;

		eeprom->getObject(EEPROM_BOOT_ADDR, newData);

		if (newData.magic_num == uint32_t(BOOT_MAGIC_NUM)) {
			if (strcmp_P(newData.project.version, PSTR(VERSION)) == 0) {
				data = newData;
				debuglnF("OK");
			}
			else
			{
				debuglnF("Version ERROR!");
				reset();
				save();
			}
		}
		else {
			debuglnF("Boot Number ERROR!");
			reset();
			save();
		}

		changed = true;
	}

	void reset() {
		data.magic_num = BOOT_MAGIC_NUM;
		strncpy_P(data.project.name, PSTR(NAME), 32);
		strncpy_P(data.project.version, PSTR(VERSION), 32);

		data.wifi_mode = WIFI_MODE;

		strncpy_P(data.wifi_ap.ssid, PSTR(AP_SSID), 32);
		strncpy_P(data.wifi_ap.password, PSTR(AP_PASSWD), 64);
		data.wifi_ap.hidden = AP_HIDDEN;
		uint8_t ipap[4] = AP_IP_ADDR;
		memcpy(data.wifi_ap.ip, ipap, 4);

		strncpy_P(data.wifi_st.ssid, PSTR(ST_SSID), 32);
		strncpy_P(data.wifi_st.password, PSTR(ST_PASSWD), 64);
		data.wifi_st.static_ip = ST_STATIC;
		uint8_t ipst[4] = ST_IP_ADDR;
		uint8_t gatewayst[4] = ST_IP_GATEWAY;
		uint8_t subnetst[4] = ST_IP_SUBNET;
		memcpy(data.wifi_st.ip, ipst, 4);
		memcpy(data.wifi_st.gateway, gatewayst, 4);
		memcpy(data.wifi_st.subnet, subnetst, 4);

		changed = true;

		if (_onResetFun != nullptr)
			_onResetFun();
		debuglnF("Storage reset");
	}

	bool save(bool force) {
		if (force || changed)
			if (eeprom->saveObject(EEPROM_BOOT_ADDR, data)) {
				debuglnF("Storage saved");
				changed = false;
				return true;
			}
			else
				debuglnF("Storage save ERROR!");
		return false;
	}

	void print() {
		debuglnF("---------------Storage---------------");
		debugf(PSTR("Project Name:       %s\n"), data.project.name);
		debugf(PSTR("Project Version     %s\n\n"), data.project.version);

		debugf(PSTR("EEPROM SIZE:        %dB\n"), EEPROM_SIZE);
		debugf(PSTR("EEPROM BOOT ADDR:   %dB\n"), EEPROM_BOOT_ADDR);
		debugf(PSTR("Storage Size:       %dB\n"), sizeof(struct storage_t));
		debugf(PSTR("BOOT MAGIC NUM:     %d\n\n"), data.magic_num);

		debugf(PSTR("WiFi Mode:          %d\n\n"), data.wifi_mode);


		debugf(PSTR("WiFi AP SSID:       %s\n"), data.wifi_ap.ssid);
		debugf(PSTR("WiFi AP PASS:       ********\n"));
		debugf(PSTR("WiFi AP IP:         %d.%d.%d.%d\n"), data.wifi_ap.ip[0], data.wifi_ap.ip[1], data.wifi_ap.ip[2], data.wifi_ap.ip[3]);
		debugf(PSTR("WiFi AP Hidden:     %s\n\n"), data.wifi_ap.hidden ? F("True") : F("False"));

		debugf(PSTR("WiFi ST SSID:       %s\n"), data.wifi_st.ssid);
		debugf(PSTR("WiFi ST PASS:       ********\n"));
		debugf(PSTR("WiFi ST IP:         %d.%d.%d.%d\n"), data.wifi_st.ip[0], data.wifi_st.ip[1], data.wifi_st.ip[2], data.wifi_st.ip[3]);
		debugf(PSTR("WiFi ST Gateway:    %d.%d.%d.%d\n"), data.wifi_st.gateway[0], data.wifi_st.gateway[1], data.wifi_st.gateway[2], data.wifi_st.gateway[3]);
		debugf(PSTR("WiFi ST Subnet:     %d.%d.%d.%d\n"), data.wifi_st.subnet[0], data.wifi_st.subnet[1], data.wifi_st.subnet[2], data.wifi_st.subnet[3]);
		debugf(PSTR("WiFi ST Static:     %s\n\n"), data.wifi_st.static_ip ? F("True") : F("False"));

		debuglnF("---------------Storage---------------\n");

	}

	//get

	const storage_t& getAllSettings() {
		return data;
	}

	const uint8_t getWiFiMode() {
		return data.wifi_mode;
	}

	const project_t& getProject() {
		return data.project;
	}

	const wifi_ap_settings_t& getWifiApSettings() {
		return data.wifi_ap;
	}

	const wifi_st_settings_t& getWifiStSettings() {
		return data.wifi_st;
	}

	//set

	void setAllSettings(storage_t& newSettings) {
		data = newSettings;
		strncpy_P(data.project.version, newSettings.project.version, 32);
		changed = true;
	}

	void setWiFiModeSettings(uint8_t mode) {
		data.wifi_mode = mode;
		changed = true;
	}

	void setProjectSettings(const project_t& project_t) {
		data.project = project_t;
		changed = true;
	}

	void setWifiApSettings(const wifi_ap_settings_t& wifi) {
		data.wifi_ap = wifi;
		changed = true;
	}

	void setWifiStSettings(const wifi_st_settings_t& wifi) {
		data.wifi_st = wifi;
		changed = true;
	}

	void setWifiStAllStaticIpSettings() {
		IPAddress ip = WiFi.localIP();
		IPAddress gateway = WiFi.gatewayIP();
		IPAddress subnet = WiFi.subnetMask();

		for (uint8_t i = 0; i < 4; i++)
			data.wifi_st.ip[i] = ip[i];
		for (uint8_t i = 0; i < 4; i++)
			data.wifi_st.gateway[i] = gateway[i];
		for (uint8_t i = 0; i < 4; i++)
			data.wifi_st.subnet[i] = subnet[i];

		changed = true;
	}

	void setWifiStStaticIpSettings(IPAddress& ip) {
		for (uint8_t i = 0; i < 4; i++)
			data.wifi_st.ip[i] = ip[i];

		changed = true;
	}

	void setWifiStStaticSettings(bool sta) {
		data.wifi_st.static_ip = sta;

		changed = true;
	}

}