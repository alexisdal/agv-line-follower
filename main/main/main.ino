
#include <Pixy2.h>
Pixy2 pixy;

#include <Servo.h>
Servo myservo_left;
Servo myservo_right;
#define pin_motor_pwm_left 3
#define pin_motor_pwm_right 10

#define ESC_MIN 1000
#define ESC_MAX 2000
#define ESC_STOP 1450 // ATTENTION: au debug on s'apercoit qu'en fait la roue s'arrete de tourner dès 1634 -- MEME A VIDE --
#define NOMINAL_SPEED 1750
//#define NOMINAL_SPEED 1850 // trop vite
#define NOMINAL_SPEED_WARNING 1700 

//bool crit_stop = false;

//#define KSTEEP 1.2 //0.8
float KSTEEP = (NOMINAL_SPEED - ESC_STOP);

int motor_speed_left = ESC_STOP;
int motor_speed_right = ESC_STOP;

int nominal_speed = NOMINAL_SPEED;
//const int nominal_speed_warning = NOMINAL_SPEED - ((NOMINAL_SPEED + ESC_STOP) / 2);
//const int nominal_speed_warning = (NOMINAL_SPEED*3 + ESC_STOP*2) / 5;

#define X_CENTER  (pixy.frameWidth/2) // position de la ligne sur le capteur

int linePosition = 0;
unsigned long nbError = 9999;


long last_tick = 0;
long current_tick = 0;
long duration = 0;

//Lidar data
#define PIN_LIDAR_DATA_0   6 //receive data from lidar arduino
#define PIN_LIDAR_DATA_1   7 //receive data from lidar arduino
#define PIN_LIDAR_BUMP     4 //receive default bumper signal

#define COMM_ALL_OK   0
#define COMM_WARN     1
#define COMM_CRIT     2
#define COMM_ERR      3



void setup()
{
  last_tick = millis();
  Serial.begin(115200);
  Serial.print("Starting...\n");

  pinMode(PIN_LIDAR_DATA_0, INPUT);
  pinMode(PIN_LIDAR_DATA_1, INPUT);
  pinMode(PIN_LIDAR_BUMP, INPUT_PULLUP);
    
  setup_motors();
  stop_motors();
  update_motors();
  delay(3000); // stay stopped for a while (safe start)

  // we need to initialize the pixy object
  pixy.init();
  pixy.setLED(255, 255, 255); // white
  // Change to line tracking program
  pixy.changeProg("line");

  current_tick = millis();
  delay(1000);
  pixy.setLED(0, 0, 0); // off
  pixy.setLamp(1, 1); // Turn on both lamps, upper and lower for maximum exposure


}

void setup_motors() 
{
  pinMode(pin_motor_pwm_left, OUTPUT);
  myservo_left.attach(pin_motor_pwm_left);
  pinMode(pin_motor_pwm_right, OUTPUT);
  myservo_right.attach(pin_motor_pwm_right);
}


void loop()
{
  current_tick = millis();
  //bumper_detection();
  //obstacle_detection();
  lecture_pixy_front();
  update_motors();
  //long duration = current_tick - last_tick;
  //Serial.print(duration );
  //Serial.print("\t");
  //float hz = 1000.0 / ((float)(duration));
  //Serial.print(hz);
  Serial.print("\n");
  last_tick = current_tick;

}

void bumper_detection(){
 if (digitalRead(PIN_LIDAR_BUMP) == 1)
 {
    stop_motors(); update_motors(); delay(2000);
    while(digitalRead(PIN_LIDAR_BUMP) == 1) {
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
  int state = get_lidar_state();
  if ((state == COMM_CRIT) || (state == COMM_ERR)) {
    stop_motors(); update_motors(); delay(4000); // crit_stop = true; //delay(10000);
  } else if (state == COMM_WARN) {
    //if (crit_stop) { delay(5000); crit_stop = false; }
    nominal_speed = NOMINAL_SPEED_WARNING;
    KSTEEP = (NOMINAL_SPEED_WARNING - ESC_STOP);    
  } else {
    //if (crit_stop) { delay(5000); crit_stop = false; }
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
  res = pixy.line.getMainFeatures();

  // If error or nothing detected, stop motors
  if (res <= 0)
  {
    nbError += 1;
    //Serial.print("KO\t");
    //Serial.print(nbError);
    Serial.print("\t");
    if (nbError > 60) // given that loops runs at 30Hz => let's give it 2 seconds (was 60. now 120)
    {
      stop_motors();
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
      // do nothing for now
      if (pixy.line.barcodes->m_code == 0) { // barcode 0 is full stop
        Serial.print("\n\n *** FULL STOP *** \n\n");
        brake_and_stop_motors();
      } else if (pixy.line.barcodes->m_code == 1) { // barcode 1 is u-turn left
        Serial.print("\n\n *** U-TURN LEFT *** \n\n");
        u_turn(true);
      } else if (pixy.line.barcodes->m_code == 2) { // barcode 2 is u-turn right
        Serial.print("\n\n *** U-TURN RIGHT *** \n\n");
        u_turn(false);
      }
    }
    suiviLigne();
  }
  
}

void u_turn(bool left) {
  pixy.setLamp(0, 0); // Turn off both lamps
  pixy.setLED(0, 0, 255); // blue
  stop_motors(); update_motors(); delay(500);
  pixy.setLED(0, 255, 0); // green
  if (left) {
    motor_speed_left  = ESC_STOP - KSTEEP * 0.7;
    motor_speed_right = ESC_STOP + KSTEEP * 0.7;
  } else {
    motor_speed_left  = ESC_STOP + KSTEEP * 0.7;
    motor_speed_right = ESC_STOP - KSTEEP * 0.7;
  }
  update_motors();
  delay(2250);
  pixy.setLED(0, 0, 0); // off
  pixy.setLamp(1, 1);
  linePosition  = (left ? -1 : +1) * (pixy.frameWidth / 2);
}


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


void stop_motors() {
  motor_speed_left = ESC_STOP;
  motor_speed_right = ESC_STOP;
  Serial.print("\n\n **** STOP MOTORS **** \n\n");
}



void suiviLigne()
{
  float factor = (float)linePosition / (pixy.frameWidth / 2);

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

  //Serial.print(motor_speed_left);
  //Serial.print("\t");
  //Serial.print(motor_speed_right);
  //Serial.print("\n");

}



void update_motors() {
  moveMotorLeft(motor_speed_left);
  moveMotorRight(motor_speed_right);
}

void moveMotorLeft(int motor_speed) {
  if (motor_speed < ESC_MIN) motor_speed = ESC_MIN;
  if (motor_speed > ESC_MAX) motor_speed = ESC_MAX;
  myservo_left.writeMicroseconds(motor_speed);
  Serial.print(motor_speed);
  Serial.print("\t");
}
void moveMotorRight(int motor_speed) {
  if (motor_speed < ESC_MIN) motor_speed = ESC_MIN;
  if (motor_speed > ESC_MAX) motor_speed = ESC_MAX;
  myservo_right.writeMicroseconds(motor_speed);
  Serial.print(motor_speed);
  Serial.print("\t");
}
