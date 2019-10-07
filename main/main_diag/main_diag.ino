#include <stdint.h>
#include <VescUart.h>
#include <datatypes.h>

#define VERSION "0.0.1.0" // diagnostic tool with ramped velocity

String arduino_serial = "";

// To use VescUartControl stand alone you need to define a config.h file, that should contain the Serial or you have to comment the line
// #include Config.h out in VescUart.h
// This lib version tested with vesc fw 3.38 and 3.40 on teensy 3.2 and arduino uno

//Serial ports
#define SERIAL_RIGHT Serial1
#define SERIAL_LEFT Serial2



#define ESC_MIN -5000
#define ESC_STOP 150 
#define ESC_MAX 5000
#define NOMINAL_SPEED 3000
#define NOMINAL_SPEED_WARNING 1500

#define OBSERVED_FPS 60.0f // how much fps i have measure in my console given everything i do
const uint32_t target_loop_duration_in_us = (uint32_t)(1000000/OBSERVED_FPS);

// led
// PROD
#define PIN_LED_RED 6
#define PIN_LED_BLUE 5
// // OLD DEV BOARD
//#define PIN_LED_RED 5
//#define PIN_LED_BLUE 6

#define PIN_LED_GREEN 7


uint32_t last_tick = 0;
uint32_t current_tick = 0;


float acc_limit_positive = (NOMINAL_SPEED - ESC_STOP) / 4.00 / OBSERVED_FPS; // from stop to top speed in 4.0sec  => how much incremental speed we allow per frame
float acc_limit_negative = (NOMINAL_SPEED - ESC_STOP) / 0.50 / OBSERVED_FPS; // in 0.5sec => when we brake, we want to brake faster


int16_t motor_speed_left = 0;
int16_t motor_speed_right = 0;
int16_t motor_speed_left_prev = 0;
int16_t motor_speed_right_prev = 0;

int16_t nominal_speed = ESC_STOP;
int16_t target_nominal_speed = NOMINAL_SPEED;


#define NUM_COLORS 8
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

int16_t rgb[NUM_COLORS][3] = {
  { 0, 0, 0 },   // C_OFF 
  { 1, 0, 0 },   // C_RED
  { 0, 1, 0 },   // C_GREEN
  { 0, 0, 1 },   // C_BLUE
  { 1, 0, 1 },   // C_PURPLE
  { 1, 1, 0 },   // C_LIME
  { 0, 1, 1 },   // C_CYAN
  { 1, 1, 1 }    // C_WHITE
};

#define GO_FORWARD   0
#define GO_BACKWARDS 1
int cruise = GO_FORWARD;

#define DELAY_BETWEEN_TRANSITIONS_IN_MS 5000

void setup() {
  Serial.begin(115200);
  

  Serial.print("Starting Diag v");
  Serial.print(VERSION);
  Serial.print("\n");

  

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  
  setup_motors();
  stop_motors(); update_motors();
  
  set_led_color(C_RED);
  


  

  current_tick = millis();
  last_tick = millis();
  delay(1000.0/OBSERVED_FPS);
  
  
  nominal_speed = 10; 
  target_nominal_speed = NOMINAL_SPEED; // resume normal speed
  cruise = GO_FORWARD;

  Serial.print("init done\n");
}


void set_led_color(int c) {
  digitalWrite(PIN_LED_RED   , rgb[c][_R]);
  digitalWrite(PIN_LED_GREEN,  rgb[c][_G]);
  digitalWrite(PIN_LED_BLUE  , rgb[c][_B]);
}


void setup_motors() {
  //Setup UART port
  SERIAL_RIGHT.begin(115200);
  SetSerialPort(&SERIAL_RIGHT);
  SERIAL_LEFT.begin(115200);
  SetSerialPort(&SERIAL_LEFT);
}



void stop_motors() {
  nominal_speed = ESC_STOP;
  target_nominal_speed = ESC_STOP;
  motor_speed_left = 0;
  motor_speed_right = 0;
}

void set_motor_speed() {
  if (cruise == GO_FORWARD) {
    // handle acceleration
    if (nominal_speed < target_nominal_speed) {
      nominal_speed += acc_limit_positive;
      if (nominal_speed > target_nominal_speed) { nominal_speed = target_nominal_speed; }
    }
    // handle deceleration
    if (nominal_speed > target_nominal_speed) {
      nominal_speed -= acc_limit_negative;
      if (nominal_speed < target_nominal_speed) { nominal_speed = target_nominal_speed; }
    }

    
  } else {
    // BACKWARDS
    // handle acceleration
    if (nominal_speed > target_nominal_speed) {
      nominal_speed -= acc_limit_positive;
      if (nominal_speed < target_nominal_speed) { nominal_speed = target_nominal_speed; }
    }
    // handle deceleration
    if (nominal_speed < target_nominal_speed) {
      nominal_speed += acc_limit_negative;
      if (nominal_speed > target_nominal_speed) { nominal_speed = target_nominal_speed; }
    }

  }
    
  motor_speed_left = nominal_speed;
  motor_speed_right = nominal_speed;
  Serial.print("t:");
  Serial.print(target_nominal_speed);
  Serial.print("\t");
  Serial.print("s:");
  Serial.print(nominal_speed);
  Serial.print("\t");
  
}


void update_motors() {
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


void handle_transitions() {
  if (cruise == GO_FORWARD) {
    // if we reach max speed => brake
    if (nominal_speed == target_nominal_speed && target_nominal_speed == NOMINAL_SPEED) {
      target_nominal_speed = 0;
    }
    // if we are stopped => go backward
    if (nominal_speed == 0 && target_nominal_speed == 0) {
      Serial.print("going backwards\n");
      target_nominal_speed = -NOMINAL_SPEED;
      cruise = GO_BACKWARDS;
      delay(DELAY_BETWEEN_TRANSITIONS_IN_MS);
    }
    
  } else {
    // BACKWARDS
    // if we reached max negative speed => brake
    if (nominal_speed == target_nominal_speed && target_nominal_speed == -NOMINAL_SPEED) {
      target_nominal_speed = 0;
    }
    // if we reached zero speed => go forward again
    if (nominal_speed == 0 && target_nominal_speed == 0) {
      Serial.print("going forward\n");
      target_nominal_speed = +NOMINAL_SPEED;
      cruise = GO_FORWARD;
      delay(DELAY_BETWEEN_TRANSITIONS_IN_MS);
    }
  }
}


void loop() {
  current_tick = millis();
  
  set_motor_speed();
  //motor_speed_left = 0;
  //motor_speed_right = 0;
  
  update_motors();
  
  handle_transitions();
  
  uint32_t duration = current_tick - last_tick;
  Serial.print("d:");
  Serial.print(duration);
  Serial.print("\n");
  // loop duration without delay is 1~2 ms.
  // in order to reach 16.66ms, let's delay 15
  delay(15);
  
  
  

  last_tick = current_tick;
}
