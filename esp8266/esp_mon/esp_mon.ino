
// based on: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiClient/WiFiClient.ino
//           https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/examples/BasicHttpClient/BasicHttpClient.ino

#include <ESP8266WiFi.h>  // to connect to a wifi network just with ssid/password
#include <ESP8266HTTPClient.h>  // to make HTTP requests
#include "wifi_settings.h"  // where we store custome ssid/password (not pushed on github)


// warning: the builtin led has an inversed logic: needs to be LOW to be turned on. HIGH turns it off. go figure!?
#define _WORKING_LED_BUILTIN 2

#define VERSION "0.1"


#define QRY_OK                         0
#define QRY_ERR_HTTP_ERROR_CODE        1
#define QRY_ERR_HTTP_UNABLE_TO_CONNECT 2
#define QRY_ERR_WIFI_NOT_CONNECTED     3
#define QRY_NO_QUERY                   4
struct query_response {
  unsigned long duration_in_ms = 0;
  int query_status = QRY_NO_QUERY;
  String server_response = "";
};
query_response my_response;

String url_prefix = "http://10.155.100.89/cgi-bin/insert.py?NAME=WIFI_MON";
String url;

void setup() {
  Serial.begin(115200);

  pinMode(_WORKING_LED_BUILTIN, OUTPUT);
  digitalWrite(_WORKING_LED_BUILTIN, HIGH); // HIGH to turn off | LOW to turns on /!\


  // We start by connecting to a WiFi network
  Serial.print("start wifi scan mon using: ");
  Serial.println(WIFI_SSID);
  Serial.print("\n");

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // this will start autoconnect

}




void wget(){
  long start, end = 0;
  start = millis();
  my_response.duration_in_ms = 0;
  my_response.query_status = QRY_OK;
  my_response.server_response = "";
  
  // wait for WiFi connection
  if (WiFi.status() == WL_CONNECTED) {
    
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
  end = millis();
  my_response.duration_in_ms = end - start;
}

void loop() {
  
  WiFi.disconnect();
  delay(100);

  long start, end, duration, dur_scan = 0;
  Serial.print("start scan\n");
  start = millis();
  int num_ssid_found = WiFi.scanNetworks();
  end = millis();
  dur_scan = end - start;
  
  Serial.print(num_ssid_found);
  Serial.print("\t");
  Serial.print(dur_scan);
  Serial.print("\n");
  if (num_ssid_found <= 0) {
    delay(1000);
    return;
  }
  
  Serial.print("Trying to report results through wifi...\n");
  start = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // this will start wifi connect
  delay(100);
  int status = 0;
  end = millis();
  duration = end - start;
  
  my_response.duration_in_ms = 0;
  my_response.query_status = QRY_NO_QUERY;
  my_response.server_response = "";  
  boolean success = false;
  while(duration < 10000) { // 10 sec timeout to wifi connect
    status = WiFi.status();
    if (status != WL_CONNECTED) {
      delay(500); // wait
    } else {
      // ... let's go !
      success = true;
      Serial.print("connected to ");
      Serial.println(WIFI_SSID);
      Serial.print("\n");

      int channel = WiFi.channel();
      int rssi = WiFi.RSSI();
      url = url_prefix 
        +"&fw="+String(VERSION)
        +"&num_ssid="+String(num_ssid_found)
        +"&dur_scan="+String(dur_scan)
        +"&rssi="+String(rssi)+"&channel="+String(channel);
      
      Serial.print(url);
      Serial.print("\n");
      wget();
      WiFi.disconnect();
      Serial.print(my_response.server_response);
      Serial.print("\n");
      break;
    }
    end = millis();
  }
  
  digitalWrite(_WORKING_LED_BUILTIN, success ? LOW : HIGH); // HIGH to turn off | LOW to turns on 

  delay(1000);
}