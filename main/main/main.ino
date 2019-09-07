#include <stdint.h>
#define VERSION "0.9.2.1" // report events false positive fix line_lost


#include <ArduinoUniqueID.h> // to read serial number of arduino board
String arduino_serial = "";
#define AGV_NAME "AGV_DEV"
#define WIFI_REPORTING_INTERVAL_IN_SECS 30


// To use VescUartControl stand alone you need to define a config.h file, that should contain the Serial or you have to comment the line
// #include Config.h out in VescUart.h
// This lib version tested with vesc fw 3.38 and 3.40 on teensy 3.2 and arduino uno

//VESC
#include <VescUart.h>
#include <datatypes.h>

//Pixy 
#include <Pixy2.h>

//Serial ports
#define SERIAL_RIGHT Serial1
#define SERIAL_LEFT Serial2
#define SERIAL_WIFI Serial3

#define VOLTAGE_WARNING_LEVEL 23.0 // 2x12v lead batteries
#define VOLTAGE_STOP_LEVEL 22.5 

#define FOUR_INCHES_WHEEL_GEARED
/*#define OVERBOARD_WHEEL

#ifdef OVERBOARD_WHEEL
#define ESC_MIN -2500
#define ESC_STOP 150 // ATTENTION: au debug on s'apercoit qu'en fait la roue s'arrete de tourner dès 1634 -- MEME A VIDE --
#define ESC_MAX 2500
#define NOMINAL_SPEED 900
#define NOMINAL_SPEED_WARNING 550
#endif*/

#ifdef FOUR_INCHES_WHEEL_GEARED
#define ESC_MIN -5000
#define ESC_STOP 150 
#define ESC_MAX 5000
#define NOMINAL_SPEED 3000
#define NOMINAL_SPEED_WARNING 1500
#endif

#define OBSERVED_FPS 60.0 // how much fps i have measure in my console given everything i do
#define MAX_LINE_MISS 90 // at 60fps, 90 errors makes 1.5sec without seeing the line
#define X_CENTER  (pixy.frameWidth/2) // position de la ligne sur le capteur

//Lidar data
#define PIN_LIDAR_DATA_0   10 //receive data from lidar arduino
#define PIN_LIDAR_DATA_1   11 //receive data from lidar arduino
#define PIN_LIDAR_BUMP     4 //receive default bumper signal

#define COMM_ALL_OK   0
#define COMM_WARN     1
#define COMM_CRIT     2
#define COMM_ERR      3


#define NUM_READING_VOLTAGE 10
#define NUM_READING_DUTY 10

// Bandeau led
// PROD
#define PIN_LED_RED 6
#define PIN_LED_BLUE 5
// // OLD DEV BOARD
//#define PIN_LED_RED 5
//#define PIN_LED_BLUE 6

#define PIN_LED_GREEN 7

#define NUM_COLORS 8

//Barcode Stops
#define STOP_EVERY_X_BARCODE  3
#define METRO_STATION_STOP_DURATION_IN_SECONDS 10
#define PORT_STOP_DURATION_IN_SECONDS 30
#define LIDAR_CRIT_ERR_STOP_DURATION_IN_SECONDS 5
#define BUMP_STOP_BEFORE_RESUME_DURATION_IN_SECONDS 5

Pixy2 pixy;

//Handle km 
#include <EEPROM.h>
#define FORCE_EEPROM_RESET 0
#define EE_ADDRESS 20  // a bug in previous release damaged the flash at address 0x00
#define MM_PER_TACHO_UNIT  1.377467548  // 260 tacho per rev  / 358.11 mm circumference (114 mm diamter * pi)
struct dataInEEPROM {
  char fw_version[10];
  int64_t km; // current km value (will be incremeted by 1 for each 1000m
  float meters;
};
dataInEEPROM mydataInEEPROM;

float meters_from_vesc_at_arduino_boot = 0.0f;
float current_meters = 0.0f;
#define SAVE_KM_INTERVAL_IN_SEC  600 // every 10min gives EEPROM a theorical  lifetime of 3.5years on a 2 shifts warehouse.
                                     // worst case scenario, someone turns off the AGV 599sec after the previous save: 
                                     // at 0.33m/s average speed => 0.33*599s = 200m lost in the counter => ok 
