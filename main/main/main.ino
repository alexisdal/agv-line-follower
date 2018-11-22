
#include <Pixy2.h>
Pixy2 pixy;

#include <Servo.h>
Servo myservo_left;
Servo myservo_right;
#define pin_motor_pwm_left 3
#define pin_motor_pwm_right 10

#define ESC_MIN 1000
#define ESC_MAX 2000
#define ESC_STOP 1450
#define NOMINALSPEED 1750
//#define KSTEEP 1.2 //0.8
const float KSTEEP = (NOMINALSPEED - ESC_STOP);

int motor_speed_left = ESC_STOP;
int motor_speed_right = ESC_STOP;

#define X_CENTER  (pixy.frameWidth/2) // position de la ligne sur le capteur

int linePosition = 0;
unsigned long nbError = 9999;


long last_tick = 0;
long current_tick = 0;
long duration = 0;

void setup()
{
  last_tick = millis();
  Serial.begin(115200);
  Serial.print("Starting...\n");

  setup_motors();
  stop_motors();
  update_motors();
  delay(3000); // stay stopped for a while (safe start)

  // we need to initialize the pixy object
  pixy.init();
  // Change to line tracking program
  pixy.changeProg("line");

  current_tick = millis();

}

void setup_motors() {
  pinMode(pin_motor_pwm_left, OUTPUT);
  myservo_left.attach(pin_motor_pwm_left);
  pinMode(pin_motor_pwm_right, OUTPUT);
  myservo_right.attach(pin_motor_pwm_right);
}

void loop()
{
  current_tick = millis();
  lecturePixyFront();
  update_motors();
  //long duration = current_tick - last_tick;
  //Serial.print(duration );
  //Serial.print("\t");
  //float hz = 1000.0 / ((float)(duration));
  //Serial.print(hz);
  Serial.print("\n");
  last_tick = current_tick;

}

void lecturePixyFront()
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
    Serial.print("KO\t");
    Serial.print(nbError);
    Serial.print("\t");
    if (nbError > 60) // given that loops runs at 30Hz => let's give it 2 seconds
    {
      stop_motors();
    }
  } else
  {
    Serial.print("ok\t");
    if (res & LINE_VECTOR) {
      Serial.print("ok\t");

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
    }

    suiviLigne();
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
    motor_speed_left  = NOMINALSPEED;
    motor_speed_right = NOMINALSPEED;
  }

  if (linePosition > 0)//turn right
  {
    motor_speed_left  = NOMINALSPEED;
    motor_speed_right = NOMINALSPEED - KSTEEP * factor;
  }

  if (linePosition < 0)//turn left
  {
    motor_speed_left  = NOMINALSPEED - KSTEEP * (-1) * factor;
    motor_speed_right = NOMINALSPEED;
  }

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
