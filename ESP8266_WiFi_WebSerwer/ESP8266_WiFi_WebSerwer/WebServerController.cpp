#include "WebServerController.h"
using namespace std::placeholders;

WebServerController::WebServerController() : server(80), ws("/ws"), WiFiContr() {
	ws.enable(0);
}

WebServerController& WebServerController::beginOTA(uint8_t minutes, const char * name, const char * pass) {
	ArduinoOTA.setHostname(name);
	ArduinoOTA.setPassword(pass);
	ArduinoOTA.setPort(OTA_PORT);
	_otaTimer = minutes * 60e3;
	_otaStatus = true;

	ArduinoOTA.onStart([]() { debuglnF("Start");});
	ArduinoOTA.onEnd([]() {debuglnF("End\n");});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {debugln((PSTR("Progress: ") + (progress / (total / 100))));});
	ArduinoOTA.onError([](ota_error_t error) {
		debugF("Error: ");
		if (error == OTA_AUTH_ERROR) { debuglnF("Auth Failed"); }
		else if (error == OTA_BEGIN_ERROR) { debuglnF("Begin Failed"); }
		else if (error == OTA_CONNECT_ERROR) { debuglnF("Connect Failed"); }
		else if (error == OTA_RECEIVE_ERROR) { debuglnF("Receive Failed"); }
		else if (error == OTA_END_ERROR) { debuglnF("End Failed"); }
	});
	ArduinoOTA.begin();
	debuglnF("OTA server started.");

	yield();
	return *this;
}

WebServerController& WebServerController::beginSPIFFS() {
	SPIFFS.begin();
	Serial.println(F("\nSPIFFS started. Contents:"));
	{
		Dir dir = SPIFFS.openDir(F("/"));
		while (dir.next())
			Serial.println(PSTR("FS File: ") + dir.fileName() + PSTR(", size: ") + formatBytes(dir.fileSize()));
		Serial.println(F("End of content."));
	}
	
	yield();
	return *this;
}


void WebServerController::addWsInitial(const char* name, std::function<String()> func) {
	_wsInitial[name] = func;
};

void WebServerController::addWsEvent(const char * name, std::function<void(void * arg, uint8_t *data, size_t len)> func) {
	_wsOnEvent[name] = func;
};

void WebServerController::onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
	if (type == WS_EVT_CONNECT) {
		debugf(PSTR("ws[%s][%u] connect\n"), server->url(), client->id());

		IPAddress ip((WiFi.getMode() == 2 ? storage::getWifiStSettings().ip : WiFi.localIP()));
		
		char systemMsg[150];	
		snprintf_P(systemMsg, 150, PSTR("{\"name\":\"%s\",\"version\":\"%s\",\"wifiMode\":%d,\"staticIp\":\"%s\",\"isStatic\":%d"),storage::getProject().name, storage::getProject().version, storage::getWiFiMode(), ip.toString().c_str(), storage::getWifiStSettings().static_ip);

		String userMsg;
		for (auto map : _wsInitial)
			userMsg += String(F(",\"")) + String(map.first) + String(F("\":\"")) + map.second() + String(F("\""));

		client->text(systemMsg + userMsg + '}');
		client->ping();

		debugF("ws initial message: ");
		debugf(PSTR("%s%s}\n"),systemMsg, userMsg.c_str());
	}
	else if (type == WS_EVT_DISCONNECT) {
		Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
	}
	else if (type == WS_EVT_ERROR) {
		Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
	}
	else if (type == WS_EVT_PONG) {
		Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char*)data : "");
	}
	else if (type == WS_EVT_DATA) {
		AwsFrameInfo * info = (AwsFrameInfo*)arg;

		if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
			data[len] = 0;
			debugf(PSTR("ws[%s][%u] %s-message[%llu]: %s\n"), server->url(), client->id(), (info->opcode == WS_TEXT) ? F("text") : F("binary"), info->len, (char*)data);

			if ((char)data[0] == '{' && (char)data[len - 1] == '}' && strchr((char*)data, ':') != NULL) {

				data[len-1] = 0;
				char * event = strtok((char*)data, ":");
				uint8_t eventLen = strlen(event);

				if (event[1] == '"' && event[eventLen - 1] == '"')
				{
					event[eventLen - 1] = 0;
					event++;
				}
				event++;
				
				auto e = _wsOnEvent.find(event);

				if (e != _wsOnEvent.end()) 
					e->second(arg, data + eventLen + 1, len - eventLen - 2);	// len - {"prop" - ':' - '}'
				else {
					debugf("Event %s not found. %d events available\n", event, _wsOnEvent.size());
				}

			}


			
				
		}
	}
}


