
// the setup function runs once when you press reset or power the board
// To use VescUartControl stand alone you need to define a config.h file, that should contain the Serial or you have to comment the line
// #include Config.h out in VescUart.h
// This lib version tested with vesc fw 3.38 and 3.40 on teensy 3.2 and arduino uno

//Include libraries copied from VESC
#include <VescUart.h>
#include <datatypes.h>

#include <Pixy2.h>
Pixy2 pixy;

//#define DEBUG
// in firmware 6.4 a  nd wheels in the middle, we revert back to 6.2 configuration
#define SERIAL_RIGHT Serial1
#define SERIAL_LEFT Serial2
#define SERIAL_WIFI Serial3
#define DEBUGSERIAL Serial // usb serial

struct bldcMeasure measuredVal; // to read battery voltage

bool voltage_ready = false;
bool duty_ready = false;
bool battery_warning = false;

#define NUM_READING_VOLTAGE 10
#define NUM_READING_DUTY 10
float inpVoltages[NUM_READING_VOLTAGE];
float inpDutyCycle[NUM_READING_DUTY];
int voltage_index = 0;
int duty_index = 0;
unsigned long last_voltage_check_in_ms = 0;
unsigned long last_wifi_data_in_ms = 0;
unsigned long last_duty_check_in_ms = 0;
// with voltmeter I measure 39.89v with a full battery.
//                          36.60v with nearly dead battery
// with VESC averaging 100 values i read 37.02906504
// let's use 37v for now
#define VOLTAGE_WARNING_LEVEL 22 // 2x12v lead batteries

#define FOUR_INCHES_WHEEL_GEARED
//#define OVERBOARD_WHEEL

#ifdef OVERBOARD_WHEEL
#define ESC_MIN -2500
#define ESC_STOP 150 // ATTENTION: au debug on s'apercoit qu'en fait la roue s'arrete de tourner dès 1634 -- MEME A VIDE --
#define ESC_MAX 2500
#define NOMINAL_SPEED 900
#define NOMINAL_SPEED_WARNING 550
#endif

#ifdef FOUR_INCHES_WHEEL_GEARED
#define ESC_MIN -5000
#define ESC_STOP 150 // ATTENTION: au debug on s'apercoit qu'en fait la roue s'arrete de tourner dès 1634 -- MEME A VIDE --
#define ESC_MAX 5000
#define NOMINAL_SPEED 3000
#define NOMINAL_SPEED_WARNING 1500
#endif



#define OBSERVED_FPS 60.0 // how much fps i have measure in my console given everything i do
float acc_limit_positive = (NOMINAL_SPEED - ESC_STOP) / 0.25 / OBSERVED_FPS; // from stop to top speed on 0.50sec  => how much incremental speed we allow per frame
float acc_limit_negative = (NOMINAL_SPEED - ESC_STOP) / 0.15 / OBSERVED_FPS; // in 0.30sec => when we brake, we want to brake faster
#define MAX_LINE_MISS 90 // at 60fps 

//#define KSTEEP 1.2 //0.8
float KSTEEP = (NOMINAL_SPEED - ESC_STOP) * 1.025;

//int motor_speed_left = ESC_STOP;
//int motor_speed_right = ESC_STOP;
int motor_speed_left = 0;
int motor_speed_right = 0;
int motor_speed_left_prev = 0;
int motor_speed_right_prev = 0;

int nominal_speed = NOMINAL_SPEED;

#define X_CENTER  (pixy.frameWidth/2) // position de la ligne sur le capteur

bool motorstop=false;
int linePosition = 0;
unsigned long nbError = 9999;

long last_tick = 0;
long current_tick = 0;
long duration = 0;

float average_voltage = 0.0;
float average_duty = 0.0;
//Lidar data
#define PIN_LIDAR_DATA_0   10 //receive data from lidar arduino
#define PIN_LIDAR_DATA_1   11 //receive data from lidar arduino
#define PIN_LIDAR_BUMP     4 //receive default bumper signal

// Bandeau led
#define PIN_LED_RED 6//5
#define PIN_LED_BLUE 5//6
#define PIN_LED_GREEN 7

// MP3 Grove
#include <MP3Player_KT403A.h>
#include <SoftwareSerial.h>
SoftwareSerial mp3(2, 3);

