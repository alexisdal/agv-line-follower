#include <stdint.h>
#include <RPLidar.h>

#define VERSION "0.7.2" // increased detection zones with two different left/right critical zones

// You need to create an driver instance
RPLidar lidar;

#define RPLIDAR_MOTOR_PWM A0 // The PWM pin for control the speed of RPLIDAR's motor.
// This pin should connected with the RPLIDAR's MOTOCTRL signal

#define PIN_LED_WARNING  A5 // to light something up
#define PIN_LED_CRITICAL A4 // to light something up
#define PIN_LIDAR_DATA_0   10 //send data to arduino
#define PIN_LIDAR_DATA_1   11 //send data to arduino

#define PIN_LED_OVERALL_STATUS 12 // to light red reset button


#define COMM_ALL_OK   0
#define COMM_WARN     1
#define COMM_CRIT     2
#define COMM_ERR      3
//         D0   D1
// ALL OK   0    0
// WARN     0    1
// CRIT     1    0
// ERR      1    1


#define ZONE_CRIT_LEFT  0
#define ZONE_CRIT_RIGHT 1
#define ZONE_A          2
#define ZONE_B          3
#define ZONE_C          4
#define ZONE_D          5
#define ZONE_E          6
#define ZONE_F          7
#define ZONE_G          8
#define NUM_ZONES 9
uint16_t min_distances[NUM_ZONES];
uint16_t tolerated_distances[NUM_ZONES];
uint16_t tolerated_angles[NUM_ZONES][2]; // 0 start | 1 end
#define MAXIMUM_DISTANCE 9999


//bumper data
#define PIN_BUMPER_1   A2
#define PIN_BUMPER_2   A3
#define PIN_LED_BUMPER  2
#define PIN_BUMPER_DATA  4

#define NUM_BUMP_READ 3
int16_t bump_values_1[NUM_BUMP_READ];
int16_t bump_values_2[NUM_BUMP_READ];
int16_t index_bumper = 0;

#define NUM_ENTERED_EVENTS 3
#define NUM_ENTERED_TO_TRIG 2  // 2 "entered" events within the last 3 events trig a positive event (cheap debounce)
uint16_t index_trig = 0;
#define ZONE_CRITICAL  0
#define ZONE_WARNING   1
bool last_lidar_entered_events[NUM_ENTERED_EVENTS][2];


