#include <Arduino.h>
#include <DNSServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include "HTTPClient.h"

#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>

//HTML pages
#include "debugpage.h"
#include "confpage.h"
#include "readerpage.h"
#include "resppage.h"

//MQTT topics
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

DNSServer dnsServer;
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

class CaptiveRequestHandler : public AsyncWebHandler {
	public:
		CaptiveRequestHandler() {}
		virtual ~CaptiveRequestHandler() {}
		bool canHandle(AsyncWebServerRequest *request){
			return true;
		}
		void handleRequest(AsyncWebServerRequest *request) {
			request->send_P(200, "text/html", CONF_page);
		}
};

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
	switch (type) {
	case WStype_DISCONNECTED:
		Serial.printf("[%u] Disconnected!\n", num);
		break;
	case WStype_CONNECTED: {
		IPAddress ip = webSocket.remoteIP(num);
		Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
		webSocket.sendTXT(num, "Connected to reader...");
	}
	break;
	e_TEXT:
		Serial.printf("[%u] got text: %s\n", num, payload);
		break;
	case WStype_BIN:
		Serial.printf("[%u] got binary length: %u\n", num, length);
		break;
	}
}

#define RST_PIN 0 
#define SS_PIN 5  
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";

// Variables to save values from HTML form
String ssid;
String pass;

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";

IPAddress subnet(255, 255, 0, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)

// Set LED GPIO
const int ledPin = 2;
String ledState;

// Initialize SPIFFS
void initSPIFFS() {
	if (!SPIFFS.begin(true)){
		Serial.println("An error has occurred while mounting SPIFFS");
	}
	Serial.println("SPIFFS mounted successfully");
}

String readFile(fs::FS &fs, const char *path) {
	Serial.printf("Reading file: %s\r\n", path);
	File file = fs.open(path);
	if (!file || file.isDirectory()) {
		Serial.println("- failed to open file for reading");
		return String();
	}

	String fileContent;
	while (file.available()){
		fileContent = file.readStringUntil('\n');
		break;
	}
	return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message){
	Serial.printf("Writing file: %s\r\n", path);
	File file = fs.open(path, FILE_WRITE);
	if (!file) {
		Serial.println("- failed to open file for writing");
		return;
	}
	if (file.print(message)) {
		Serial.println("- file written");
	}
	else {
		Serial.println("- frite failed");
	}
}

// Initialize WiFi
bool initWiFi() {

  if (ssid == "") {
		Serial.println("Undefined SSID");
		return false;
	}

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid.c_str(), pass.c_str());
	Serial.println("Connecting to WiFi...");

	unsigned long currentMillis = millis();
	previousMillis = currentMillis;

	while (WiFi.status() != WL_CONNECTED) {
		currentMillis = millis();
		if (currentMillis - previousMillis >= interval) {
			Serial.println("Failed to connect.");
			return false;
		}
	}

	Serial.println(WiFi.localIP());
	return true;
}

// Replaces placeholder with LED state value
String processor(const String &var) {
	if (var == "STATE") {
		if (digitalRead(ledPin)) {
			ledState = "ON";
		} else {
			ledState = "OFF";
		}
		return ledState;
	}
	return String();
}

byte nuidPICC[4];

void printHex(byte *buffer, byte bufferSize) {
	String fnord;
	for (byte i = 0; i < bufferSize; i++)
	{
		fnord += buffer[i] < 0x10 ? " 0" : " ";
		fnord += buffer[i];
	}
	Serial.print(fnord);
	webSocket.broadcastTXT(fnord);
}


void printTag(byte *buffer, byte bufferSize) {
	String fnord = "#";
	for (byte i = 0; i < bufferSize; i++) {
		fnord += buffer[i] < 0x10 ? " 0" : " ";
		fnord += buffer[i];
	}
	Serial.print(fnord);
	webSocket.broadcastTXT(fnord);

	String new_string = fnord;
	new_string.replace(" ", ""); // remove spaces
	new_string.replace("#", ""); // remove #

  HTTPClient http;
  String url = "https://gip.0xed.io/index.php?card=" + new_string + "&country=" + country.c_str(); // Use the ESP32's IP address as parameter
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();  //Make the request
  if (httpCode > 0) { //Check for the returning code
    Serial.println(httpCode);
  } else {
    Serial.println("Error on HTTP request");
  }
  http.end();
}