uint64_t last_km_save_in_ms = 0;
int64_t meters_already_saved = 0;

struct bldcMeasure measuredValLeft; // to read battery voltage, tachometer, etc...
struct bldcMeasure measuredValRight;

bool voltage_ready = false;
bool duty_ready = false;
bool battery_warning = false;
bool battery_low = false;

float inpVoltages[NUM_READING_VOLTAGE];
float inpDutyCycle[NUM_READING_DUTY];
int32_t voltage_index = 0;
int32_t duty_index = 0;

float average_voltage = 0.0;
float average_duty = 0.0;

uint64_t last_voltage_check_in_ms = 0;
uint64_t last_wifi_data_in_ms = 0;
uint64_t last_duty_check_in_ms = 0;

uint64_t last_tick = 0;
uint64_t current_tick = 0;
uint64_t duration = 0;
uint64_t num_loop = 0;


// this will count how many bumps / lidar_* / line_lost we have to report over wifi
struct events_to_reports {
  int32_t num_bumps = 0;
  int32_t num_lidar_crit = 0;
  int32_t num_lidar_err = 0;
  int32_t num_line_lost = 0;
};
events_to_reports my_events_to_report;

// this will let us stop the AGV without stopping the loop() with a delay + detect the event only once to correctly count them
struct event_detect {
  bool active = false; // if the event is still ongoing or if it's gone
  int64_t last_seen_in_ms = 0; // when the event happened for the last time
};
event_detect bumped;
event_detect lidar_crit;
event_detect lidar_err;
event_detect line_lost;

int previous_bump_pin_read = 0; // debounce bump events

float acc_limit_positive = (NOMINAL_SPEED - ESC_STOP) / 0.25 / OBSERVED_FPS; // from stop to top speed on 0.50sec  => how much incremental speed we allow per frame
float acc_limit_negative = (NOMINAL_SPEED - ESC_STOP) / 0.15 / OBSERVED_FPS; // in 0.30sec => when we brake, we want to brake faster

//#define KSTEEP 1.2 //0.8
float KSTEEP = (NOMINAL_SPEED - ESC_STOP) * 1.025;

int32_t motor_speed_left = 0;
int32_t motor_speed_right = 0;
int32_t motor_speed_left_prev = 0;
int32_t motor_speed_right_prev = 0;

int32_t nominal_speed = NOMINAL_SPEED;

bool motorstop = false;
int32_t linePosition = 0;
uint64_t nbError = 9999;
#define NUM_LINES_OK_TO_START_MOVING 10
uint64_t nb_line_ok = 0;

int32_t lidar_state = COMM_ALL_OK;

// metro stop
uint64_t metro_stop_counter = 0;
uint64_t last_barcode_read_tick = 0;

//enum color{C_OFF, C_RED, C_GREEN, C_BLUE, C_PURPLE, C_LIME, C_CYAN, C_WHITE}; 
//enum c{_R, _G, _B}; 
#define _R 0
#define _G 1
#define _B 2
#define C_OFF    0
#define C_RED    1
#define C_GREEN  2
#define C_BLUE   3
#define C_PURPLE 4
#define C_LIME   5
#define C_CYAN   6
#define C_WHITE  7 



int32_t rgb[NUM_COLORS][3] = {
  { 0, 0, 0 },   // C_OFF 
  { 1, 0, 0 },   // C_RED
  { 0, 1, 0 },   // C_GREEN
  { 0, 0, 1 },   // C_BLUE
  { 1, 0, 1 },   // C_PURPLE
  { 1, 1, 0 },   // C_LIME
  { 0, 1, 1 },   // C_CYAN
  { 1, 1, 1 }    // C_WHITE
};

