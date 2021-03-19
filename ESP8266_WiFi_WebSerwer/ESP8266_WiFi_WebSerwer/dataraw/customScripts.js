(function($) {
  "use strict"; // Start of use strict

  // Smooth scrolling using jQuery easing
  $('a.js-scroll-trigger[href*="#"]:not([href="#"])').click(function() {
    if (location.pathname.replace(/^\//, '') == this.pathname.replace(/^\//, '') && location.hostname == this.hostname) {
      var target = $(this.hash);
      target = target.length ? target : $('[name=' + this.hash.slice(1) + ']');
      if (target.length) {
        $('html, body').animate({
          scrollTop: (target.offset().top - 72)
        }, 1000, "easeInOutExpo");
        return false;
      }
    }
  });

  // Closes responsive menu when a scroll trigger link is clicked
  $('.js-scroll-trigger').click(function() {
    $('.navbar-collapse').collapse('hide');
  });

  // Activate scrollspy to add active class to navbar items on scroll
  $('body').scrollspy({
    target: '#mainNav',
    offset: 75
  });

  // Collapse Navbar
  var navbarCollapse = function() {
    if ($("#mainNav").offset().top > 100) {
      $("#mainNav").addClass("navbar-scrolled");
    } else {
      $("#mainNav").removeClass("navbar-scrolled");
    }
  };
  // Collapse now if page is not at top
    navbarCollapse();

    initWebSocket();
  // Collapse the navbar when page is scrolled
  $(window).scroll(navbarCollapse);


})(jQuery);


//Custom scripts

var rangeSlider = document.getElementById("rs-range-line");
var rangeBullet = document.getElementById("rs-bullet");
var resizeVar = 0;

window.addEventListener('resize', function () { showSliderValue(parseInt(resizeVar));}, false);


function showSliderValue(value = 0) {
    if (!Number.isInteger(value)) value = 999;
    let percent = value / rangeSlider.max;
    resizeVar = rangeBullet.innerHTML = value; 
    var bulletPosition = rangeSlider.offsetWidth * percent;
    rangeBullet.style.left = bulletPosition - 20 * percent + "px";
}

//WebSocket
          
var timerWebSocket = 0;
var webSocket;
var lastSend = 0;

rangeSlider.addEventListener("input", sendVolume, false);

function initWebSocket() {
    webSocket = new WebSocket("ws:" + "/" + window.location.hostname + ':81/');
    webSocket.onopen = function (evt) {
      console.log('WebSocket open');

        if (timerWebSocket) {
            clearInterval(timerWebSocket);
            timerWebSocket = 0;
        }
    };
    webSocket.onclose = function (evt) {
        console.log('WebSocket close');

        webSocket = null;
        if (!timerWebSocket)
            timerWebSocket = setInterval(function () {console.log("WebSocket retry"); initWebSocket(); }, 1000);
    };
    webSocket.onerror = function (evt) {console.log(evt); location.reload()};
    webSocket.onmessage = function (evt) {
        console.log(evt);

        if (evt.data[0] === 'I') {  //Initialization
            let words = evt.data.substring(1).split(",");
            let current = 0;

            let vol = parseInt(words[current++]);
            if (vol <= 180) {
                vol = scale(vol, 0, 180, 0, 100);
                showSliderValue(vol);
                rangeSlider.value = vol;
            }
            else {
                showSliderValue(999);
                rangeSlider.value = 50;
            }

            document.getElementById("btn-rest-b").innerHTML = "Restore volume: " + scale(parseInt(words[current++]), 0, 180, 0, 100) + "%";
            
            if (words[current++] == 1) {
              $("#btn-change-a").removeClass("btn-secondary").addClass("btn-success");
            }


            if (words[current++] == 'S') {
                $("#ipDynamic").removeClass("btn-light").addClass("btn-success");
                $("#ipStatic").removeClass("btn-light").addClass("btn-secondary");
            }
            else {
                $("#ipDynamic").removeClass("btn-light").addClass("btn-secondary");
                $("#ipStatic").removeClass("btn-light").addClass("btn-success");
            }
            
            let ip = words[current++].split(".");
            $("#sip1").val(ip[0]);
            $("#sip2").val(ip[1]);
            $("#sip3").val(ip[2]);
            $("#sip4").val(ip[3]);
        }
        else if (evt.data[0] === 'A') 
          showSliderValue(scale(parseInt(evt.data.substring(1)),0,180,0,100));

        else if (evt.data[0] === 'B') 
          document.getElementById("btn-rest-b").innerHTML = "Restore volume: " + scale(parseInt(evt.data.substring(1)), 0, 180, 0, 100) + "%";

        else if (evt.data[0] === 'C')
          if(evt.data.substring(1) == 1)
            $("#btn-change-a").removeClass("btn-secondary").addClass("btn-success");
          else
            $("#btn-change-a").removeClass("btn-success").addClass("btn-secondary");

        else 
          console.log('Unknown event');
    };
}

function sendVolume() {
    let now = (new Date).getTime();
    if (lastSend > now - 20) return;
    lastSend = now;
    webSocket?.send('a' + scale(rangeSlider.value,0,100,0,180));
}

function sendIP() {
    webSocket?.send('}' + $("#sip1").val() + '.' + $("#sip2").val() + '.' + $("#sip3").val() + '.' + $("#sip4").val());
}

function sendButton(button) {
    webSocket?.send('b' + button);
}

function sendBuiltInButton(button) {
  webSocket?.send('{' + button);
}

const scale = (num, in_min, in_max, out_min, out_max) => {
    return Math.round((num - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}