#include <ESP8266WiFi.h> // to connect to a wifi network with a single ssid/password
#include <ESP8266HTTPClient.h> // to make HTTP requests
#include "wifi_settings.h"  // where we store custome ssid/password (not pushed on github)
#include "queue.h"  // where we keep request to be sent

#define VERSION "0.5.0.2" // url replay

// based on: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiClient/WiFiClient.ino
//           https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/examples/BasicHttpClient/BasicHttpClient.ino


// warning: the builtin led has an inversed logic: needs to be LOW to be turned on. HIGH turns it off. go figure!?
#define _WORKING_LED_BUILTIN 2


#define QRY_OK                         0
#define QRY_ERR_HTTP_ERROR_CODE        1
#define QRY_ERR_HTTP_UNABLE_TO_CONNECT 2
#define QRY_ERR_WIFI_NOT_CONNECTED     3
#define QRY_NO_QUERY                   4
struct query_response {
  int32_t duration_in_ms = 0;  // on arduino, millis() returns "a long on arduino", which is =>  Long variables are extended size variables for number storage, and store 32 bits (4 bytes), from -2,147,483,648 to 2,147,483,647. see https://www.arduino.cc/reference/en/language/variables/data-types/long/
  uint8_t query_status = QRY_NO_QUERY; // int -> uint8_t because this int needs just a few values
  String server_response = "";
};
query_response my_response;

Queue q;

// on Sept 12th, the typical request would be like
// ?n=WIFI_DEV&v=25.00&tc=946207&dc=0.21&ct=6155831&sl=6e756e6b776f0ed&fw=0.9.3&km=154&m=905.74&nb=0&nc=1&ne=0&ll=0
// 112 bytes (what goes over Serial as input => less keep 160 bytes as max input buffer (so that we have space for future)
#define MAX_BUF_LEN 160
char buffer[MAX_BUF_LEN];
uint16_t bufindex = 0;


//String url; // we keep a String var because the HTTP lib example uses it
#define MAX_URL_LEN 300
char url[MAX_URL_LEN];

#define IDLE_TIME_BETWEEN_HTTP_REQUESTS_IN_MS 200
uint32_t current_tick = 0;
uint32_t last_attempt_in_ms = 0;

void setup() {

  Serial.begin(115200);
  Serial.setTimeout(500); // sets the maximum milliseconds to wait for serial data. It defaults to 1000 milliseconds.

  pinMode(_WORKING_LED_BUILTIN, OUTPUT);
  digitalWrite(_WORKING_LED_BUILTIN, HIGH); // HIGH to turn off | LOW to turns on /!\
  
  Serial.print("start wifi v");
  Serial.print(VERSION);
  Serial.print("\n");

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println();
  Serial.print("MAC: ");
  Serial.print(WiFi.macAddress());
  Serial.print("\n");

  delay(100);
}


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
  if( WiFi.status() == WL_CONNECTED ) {
    Serial.print(WiFi.localIP());
    Serial.print("\n");
  } else {
    Serial.print("NOT_CONNECTED\n");
  }
}

void print_status(int32_t status){
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
  Serial.print(get_signal_strengh());
  Serial.print("\n");
}

// https://www.arduino.cc/en/Reference/WiFiRSSI
int32_t get_signal_strengh() {
  if (WiFi.status() != WL_CONNECTED) {
    return 0;
  } else {
    return WiFi.RSSI();
  }
}