void setup() {
  Serial.begin(115200);
  SERIAL_WIFI.begin(115200);
  

  Serial.print("Starting v");
  Serial.print(VERSION);
  Serial.print("\n");

  Serial.print("name:");
  Serial.print(String(AGV_NAME));
  Serial.print("\n");
  
  arduino_serial = "";
  for (size_t i = 0; i < UniqueIDsize; i++){
    arduino_serial += String(UniqueID[i], HEX);
  }
  Serial.print("serial:");
  Serial.print(arduino_serial);
  Serial.print("\n");
  

  pinMode(PIN_LIDAR_DATA_0, INPUT);
  pinMode(PIN_LIDAR_DATA_1, INPUT);
  pinMode(PIN_LIDAR_BUMP, INPUT_PULLUP);

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  
  setup_motors();
  stop_motors(); update_motors();
  
  set_led_color(C_GREEN);




  read_km_from_eeprom();
  update_measured_values() ; meters_from_vesc_at_arduino_boot = get_meters_from_vesc();
  Serial.print("KM:");
  Serial.print(mydataInEEPROM.km);
  Serial.print("\t");
  Serial.print("M:");
  Serial.print(mydataInEEPROM.meters);
  Serial.print("\t");
  Serial.print("V:");
  Serial.print(meters_from_vesc_at_arduino_boot);
  Serial.print("\n");

  //mydataInEEPROM.meters = 312.91f;
  //EEPROM.put(EE_ADDRESS, mydataInEEPROM); 
  //while(1);
  
  
  //stay stopped for a while (safe start 3sec)
  int32_t t = 1000 / NUM_COLORS * 3;
  for (size_t i = 0 ; i < 1 ; i++) {
    for (size_t j = 0 ; j < NUM_COLORS ; j++) {
      set_led_color(j); delay(t);
    }
  }
  set_led_color(C_GREEN);
  delay(250);
  

  pixy.init(); // it always output a single "error: no response" and then will work
  // i dont know how to get rid of id (it's in the supplier lib, i will not touch it. i have same behavior with bundled example)
  pixy.setLED(255, 255, 255); // white
  
  //Change to line tracking program
  pixy.changeProg("line");

  //initialize the voltage checks
  for (size_t i = 0 ; i < NUM_READING_VOLTAGE ; i++) {
    inpVoltages[i] = -1.0;
  }

  current_tick = millis();
  last_tick = millis();
  delay(1000.0/OBSERVED_FPS);
  
  pixy.setLED(255, 255, 255); // off
  
  pixy.setLamp(1, 1); // Turn on both lamps, upper and lower for maximum exposure
  
  Serial.print("init done\n");
}

void read_km_from_eeprom () {
 // read values in memory
  EEPROM.get(EE_ADDRESS, mydataInEEPROM); 
  // reset if EEPROM is unexpected
  if ((mydataInEEPROM.fw_version[0] < '0' && mydataInEEPROM.fw_version[0] > '5') || FORCE_EEPROM_RESET) {
    Serial.print("invalid data... resetting EEPROM!\n");
    strncpy(mydataInEEPROM.fw_version, VERSION, 10);
    mydataInEEPROM.km = 0L;
    mydataInEEPROM.meters = 0.0f;
    EEPROM.put(EE_ADDRESS, mydataInEEPROM); 
  }
}

void update_measured_values(){
  VescUartGetValue(measuredValRight, &SERIAL_RIGHT);
  VescUartGetValue(measuredValLeft, &SERIAL_LEFT);
}

float get_meters_from_vesc() {
  // update_measured_values() must have been called beforehand
  // /!\ while doing debug with Serial and laptop, resetting the arduino does *NOT* reset the VESC.
  // therefore value of tachometer will be kept unless you manually power off/on the main battery like a regular operator would do 
  int64_t mytacho = measuredValLeft.tachometer/2 + measuredValRight.tachometer/2;
  return (float)(mytacho) * MM_PER_TACHO_UNIT / 1000.0f ;
}

