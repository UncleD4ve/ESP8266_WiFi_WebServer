//Custom scripts

var initCustomScript = true;
let initObject = {};
let timerWebSocket = 0;
let webSocket;
let lastSend = 0;

//Color picker

let colorPicker = new iro.ColorPicker("#colorPicker", {
  width: 300,
  color: "rgb(255, 0, 0)",
  borderWidth: 3,
  borderColor: "#fff",
});

//Slider

var rangeSlider = document.getElementById("rs-range-line");
var rangeBullet = document.getElementById("rs-bullet");
var resizeVar = 0;
window.addEventListener(
  "resize",
  function () {
    showSliderValue(parseInt(resizeVar));
  },
  false
);
function showSliderValue(value = 0) {
  if (!Number.isInteger(value)) value = 999;
  let percent = value / rangeSlider.max;
  resizeVar = rangeBullet.innerHTML = value;
  var bulletPosition = rangeSlider.offsetWidth * percent;
  rangeBullet.style.left = bulletPosition - 20 * percent + "px";
}
rangeSlider.addEventListener("input", sendVolume, false);

//WebSocket

function initWebSocket() {
  webSocket = new WebSocket("ws://" + document.location.host + "/ws");
  webSocket.onopen = function (evt) {
    console.log("WebSocket open");

    if (timerWebSocket) {
      clearInterval(timerWebSocket);
      timerWebSocket = 0;
    }
  };
  webSocket.onclose = function (evt) {
    console.log("WebSocket close");

    webSocket = null;
    if (!timerWebSocket)
      timerWebSocket = setInterval(function () {
        console.log("WebSocket retry");
        initWebSocket();
      }, 1000);
  };
  webSocket.onerror = function (evt) {
    console.log(evt);
    location.reload();
  };
  webSocket.onmessage = function (evt) {
    try {
      let data = JSON.parse(evt.data.replace("\u0000", ""));
      if (data.hasOwnProperty("name") && data.hasOwnProperty("version"))
        initObject = JSON.parse(evt.data);
      console.log(data);

      if (initObject.hasOwnProperty("name")) {
        $("#projectName").html(initObject.name);
        $("#projectNavbar").html(initObject.name);
        document.title = initObject.name;
      }
      if (initObject.hasOwnProperty("version")) {
        $("#projectSubName").html(initObject.name + " " + initObject.version);
      }
      if (initObject.hasOwnProperty("wifiMode")) {
        $("#changeWiFiButton").html(
          "Change WiFi mode - " +
          (initObject.wifiMode == 1
            ? "AP"
            : initObject.wifiMode == 2
              ? "STA"
              : "?")
        );
        $("#setWiFiButton").html(
          "Set WiFi mode - " +
          (initObject.wifiMode == 1
            ? "AP"
            : initObject.wifiMode == 2
              ? "STA"
              : "?")
        );
      }
      if (initObject.hasOwnProperty("isStatic")) {
        $("#ipDynamic").removeClass("btn-light").addClass("btn-success");
        $("#ipStatic").removeClass("btn-light").addClass("btn-secondary");
      } else {
        $("#ipDynamic").removeClass("btn-light").addClass("btn-secondary");
        $("#ipStatic").removeClass("btn-light").addClass("btn-success");
      }

      if (initObject.hasOwnProperty("staticIp")) {
        let ip = initObject.staticIp.split(".");
        $("#sip1").val(ip[0]);
        $("#sip2").val(ip[1]);
        $("#sip3").val(ip[2]);
        $("#sip4").val(ip[3]);
      }

      // ========== User code ============

      if (data.hasOwnProperty("initPosition")) {
        let vol = parseInt(data.initPosition);
        if (vol <= 180) {
          vol = scale(vol, 0, 180, 0, 100);
          showSliderValue(vol);
          rangeSlider.value = vol;
        } else {
          showSliderValue(999);
          rangeSlider.value = 50;
        }
      }

      if (data.hasOwnProperty("initSaveA")) {
        document.getElementById("btn-rest-b").innerHTML =
          "Restore volume: " +
          scale(parseInt(data.initSaveA), 0, 180, 0, 100) +
          "%";
      }

      if (
        data.hasOwnProperty("initChangeButton") &&
        data.initChangeButton == 1
      ) {
        $("#btn-change-a").removeClass("btn-secondary").addClass("btn-success");
      }

      if (data.hasOwnProperty("position")) {
        showSliderValue(scale(parseInt(data.position), 0, 180, 0, 100));
      }

      if (data.hasOwnProperty("saveA")) {
        document.getElementById("btn-rest-b").innerHTML =
          "Restore volume: " +
          scale(parseInt(data.saveA), 0, 180, 0, 100) +
          "%";
      }

      if (data.hasOwnProperty("changeButton")) {
        if (data.changeButton == 1)
          $("#btn-change-a")
            .removeClass("btn-secondary")
            .addClass("btn-success");
        else
          $("#btn-change-a")
            .removeClass("btn-success")
            .addClass("btn-secondary");
      }

      // ========== End of User code ==========
    } catch (e) {
      console.log(evt);
    }
  };
}

function sendVolume() {
  let now = new Date().getTime();
  if (lastSend > now - 20) return;
  lastSend = now;
  webSocket?.send(`{"slider":${scale(rangeSlider.value, 0, 100, 0, 180)}}`);
}

colorPicker.on(["color:change"], function (color) {
  let now = new Date().getTime();
  if (lastSend > now - 20) return;
  lastSend = now;
  webSocket?.send(`{"colorPicker":${color.hexString.substring(1)}}`);
});

$("button[espbutton]").click(function () {
  webSocket?.send(
    `{"${$(this).attr("espbutton")}":${$(this).attr("value") ?? 1}}`
  );
});
const sendIP = () =>
  webSocket?.send(
    `{"_setStatic_":"${$("#sip1").val()}.${$("#sip2").val()}.${$(
      "#sip3"
    ).val()}.${$("#sip4").val()}"}`
  );
const scale = (num, in_min, in_max, out_min, out_max) =>
  Math.round(
    ((num - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min
  );
initWebSocket();