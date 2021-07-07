#include <Arduino.h>
#include <ESP8266WiFi.h> 
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ArduinoOTA.h>
#include "index_page.h"
#include "style.h"

//Wifi Config
AsyncWebServer server(80);

const char* ssid          = "WiFi-SSID";
const char* password      = "you-will-never-know";

IPAddress staticIP(192,168,1,100);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

String Gatestate;
bool FLAG_ERROR;
char data; 

//OTA SETUP
void ota_setup(){
  ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin();
}

void setup() {
  Serial.begin(9600); 
  

  /* 
  WiFi.softAP("Gate Controller");

  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP()); */


  WiFi.mode(WIFI_STA);
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("WiFi Failed!\n");
      return; 
  
  } 
  Serial.println(WiFi.localIP()); 

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/css", page_css);
  });

  server.on("/gate_signal", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.write('s');
    request->send(200, "text/plain", "ok");
  });

  server.on("/gatestate", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(Gatestate).c_str() );
  });
  server.on("/gateerror", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(FLAG_ERROR).c_str() );
  });
  
  ota_setup();
  server.onNotFound(notFound);
  server.begin();

}

void loop() {
  ArduinoOTA.handle();
  
  if (Serial.available()){
    data = Serial.read();
    FLAG_ERROR  = bitRead(data, 3);
    Gatestate   = (data & 0b0000111);
  } 
}
  


