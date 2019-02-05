#include <WiFiManager.h>

/* Connect to SHARP PM2.5 Sensor*/
#include <PMsensor.h>
PMsensor PM;
#include "DHT.h"
#define DHTPIN 13   // NodeMCU PIN D1
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

//for LED status
#include <Ticker.h>
Ticker ticker;
int LED = BUILTIN_LED;

void tick()
{
  //toggle state
  digitalWrite(LED, !digitalRead(LED));     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

// HTTP API 
const char* host = "api.thingspeak.com";
const char* api   = "********";  //API Key

int value = 0;
void connect() {  
  dht.begin();

  // LED Stop
  //digitalWrite(LED_BUILTIN, HIGH);
  //delay(10);
  //delay(60000);
  ++value;
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.print(hif);
  Serial.println(" *F");
  Serial.print("connecting to ");
  Serial.println(host);

  float pm = 0;
  int err = PMsensorErrSuccess;
  
  if ((err = PM.read(&pm, true, 0.1)) != PMsensorErrSuccess) {
    Serial.print("data Error = ");
    Serial.println(err);
    return;
  }
  Serial.print("PM2.5: ");
  Serial.print(pm);
  Serial.println(" ppm");

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/update?api_key=";
  url += api;
  url += "&field1=";
  url += t;
  url += "&field2=";  
  url += h;
  url += "&field3=";  
  url += pm;
  // https://api.thingspeak.com/update?api_key=xxxxxxxxxx&field1=t&field2=h&field3=pm
  Serial.print("Requesting URL: ");
  Serial.println(url);
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
   //Wait up to 10 seconds for server to respond then read response
   int i=0;
   while((!client.available()) && (i<1000)){
     // LED Blink
     digitalWrite(LED_BUILTIN, LOW);
     delay(10);
     //Serial.println(i);
     i++;
   }
   while(client.available()){
     String line = client.readStringUntil('\r');
     Serial.print(line);
   }
   Serial.println();
   Serial.println("closing connection");
}
void setup() {
  // WiFi Manager
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  //set led pin as output
  pinMode(LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  //reset settings - for testing
  // wm.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(LED, LOW);

  // End WiFi Manager
  
  Serial.setTimeout(2000);
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  /////(infrared LED pin, sensor pin)  /////
  PM.init(16, 36);

  
  
  // Wait for serial to initialize.
  while (!Serial) { }

  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.println("Running Deep Sleep Firmware!");
  Serial.println("-------------------------------------");

  connect();

  Serial.println("Sleeping 60 seconds ..");
  // Deep Sleep 60 seconds
  ESP.deepSleep(58e6); // 60e6 is 60 microsecondsESP.
  //ESP.deepSleep(298e6); // 5*60 microsecondsESP.
}

void loop() {
}
