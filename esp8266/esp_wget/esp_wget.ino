// forked from https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/examples/BasicHttpClient/BasicHttpClient.ino


#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

ESP8266WiFiMulti WiFiMulti;

String url;

#include "wifi_settings.h"

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.print("start wifi\n");
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD); // defined in wifi_settings.h 
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

}

void wget(){
  // wait for WiFi connection
  if (WiFiMulti.run() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    //Serial.print("[HTTP] begin...\n");
    // url should be something like "http://192.168.1.19/index.html"
    if (http.begin(client, url)) {  // HTTP
      //Serial.print("[HTTP] GET " + url + " ");
      // start connection and send HTTP header
      int httpCode = http.GET();
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        //Serial.print("[HTTP] GET " + url + " ");
        //Serial.printf("code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.print(payload);
        }
      } else {
        //Serial.print("[HTTP] GET " + url + " "); 
        Serial.printf("failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.print("[HTTP} Unable to connect to " + url + "\n");
    }
  } else {
    Serial.println("No Wireless connection available");
  }
}

void loop() {
  while (Serial.available() > 0) {
    url = Serial.readString(); // read the incoming data as string
  }
  if (url != "") {
    wget();
    url = "";
  }
}