int64_t handle_kmeters(){
  current_meters = get_meters_from_vesc() - meters_from_vesc_at_arduino_boot;
  if ((current_tick - last_km_save_in_ms) > (SAVE_KM_INTERVAL_IN_SEC * 1000L)) {  
    // needs saving
    float meters_to_save = mydataInEEPROM.meters + (current_meters - meters_already_saved);
    if (meters_to_save > 1000.0) {
      // increase km
      int64_t extra_km = (long)(meters_to_save/1000.0);
      mydataInEEPROM.km += extra_km;
      mydataInEEPROM.meters = meters_to_save - (extra_km*1000.0);
    } else {
      // dont touch km, just meters
      mydataInEEPROM.meters = meters_to_save;
    }
    Serial.print("Saving km:");
    Serial.print(mydataInEEPROM.km);
    Serial.print("\t");
    Serial.print("m:");
    Serial.print(mydataInEEPROM.meters);
    Serial.print("\n");
    EEPROM.put(EE_ADDRESS, mydataInEEPROM);
    
    meters_already_saved = current_meters;
    last_km_save_in_ms = current_tick;
  }
}


void set_led_color(int c) {
  digitalWrite(PIN_LED_RED   , rgb[c][_R]);
  digitalWrite(PIN_LED_GREEN,  rgb[c][_G]);
  digitalWrite(PIN_LED_BLUE  , rgb[c][_B]);
}


void setup_motors()
{
  //Setup UART port
  SERIAL_RIGHT.begin(115200);
  SetSerialPort(&SERIAL_RIGHT);
  SERIAL_LEFT.begin(115200);
  SetSerialPort(&SERIAL_LEFT);
}

void bumper_detection() {
  // we do have false positives from time to time (about a few times a day))
  // dunno why
  // let's debounce it (must be two consecutive events). at speed 0.4m/s@60 it's roughly 6mm distance which is perfectly fine
  int current_bump_pin_read = digitalRead(PIN_LIDAR_BUMP);
  if (current_bump_pin_read == previous_bump_pin_read && current_bump_pin_read == 1) {
    if (!bumped.active) {
      bumped.active = true;
      my_events_to_report.num_bumps += 1;
    }
  } else {
    if (bumped.active) {
      delay(BUMP_STOP_BEFORE_RESUME_DURATION_IN_SECONDS * 1000L); // wait a bit because rplidar needs ~3sec to correctly boot and detect bumps
      bumped.active = false;
    }
  }
  previous_bump_pin_read = current_bump_pin_read;
}


int32_t get_lidar_state()
{
  int32_t d0 = digitalRead(PIN_LIDAR_DATA_0);
  int32_t d1 = digitalRead(PIN_LIDAR_DATA_1);

  //         D0   D1
  // ALL OK   0    0
  // WARN     0    1
  // CRIT     1    0
  // ERR      1    1

  int32_t state = 0;

  if        ((d0 == 0) && (d1 == 0)) {
    state = COMM_ALL_OK;
  } else if ((d0 == 0) && (d1 == 1)) {
    state = COMM_WARN;
  } else if ((d0 == 1) && (d1 == 0)) {
    state = COMM_CRIT;
  } else {
    state = COMM_ERR;
  }
  return state;
}


void obstacle_detection()
{
  lidar_state = get_lidar_state();
  if (lidar_state == COMM_ERR) {
    if (!lidar_err.active) { 
      my_events_to_report.num_lidar_err += 1; 
    }
    lidar_err.active = true;
    lidar_err.last_seen_in_ms = current_tick;
  } else if (lidar_state == COMM_CRIT) {
    if (!lidar_crit.active) {
      my_events_to_report.num_lidar_crit += 1;
    }
    lidar_crit.active = true;
    lidar_crit.last_seen_in_ms = current_tick;
  } else if (lidar_state == COMM_WARN) {
    nominal_speed = NOMINAL_SPEED_WARNING;
    KSTEEP = (NOMINAL_SPEED_WARNING - ESC_STOP);
  } else {
    nominal_speed = NOMINAL_SPEED;
    KSTEEP = (NOMINAL_SPEED - ESC_STOP);
  }
  
  // reset flags after correct duration
  if (lidar_err.active) {
    if (current_tick > lidar_err.last_seen_in_ms + LIDAR_CRIT_ERR_STOP_DURATION_IN_SECONDS*1000L) {
      lidar_err.active = false;
    }
  }
  if (lidar_crit.active) {
    if (current_tick > lidar_crit.last_seen_in_ms + LIDAR_CRIT_ERR_STOP_DURATION_IN_SECONDS*1000L) {
      lidar_crit.active = false;
    }
  }
}


