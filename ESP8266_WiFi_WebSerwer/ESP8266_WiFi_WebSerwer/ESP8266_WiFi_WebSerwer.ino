#include "WiFiController.h"
#include "WebServerController.h"
#include "reServo.h"

#define DNS_SSID "IoT WebServer"

uint32_t wiFiTimer(0), buttonsTimer(0), servoIsOnTimer(0), servoWebSocket(0), dieTimer(0);
uint8_t position(0), destPosition(0), dieCounter(0);

bool changeButton(false);

class OwnWebServer : public WebServerController {
public:
	uint8_t & _position = position;
	uint8_t & _destPosition = destPosition;
	bool & _changeButton = changeButton;

	OwnWebServer() : WebServerController()
	{
		eeprom.getVar(1, _position);
		_destPosition = _position;
	}

	char * webSocketInit() override {
		uint8_t saveA;
		char * buff = new char[40];
		eeprom.getVar(1, saveA);
		sprintf_P(buff, PSTR("I%d,%d,%d"), _position, saveA, _changeButton);
		return buff;
	}

	void webSocketSwitch(uint8_t sign, uint8_t * payload) override{

		if (sign == 'a') {
			_destPosition = atoi((char*)&payload[0]);
			return;
		}

		if (sign == 'b') {
			uint8_t button(atoi((char*)&payload[0]));
			switch (button)
			{
				case 0:
				{
					eeprom.setVar(1, _position);
					webSocketSend('B', _position);
					break;
				}
				case 1:
				{
					eeprom.getVar(1, _destPosition);
					Serial.printf_P(PSTR("Restore Value: %d\n"), _destPosition);
					break;
				}
				case 2:
				{
					_changeButton = !_changeButton;
					Serial.printf_P(PSTR("Change Button: %d\n"), _changeButton);
					webSocketSend('C', _changeButton);
					break;
				}
				case 3:
				{
					Serial.println(F("Click Button"));
					digitalWrite(BUILTIN_LED, LOW);
					delay(500);
					digitalWrite(BUILTIN_LED, HIGH);
					break;
				}
				default:
				{
					break;
				}
			}
			yield();
			return;
		}


	}
}WebServerContr;


void preinit() {
	ESP8266WiFiClass::preinitWiFiOff();
}

void setup()
{
	Serial.begin(115200);
	yield();

	Serial.printf_P(PSTR("MAC: %s Heap: %d\n"), WiFi.macAddress().c_str(), ESP.getFreeHeap());

	//WebServerContr.WiFiContr.forceWifiERegister();
	if (WebServerContr.WiFiContr.begin(DNS_SSID)) {
		WebServerContr.beginSPIFFS();
		WebServerContr.beginWebSocket();
		WebServerContr.beginServer();
		//WebServerContr.beginOTA("root","root");
	}

	pinMode(BUILTIN_LED, OUTPUT);
	digitalWrite(BUILTIN_LED, HIGH);
}
void loop()
{
	yield();

	WebServerContr.webSocketLoop();
	WebServerContr.handleClientLoop();
	//WebServerContr.otaLoop();


	if (millis() - servoWebSocket >= 20)
	{
		servoWebSocket = millis(); yield();
		if (position != destPosition)
		{
			position = destPosition;
			WebServerContr.webSocketSend('A', position);
			yield();
		}
	}

	if (millis() - wiFiTimer >= 660000) {
		wiFiTimer = millis();
		WebServerContr.WiFiContr.connect();
	}

	WebServerContr.PreventEspStuck();
}
