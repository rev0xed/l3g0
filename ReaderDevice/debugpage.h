#include <pgmspace.h>

const char DEBUG_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <title>GJ</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" type="text/css" href="https://content.0xed.io/bo/style.css">
    <link rel="icon" href="https://0xed.io/favicon.ico">
  </head>

<style>
	.object-output {
		margin-bottom: 1rem;
	}
	body {
		background-color: #000;
	}
	.content {
		width: 100%;
		/* height:900px; */
		position:absolute;
		top:0;
		left: 0;
		/* border: solid 1px #00f; */
		padding: 0;
	}
  #title {
    position:absolute;
    width: 300px;
    height: 300px;
    top: 0px;
    left: 0px;
    background: black;
    color: white;
  }
</style>

<script>
var websock;

function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    console.log(evt);
    var e = document.getElementById('title');
    if (evt.data) { 
      if (evt.data.charAt(0) === "#") {
        var tid = evt.data.replace(/\s+/g, '');
					console.log(tid);
          e.innerHTML += tid + '<br>';
      } else {
        cont = evt.data + '<br>'
      }
    }
  };
}
</script>
  <body onload="javascript:start();">
    <div class="content">
      <div class="card-title"><h1>DEBUG:</h1></div>
      <div id="title" class="card-title"></div>
    </div>
  </body>
</html>
)=====";