void lecture_pixy_front()
{

  int8_t res;
  int32_t error;
  // int left, right;
  // char buf[96];
  // Get latest data from Pixy, including main vector, new intersections and new barcodes.
  res = pixy.line.getMainFeatures(); // using getMainFeature will give barcode only "once" (whereas getAllFeatures show barcodes all the time)

  // If error or nothing detected, stop motors
  if (res <= 0)
  {
    nbError += 1;
    //Serial.print("KO\t");
    //Serial.print(nbError);
    //Serial.print("\t");
    if (nbError > MAX_LINE_MISS) // given that loops runs at 62 fps => 90 errors makes it 1.5sec => at 0.4m/s it's about 60cm run
    {
      stop_motors(); update_motors();
      if (!line_lost.active) {
        line_lost.active = true;
        my_events_to_report.num_line_lost += 1;
        nb_line_ok = 0;
      }
    }
  } else {
    //Serial.print("ok\t");
    if (res & LINE_VECTOR) {
      //Serial.print("ok\t");
      nbError = 0; 
      nb_line_ok += 1;
      // require 5 correct line to start moving => goal = when line is lost, do not trigger an addtional line_lost event when a line appears in a frame and disappears in the following frame
      if (line_lost.active && nb_line_ok <= NUM_LINES_OK_TO_START_MOVING) { return; }
      line_lost.active = false;
      // Calculate heading error with respect to m_x1, which is the far-end of the vector,
      // the part of the vector we're heading toward.
      linePosition = (int32_t)pixy.line.vectors->m_x1 - (int32_t)X_CENTER;
      //Serial.print(linePosition);
      //Serial.print(" : ");

    }

    // Managing barcodes
    if (res & LINE_BARCODE)
    {
      if (pixy.line.barcodes->m_code == 0) { // barcode 0 is metro station
        //Serial.print("*** METRO STATION *** \n");
        //handle_metro_stop();
      } else if (pixy.line.barcodes->m_code == 1) { // barcode 1 is for unloading boxes stop
        //Serial.print("*** STOP: UNLOADING BOXES *** \n");
        unloading_stop();
      } else if (pixy.line.barcodes->m_code == 2) { // barcode 2 is stop: low battery 
        //Serial.print("*** STOP: LOW BATTERY *** \n");
        batt_low_stop();
      }/* else {
        stop_motors(); update_motors();
        while(1) {
          set_led_color(C_PURPLE);          
          delay(200);
          set_led_color(C_WHITE);          
          delay(200);
        }
      }*/
    }
  }
}

void handle_metro_stop() {
  if (current_tick - last_barcode_read_tick > 1000) {
    //Serial.print("*** BARCODE ACCEPTED *** \n");
    if (metro_stop_counter % STOP_EVERY_X_BARCODE == 0) {
      stop_motors(); update_motors();
      set_led_color(C_CYAN);
      //Serial.print("*** METRO STATION STOP *** \n");

      delay(1000L * METRO_STATION_STOP_DURATION_IN_SECONDS);
    }
    metro_stop_counter++;
  }
  last_barcode_read_tick = millis();
}

void unloading_stop() {
  if (current_tick - last_barcode_read_tick > 1000) {
      stop_motors(); update_motors();
      set_led_color(C_CYAN);
      delay(1000L * PORT_STOP_DURATION_IN_SECONDS);
  }
  last_barcode_read_tick = millis();
}

void batt_low_stop() {
  if (battery_low) {
  
  stop_motors(); 
  update_motors();
  
  while(1) {
    set_led_color(C_BLUE);
    delay(1000);
      set_led_color(C_OFF);
    delay(1000);
    }
  } 
}


void stop_motors() {
  
  motor_speed_left = 0;
  motor_speed_right = 0;
  motorstop=true;
}

void set_motor_speed() {
  
  if (line_lost.active) {
    stop_motors(); set_led_color(C_PURPLE);
    return;
  }
  if (lidar_err.active) {
    stop_motors(); set_led_color(C_LIME);
    return;
  }
  if (lidar_crit.active || bumped.active) {
    stop_motors(); set_led_color(C_RED);
    return;
  }
  
  if (battery_low) {
    set_led_color( ((current_tick/1000L) % 2 == 0) ? C_BLUE : C_OFF); // blink blue/off every second
  } else if (battery_warning) {
    set_led_color(C_BLUE);
  } else {
    set_led_color(C_GREEN);
  }
  follow_line();
}

