//Include libraries copied from VESC
#include <VescUart.h>
#include <datatypes.h>

#define DEBUG
#define DEBUGSERIAL Serial
#define SERIAL_WIFI Serial3

String url = "http://192.168.1.20/cgi-bin/insert.py?NAME=A&VOLTAGE=30";
String read_buf;

void setup() {
  DEBUGSERIAL.begin(115200);
  #ifdef DEBUG
    //SEtup debug port
    SetDebugSerialPort(&DEBUGSERIAL);
  #endif
  
  SERIAL_WIFI.begin(115200);
  SetSerialPort(&SERIAL_WIFI);
}

void loop() {
  DEBUGSERIAL.println("MEGA: " + url);
  SERIAL_WIFI.print(url);
  SERIAL_WIFI.flush();

  while(true){
    while (SERIAL_WIFI.available() > 0) {
      read_buf = SERIAL_WIFI.readString();
    }
  
    if (read_buf.length() > 0) {
      DEBUGSERIAL.print("ESP: " + read_buf);
      read_buf = "";
      break;
    }
  }
  
  //delay(1000);
}