void setup() {

  // just to display fw version (does not interfere with rplidar)
  Serial.begin(115200); 
  Serial.print("Starting v");
  Serial.print(VERSION);
  Serial.print("\n");


  analogWrite(PIN_LED_WARNING, 0);
  analogWrite(PIN_LED_CRITICAL, 0);

  // reset button
  pinMode(PIN_LED_OVERALL_STATUS,   OUTPUT);
  digitalWrite(PIN_LED_OVERALL_STATUS, 1);
  delay(300);
  

  // set pin modes
  pinMode(RPLIDAR_MOTOR_PWM,OUTPUT);
  pinMode(PIN_LED_WARNING,  OUTPUT);
  pinMode(PIN_LED_CRITICAL, OUTPUT);
  pinMode(PIN_LIDAR_DATA_0, OUTPUT);
  pinMode(PIN_LIDAR_DATA_1, OUTPUT);

  // set bumper pin mode
  pinMode(PIN_BUMPER_1,    INPUT_PULLUP);
  pinMode(PIN_BUMPER_2,    INPUT_PULLUP);
  pinMode(PIN_LED_BUMPER,   OUTPUT);
  pinMode(PIN_BUMPER_DATA, OUTPUT);

  send_communication(COMM_ALL_OK);    // send lidar_ok  to the main arduino
  digitalWrite(PIN_BUMPER_DATA, LOW); // send bumper_ok to the main arduino


  tolerated_distances[ZONE_CRIT_RIGHT] = 350;
  tolerated_distances[ZONE_CRIT_LEFT]  = 290;
  tolerated_distances[ZONE_G] = 825;
  tolerated_distances[ZONE_F] = 665;
  tolerated_distances[ZONE_E] = 547;
  tolerated_distances[ZONE_D] = 489;
  tolerated_distances[ZONE_C] = 442;
  tolerated_distances[ZONE_B] = 414;
  tolerated_distances[ZONE_A] = 400;
  tolerated_angles[ZONE_G][0] = 0;
  tolerated_angles[ZONE_G][1] = 29;
  tolerated_angles[ZONE_F][0] = tolerated_angles[ZONE_G][1];
  tolerated_angles[ZONE_F][1] = tolerated_angles[ZONE_G][1] + 8;
  tolerated_angles[ZONE_E][0] = tolerated_angles[ZONE_F][1];
  tolerated_angles[ZONE_E][1] = tolerated_angles[ZONE_F][1] + 10;
  tolerated_angles[ZONE_D][0] = tolerated_angles[ZONE_E][1];
  tolerated_angles[ZONE_D][1] = tolerated_angles[ZONE_E][1] + 8;
  tolerated_angles[ZONE_C][0] = tolerated_angles[ZONE_D][1];
  tolerated_angles[ZONE_C][1] = tolerated_angles[ZONE_D][1] + 10;
  tolerated_angles[ZONE_B][0] = tolerated_angles[ZONE_C][1];
  tolerated_angles[ZONE_B][1] = tolerated_angles[ZONE_C][1] + 10;
  tolerated_angles[ZONE_A][0] = tolerated_angles[ZONE_B][1];
  tolerated_angles[ZONE_A][1] = tolerated_angles[ZONE_B][1] + 38;
  
  // trick to avoid dev errors (stop execution on dev mistake. sum must be 113. see freecad plan)
  if (tolerated_angles[ZONE_A][1] != (113)) { while(1) {;} }

  tolerated_angles[ZONE_CRIT_RIGHT][0] = 0;
  tolerated_angles[ZONE_CRIT_RIGHT][1] = 113;
  tolerated_angles[ZONE_CRIT_LEFT][0] = 360-113;
  tolerated_angles[ZONE_CRIT_LEFT][1] = 360;
  

  reset_values();
  for (size_t i = 0 ; i < NUM_BUMP_READ ; i++) {
    bump_values_1[i] = false;
    bump_values_2[i] = false;
  }
  
  // reset statuses to later debounce
  for (size_t i = 0 ; i < NUM_ENTERED_EVENTS ; i++) {
    last_lidar_entered_events[i][ZONE_CRITICAL] = false;
    last_lidar_entered_events[i][ZONE_WARNING]  = false;
  }


  //Serial.print("send comm ok");
  blink_led();
  //Serial.print("blinkled ok");
  // bind the RPLIDAR driver to the arduino hardware serial
  lidar.begin(Serial);
  //Serial.print("Begin lidar");




}

void reset_values()
{
  for (int16_t i = 0 ; i < NUM_ZONES ; i++)
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
  return   (      (min_distances[ZONE_A] <= tolerated_distances[ZONE_A])
               || (min_distances[ZONE_B] <= tolerated_distances[ZONE_B])
               || (min_distances[ZONE_C] <= tolerated_distances[ZONE_C])
               || (min_distances[ZONE_D] <= tolerated_distances[ZONE_D])
               || (min_distances[ZONE_E] <= tolerated_distances[ZONE_E])
               || (min_distances[ZONE_F] <= tolerated_distances[ZONE_F])
               || (min_distances[ZONE_G] <= tolerated_distances[ZONE_G]) );
}

