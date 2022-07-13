#include "WiFiRegister.h"

WiFiRegister::WiFiRegister(const char * ssid) :_server(80), _apName(ssid) {}
WiFiRegister::WiFiRegister() : _server(80), _apName(NULL) {}

void WiFiRegister::begin()
{
	WiFi.forceSleepWake();
	delay(500);
	uint8_t apIP[] = { 5, 5, 5, 5 };
	WiFi.disconnect(true);
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	dnsServer.setTTL(60);
	dnsServer.start(53, "*", apIP);

	if (WiFi.softAP(!_apName ? String(F("ESP WiFi Register")) : _apName)) {
		debugF("\nWifi Register\nAccess Point name: ");
		debug(WiFi.softAPSSID());
		debugF("\nIP addres:");
		debugln(WiFi.softAPIP().toString().c_str());
	}
	else {
		debuglnF("FAILED to create Access Point, restart!");
		yield();
		ESP.restart();
	}

	WiFi.scanNetworks(true);

	_server.on(PSTR("/"), HTTP_GET, [&](AsyncWebServerRequest *request) {
		request->send(200, F("text/html"), constructHTMLpage());
	});

	_server.on(PSTR("/connect"), HTTP_GET, [&](AsyncWebServerRequest *request) {
		if (request->arg(F("key")) == F("19") && _status[0] != 'W') {
			request->send(200, F("text/html"), F("ok"));

			strcpy_P(_status, PSTR("W1"));
			debuglnF("Connecting");

			strncpy(_ssid, request->arg(F("ssid")).c_str(), 32);
			strncpy(_pass, request->arg(F("pass")).c_str(), 64);

			action = MODE_CONNECTION;
		}
		else
			request->send(200, F("text/html"), F("nok"));
	});

	_server.on(PSTR("/status"), HTTP_GET, [&](AsyncWebServerRequest *request) {
		request->send(200, F("text/plain"), _status);
	});

	_server.on(PSTR("/restart"), HTTP_GET, [&](AsyncWebServerRequest *request) {
		action = MODE_RESTART;
	});
	_server.begin();

	while (true)
	{
		dnsServer.processNextRequest();
		yield();
		switch (action)
		{
		case MODE_CONNECTION:
			ssidFromWeb();
			break;
		case MODE_RESTART:
			restart();
			break;
		default:
			break;
		}	
	}
}

void WiFiRegister::restart()
{
	if (_status[0] == 'I')
	{
		storage::setWifiStAllStaticIpSettings();
		wifi_st_settings_t newData = storage::getWifiStSettings();
		strncpy(newData.ssid,_ssid,32);
		strncpy(newData.password, _pass, 64);
		storage::setWifiStSettings(newData);
		
		if (storage::save()) {
			debuglnF("Device is restarting!");
			WiFi.softAPdisconnect();
			yield(); 
			WiFi.disconnect(true);
			delay(3000);
			ESP.restart();
		}
	}
}

void WiFiRegister::ssidFromWeb() {

	uint8_t i(0), multi(4);
	unsigned long previousMillis = 0;

	WiFi.begin(_ssid, _pass);
	while (WiFi.status() != WL_CONNECTED && i <= 10 * multi) {

		unsigned long currentMillis = millis();
		if (currentMillis - previousMillis >= 500) {
			previousMillis = currentMillis;
			if (++i % multi == 0)
			{
				sprintf_P(_status,PSTR("W%d"), i / multi);
				debugln(i / multi);
			}
		}
		dnsServer.processNextRequest();
		yield();
	}
	yield();
	if (WiFi.status() == WL_CONNECTED)		
		sprintf_P(_status, PSTR("I%s"), WiFi.localIP().toString().c_str());	
	else
	{
		strcpy_P(_status, PSTR("N"));
		WiFi.scanNetworks(true);
	}

	action = MODE_DEFAULT;
}

uint8_t WiFiRegister::encryptionTypeStr(uint8_t authmode) {
	switch (authmode) {
	case ENC_TYPE_NONE:
		return 5;
	default:
		return 4;
	}
}

uint8_t WiFiRegister::encryptionPowerStr(int8_t power) {
	if (power <= -100)
		return 0;
	else if (power >= -50)
		return 100;
	else
		return 2 * (power + 100);
}

