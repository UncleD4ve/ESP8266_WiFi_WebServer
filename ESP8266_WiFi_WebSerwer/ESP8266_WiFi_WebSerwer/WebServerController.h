#ifndef WebServerController_h
#define WebServerController_h
#include "Arduino.h"
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include "WiFiController.h"
#include <DNSServer.h>

class WebServerController {
public:
	WebServerController();
	void beginOTA(const char*, const char*);
	void beginSPIFFS();
	void beginWebSocket();
	void beginServer();

	void webSocketSend(char sign, uint8_t num);
	void webSocketSendText(char * text);
	void PreventEspStuck();

	virtual void webSocketSwitch(uint8_t, uint8_t*);
	virtual char * webSocketInit();

	void webSocketLoop();
	void handleClientLoop();
	void otaLoop();

	void webSocketOnInit(std::function<char *()>);
	void webSocketOnSwitch(std::function<void(uint8_t sign, uint8_t * payload)>);

	WiFiController WiFiContr;
	EEPROMController eeprom;
private:
	ESP8266WebServer _server;
	WebSocketsServer _webSocket;
	File _fsUploadFile;

	uint32_t dieTimer = 0;
	uint8_t  dieCounter = 0;

	void handleRoot();
	void handleFileUpload();
	bool handleFileRead(String);

	void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
	String formatBytes(size_t);
	String getContentType(String);

	std::function<char *()> _webSocketOnInit;
	bool _webSocketOnInitFunction = 0;
	std::function<void(uint8_t sign, uint8_t * payload)> _webSocketOnSwitch;
	bool _webSocketOnSwitchFunction = 0;

	char buffer[5];
};
#endif