WebServerController& WebServerController::beginWsServer() {

	ws.enable(true);

	ws.onEvent(std::bind(&WebServerController::onWsEvent, this, _1, _2, _3, _4, _5, _6));

	addWsEvent("_setStatic_", [&](void * arg, uint8_t *data, size_t len) {
		IPAddress ip;
		data[len - 1] = 0;
		ip.fromString((char*)data+1);
		storage::setWifiStStaticIpSettings(ip);
		storage::setWifiStStaticSettings(true);
		wsAction = SERVER_WS_SAVE_RESTART;
	});

	addWsEvent("_changeWiFiMode_", [&](void * arg, uint8_t *data, size_t len) {
		wsAction = SERVER_WS_CHANGE_WIFI_MODE;
	});

	addWsEvent("_setWiFiMode_", [&](void * arg, uint8_t *data, size_t len) {
		wsAction = SERVER_WS_SET_WIFI_MODE;
	});

	addWsEvent("_changeWiFiConn_", [&](void * arg, uint8_t *data, size_t len) {
		storage::setWifiStStaticSettings(false);
		wsAction = SERVER_WS_SAVE_RESTART;
	});

	addWsEvent("_restart_", [&](void * arg, uint8_t *data, size_t len) {
		wsAction = SERVER_WS_RESTART;
	});

	addWsEvent("_turnOff_", [&](void * arg, uint8_t *data, size_t len) {
		wsAction = SERVER_WS_TURN_OFF;
	});

	server.addHandler(&ws);

	debuglnF("WebSocet server started.");

	yield();
	return *this;
}

WebServerController& WebServerController::beginServer(bool editor) {

	server.on(PSTR("/heap"), HTTP_GET, [](AsyncWebServerRequest *request) {request->send(200, "text/plain", String(ESP.getFreeHeap()));});

	server.serveStatic(PSTR("/"), SPIFFS, PSTR("/")).setDefaultFile(PSTR("index.html")).setCacheControl(PSTR("max-age=600"));

	if(editor)
		server.addHandler(new SPIFFSEditor(PSTR(SPIFFS_SSID), PSTR(SPIFFS_PASSWD)));

	server.onNotFound([](AsyncWebServerRequest *request) {
		if (request->url() == F("/generate_204") || request->url() == F("/connecttest.txt") || request->url() == F("/gen_204"))
			return;

		debugF("NOT_FOUND: ");
		if (request->method() == HTTP_GET) {
			debugF("GET");
		}
		else if (request->method() == HTTP_POST) {
			debugF("POST");
		}
		else if (request->method() == HTTP_DELETE) {
			debugF("DELETE");
		}
		else if (request->method() == HTTP_PUT) {
			debugF("PUT");
		}
		else if (request->method() == HTTP_PATCH) {
			debugF("PATCH");
		}
		else if (request->method() == HTTP_HEAD) {
			debugF("HEAD");
		}
		else if (request->method() == HTTP_OPTIONS) {
			debugF("OPTIONS");
		}
		else {
			debugF("UNKNOWN");
		}
		debugf(PSTR(" http://%s%s\n"), request->host().c_str(), request->url().c_str());

		request->send(404);
	});
	server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
		if (!index) {
			debugf(PSTR("UploadStart: %s\n"), filename.c_str());
		}
		debugf(PSTR("%s"), (const char*)data);
		if (final) {
			debugf(PSTR("UploadEnd: %s (%u)\n"), filename.c_str(), index + len);
		}
	});
	server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
		if (!index) {
			debugf(PSTR("BodyStart: %u\n"), total);
		}
		debugf(PSTR("%s"), (const char*)data);
		if (index + len == total) {
			debugf(PSTR("BodyEnd: %u\n"), total);
		}
	});
	server.begin();

	debuglnF("HTML server started.");
	
	yield();
	return *this;
}

















