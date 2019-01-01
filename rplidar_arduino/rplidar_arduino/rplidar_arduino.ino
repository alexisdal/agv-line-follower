
// This sketch code is based on the RPLIDAR driver library provided by RoboPeak
#include <RPLidar.h>

// You need to create an driver instance
RPLidar lidar;

#define RPLIDAR_MOTOR A0 // The PWM pin for control the speed of RPLIDAR's motor.
// This pin should connected with the RPLIDAR's MOTOCTRL signal

#define PIN_LED_WARNING  A5 // to light something up
#define PIN_LED_CRITICAL A4 // to light something up
#define PIN_LIDAR_DATA_0   10 //send data to arduino
#define PIN_LIDAR_DATA_1   11 //send data to arduino

#define COMM_ALL_OK   0
#define COMM_WARN     1
#define COMM_CRIT     2
#define COMM_ERR      3
//         D0   D1
// ALL OK   0    0
// WARN     0    1
// CRIT     1    0
// ERR      1    1


#define ZONE_CRITICAL  0
#define ZONE_A         1
#define ZONE_B         2
#define ZONE_C         3
#define ZONE_D         4
#define ZONE_E         5
#define NUM_ZONES 6
float min_distances[NUM_ZONES];
float tolerated_distances[NUM_ZONES];
#define MAXIMUM_DISTANCE 9999.0

void setup() {

  analogWrite(PIN_LED_WARNING, 0);
  analogWrite(PIN_LED_CRITICAL, 0);

  Serial.begin(115200);
  Serial.print("start\n");

  Serial.print("blinking done\n");


  // set pin modes
  pinMode(RPLIDAR_MOTOR, OUTPUT);
  pinMode(PIN_LED_WARNING, OUTPUT);
  pinMode(PIN_LED_CRITICAL, OUTPUT);
  pinMode(PIN_LIDAR_DATA_0, OUTPUT);
  pinMode(PIN_LIDAR_DATA_1, OUTPUT);

  tolerated_distances[ZONE_CRITICAL] = 210.0;
  tolerated_distances[ZONE_A] = 250.0;
  tolerated_distances[ZONE_B] = 260.0;
  tolerated_distances[ZONE_C] = 300.0;
  tolerated_distances[ZONE_D] = 360.0;
  tolerated_distances[ZONE_E] = 450.0;
  reset_values();

  send_communication(COMM_ERR);
  blink_led();
  
  // bind the RPLIDAR driver to the arduino hardware serial
  lidar.begin(Serial);



}

void reset_values()
{
  for (int i = 0 ; i < NUM_ZONES ; i++)
  {
    min_distances[i] = MAXIMUM_DISTANCE;
  }

}

//float get_min_warning_zone() {
//  return min(min_distances[ZONE_E], min(min_distances[ZONE_D],
//    min(min_distances[ZONE_C], min(min_distances[ZONE_B], min_distances[ZONE_A]))));
//}

bool is_warning_zone_entered()
{

  return   (   (min_distances[ZONE_A] < tolerated_distances[ZONE_A])
               || (min_distances[ZONE_B] < tolerated_distances[ZONE_B])
               || (min_distances[ZONE_C] < tolerated_distances[ZONE_C])
               || (min_distances[ZONE_D] < tolerated_distances[ZONE_D])
               || (min_distances[ZONE_E] < tolerated_distances[ZONE_E]) );
}

bool is_critical_zone_entered()
{
  return (min_distances[ZONE_CRITICAL] < tolerated_distances[ZONE_CRITICAL]);
}

void dump_zones_distances() {
  Serial.print(min_distances[ZONE_A]);
  Serial.print("\t");
  Serial.print(min_distances[ZONE_B]);
  Serial.print("\t");
  Serial.print(min_distances[ZONE_C]);
  Serial.print("\t");
  Serial.print(min_distances[ZONE_D]);
  Serial.print("\t");
  Serial.print(min_distances[ZONE_E]);
  Serial.print("\t");
  Serial.print(min_distances[ZONE_CRITICAL]);
  Serial.print("\n");
}

void handle_complete_rotation()
{
  //dump_zones_distances();
  int led_warning = 0;
  int led_critical = 0;
  int communication = 0;
  bool warn_entered = is_warning_zone_entered();
  bool crit_entered = is_critical_zone_entered();
  int state = COMM_ALL_OK;
  if (warn_entered)
  {
    led_warning = 255;
    state = COMM_WARN;
  }
  if (crit_entered)
  {
    led_critical = 255;
    state = COMM_CRIT;
  }
  if (!warn_entered && !crit_entered)
  {
    led_warning = 0;
    led_critical = 0;
    state = COMM_ALL_OK;
  }

  analogWrite(PIN_LED_WARNING, led_warning);
  analogWrite(PIN_LED_CRITICAL, led_critical);
  send_communication(state);
  reset_values();
}