const char * WiFiRegister::encryptionColorStr(int8_t power)
{
	if (power < -80)
		return PSTR("salmon");
	else if (power >= -50)
		return PSTR("lightgreen");
	else if (power >= -80 && power < -70)
		return PSTR("orange");
	else
		return PSTR("yellow");
}

const char indexBegin[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, height=device-height, user-scalable=no, minimum-scale=1.0, maximum-scale=1.0">
    <title>SmartWeather</title>
    <style>@font-face{font-family:Montserrat;font-style:normal;font-weight:400;src:local('Montserrat Regular'),local('Montserrat-Regular'),url(https://fonts.gstatic.com/s/montserrat/v14/JTUSjIg1_i6t8kCHKm459W1hyzbi.woff2) format('woff2');unicode-range:U+0400-045F,U+0490-0491,U+04B0-04B1,U+2116}@font-face{font-family:Montserrat;font-style:normal;font-weight:400;src:local('Montserrat Regular'),local('Montserrat-Regular'),url(https://fonts.gstatic.com/s/montserrat/v14/JTUSjIg1_i6t8kCHKm459WZhyzbi.woff2) format('woff2');unicode-range:U+0102-0103,U+0110-0111,U+0128-0129,U+0168-0169,U+01A0-01A1,U+01AF-01B0,U+1EA0-1EF9,U+20AB}@font-face{font-family:Montserrat;font-style:normal;font-weight:400;src:local('Montserrat Regular'),local('Montserrat-Regular'),url(https://fonts.gstatic.com/s/montserrat/v14/JTUSjIg1_i6t8kCHKm459Wdhyzbi.woff2) format('woff2');unicode-range:U+0100-024F,U+0259,U+1E00-1EFF,U+2020,U+20A0-20AB,U+20AD-20CF,U+2113,U+2C60-2C7F,U+A720-A7FF}@font-face{font-family:Montserrat;font-style:normal;font-weight:400;src:local('Montserrat Regular'),local('Montserrat-Regular'),url(https://fonts.gstatic.com/s/montserrat/v14/JTUSjIg1_i6t8kCHKm459Wlhyw.woff2) format('woff2');unicode-range:U+0000-00FF,U+0131,U+0152-0153,U+02BB-02BC,U+02C6,U+02DA,U+02DC,U+2000-206F,U+2074,U+20AC,U+2122,U+2191,U+2193,U+2212,U+2215,U+FEFF,U+FFFD}*{-moz-user-select:none;-khtml-user-select:none;-webkit-user-select:none;-ms-user-select:none;user-select:none}body{margin:0;min-height:100vh;font:16px/1 sans-serif;display:flex;flex-direction:column;align-content:center;justify-content:center;font-family:Montserrat,sans-serif;background-repeat:no-repeat;background-image:linear-gradient(-30deg,#a8edff 0,#fbfcdb 100%)}h1{margin-top:0;margin-bottom:0;margin-left:auto;margin-right:auto;padding-top:40px;font-size:50px}h3{text-align:center}h4{margin:auto;padding-top:1em}form{margin:0;margin-left:auto;margin-right:auto;max-width:90%;width:420px}input{width:400px;margin:2%;max-width:96%}.alpha{background-image:linear-gradient(to right,hsla(66,31%,94%,.18),#fff);border-radius:3px;border-style:solid;border-width:thin;border-color:Grey}input[type=text],select{padding:12px 20px;display:inline-block;border:1px solid #ccc;box-sizing:border-box;font-weight:700}input[type=button],select{width:100%;background-color:#add8e6;color:#fff;padding:14px 20px;border:none;border-radius:4px;cursor:pointer;font-weight:700;font-size:18px}input[type=password],select{padding:12px 20px;display:inline-block;border:1px solid #ccc;box-sizing:border-box;font-weight:700}.button{width:100%;background-color:grey;color:#fff;padding:14px 1px;border:none;border-radius:4px;cursor:pointer;font-weight:700;transition:all .8s}.button:hover,input[type=button]:hover{-webkit-box-shadow:0 5px 30px -10px rgba(0,0,0,.57);-moz-box-shadow:0 5px 30px -10px rgba(0,0,0,.57);box-shadow:5px 30px -10px rgba(0,0,0,.57);transition:all .2s}.button:active{background-image:#00f}.buttonMarg{margin-left:10px;margin-right:10px}.left{display:table-cell;width:80%;margin-left:5%}.center{display:table-cell;text-align:center;width:5%}.right{display:table-cell;text-align:right;width:15%}#avNetworks{padding:0}.animate-bottom{position:relative;-webkit-animation-name:animatebottom;-webkit-animation-duration:1s;animation-name:animatebottom;animation-duration:1s}@-webkit-keyframes animatebottom{from{bottom:-100px;opacity:0}to{bottom:0;opacity:1}}@keyframes animatebottom{from{bottom:-100px;opacity:0}to{bottom:0;opacity:1}}.animate-top{position:relative;-webkit-animation-name:animatetop;-webkit-animation-duration:1s;animation-name:animatetop;animation-duration:1s}@-webkit-keyframes animatetop{from{bottom:100px;opacity:0}to{bottom:0;opacity:1}}@keyframes animatetop{from{bottom:100px;opacity:0}to{bottom:0;opacity:1}}#smartWeatherConnect{padding:0}.lds-ring{align-items:center;display:inline-block;position:relative;width:64px;height:64px}.lds-ring div{box-sizing:border-box;display:block;position:absolute;width:51px;height:51px;margin:6px;border:6px solid #fff;border-radius:50%;animation:lds-ring 1.2s cubic-bezier(.5,0,.5,1) infinite;border-color:#fff transparent transparent transparent}.lds-ring div:nth-child(1){animation-delay:-.45s}.lds-ring div:nth-child(2){animation-delay:-.3s}.lds-ring div:nth-child(3){animation-delay:-.15s}@keyframes lds-ring{0%{transform:rotate(0)}100%{transform:rotate(360deg)}}#loader{margin-left:auto;margin-right:auto}</style>
</head>
<body>
	<h1 id="smartWeather"class="animate-top">Smart<br>&nbsp; &nbsp; &nbsp;Register</h1><br>
	<h1 id="smartWeatherConnect" style="display:none;" class="animate-top">Smart<br>&nbsp; &nbsp; &nbsp;Register</h1><br>

	<div style = "display:none;" id = "loaderMessage">
		<h3 id = "Message"></h3>
		<form>
    		<input type = "button" value = "Reload in 5.." onclick = "reload()" id = "buttonConnect">
		</form>
	</div>        
	<div id="loader" style="display:none;" class="animate-bottom">
		<h3 id="loaderTry"></h3>
		<div class="lds-ring">
	<div></div><div></div><div></div><div></div></div></div>   
	<form id="ssidForm" class="animate-bottom">
)=====";

const char indexEnd[] PROGMEM = R"=====(
    <h6>FridayCorp 2020</h6> 
</form>
<script>
var state = 0;
var lastTry = 0;
var timer = [];
var interval;

function getSSID(value){
	document.getElementById("SSID").value=value;
}

function clearTimer(){
	for(var i = 0;i < timer.length;i++)
		clearTimeout(timer[i]);
	timer.length = 0;
}

function connect()
{
  api = "connect" + "?ssid=" +
  document.getElementById("SSID").value + "&pass=" +
  document.getElementById("Password").value + "&key=19";

  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      connectionPage();
    }
  };
  xhttp.open("GET", api, true);
  xhttp.send();
}