//void WebServerController::handleFileUpload() {
//	_server.send(200, F("text/plain"), "");
//	HTTPUpload& upload = _server.upload();
//	String path;
//	if (upload.status == UPLOAD_FILE_START) {
//		path = upload.filename;
//		if (!path.startsWith("/")) path = "/" + path;
//		if (!path.endsWith(".gz")) {
//			String pathWithGz = path + ".gz";
//			if (SPIFFS.exists(pathWithGz))
//				SPIFFS.remove(pathWithGz);
//		}
//		Serial.print(F("handleFileUpload Name: ")); Serial.println(path);
//		_fsUploadFile = SPIFFS.open(path, "w");
//		path = String();
//	}
//	else if (upload.status == UPLOAD_FILE_WRITE) {
//		if (_fsUploadFile)
//			_fsUploadFile.write(upload.buf, upload.currentSize);
//	}
//	else if (upload.status == UPLOAD_FILE_END) {
//		if (_fsUploadFile) {
//			_fsUploadFile.close();
//			Serial.print(F("handleFileUpload Size: ")); Serial.println(upload.totalSize);
//			_server.sendHeader(F("Location"), F("/success.html"));
//			_server.send(303);
//		}
//		else {
//			_server.send(500, F("text/plain"), F("500: couldn't create file"));
//		}
//	}
//}

//void WebServerController::webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
//	Serial.println(F("\nwebSocketEvent"));
//	switch (type) {
//	case WStype_DISCONNECTED:
//		yield();
//		Serial.printf_P(PSTR("[%u] Disconnected!\n"), num);
//		break;
//	case WStype_CONNECTED:
//	{
//		yield();
//		//IPAddress ip = _webSocket.remoteIP(num), staticIP;
//		IPAddress ip , staticIP;
//
//		Serial.printf_P(PSTR("[%u] Connected from %d.%d.%d.%d url: %s\r\n"), num, ip[0], ip[1], ip[2], ip[3], payload);
//
//		staticIP = (WiFi.getMode() == 2 ? storage::getWifiStSettings().ip : WiFi.localIP());
//
//		char * buff(_webSocketOnInitFunction ? _webSocketOnInit() : webSocketInit());
//		strcat(buff, (storage::getWifiStSettings().static_ip ? ",S," : ",D,"));
//		strcat(buff, staticIP.toString().c_str());
//
//		Serial.printf_P(PSTR("Init data: %s\n"), buff);
//		//_webSocket.sendTXT(num, buff);
//		delete[] buff;
//		
//		break;
//	}
//	case WStype_TEXT:	//TODO JSON
//		yield();
//		Serial.printf_P(PSTR("[%u] get Text: %s\r\n"), num, payload);
//
//		if (payload[0] == '}')
//		{
//			IPAddress ip;
//			ip.fromString((char*)&payload[1]);
//			Serial.println(ip);
//			storage::setWifiStStaticIpSettings(ip);
//			storage::setWifiStStaticSettings(true); //
//			storage::save();
//			WiFiContr.restartESP();
//
//			return;
//		}
//
//		if (payload[0] == '{') {
//			
//			uint8_t button(atoi((char*)&payload[1]));
//			switch (button)
//			{
//			case 0:
//			{
//				WiFiContr.changeMode(WIFI_AP_OR_STA,true);
//				beginServer();
//				beginWebSocket();
//				yield();
//				return;
//			}
//			case 1:
//			{
//				WiFiContr.restartESP();
//				return;
//			}
//			case 2:
//			{
//				storage::setWifiStStaticSettings(false); //
//				if (storage::save())
//					WiFiContr.connect();
//				return;
//			}
//			case 3:
//			{
//				ESP.deepSleep(0);
//				return;
//			}
//			default:
//			{
//				return;
//			}
//			}
//		}
//
//		
//
//		if (_webSocketOnSwitchFunction) 
//			_webSocketOnSwitch(payload[0], (uint8_t*)&payload[1]);
//		else
//			webSocketSwitch(payload[0], (uint8_t*)&payload[1]);
//		break;
//
//	case WStype_BIN:
//		yield();
//		Serial.println(F("Get binary"));
//		hexdump(payload, length);
//		//_webSocket.sendBIN(num, payload, length);
//		break;
//	default:
//		yield();
//		Serial.println(F("Invalid WStype"));
//		break;
//	}
//}

