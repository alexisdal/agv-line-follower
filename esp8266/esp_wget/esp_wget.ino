#include <stdint.h>
// based on https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/examples/BasicHttpClient/BasicHttpClient.ino

#define VERSION "0.3.0.a"

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

ESP8266WiFiMulti WiFiMulti;

#include "wifi_settings.h"

String url;

#define QRY_OK                         0
#define QRY_ERR_HTTP_ERROR_CODE        1
#define QRY_ERR_HTTP_UNABLE_TO_CONNECT 2
#define QRY_ERR_WIFI_NOT_CONNECTED     3
#define QRY_NO_QUERY                   4

struct query_response {
  int query_status = QRY_NO_QUERY;
  String server_response = "NOT_APPLICABLE";
};
query_response my_response;


void setup() {

  Serial.begin(115200);
  Serial.setTimeout(500); // sets the maximum milliseconds to wait for serial data. It defaults to 1000 milliseconds.


  Serial.print("start wifi v");
  Serial.print(VERSION);
  Serial.print("\n");
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

}

// http://10.155.100.89/cgi-bin/insert.py?NAME=WIFI_DEV


void print_query_status(){
  if        (my_response.query_status == QRY_OK) {
    Serial.print("QRY_OK");
  } else if (my_response.query_status == QRY_ERR_HTTP_ERROR_CODE) {
    Serial.print("QRY_ERR_HTTP_ERROR_CODE");
  } else if (my_response.query_status == QRY_ERR_HTTP_UNABLE_TO_CONNECT) {
    Serial.print("QRY_ERR_HTTP_UNABLE_TO_CONNECT");
  } else if (my_response.query_status == QRY_ERR_WIFI_NOT_CONNECTED) {
    Serial.print("QRY_ERR_WIFI_NOT_CONNECTED");
  } else if (my_response.query_status == QRY_NO_QUERY) {
    Serial.print("QRY_NO_QUERY");
  } else {
    Serial.print("QRY_UNKNOWN");
  }
  Serial.print("\n");
}

void print_server_response() {
  Serial.print(my_response.server_response);
  if (!my_response.server_response.endsWith("\n")) { Serial.print("\n"); }
}

void print_mac(){
  Serial.print(WiFi.macAddress());
  Serial.print("\n");
}   
void print_fw(){
  Serial.print(VERSION);
  Serial.print("\n");
}   

void print_ip(){
  if( WiFiMulti.run() == WL_CONNECTED) {
    Serial.print(WiFi.localIP());
    Serial.print("\n");
  } else {
    Serial.print("NOT_CONNECTED\n");
  }
}

void print_status(){
  int status = WiFiMulti.run();
  if      (  status == WL_IDLE_STATUS)     { Serial.print("WL_IDLE_STATUS"); }
  else if (  status == WL_NO_SSID_AVAIL)   { Serial.print("WL_NO_SSID_AVAIL"); }
  else if (  status == WL_SCAN_COMPLETED)  { Serial.print("WL_SCAN_COMPLETED"); }
  else if (  status == WL_CONNECTED)       { Serial.print("WL_CONNECTED"); }
  else if (  status == WL_CONNECT_FAILED)  { Serial.print("WL_CONNECT_FAILED"); }
  else if (  status == WL_CONNECTION_LOST) { Serial.print("WL_CONNECTION_LOST"); }
  else if (  status == WL_DISCONNECTED)    { Serial.print("WL_DISCONNECTED"); }
  else if (  status == WL_NO_SHIELD)       { Serial.print("WL_NO_SHIELD"); }
  else    {  Serial.print("UNKNOWN("); Serial.print(status, HEX); Serial.print(")"); }
  Serial.print("\n");  
}

void print_signal_strength(){
  int ss = get_signal_strengh();
  Serial.print(ss);
  Serial.print("\n");
}

//https://github.com/tttapa/Projects/blob/master/ESP8266/WiFi/RSSI-WiFi-Quality/RSSI-WiFi-Quality.ino
/*
   Return the quality (Received Signal Strength Indicator)
   of the WiFi network.
   Returns a number between 0 and 100 if WiFi is connected.
   Returns -1 if WiFi is disconnected.
int get_signal_strengh() {
  if (WiFi.status() != WL_CONNECTED)
    return -1;
  int dBm = WiFi.RSSI();
  if (dBm <= -100)
    return 0;
  if (dBm >= -50)
    return 100;
  return 2 * (dBm + 100);
}*/
int get_signal_strengh() {
  if (WiFi.status() != WL_CONNECTED) {
    return 0;
  } else {
    return WiFi.RSSI();
  }
}

void add_own_params_to_url(){
  
}

int wget(){
  my_response.query_status = QRY_OK;
  my_response.server_response = "";
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
          //Serial.print(payload);
          my_response.query_status    = QRY_OK;
          my_response.server_response = payload.substring(0, 100); // we keep only 100 first characters
        } else {
          my_response.query_status    = QRY_ERR_HTTP_ERROR_CODE;
          my_response.server_response = "ERROR: HTTP(" + String(httpCode) + ") " + http.errorToString(httpCode).c_str();
        }
      } else {
        my_response.query_status    = QRY_ERR_HTTP_ERROR_CODE;
        my_response.server_response = "ERROR: HTTP(" + String(httpCode) + ") " + http.errorToString(httpCode).c_str();
      }
      http.end();
    } else {
      //Serial.print("[HTTP} Unable to connect to " + url + "\n");
      my_response.query_status    = QRY_ERR_HTTP_UNABLE_TO_CONNECT;
      my_response.server_response = "NOT_APPLICABLE";
    }
  } else {
    //Serial.println("No Wireless connection available");
      my_response.query_status    = QRY_ERR_WIFI_NOT_CONNECTED;
      my_response.server_response = "NOT_APPLICABLE";
  }
}

void loop() {
  while (Serial.available() > 0) {
    url = Serial.readStringUntil('\n'); // read the incoming data as string
  }
  if (url != "") {
    if (url.endsWith("\n")) {
      url.remove(url.length()-1);
    }
    
    if (url.startsWith("_STS")) {
      print_status();
    } else if (url.startsWith("_IP")) {
      print_ip();
    } else if (url.startsWith("_FW")) {
      print_fw();
    } else if (url.startsWith("_MAC")) {
      print_mac();
    } else if (url.startsWith("_QS")) { // query_status
      print_query_status();
    } else if (url.startsWith("_SR")) { // server_response
      print_server_response();
    } else if (url.startsWith("_SS")) { // signal_strengh
      print_signal_strength();
    } else {
      wget();
    }
    url = "";
  }
}
