#include "WiFiController.h"
#include "WebServerController.h"
#include "reServo.h"
#include "StorageController.h"
#include "debug.h"

#include <Adafruit_NeoPixel.h>

void preinit() {
	ESP8266WiFiClass::preinitWiFiOff();
}

WebServerController WebServerContr;

// Example variables

Adafruit_NeoPixel led(40, D2, NEO_GRB + NEO_KHZ800);
uint8_t red(0), green(0), blue(0);

uint32_t secondTimer(0), servoWebSocket(0);
uint16_t seconds(0);
uint8_t position(0), destPosition(0), dieCounter(0);
uint8_t saveA;

bool changeButton(false);

enum wsEvents {
	WS_NULL = 0,
	WS_BUTTON_CLICK = 1,
	WS_COLOR_PICKER = 2
};
uint8_t wsEvent = WS_NULL;

// End of example variables

void setup()
{
	storage::serialBegin();
	storage::initialPrint();

	storage::addVar("saveA", saveA);
	storage::setOnResetFunction([]() {
		saveA = 0;
		storage::saveVar("saveA", saveA);
	});
	storage::getVar("saveA", saveA);


	storage::resetByButton();
	storage::load();

	storage::print();


	WebServerContr.addWsInitial(PSTR("initText"), []() {return "initialText"; });
	WebServerContr.addWsInitial(PSTR("initseconds"), []() {return String(seconds); });
	WebServerContr.addWsInitial(PSTR("initPosition"), []() {return String(position); });
	WebServerContr.addWsInitial(PSTR("initSaveA"), []() {return String(saveA); });
	WebServerContr.addWsInitial(PSTR("initChangeButton"), []() {return String(changeButton); });

	WebServerContr.addWsEvent("colorPicker", [](void * arg, uint8_t *data, size_t len) {
		char temp[2] = { data[0] , data[1] };
		red = (uint8_t)strtol(temp, NULL, 16);
		temp[0] = data[2]; temp[1] = data[3];
		green = (uint8_t)strtol(temp, NULL, 16);
		temp[0] = data[4]; temp[1] = data[5];
		blue = (uint8_t)strtol(temp, NULL, 16);
		wsEvent = WS_COLOR_PICKER;
	});

	WebServerContr.addWsEvent("slider", [](void * arg, uint8_t *data, size_t len) {
		destPosition = atoi((char*)data);
	});

	WebServerContr.addWsEvent("buttonClick", [](void * arg, uint8_t *data, size_t len) {
		wsEvent = WS_BUTTON_CLICK;
	});

	WebServerContr.addWsEvent("buttonChange", [](void * arg, uint8_t *data, size_t len) {
		changeButton = !changeButton;
		debugf(PSTR("Change Button: %d\n"), changeButton);
		WebServerContr.ws.printfAll_P(PSTR("{\"changeButton\":%d}"), changeButton);
	});

	WebServerContr.addWsEvent("buttonSetValue", [](void * arg, uint8_t *data, size_t len) {
		WebServerContr.ws.printfAll_P(PSTR("{\"saveA\":%d}"), position);
	});

	WebServerContr.addWsEvent("buttonRestoreValue", [](void * arg, uint8_t *data, size_t len) {
		storage::getVar("saveA", destPosition);
		debugf(PSTR("Restore Value: %d\n"), destPosition);
	});

	//WebServerContr.WiFiContr.forceWifiRegister();
	if (WebServerContr.WiFiContr.begin())
		WebServerContr.beginSPIFFS().beginServer(true).beginWsServer().beginOTA(10);



	pinMode(BUILTIN_LED, OUTPUT);
	digitalWrite(BUILTIN_LED, HIGH);
	led.begin();
}

void loop()
{
	WebServerContr.WebServerLoop();

	if (millis() - servoWebSocket >= 20)
	{
		servoWebSocket = millis();

		if (position != destPosition)
		{
			position = destPosition;
			WebServerContr.ws.printfAll_P(PSTR("{\"position\":%d}"), position);
			yield();
		}

		switch (wsEvent)
		{
		case WS_BUTTON_CLICK:
			wsEvent = WS_NULL;
			blink();
			break;

		case WS_COLOR_PICKER:
			wsEvent = WS_NULL;
			changeColor();
			break;
		default:
			break;
		}
	}

	if (millis() - secondTimer >= 1e3)
	{
		secondTimer = millis(); yield();
		seconds++;
	}

}

void blink() {
	digitalWrite(BUILTIN_LED, LOW);
	delay(500);
	digitalWrite(BUILTIN_LED, HIGH);
}

void changeColor() {
	debugf(PSTR("R:%d, G:%d, B:%d\n"), red, green, blue);
	led.fill(led.Color(red, green, blue));
	led.show();
}






// Var examples

//int integer = 123456789;
//storage::addVar("intiger", integer);

//char character = 'A';
//storage::addVar("char", character);

//char str[] = "abc";
//storage::addVar("str", str);
//str[0] = '1'; str[1] = '2'; str[2] = '3';

////storage::saveVar("str", str);

//debugf(PSTR("Storage get Int: %d\n"),storage::getVar("intiger", integer));
//debugf(PSTR("Storage get char: %d\n"), storage::getVar("char", character));
//storage::getVar("str", str);
//debugf(PSTR("Storage get str: %s\n"), str);
//debugf(PSTR("Storage get null: %d\n"), storage::getVar("null", integer));