bool is_critical_zone_entered()
{
  return ((min_distances[ZONE_CRIT_LEFT]  <= tolerated_distances[ZONE_CRIT_LEFT])
        ||(min_distances[ZONE_CRIT_RIGHT] <= tolerated_distances[ZONE_CRIT_RIGHT]) );
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
  int16_t led_warning = 0;
  int16_t led_critical = 0;
  int16_t communication = 0;
  bool warn_entered = is_warning_zone_entered();
  bool crit_entered = is_critical_zone_entered();
  last_lidar_entered_events[index_trig][ZONE_WARNING]  = warn_entered;
  last_lidar_entered_events[index_trig][ZONE_CRITICAL] = crit_entered;
  index_trig += 1;
  if (index_trig >= NUM_ENTERED_EVENTS) { index_trig = 0; }

  // sum the latest 3 events
  uint16_t total_warn_entered = 0;
  uint16_t total_crit_entered = 0;
  for (uint16_t i = 0 ; i < NUM_ENTERED_EVENTS; i++) {
    // false casts to 0 (nothing) / true casts to 1 (something)
    total_warn_entered += (uint16_t)last_lidar_entered_events[i][ZONE_WARNING];
    total_crit_entered += (uint16_t)last_lidar_entered_events[i][ZONE_CRITICAL];
  }
  
  // trig when total hits threshold
  bool warn_trig = total_warn_entered >= NUM_ENTERED_TO_TRIG;
  bool crit_trig = total_crit_entered >= NUM_ENTERED_TO_TRIG;
  
  int16_t state = COMM_ALL_OK;
  if (warn_trig)
  {
    led_warning = 255;
    state = COMM_WARN;
  }
  if (crit_trig)
  {
    led_critical = 255;
    state = COMM_CRIT;
  }
  if (!warn_trig && !crit_trig)
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

void send_communication(int16_t state) {
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
  for (size_t i = 0 ; i < 10 ; i++) {
    digitalWrite(PIN_LED_OVERALL_STATUS, 1);
    analogWrite(PIN_LED_CRITICAL, 255);
    analogWrite(PIN_LED_WARNING, 255);
    digitalWrite(PIN_LED_BUMPER, HIGH);
    delay(50);
    digitalWrite(PIN_LED_OVERALL_STATUS, 0);
    analogWrite(PIN_LED_CRITICAL, 0);
    analogWrite(PIN_LED_WARNING, 0);
    digitalWrite(PIN_LED_BUMPER, LOW);
    delay(50);
  }
}

void bumper_detection()
{
  // COM to GND && NC to analog read sur des input_pullup

  //Serial.println(analogRead(PIN_BUMPER_1));
  //Serial.println(analogRead(PIN_BUMPER_2));
  
  bump_values_1[index_bumper] = analogRead(PIN_BUMPER_1);
  bump_values_2[index_bumper] = analogRead(PIN_BUMPER_2);
  
  index_bumper++;
  if (index_bumper >= NUM_BUMP_READ) {index_bumper = 0; }

  int16_t max_bumps = 3; // hit ratio 3/6 instaed of 1/1 to accomodate for false positives due to vibration of the vehicule while using a mechanical switch
  int16_t nb_bumps = 0;
  for (uint16_t i = 0 ; i < NUM_BUMP_READ ; i++) {
    nb_bumps += (uint16_t)(bump_values_1[i] > 900);
    nb_bumps += (uint16_t)(bump_values_2[i] > 900);
  }
  
  if (nb_bumps >= max_bumps) { 
    digitalWrite(PIN_LED_BUMPER, HIGH); // led on
    digitalWrite(PIN_LED_OVERALL_STATUS, 1); // led on
    digitalWrite(PIN_BUMPER_DATA, HIGH); // send stop signal to the main arduino
  }
}

void loop() {
  
  bumper_detection();
  
  if (IS_OK(lidar.waitPoint()))
  { 
    uint16_t distance = (uint16_t)lidar.getCurrentPoint().distance; //distance value in mm unit
    uint16_t angle    = (uint16_t)lidar.getCurrentPoint().angle; //anglue value in degree (as float, observed values from 0.02 to 510.92. surpringly, ~3% of values have an angle above 360... go figure
    bool  startBit = lidar.getCurrentPoint().startBit; //whether this point is belong to a new scan
    uint8_t  quality  = lidar.getCurrentPoint().quality; //quality of the current measurement

    //perform data processing here...
    if (startBit)
    { // we made a complete rotation
      handle_complete_rotation();
    }

    if ((quality > 10) && (distance > 5))
    {

      // warning zone
      //if      ( ((  0 <= angle) && (angle <  35)) && (distance < min_distances[ZONE_F]) ) {
      //  min_distances[ZONE_F] = distance;
      //}
      //else if ( (( 35 <= angle) && (angle <  47)) && (distance < min_distances[ZONE_E]) ) {
      //  min_distances[ZONE_E] = distance;
      //}
      //else if ( (( 47 <= angle) && (angle <  55)) && (distance < min_distances[ZONE_D]) ) {
      //  min_distances[ZONE_D] = distance;
      //}
      //else if ( (( 55 <= angle) && (angle <  65)) && (distance < min_distances[ZONE_C]) ) {
      //  min_distances[ZONE_C] = distance;
      //}
      //else if ( (( 65 <= angle) && (angle <  75)) && (distance < min_distances[ZONE_B]) ) {
      //  min_distances[ZONE_B] = distance;
      //}
      //else if ( (( 75 <= angle) && (angle < 113)) && (distance < min_distances[ZONE_A]) ) {
      //  min_distances[ZONE_A] = distance;
      //}
      //else if ( ((247 <= angle) && (angle < 285)) && (distance < min_distances[ZONE_A]) ) {
      //  min_distances[ZONE_A] = distance;
      //}
      //else if ( ((285 <= angle) && (angle < 295)) && (distance < min_distances[ZONE_B]) ) {
      //  min_distances[ZONE_B] = distance;
      //}
      //else if ( ((295 <= angle) && (angle < 305)) && (distance < min_distances[ZONE_C]) ) {
      //  min_distances[ZONE_C] = distance;
      //}
      //else if ( ((305 <= angle) && (angle < 313)) && (distance < min_distances[ZONE_D]) ) {
      //  min_distances[ZONE_D] = distance;
      //}
      //else if ( ((313 <= angle) && (angle < 325)) && (distance < min_distances[ZONE_E]) ) {
      //  min_distances[ZONE_E] = distance;
      //}
      //else if ( ((325 <= angle) && (angle <= 360)) && (distance < min_distances[ZONE_F]) ) {
      //  min_distances[ZONE_F] = distance;
      //}
      
      //// critical zone
      //if      ( ((  0 <= angle) && (angle <= 113)) && (distance < min_distances[ZONE_CRITICAL]) ) {
      //  min_distances[ZONE_CRITICAL] = distance;
      //}
      //else if ( ((247 <= angle) && (angle <= 360)) && (distance < min_distances[ZONE_CRITICAL]) ) {
      //  min_distances[ZONE_CRITICAL] = distance;
      //}

      uint16_t start_angle = 0;
      uint16_t end_angle = 0;
      uint8_t i = 0;
      // warning zones (both sides)
      for (i = ZONE_A ; i < NUM_ZONES ; i++) {
        // left
        start_angle = tolerated_angles[i][0];
        end_angle   = tolerated_angles[i][1];
        if ((start_angle <= angle) && (angle <= end_angle) && (distance < min_distances[i])) {
          min_distances[i] = distance;
        }
        // right
        start_angle = 360 - tolerated_angles[i][1];
        end_angle   = 360 - tolerated_angles[i][0];
        if ((start_angle <= angle) && (angle <= end_angle) && (distance < min_distances[i])) {
          min_distances[i] = distance;
        }
      }
      
      // critical zones
      i = ZONE_CRIT_LEFT;
      start_angle = tolerated_angles[i][0];
      end_angle   = tolerated_angles[i][1];
      if ((start_angle <= angle) && (angle <= end_angle) && (distance < min_distances[i])) {
        min_distances[i] = distance;
      }
      i = ZONE_CRIT_RIGHT;
      start_angle = tolerated_angles[i][0];
      end_angle   = tolerated_angles[i][1];
      if ((start_angle <= angle) && (angle <= end_angle) && (distance < min_distances[i])) {
        min_distances[i] = distance;
      }

      
    }

  }
  else
  {

    analogWrite(RPLIDAR_MOTOR_PWM, 0); //stop the rplidar motor
    
    // try to detect RPLIDAR...
    rplidar_response_device_info_t info;
    if (IS_OK(lidar.getDeviceInfo(info, 100))) {
      // detected...
      lidar.startScan();

      // start motor rotating at max allowed speed
      analogWrite(RPLIDAR_MOTOR_PWM, 255);
      delay(1000);
    } else {
      send_communication(COMM_ERR);
      blink_led();
    }
  }
}