void do_http_requests() {
  if (q.getNumUsedSlots() <= 0) {
    return; // nothing to do since queue is empty
  }
  
  //sprintf(url, "http://10.155.100.89/cgi-bin/test.py%s&rssi=%d&channel=%d&queue_size=%d", q.peek(), get_signal_strengh(), WiFi.channel(), q.getNumUsedSlots());
  //sprintf(url, "http://10.155.100.299/invalidip");
  //sprintf(url, "http://10.155.249.179/valid_server_but_apache_not_running");
  //sprintf(url, "http://10.155.249.179/cgi-bin/test.py%s&rssi=%d&channel=%d&queue_size=%d", q.peek(), get_signal_strengh(), WiFi.channel(), q.getNumUsedSlots());
  sprintf(url, "http://10.155.249.179/cgi-bin/insert.py%s&rssi=%d&channel=%d&queue_size=%d", q.peek(), get_signal_strengh(), WiFi.channel(), q.getNumUsedSlots());
  Serial.print(url);
  Serial.print("\n");
  wget();
  
  
  // pop request if server response starts with 'OK' (otherwise it will be played again
  if (my_response.query_status == QRY_OK 
    && (my_response.server_response.startsWith("OK") || my_response.server_response.startsWith("ok") ) ) {
    q.pop();
  }
  //print_query_status();
  //print_server_response();
  //Serial.print(q.getNumUsedSlots()); Serial.print("\n");
    
}

void wget(){
  int32_t start, end = 0;
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
      int32_t httpCode = http.GET();  // returns an 'int' so int32_t on ESP8266 => https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.h
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


void handle_input_request() {
  if (strlen(buffer) <= 0) {
    return;
  }
  
  if (buffer[0] == '?') {
    // drop old request if we have no more space left :(
    if (q.getNumFreeSlots() <= 0) { q.pop(); }
    q.push(buffer); // enqueue request
    // reset previous response
     my_response.duration_in_ms = 0;
     my_response.query_status = QRY_NO_QUERY;
     my_response.server_response = "";
    return;
  } 
  
  if (buffer[0] != '_') {
    Serial.print("ERROR: invalid request (must start with '?' or '_')\n");
    return;
  }
  
  if (strlen(buffer) < 3) {
    Serial.print("ERROR: invalid request code\n");
    return;
  }

  if        (strncmp("_ST", buffer, 3) == 0) {
    print_status(WiFi.status());
  } else if (strncmp("_IP", buffer, 3) == 0) {
    print_ip();
  } else if (strncmp("_FW", buffer, 3) == 0) {
    print_fw();
  } else if (strncmp("_MC", buffer, 3) == 0) {
    print_mac();
  } else if (strncmp("_QR", buffer, 3) == 0) { // next query in queue
    Serial.print(q.peek());Serial.print("\n");
  } else if (strncmp("_QL", buffer, 3) == 0) { // queue length
    Serial.print(q.getNumUsedSlots());Serial.print("\n");
  } else if (strncmp("_QS", buffer, 3) == 0) { // last query status
    print_query_status();
  } else if (strncmp("_SR", buffer, 3) == 0) { // last server response
    print_server_response();
  } else if (strncmp("_SS", buffer, 3) == 0) { // signal strength
    print_signal_strength();
  } else if (strncmp("_FL", buffer, 3) == 0) { // flush queue
    q.flush();
  } else {
    Serial.print("ERROR: unknown request\n");
  }
}      

// as a general rule, if we push requests too fast while the ESP8266 is busy trying to send requests over a crowded network, then we risk to drop some data on the Serial link...
void loop() {
  current_tick = millis();
  
  digitalWrite(_WORKING_LED_BUILTIN, WiFi.status() == WL_CONNECTED ? LOW : HIGH); // HIGH to turn off | LOW to turns on 

  while (Serial.available() > 0) {
    char c = Serial.read(); 
    if (bufindex < MAX_BUF_LEN - 1 && c != '\n') {
      buffer[bufindex++] = c;
    }
    if (c == '\n') {
      buffer[bufindex] = '\0';
      handle_input_request();
      // reset buffer
      buffer[0] = '\0'; 
      bufindex = 0;
    }
  } 
  
  // dont flood the wifi network in case the server answers with an error message
  if (current_tick - last_attempt_in_ms > IDLE_TIME_BETWEEN_HTTP_REQUESTS_IN_MS) {
    do_http_requests();
    last_attempt_in_ms = millis();
  }
  
}
