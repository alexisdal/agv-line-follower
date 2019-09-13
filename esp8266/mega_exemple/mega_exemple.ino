#define VERSION "0.5.0"

#define DEBUGSERIAL Serial
#define SERIAL_WIFI Serial3

String url = "?NAME=AGV_DEV&VOLTAGE=27.1\n";
String read_buf;
String msg = "";

void setup() {
  DEBUGSERIAL.begin(115200);


  DEBUGSERIAL.print("start WIFI_DEV v");
  DEBUGSERIAL.print(VERSION);
  DEBUGSERIAL.print("\n");
  
   
  SERIAL_WIFI.begin(115200);
  DEBUGSERIAL.print("url: " + url);
  DEBUGSERIAL.print("\n");
  DEBUGSERIAL.print(url.length());
  DEBUGSERIAL.print(" (bytes)\n");
  
  delay(500);
}

long index = 0;
long url_index = 0;
void loop() {
  long start, end, duration;
  
  if      (index % 6 == 0 ) { msg = "_FW"; } 
  else if (index % 6 == 1 ) { msg = "_MC"; }
  else if (index % 6 == 2 ) { msg = "_IP"; }
  else if (index % 6 == 3 ) { msg = "_SS"; }
  else if (index % 6 == 4 ) { msg = url; }
  else if (index % 6 == 5 ) { msg = "_QL"; }
  else if (index % 6 == 5 ) { msg = "_SR"; }
  index++;

  DEBUGSERIAL.print("MEGA: " + msg);
  DEBUGSERIAL.print("\n");
  //start  = micros();
  SERIAL_WIFI.print(msg);
  SERIAL_WIFI.print("\n");
  SERIAL_WIFI.flush();
  //end = micros();
  //duration = end - start;
  //DEBUGSERIAL.print(duration);
  //DEBUGSERIAL.print("\n");
  delayMicroseconds(500);
  start = micros();
  //while (SERIAL_WIFI.available() > 0) {
    read_buf = SERIAL_WIFI.readStringUntil('\n');
  //}
  end = micros();

  if (read_buf.length() > 0) {
    DEBUGSERIAL.print("ESP: " + read_buf);
    if (!read_buf.endsWith("\n")) {  DEBUGSERIAL.print("\n");  }
    read_buf = "";
  } else {
    DEBUGSERIAL.print("ESP: <no answer>\n");
  }
  duration = end - start;
  DEBUGSERIAL.print(duration);
  DEBUGSERIAL.print("\n");
  
  delay(1000);
}