function reload()
{
  location.reload();
}

function connectionPage() { 
	state = 1; lastTry = 0; interval = setInterval(checkStatus, 1000);
	timer.push(setTimeout(function() {loaderMessage("Time out!");}, 22000));
	document.getElementById("ssidForm").style.display = "none";
	document.getElementById("loader").style.display = "block";
	document.getElementById("loaderTry").innerHTML=("try 1/10");
	document.getElementById("smartWeather").style.display = "none";
	document.getElementById("smartWeatherConnect").style.display = "block";
}

function checkStatus(){
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
			console.log(this.responseText);
			if (this.responseText[0] === 'W') 
			{
    			let trys = this.responseText.substring(1);
				if(trys > lastTry)
					document.getElementById("loaderTry").innerHTML=("try "+trys+"/10");
				lastTry = trys;

    			if(trys == 1)
				{
        			clearTimer();
					timer.push(setTimeout(function() {loaderMessage("Time out!");}, 25000));
				}
			}
			else if (this.responseText[0] === 'I') 
			{
				clearTimer();
				loaderMessage("Connected successfully!<br><br>IP Address: "+this.responseText.substring(1));
				var xhttp = new XMLHttpRequest();
				xhttp.onreadystatechange = function() {
				if (this.readyState == 4 && this.status == 200) 
					document.getElementById("Message").innerHTML = this.responseText;
				};
				xhttp.open("GET", "restart", true);
				xhttp.send();
			}
			else if (this.responseText[0] === 'N') 
			{
				clearTimer();
				loaderMessage("Wrong credentials!");
			}
			else 
			  console.log('Unknown event');
		}
	};
	xhttp.open("GET", "status", true);
	xhttp.send();
}

