#define VERSION "0.2.d"

#define DEBUGSERIAL Serial
#define SERIAL_WIFI Serial3

//String url = "http://10.155.100.89/cgi-bin/insert.py?NAME=WIFI_DEV&VOLTAGE=25";
String url = "http://10.155.100.89/cgi-bin/insert.py?NAME=WIFI_DEV&VOLTAGE=24.20&TACHOMETER=1589724&DUTYCYCLE=0.23&CURRENT_TICK=11202735&SERIAL=6e756e6b776f04d&FW=0.9.2.1&KM=137&M=907.55&num_bumps=0&num_lidar_crit=0&num_lidar_err=0&num_line_lost=0"; // 229 bytes :-( @112500
//String url = "n=DEV&v=24.20&t=1589724&dty=0.23&ct=11202735&sr=6e756e6b776f04d&fw=0.9.2.1&km=137&m=907.55&bps=0&lc=0&le=0&ll=0"; // 112 bytes :-( @112500 -> 9ms
String read_buf;
String msg = "";

void setup() {
  DEBUGSERIAL.begin(115200);


  DEBUGSERIAL.print("start WIFI_TEST v");
  DEBUGSERIAL.print(VERSION);
  DEBUGSERIAL.print("\n");
  
   
  SERIAL_WIFI.begin(115200);
  DEBUGSERIAL.print("url: " + url);
  DEBUGSERIAL.print("\n");
  DEBUGSERIAL.print(url.length());
  DEBUGSERIAL.print(" (bytes)\n");
  
  delay(5000);
}

long index = 0;
void loop() {
  long start, end, duration;
  
  //if      (index % 5 == 0 ) { msg = "_FW"; } 
  //else if (index % 5 == 1 ) { msg = "_MAC"; }
  //else if (index % 5 == 2 ) { msg = "_IP"; }
  //else if (index % 5 == 3 ) { msg = "_STATUS"; }
  //else                      { msg = url; }
  //index++;
  msg = "_SS";

  //DEBUGSERIAL.print("MEGA: " + msg);
  //DEBUGSERIAL.print("\n");
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