void printDec(byte *buffer, byte bufferSize) {
	for (byte i = 0; i < bufferSize; i++) {
		Serial.print(buffer[i] < 0x10 ? " 0" : " ");
		Serial.print(buffer[i], DEC);
	}
}

void connectAWS() {
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}

void publishMessage() {
  StaticJsonDocument<200> doc;
  doc["timestamp"] = String(millis());
  doc["serialNumber"] = "FNORD";

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.println("Message published");
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void setup() {
	Serial.begin(115200);

	initSPIFFS();
	SPI.begin();		 // Init SPI bus
	rfid.PCD_Init(); // Init MFRC522

	for (byte i = 0; i < 6; i++) {
		key.keyByte[i] = 0xFF;
	}

	// Set GPIO 2 as an OUTPUT
	// pinMode(ledPin, OUTPUT);
	// digitalWrite(ledPin, LOW);

	Serial.print(F("Using the following key:"));
	printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
	Serial.println("");

	ssid = WIFI_SSID;
	pass = WIFI_PASSWORD;

	// Load values saved in SPIFFS if not hardcoded
  if (ssid == "") {
		ssid = readFile(SPIFFS, ssidPath);
		pass = readFile(SPIFFS, passPath);
	}

	Serial.println(ssid);
	Serial.println(pass);

	if (initWiFi()) {

		IPAddress ip = WiFi.localIP();
  	String ipAddress = ip.toString();

    connectAWS();

		HTTPClient http;
		String url = "https://gip.0xed.io/index.php?ip=" + ipAddress + "&country=" + country.c_str(); // Use the ESP32's IP address as parameter
    http.begin(url);
    int httpCode = http.GET();  //Make the request
    if (httpCode > 0) { //Check for the returning code
      //String payload = http.getString();
      Serial.println(httpCode);
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();

		webSocket.begin();
		webSocket.onEvent(webSocketEvent);

		server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", READER_page); 
		});

		server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", DEBUG_page); 
		});

		server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->send_P(200, "text/html", CONF_page); 
		});

		// server.serveStatic("/", SPIFFS, "/");
		server.begin();

	} else {
		
		Serial.println("Setting AP (Access Point)");
		WiFi.softAP("READER-CONFIG");

		IPAddress IP = WiFi.softAPIP();
		Serial.print("AP IP address: ");
		Serial.println(IP);

		server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", CONF_page); 
		});

		server.serveStatic("/", SPIFFS, "/");

		server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      Serial.print("called / ");
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
			request->send_P(200, "text/html", RESP_page); 
			delay(3000);
      ESP.restart(); 
		});

		dnsServer.start(53, "*", WiFi.softAPIP());
  	server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
		
		server.begin();
	}
}

void loop() {
	dnsServer.processNextRequest();
	webSocket.loop();

  // publishMessage();
  client.loop();
  delay(2000);

	// Look for new cards
	if (!rfid.PICC_IsNewCardPresent())
		return;

	// Verify if the NUID has been read
	if (!rfid.PICC_ReadCardSerial())
		return;

	Serial.println();
	Serial.print(F("PICC type: "));
	MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
	Serial.println(rfid.PICC_GetTypeName(piccType));

	// Store NUID into nuidPICC array
	for (byte i = 0; i < 4; i++) {
		nuidPICC[i] = rfid.uid.uidByte[i];
	}

	Serial.print(F("Tag: "));
	printTag(rfid.uid.uidByte, rfid.uid.size);

	Serial.println();
	rfid.PICC_HaltA();
	rfid.PCD_StopCrypto1();

}