function loaderMessage(message) {
	clearInterval(interval);
	document.getElementById("Message").innerHTML = message;
	document.getElementById("loader").style.display = "none";
	document.getElementById("loaderMessage").style.display = "block";
    var x = document.getElementById("buttonConnect");
	x.value="Reload in 5.."; state = 0;
    setTimeout(function(){ x.value="Reload in 4.." }, 1000);
  	setTimeout(function(){ x.value="Reload in 3.." }, 2000);
  	setTimeout(function(){ x.value="Reload in 2.." }, 3000);
  	setTimeout(function(){ x.value="Reload in 1.." }, 4000);
    setTimeout(function(){ x.value="Reload in 0..";location.reload();}, 5000);
}
</script>
</body>
</html>
)=====";

String WiFiRegister::constructHTMLpage() {

	String HTMLpage(FPSTR(indexBegin));

	int indices[32];
	int numSSID = WiFi.scanComplete();

	if (numSSID < 1) {
		HTMLpage.concat(F("<h4 id='avNetworks'>No networks available!</h4><br>"));
		WiFi.scanNetworks(true);
	}
	else if (numSSID) {

		for (int i = 0; i < numSSID; i++)
			indices[i] = i;

		for (int i = 0; i < numSSID; i++)
			for (int j = i + 1; j < numSSID; j++)
				if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
					std::swap(indices[i], indices[j]);

		HTMLpage += F("<h4 id='avNetworks'>Available networks</h4><br>");
		for (int i = 0; i < numSSID; ++i) {
			HTMLpage.concat(F("<div class = \"button\" onclick = \"getSSID('"));
			HTMLpage.concat(WiFi.SSID(indices[i]));
			HTMLpage.concat(F("')\" style = \"background-image: linear-gradient(-225deg, #a8edea 60%, "));
			HTMLpage.concat(encryptionColorStr(WiFi.RSSI(indices[i])));
			HTMLpage.concat(F(" 100% ); \"> <div class = \"buttonMarg\"><div class = \"left\">"));
			HTMLpage.concat(WiFi.SSID(indices[i]));
			HTMLpage.concat(F("</div><div class = \"center\">&#12827"));
			HTMLpage.concat(encryptionTypeStr(WiFi.encryptionType(indices[i])));
			HTMLpage.concat(F("</div><div class = \"right\">"));
			HTMLpage.concat(encryptionPowerStr(WiFi.RSSI(indices[i])));
			HTMLpage.concat(F("%</div></div></div><br>"));
		}

		WiFi.scanDelete();
		if (WiFi.scanComplete() == -2) {
			WiFi.scanNetworks(true);
		}
	}

	HTMLpage.concat(F("<h4>SSID</h4><div class=\"alpha\"><input type=\"text\" id=\"SSID\" name=\"SSID\" placeholder=\"SSID..\"></div><h4>Password</h4><div class=\"alpha\"><input type=\"Password\" id=\"Password\" name=\"Password\" placeholder=\"Pass..\"></div></p><input type=\"button\" value=\"Connect\" onclick=\"connect()\"><input type=\"button\" value=\"Reload\" onclick=\"reload()\"><h4>MAC Address:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"));
	HTMLpage.concat(WiFi.macAddress());
	HTMLpage.concat(F("</h4>"));
	HTMLpage.concat(FPSTR(indexEnd));
	return HTMLpage;
}