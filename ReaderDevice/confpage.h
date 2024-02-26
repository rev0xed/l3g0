#include <pgmspace.h>

const char CONF_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>READER CONFIGURATION</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="https://0xed.io/favicon.ico">
</head>
<style>
  button,input[type=submit]{color:#fefcfb;text-align:center;width:100px;
  transition-duration:.4s}html{font-family:Arial,Helvetica,sans-serif;
  display:inline-block;text-align:center}h1{font-size:1.8rem;color:#fff}
  p{font-size:1.4rem}.card-title,.state,.value,label{font-size:1.2rem}
  .topnav{overflow:hidden;background-color:#0a1128}body{margin:0}
  .content{padding:5%}
	.card-grid{max-width:800px;margin:0 auto;display:grid;grid-gap:2rem;grid-template-columns:repeat(auto-fit,minmax(300px,1fr))}
	.card{background-color:#fff;box-shadow:2px 2px 12px 1px rgba(140,140,140,.5)}
	.button-on,input[type=submit]{background-color:#034078}.card-title{font-weight:700;color:#034078;margin:50px;}
	input[type=submit]{border:none;padding:15px;text-decoration:none;display:inline-block;font-size:16px;margin-right:10px;border-radius:4px}
  .button-on:hover,input[type=submit]:hover{background-color:#1282a2}input[type=number],input[type=text],select{width:50%;padding:12px 20px;margin:18px;display:inline-block;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}
  .state,.value{color:#1282a2}button{border:none;padding:15px 32px;font-size:16px;border-radius:4px}
  .button-off{background-color:#858585}.button-off:hover{background-color:#252524}
</style>
<body>
  <div class="topnav">
    <h1>READER CONFIGURATION</h1>
  </div>
  <div class="content">
    <div class="card-grid">
      <div class="card">
				<p class="card-title">After submission please wait a few seconds
				for the reader to reset and connect to the WiFi provided, 
				then visit <a href="http://gip.0xed.io">http://gip.0xed.io</a> 
				and click on the most recent IP to view the page. Please ensure you're connected to the same network as the board.</p>
        <form action="/" method="POST">
          <p>
					  <label for="ssid">COUNTRY</label>
            <input type="text" id="country" name="country"><br>
            <label for="ssid">SSID</label>
            <input type="text" id="ssid" name="ssid"><br>
            <label for="pass">Password</label>
            <input type="text" id="pass" name="pass"><br>
            <input type ="submit" value ="Submit">
          </p>
        </form>
      </div>
    </div>
  </div>
</body>
</html>
)=====";