#define COMM_ALL_OK   0
#define COMM_WARN     1
#define COMM_CRIT     2
#define COMM_ERR      3
int lidar_state = COMM_ALL_OK;

// metro stop
unsigned long metro_stop_counter = 0;
#define STOP_EVERY_X_BARCODE  3
#define METRO_STATION_STOP_DURATION_IN_SECONDS 10
#define LIDAR_CRIT_ERR_STOP_DURATION_IN_SECONDS 5

#define PIN_LED_TURN_LEFT A14  // green
#define PIN_LED_TURN_RIGHT A15 // red
unsigned long last_barcode_read_tick = 0;


enum color{C_OFF, C_RED, C_GREEN, C_BLUE, C_PURPLE, C_LIME, C_CYAN, C_WHITE}; 
#define NUM_COLORS 8
enum c{_R, _G, _B}; 
int rgb[NUM_COLORS][3] = {
  { 0, 0, 0 },   // OFF (black?)
  { 1, 0, 0 },   // RED
  { 0, 1, 0 },   // GREEN
  { 0, 0, 1 },   // BLUE
  { 1, 0, 1 },   // PURPLE
  { 1, 1, 0 },   // LIME
  { 0, 1, 1 },   // CYAN
  { 1, 1, 1 }    // WHITE
};

void setup() {
  DEBUGSERIAL.begin(115200);
#ifdef DEBUG
  //SEtup debug port
  SetDebugSerialPort(&DEBUGSERIAL);
#endif

  // https://github.com/Seeed-Studio/Grove_Serial_MP3_Player_V2.0
  mp3.begin(9600);
  PlayLoop();

  SERIAL_WIFI.begin(115200);
  
  last_tick = millis();

  Serial.print("Starting v0.8.6 WiFi: WL:22 \n");

  pinMode(PIN_LIDAR_DATA_0, INPUT);
  pinMode(PIN_LIDAR_DATA_1, INPUT);
  pinMode(PIN_LIDAR_BUMP, INPUT_PULLUP);

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  
  setup_motors();
  stop_motors(); update_motors();
  set_led_color(C_GREEN);

  // stay stopped for a while (safe start 3sec)
  int t = 1000 / NUM_COLORS * 3;
  for (int i = 0 ; i < 1 ; i++) {
    for (int j = 0 ; j < NUM_COLORS ; j++) {
      set_led_color(j);    delay(t);
    }
  }
  set_led_color(C_GREEN);
  delay(1000);


  // we need to initialize the pixy object
  pixy.init();
  pixy.setLED(255, 255, 255); // white
  // Change to line tracking program
  pixy.changeProg("line");

  // initialize the voltage checks
  for (int i = 0 ; i < NUM_READING_VOLTAGE ; i++) {
    inpVoltages[i] = -1.0;
  }


  current_tick = millis();
  delay(60);
  pixy.setLED(255, 255, 255); // off
  pixy.setLamp(1, 1); // Turn on both lamps, upper and lower for maximum exposure
  //Serial.print("NOMINAL_SPEED:"); Serial.print(NOMINAL_SPEED); Serial.print("\n");
  //Serial.print("acc_limit_positive:"); Serial.print(acc_limit_positive); Serial.print("\n");
  //Serial.print("acc_limit_positive:"); Serial.print(acc_limit_negative); Serial.print("\n");
  //while(1) { ;}

  Serial.print("init done\n");
}

void set_led_color(color c) {
  digitalWrite(PIN_LED_RED   , rgb[c][_R]);
  digitalWrite(PIN_LED_GREEN,  rgb[c][_G]);
  digitalWrite(PIN_LED_BLUE  , rgb[c][_B]);
}