void send_communication(int state) {
  //         D0   D1
  // ALL OK   0    0
  // WARN     0    1
  // CRIT     1    0
  // ERR      1    1
  if (state == COMM_ALL_OK) {
    digitalWrite(PIN_LIDAR_DATA_0, 0);
    digitalWrite(PIN_LIDAR_DATA_1, 0);
  } else if (state == COMM_WARN) {
    digitalWrite(PIN_LIDAR_DATA_0, 0);
    digitalWrite(PIN_LIDAR_DATA_1, 1);
  } else if (state == COMM_CRIT) {
    digitalWrite(PIN_LIDAR_DATA_0, 1);
    digitalWrite(PIN_LIDAR_DATA_1, 0);
  } else {
    digitalWrite(PIN_LIDAR_DATA_0, 1);
    digitalWrite(PIN_LIDAR_DATA_1, 1);
  }
}

void blink_led()
{
  for (int i = 0 ; i < 10 ; i++) {
    analogWrite(PIN_LED_CRITICAL, 255);
    analogWrite(PIN_LED_WARNING, 255);
    delay(50);
    analogWrite(PIN_LED_CRITICAL, 0);
    analogWrite(PIN_LED_WARNING, 0);
    delay(50);
  }
}
void loop() {

  if (IS_OK(lidar.waitPoint()))
  {
    float distance = lidar.getCurrentPoint().distance; //distance value in mm unit
    float angle    = lidar.getCurrentPoint().angle; //anglue value in degree
    bool  startBit = lidar.getCurrentPoint().startBit; //whether this point is belong to a new scan
    byte  quality  = lidar.getCurrentPoint().quality; //quality of the current measurement

    //perform data processing here...
    if (startBit)
    { // we made a complete rotation
      handle_complete_rotation();
    }

    if ((quality > 10) && (distance > 5.0))
    {

      // warning zone
      if ( ((0 <= angle) && (angle < 35.0)) && (distance < min_distances[ZONE_E]) ) {
        min_distances[ZONE_E] = distance;
      }
      else if ( ((35.0 <= angle && angle < 45.0)) && (distance < min_distances[ZONE_D]) ) {
        min_distances[ZONE_D] = distance;
      }
      else if ( ((45.0 <= angle) && (angle < 60.0)) && (distance < min_distances[ZONE_C]) ) {
        min_distances[ZONE_C] = distance;
      }
      else if ( ((60.0 <= angle && angle < 75.0)) && (distance < min_distances[ZONE_B]) ) {
        min_distances[ZONE_B] = distance;
      }
      else if ( ((75.0 <= angle) && (angle < 90.0)) && (distance < min_distances[ZONE_A]) ) {
        min_distances[ZONE_A] = distance;
      }
      else if ( ((270.0 <= angle) && (angle < 285.0)) && (distance < min_distances[ZONE_A]) ) {
        min_distances[ZONE_A] = distance;
      }
      else if ( ((285.0 <= angle && angle < 300.0)) && (distance < min_distances[ZONE_B]) ) {
        min_distances[ZONE_B] = distance;
      }
      else if ( ((300.0 <= angle) && (angle < 315.0)) && (distance < min_distances[ZONE_C]) ) {
        min_distances[ZONE_C] = distance;
      }
      else if ( ((315.0 <= angle) && (angle < 325.0)) && (distance < min_distances[ZONE_D]) ) {
        min_distances[ZONE_D] = distance;
      }
      else if ( ((325.0 <= angle) && (angle <= 360.0)) && (distance < min_distances[ZONE_E]) ) {
        min_distances[ZONE_E] = distance;
      }


      // critical zone
      if ( ((0 <= angle) && (angle <= 90.0)) && (distance < min_distances[ZONE_CRITICAL]) ) {
        min_distances[ZONE_CRITICAL] = distance;
      }
      else if ( ((270 <= angle) && (angle <= 360.0)) && (distance < min_distances[ZONE_CRITICAL]) ) {
        min_distances[ZONE_CRITICAL] = distance;
      }

    }

  }
  else
  {

    analogWrite(RPLIDAR_MOTOR, 0); //stop the rplidar motor

    // try to detect RPLIDAR...
    rplidar_response_device_info_t info;
    if (IS_OK(lidar.getDeviceInfo(info, 100))) {
      // detected...
      lidar.startScan();

      // start motor rotating at max allowed speed
      analogWrite(RPLIDAR_MOTOR, 255);
      delay(1000);
    } else {
      send_communication(COMM_ERR);
      blink_led();
    }
  }
}