void follow_line()
{
  motorstop = false;

  float factor = (float)linePosition / (pixy.frameWidth / 2);

  motor_speed_left_prev  = motor_speed_left;
  motor_speed_right_prev = motor_speed_right;

  if (linePosition == 0)
  {
    motor_speed_left  = nominal_speed;
    motor_speed_right = nominal_speed;
  }

  if (linePosition > 0)//turn right
  {
    motor_speed_left  = nominal_speed;
    motor_speed_right = nominal_speed - KSTEEP * factor;
  }

  if (linePosition < 0)//turn left
  {
    motor_speed_left  = nominal_speed - KSTEEP * (-1) * factor;
    motor_speed_right = nominal_speed;
  }

  // handle acceleration limits
  int acc_left = motor_speed_left - motor_speed_left_prev;
  if (acc_left > 0 && acc_left > +acc_limit_positive) {
    motor_speed_left = motor_speed_left_prev + acc_limit_positive;
  }
  if (acc_left < 0 && acc_left < -acc_limit_negative) {
    motor_speed_left = motor_speed_left_prev - acc_limit_negative;
  }
  int acc_right = motor_speed_right - motor_speed_right_prev;
  if (acc_right > 0 && acc_right > +acc_limit_positive) {
    motor_speed_right = motor_speed_right_prev + acc_limit_positive;
  }
  if (acc_right < 0 && acc_right < -acc_limit_negative) {
    motor_speed_right = motor_speed_right_prev - acc_limit_negative;
  }

  //Serial.print("avgMotorCurrent Left: ");
  //Serial.print(measuredValLeft.avgMotorCurrent);
  //Serial.print("\t");
  //Serial.print("avgMotorCurrent Right: ");
  //Serial.print(measuredValRight.avgMotorCurrent);
  //Serial.print("\n");
  //
  //Serial.print("avgInputCurrent Left: ");
  //Serial.print(measuredValLeft.avgInputCurrent);
  //Serial.print("\t");
  //Serial.print("avgInputCurrent Right: ");
  //Serial.print(measuredValRight.avgInputCurrent);
  //Serial.print("\n");
}

bool test_spd = false;

void update_motors() {
  if (test_spd) {
    int test_speed = 600;
    motor_speed_left  = test_speed;
    motor_speed_right = test_speed;
  }
  moveMotorLeft(motor_speed_left);
  moveMotorRight(motor_speed_right);
}

void moveMotorLeft(int motor_speed) {
  if (motor_speed < ESC_MIN) motor_speed = ESC_MIN;
  if (motor_speed > ESC_MAX) motor_speed = ESC_MAX;

  VescUartSetRPM(motor_speed, &SERIAL_LEFT);
}
void moveMotorRight(int motor_speed) {
  if (motor_speed < ESC_MIN) motor_speed = ESC_MIN;
  if (motor_speed > ESC_MAX) motor_speed = ESC_MAX;
  
  VescUartSetRPM(motor_speed, &SERIAL_RIGHT);
}


void handle_battery_level() {
  // current_tick and last_tick are available. in millis
  if (current_tick - last_voltage_check_in_ms > 1000) {
    last_voltage_check_in_ms = current_tick;
    inpVoltages[voltage_index++] = (measuredValLeft.inpVoltage/2 + measuredValRight.inpVoltage/2);
    if (voltage_index >= NUM_READING_VOLTAGE) {
      voltage_index = 0;
      voltage_ready = true;
    }
    average_voltage = 0.0;
    if (voltage_ready) {
      for (int i = 0 ; i < NUM_READING_VOLTAGE; i++) {
        average_voltage += inpVoltages[i];
      }
      average_voltage /= NUM_READING_VOLTAGE;
      if (average_voltage <= VOLTAGE_WARNING_LEVEL){
        battery_warning = true;
      } else {
        battery_warning = false;  
      }
      if (average_voltage <= VOLTAGE_STOP_LEVEL) {
        battery_low = true; 
      }
    }
  }
}