void loop() {
  current_tick = millis();
  handle_battery_level();
  
  //long handle_battery_level_ms = millis();
  bumper_detection();
  //long bumper_detection_ms = millis();
  obstacle_detection();
  //long obstacle_detection_ms = millis();
  lecture_pixy_front();
  //long lecture_pixy_front_ms = millis();
  update_motors();
  handle_duty_level();
  //long update_motors_ms = millis();
  wifi_send_data();
  long duration = current_tick - last_tick;

  //Serial.print("millis:\t");
  //Serial.print("bat:");Serial.print(handle_battery_level_ms - current_tick); Serial.print("\t");
  //Serial.print("bmp:");Serial.print(handle_battery_level_ms - bumper_detection_ms); Serial.print("\t");
  //Serial.print("obs:");Serial.print(bumper_detection_ms - obstacle_detection_ms); Serial.print("\t");
  //Serial.print("pix:");Serial.print(obstacle_detection_ms - lecture_pixy_front_ms); Serial.print("\t");
  //Serial.print("mtr:");Serial.print(lecture_pixy_front_ms - update_motors_ms); Serial.print("\t");

  //Serial.print(duration );
  //Serial.print("\t");
  //float hz = 1000.0 / ((float)(duration));
  //Serial.print(hz);
  //Serial.print("\n");

  last_tick = current_tick;

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
  if (digitalRead(PIN_LIDAR_BUMP) == 1)
  {
    stop_motors(); update_motors(); 
    set_led_color(C_RED);
    delay(2000);
    while (digitalRead(PIN_LIDAR_BUMP) == 1) {
      delay(4000);
    }
  }
}


int get_lidar_state()
{
  int d0 = digitalRead(PIN_LIDAR_DATA_0);
  int d1 = digitalRead(PIN_LIDAR_DATA_1);

  //         D0   D1
  // ALL OK   0    0
  // WARN     0    1
  // CRIT     1    0
  // ERR      1    1

  int state = 0;

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
    stop_motors(); update_motors(); 
    set_led_color(C_LIME);
    delay(LIDAR_CRIT_ERR_STOP_DURATION_IN_SECONDS*1000);
  } else if (lidar_state == COMM_CRIT) {
    stop_motors(); update_motors(); 
    set_led_color(C_RED);
    delay(LIDAR_CRIT_ERR_STOP_DURATION_IN_SECONDS*1000);
  } else if (lidar_state == COMM_WARN) {
    nominal_speed = NOMINAL_SPEED_WARNING;
    KSTEEP = (NOMINAL_SPEED_WARNING - ESC_STOP);
  } else {
    nominal_speed = NOMINAL_SPEED;
    KSTEEP = (NOMINAL_SPEED - ESC_STOP);
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
    if (nbError > MAX_LINE_MISS) // given that loops runs at 30Hz => let's give it 2 seconds (was 60. now 120)
    {
      stop_motors(); update_motors();
      set_led_color(C_PURPLE);
    } else {
      suiviLigne();  
    }
  } else {
    //Serial.print("ok\t");
    if (res & LINE_VECTOR) {
      //Serial.print("ok\t");

      nbError = 0;
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
        Serial.print("*** METRO STATION *** \n");
        handle_metro_stop();
      }
  }

    suiviLigne();
  }

  if (current_tick - last_barcode_read_tick > 1000) { // drop led for 1sec
    digitalWrite(PIN_LED_TURN_LEFT,  LOW);
    digitalWrite(PIN_LED_TURN_RIGHT, LOW);
  } else {
    digitalWrite(PIN_LED_TURN_LEFT,  HIGH);
    digitalWrite(PIN_LED_TURN_RIGHT, HIGH);
  }

}

void handle_metro_stop() {
  if (current_tick - last_barcode_read_tick > 1000) {
    Serial.print("*** BARCODE ACCEPTED *** \n");
    if (metro_stop_counter % STOP_EVERY_X_BARCODE == 0) {
      stop_motors(); update_motors();
      set_led_color(C_CYAN);
      Serial.print("*** METRO STATION STOP *** \n");
      digitalWrite(PIN_LED_TURN_LEFT,  HIGH);
      digitalWrite(PIN_LED_TURN_RIGHT, HIGH);
      delay(1000 * METRO_STATION_STOP_DURATION_IN_SECONDS);
      digitalWrite(PIN_LED_TURN_LEFT,  LOW);
      digitalWrite(PIN_LED_TURN_RIGHT, LOW);
    }
    Serial.print(metro_stop_counter);
    Serial.print("\t");
    Serial.print(STOP_EVERY_X_BARCODE);
    Serial.print("\t");
    Serial.print(metro_stop_counter % STOP_EVERY_X_BARCODE);
    Serial.print("\n");
    metro_stop_counter++;
  }
  last_barcode_read_tick = millis();
}

