#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <stdlib.h>
#include <RCSwitch.h>

const char* host = "esp8266-room";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "OTAPASS";

const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASS";
const char* mqtt_server = "MQTT_SERVER_IP";

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
IRsend irsend(0);
RCSwitch mySwitch = RCSwitch();

int relayPin = 3;
int relayState = 0;

int buttonPin = 2;
int buttonState;
int lastButtonState = HIGH;

unsigned int result;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

char rcvd[10];
char **ptr;
unsigned long irData;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  irsend.begin(); 
  mySwitch.enableTransmit(1);
  //Serial.begin(115200, SERIAL_8N1,SERIAL_TX_ONLY);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  setup_wifi();
  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
   String strPayload = "";
   String strTopic(topic);
  for (int i = 0; i < length; i++) {
    strPayload += (char)payload[i];
  }
   if(strTopic == "/melih/oda/lamp"){
      if(strPayload == "ON"){
          if(relayState == 0){
             digitalWrite(relayPin, HIGH);
             Serial.println("Relay ON");
             relayState = 1;
          }else if(relayState == 1){
             digitalWrite(relayPin, LOW);
             Serial.println("Relay OFF");
             relayState = 0;
            }     
      }
  }else if(strTopic == "/melih/oda/remote/nec"){
    Serial.println("/melih/oda/remote/nec");
    sendCode(strPayload, 0);
  }else if(strTopic == "/melih/oda/remote/rc5"){
    Serial.println("/melih/oda/remote/rc5");
    sendCode(strPayload, 1);
  }else if(strTopic == "/melih/oda/remote/rf433"){
    Serial.println("/melih/oda/remote/rf433");
    mySwitch.send(strPayload.toInt(), 24);
  }else if(strTopic == "/melih/oda/remote/pc"){
    Serial.println("/melih/oda/remote/pc");
    
  }
}

void sendCode(String code, int tvId){
  code.toCharArray(rcvd, 11);
  irData = strtoul(rcvd,ptr,16);
  Serial.println(rcvd);
  Serial.println(irData);
  if(tvId == 0){
    irsend.sendNEC(irData,32);
  }else if(tvId == 1){
    irsend.sendRC5(irData,12);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Room")) {
      Serial.println("connected");
      client.publish("/baglanti", "Baglandi");
      // Once connected, publish an announcement...
      // ... and resubscribe
      client.subscribe("/melih/oda/lamp");
      client.subscribe("/melih/oda/remote/nec");
      client.subscribe("/melih/oda/remote/rc5");
	  client.subscribe("/melih/oda/remote/rf433");
	  client.subscribe("/melih/oda/remote/pc");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  int reading = digitalRead(buttonPin); 

  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == LOW) {
		  relayState = !relayState;
		  digitalWrite(relayPin, relayState);
        //client.publish("/melih/oda/lamp", "ON");
        Serial.println("Button Pressed!");
      }
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:

  if (!client.connected()) {
    reconnect();
  }
  httpServer.handleClient();
  client.loop();
  lastButtonState = reading;
}