void handle_duty_level() {
  
  if(motorstop==false)
  {
    if (current_tick - last_duty_check_in_ms > 1000) 
    {
      last_duty_check_in_ms = current_tick;
      inpDutyCycle[duty_index++] = (measuredValLeft.dutyNow/2 + measuredValRight.dutyNow/2);
     
      if (duty_index >= NUM_READING_DUTY) 
      {
      duty_index = 0;
      duty_ready = true;
      }
      float duty_cycle = 0.0;
      average_duty = 0.0;
     
      if (duty_ready) 
      {
        for (int i = 0 ; i < NUM_READING_DUTY; i++) 
        {
          duty_cycle = inpDutyCycle[i];
          average_duty += duty_cycle;
        }
        average_duty /= NUM_READING_DUTY;
      }
    }
  }
}

void wifi_send_data() {
  //if (current_tick - last_wifi_data_in_ms > 30*1000L) {
  if ((current_tick - last_wifi_data_in_ms) > (WIFI_REPORTING_INTERVAL_IN_SECS * 1000L)) {  
    //adresse IP au 29/07: 10.155.100.89
    String url = "http://10.155.100.89/cgi-bin/insert_magni.py?"
      + String("NAME=") + String(AGV_NAME)
      + "&VOLTAGE=" + String(average_voltage) 
      + "&TACHOMETER=" + String(measuredValLeft.tachometer) 
      + "&DUTYCYCLE=" + String(average_duty) 
      + "&CURRENT_TICK=" + String(current_tick) 
      + "&SERIAL=" + arduino_serial 
      + "&FW=" + String(VERSION) 
      + "&KM=" + String(mydataInEEPROM.km) 
      + "&M=" + String(mydataInEEPROM.meters) 
      + "&num_bumps=" + String(my_events_to_report.num_bumps) 
      + "&num_lidar_crit=" + String(my_events_to_report.num_lidar_crit) 
      + "&num_lidar_err=" + String(my_events_to_report.num_lidar_err) 
      + "&num_line_lost=" + String(my_events_to_report.num_line_lost) 
      ; 
    //Serial.print(url); Serial.print("\n");
    last_wifi_data_in_ms = current_tick;
    SERIAL_WIFI.print(url);
    SERIAL_WIFI.flush();
    // reset event counts (we report them once only
    my_events_to_report.num_bumps      = 0;
    my_events_to_report.num_lidar_crit = 0;
    my_events_to_report.num_lidar_err  = 0;
    my_events_to_report.num_line_lost  = 0;
    
  }
}


void loop() {
  current_tick = millis();
  
  update_measured_values();
  
  handle_kmeters();
  
  handle_battery_level();
  
  bumper_detection();
  
  obstacle_detection();
  
  lecture_pixy_front();
  
  set_motor_speed();
  
  update_motors();
  
  handle_duty_level();
  
  wifi_send_data();
  
  int32_t duration = current_tick - last_tick;

  //Serial.print("millis:\t");
  //Serial.print("bat:");Serial.print(handle_battery_level_ms - current_tick); Serial.print("\t");
  //Serial.print("bmp:");Serial.print(handle_battery_level_ms - bumper_detection_ms); Serial.print("\t");
  //Serial.print("obs:");Serial.print(bumper_detection_ms - obstacle_detection_ms); Serial.print("\t");
  //Serial.print("pix:");Serial.print(obstacle_detection_ms - lecture_pixy_front_ms); Serial.print("\t");
  //Serial.print("mtr:");Serial.print(lecture_pixy_front_ms - update_motors_ms); Serial.print("\t");
  
  if (num_loop % (unsigned long)(OBSERVED_FPS) == 0) {
    Serial.print("d:");
    Serial.print(duration );
    Serial.print("\t");
    //float hz = 1000.0 / ((float)(duration));
    //Serial.print(hz);
    Serial.print("m:");
    Serial.print(current_meters);
    Serial.print("\n");
  }
  last_tick = current_tick;
  num_loop++;

}