String WebServerController::formatBytes(size_t bytes) {
	if (bytes < 1024)
		return String(bytes) + 'B';
	else if (bytes < (1024 * 1024))
		return String(bytes / 1024.0) + F("KB");
	else if (bytes < (1024 * 1024 * 1024))
		return String(bytes / 1024.0 / 1024.0) + F("MB");
}

void WebServerController::WebServerLoop(bool _PreventEspStuck , bool _resetConnectionByTime ) {
	yield();

	if (WiFi.getMode() == 2)
		WiFiContr.dnsLoop();

	if(ws.enabled())
		ws.cleanupClients();

	if (_PreventEspStuck)
		PreventEspStuck();

	if (_resetConnectionByTime)
		resetConnectionByTime();

	if (_otaStatus)
	{
		ArduinoOTA.handle();
		yield();
		if (_otaTimer && millis() >= _otaTimer )
		{
			_otaStatus = false;
			debuglnF("OTA server stoped.");
			ArduinoOTA.~ArduinoOTAClass();
		}
	}

	switch (wsAction)
	{
	case SERVER_WS_CHANGE_WIFI_MODE:
		wsAction = SERVER_WS_NULL;
		WiFiContr.changeMode(WIFI_AP_OR_STA, false);
		break;
	case SERVER_WS_SET_WIFI_MODE:
		wsAction = SERVER_WS_NULL;
		WiFiContr.changeMode(WIFI_AP_OR_STA, true);
		break;
	case SERVER_WS_SAVE_RESTART:
		wsAction = SERVER_WS_NULL;
		if (storage::save());
			WiFiContr.restartESP();
		break;
	case SERVER_WS_RESTART:
		wsAction = SERVER_WS_NULL;
		WiFiContr.restartESP();
		break;
	case SERVER_WS_CHANGE_WIFI_CONNECTION:
		wsAction = SERVER_WS_NULL;
		if (storage::save())
			WiFiContr.connect();
		break;
	case SERVER_WS_TURN_OFF:
		wsAction = SERVER_WS_NULL;
		ESP.deepSleep(0);
		break;

	default:
		break;
	}

	yield();
}

void WebServerController::PreventEspStuck() {

	if (millis() - dieTimer >= 3000) {
		if (++dieCounter > 1) 
		{
			debugf(PSTR("Stuck counter: %d/3\n"), dieCounter);
		}
		if (dieCounter > 2)
			WiFiContr.restartESP();
	}
	else
		dieCounter = 0;
	dieTimer = millis();
	yield();
}

void WebServerController::resetConnectionByTime(uint16_t minutes)
{
	if (millis() - wiFiTimer >= minutes * 60e3) {
		wiFiTimer = millis();
		WiFiContr.connect();
		yield();
	}
}