/*
void brake_and_stop_motors() {
  pixy.setLamp(0, 0); // Turn off both lamps
  motor_speed_left  = nominal_speed - KSTEEP * 0.5;
  motor_speed_right = nominal_speed - KSTEEP * 0.5;
  update_motors(); delay(250);
  stop_motors(); update_motors(); delay(1000);
  while (true) {
    pixy.setLED(255, 0, 0); // red
    delay(500);
    pixy.setLED(0, 0, 0); // off
    delay(500);
  }
}
*/

void stop_motors() {
  //PlayPause();
  motor_speed_left = 0;
  motor_speed_right = 0;
  motorstop=true;
}



void suiviLigne()
{
  motorstop=false;
  PlayResume();
  if (battery_warning) {
    set_led_color(C_BLUE);
  } else {
    set_led_color(C_GREEN);
  }
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



  //Serial.print(motor_speed_left);
  //Serial.print("\t");
  //Serial.print(motor_speed_right);
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
  //myservo_left.writeMicroseconds(motor_speed);
  VescUartSetRPM(motor_speed, &SERIAL_LEFT);
  //Serial.print(motor_speed);
  //Serial.print("\t");
}
void moveMotorRight(int motor_speed) {
  if (motor_speed < ESC_MIN) motor_speed = ESC_MIN;
  if (motor_speed > ESC_MAX) motor_speed = ESC_MAX;
  //myservo_right.writeMicroseconds(motor_speed);
  VescUartSetRPM(motor_speed, &SERIAL_RIGHT);
  //Serial.print(motor_speed);
  //Serial.print("\t");
}


void handle_battery_level() {
  // current_tick and last_tick are available. in millis
  if (current_tick - last_voltage_check_in_ms > 1000) {
    last_voltage_check_in_ms = current_tick;
    inpVoltages[voltage_index++] = get_battery_voltage();
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
    }
    if (voltage_ready){
      if (average_voltage <= VOLTAGE_WARNING_LEVEL) {
      battery_warning = true;
      }else {battery_warning = false;}
    }
    
  }
}

void handle_duty_level() {
  
  if(motorstop==false)
  {
    if (current_tick - last_duty_check_in_ms > 1000) {
      last_duty_check_in_ms = current_tick;
      inpDutyCycle[duty_index++] = measuredVal.dutyNow;
      if (duty_index >= NUM_READING_DUTY) {
      duty_index = 0;
      duty_ready = true;
      }
      float duty_cycle = 0.0;
      average_duty = 0.0;
      if (duty_ready) {
        for (int i = 0 ; i < NUM_READING_DUTY; i++) {
          duty_cycle = inpDutyCycle[i];
          average_duty += duty_cycle;
        }
        average_duty /= NUM_READING_DUTY;
      }
    }
  }
}

float get_battery_voltage() {
  VescUartGetValue(measuredVal, &SERIAL_LEFT);
  return measuredVal.inpVoltage;
}


void wifi_send_data() {
  if (current_tick - last_wifi_data_in_ms > 30*1000) {
    // http://192.168.1.20/cgi-bin/insert_magni.py?NAME=A&VOLTAGE=1&TACHOMETER=1&DUTYCYCLE=1&CURRENT_TICK=1&LAST_BARCODE_READ_TICK=1&COUNT_BARCODE1=0
    //String url = "http://192.168.1.20/cgi-bin/insert_magni.py?NAME=MagniLab&VOLTAGE=" + String(average_voltage) + "&TACHOMETER=" + String(measuredVal.tachometer) + "&DUTYCYCLE=" + String(average_duty) + "&CURRENT_TICK=" + String(current_tick) + "&LAST_BARCODE_READ_TICK=" + String(last_barcode_read_tick) + "&COUNT_BARCODE1=0";
    String url = "http://10.155.100.85/cgi-bin/insert_magni.py?NAME=MagniLab&VOLTAGE=" + String(average_voltage) + "&TACHOMETER=" + String(measuredVal.tachometer) + "&DUTYCYCLE=" + String(average_duty) + "&CURRENT_TICK=" + String(current_tick) + "&LAST_BARCODE_READ_TICK=" + String(last_barcode_read_tick) + "&COUNT_BARCODE1=0";
  last_wifi_data_in_ms = current_tick;
    SERIAL_WIFI.print(url);
    SERIAL_WIFI.flush();
    //Serial.print("Wifi send Data");
  }
}