

// This sketch code is based on the RPLIDAR driver library provided by RoboPeak
#include <RPLidar.h>

// You need to create an driver instance
RPLidar lidar;

#define RPLIDAR_MOTOR 2 // The PWM pin for control the speed of RPLIDAR's motor.
 
void setup() {

  Serial.begin(115200);
  Serial.print("start\n");
  // bind the RPLIDAR driver to the arduino hardware serial
  lidar.begin(Serial2);

  // set pin modes
  pinMode(RPLIDAR_MOTOR, OUTPUT); //analogWrite(RPLIDAR_MOTOR, 0); //stop the rplidar motor
  pinMode(LED_BUILTIN, OUTPUT);

    // blink to show reset
    for (int i = 0 ; i < 5 ; i++) {
    analogWrite(LED_BUILTIN, 0);
    delay(100);
    analogWrite(LED_BUILTIN, 255);
    delay(100);
    }
    analogWrite(LED_BUILTIN, 0);

}

int light_factor = 0;
float min_distance = 9999;
unsigned long previous_event_tick = 0;
unsigned long current_tick = 0;
unsigned long count = 0;
unsigned long rev_count = 0;

void loop() 
{

  current_tick = millis();
  if (current_tick - previous_event_tick > 1000) {
    Serial.print("count: ");
    Serial.print(count);
    Serial.print("\t");
    Serial.print("rev_count: ");
    Serial.print(rev_count);
    Serial.print("\n");
    count = 0;
    rev_count = 0;
    previous_event_tick = current_tick;
  }

  if (IS_OK(lidar.waitPoint())) {
    float distance = lidar.getCurrentPoint().distance; //distance value in mm unit
    float angle    = lidar.getCurrentPoint().angle; //anglue value in degree
    bool  startBit = lidar.getCurrentPoint().startBit; //whether this point is belong to a new scan
    byte  quality  = lidar.getCurrentPoint().quality; //quality of the current measurement

    count++;
    if (startBit) { rev_count++; }
    
    /*
    Serial.print(distance);
    Serial.print("\t");
    Serial.print(angle);
    Serial.print("\t");
    Serial.print(startBit);
    Serial.print("\t");
    Serial.print(quality);
    Serial.print("\n");
    */
  } 
  
  else {

    Serial.print("lidar not ok\n");
    analogWrite(RPLIDAR_MOTOR, 0); //stop the rplidar motor

    // try to detect RPLIDAR...
    rplidar_response_device_info_t info;
    if (IS_OK(lidar.getDeviceInfo(info, 100))) {
      // detected...
      lidar.startScan();

      // start motor rotating at max allowed speed
      analogWrite(RPLIDAR_MOTOR, 255);
      delay(1000);
  }
  }
}
