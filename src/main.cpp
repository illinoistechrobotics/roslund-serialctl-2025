  #include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include "globals.h"
#include <SPI.h>
#include "MCP3XXX.h"
#include "PCF8574.h"
#include "pio_encoder.h"
// #include "dumbdisplay.h"
// #include "wifidumbdisplay.h"
#include "zserio.h"
#include "SerialUART.h"
#include <107-Arduino-Servo-RP2040.h>
#include <SerialBT.h>

const char* ssid = "Mini-Turkey";
const char* password = "pink4bubble";

packet_t pA, pB, safe;
packet_t *astate, *incoming;
comm_state cs;

PioEncoder enc1(4); // L
PioEncoder enc2(12);// R
PioEncoder enc3(0); // 3
PioEncoder enc4(8); // 4

static _107_::Servo talon1;
static _107_::Servo talon2;
static _107_::Servo talon3;
static _107_::Servo talon4;
static _107_::Servo talon5;

int count = 0;

char left_enabled = 0, right_enabled = 0;
int current_offset[2];
int setup_complete = false;

// driving vars
int target_fleft_power = 0; // This is update every tick from the joystick
int target_fright_power = 0; 
int fleft_power = 0;     // actual speed of the motor currently
int fright_power = 0;    // actual speed of the motor currently
int target_bleft_power = 0; // This is update every tick from the joystick
int target_bright_power = 0; 
int bleft_power = 0;     // actual speed of the motor currently
int bright_power = 0;    // actual speed of the motor currently
int acceleration = 6;   // acceleration is how much left_power and right_power change per tick to reach target_power
bool turbo = false;     // turbo is a boolean, ( is true when the top right shoulder button *on some controllers* is pressed ) 
int cooldown = 0;  // joystick cooldown
int olddisplay = 99999;
int mode_toggle = 0;

#define TALON1_PIN 4
#define TALON2_PIN 3
#define TALON3_PIN 2
#define TALON4_PIN 1
#define TALON5_PIN 14

// drive modes
#define ARCADE_DRIVE 0
#define TANK_DRIVE 1
#define MECANUM_DRIVE 2

int mode = ARCADE_DRIVE; // start in arcade mode

#define MAX_REVERSE 1000 // RC calibration
#define MAX_FORWARD 2000
#define NEUTRAL 1500

// Figure out what this code does below 
unsigned int getButton(unsigned int num) { // extract button data from serialctl
  if (num <= 7) {
    return (astate->btnlo >> num) & 0x01;
  } else if (num > 7 && num <= 15) {
    return (astate->btnhi >> (num - 8)) & 0x01;
  } else {
    return 0;
  }
}
unsigned int getDPad() { // extract dpad data from serialctl
  // four bits: left down right up
  return (astate->btnhi >> 4);
}

void setup() {
  WiFi.noLowPowerMode(); // disable wifi chip (includes bluetooth) power saving
  //rp2040.enableDoubleResetBootloader();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  if(rp2040.isPicoW()) { // disable power save mode
    pinMode(33, OUTPUT);
    pinMode(33, HIGH);
  } else {
    //wifi on the pico w 
    pinMode(23, OUTPUT);
    pinMode(23, HIGH);
  }
  Serial.begin(115200);
  //Serial.println("hello!");
  delay(3000); // wait for serial debug connection if present
  Serial.println("Initializing RIB subsystems..");
  // normally display and chip setup here
  Serial.println(" done");
  delay(20);
  //delay(2000);

  Serial.print("Initializing motors..");
  talon1.attach(TALON1_PIN);
  talon2.attach(TALON2_PIN);
  talon3.attach(TALON3_PIN);
  talon4.attach(TALON4_PIN);
  talon5.attach(TALON5_PIN);
  talon1.writeMicroseconds(NEUTRAL);
  talon2.writeMicroseconds(NEUTRAL);
  talon3.writeMicroseconds(NEUTRAL);
  talon4.writeMicroseconds(NEUTRAL);
  talon5.writeMicroseconds(NEUTRAL);

  Serial.println(" done");
  Serial.print("Initializing encoders..");
  enc1.begin();
  enc2.begin();
  //enc3.begin();
  //enc4.begin();
  Serial.println(" done");
  delay(20);

  Serial.print("Initializing xBee radio..");
  // SerComm.setRX(21);w
  // SerComm.setTX(20);
  // SerComm.begin(57600);
  Serial.println(" done");
  Serial.print("Initializing Bluetooth..");
  SerialBT.setName("Roslund-Bot 00:00:00:00:00:00");
  SerialBT.begin();
  comm_init(); //Initialize the communication FSM
  safe.stickLX = 127;
  safe.stickLY = 127;
  safe.stickRX = 127;
  safe.stickRY = 127;
  safe.btnhi = 0;
  safe.btnlo = 0;
  safe.cksum = 0b1000000010001011;

  
  delay(20);
  
  
  
  //WiFi.mode(WIFI_AP);
  //WiFi.beginAP(ssid, password);
  Serial.println(" done");
  
  setup_complete = true;
  rp2040.wdt_begin(500); // start watchdog with 500ms limit. Safety feature; reset during crash to disable motors!
}

void setup1() {
  while(!setup_complete)
    delay(100);
}

float get_voltage() {
  float voltage = analogRead(28) / 1024.0 * 3.3 * 4;
  return voltage;
}
void print_status() {
  Serial.print(get_voltage());
  Serial.print("V  ");
  Serial.print("ENC1: ");
  Serial.print(enc1.getCount());
  Serial.print("  ENC2: ");
  Serial.print(enc2.getCount());
  // Serial.print("  ENC3: ");
  // Serial.print(enc3.getCount());
  // Serial.print("  ENC4: ");
  // Serial.println(enc4.getCount());

  SerComm.print(get_voltage());
  SerComm.println("V  ");
  // SerComm.print("ENC1: ");
  // SerComm.print(enc1.getCount());
  // SerComm.print("  ENC2: ");
  // SerComm.print(enc2.getCount());
  // SerComm.print("  ENC3: ");
  // SerComm.print(enc3.getCount());
  // SerComm.print("  ENC4: ");
  // SerComm.println(enc4.getCount());


}

// Function created on 2024-09-25 for Roslund
// drive_motor sends a signal to run one motor
void drive_motor(int num, int power) {
  int adj_power = map(power, -127, 127, MAX_REVERSE, MAX_FORWARD);
  if (num == 1){
    talon1.writeMicroseconds(adj_power);
  }
  else if (num == 2){
    talon2.writeMicroseconds(adj_power);
  }
  else if (num == 3){
    talon3.writeMicroseconds(adj_power);
  }
  else if (num == 4){
    talon4.writeMicroseconds(adj_power);
  }
  else if (num == 5) {
    talon5.writeMicroseconds(adj_power);
  }
}


// Main 
void loop() {
  rp2040.wdt_reset();
  
  
  comm_parse(); // gets the communication data
  // without signal, default is to send 50% joystick value, ( ie 0% motor power )
  
  
  if (getButton(SHOULDER_TOP_RIGHT)) {
    turbo = true;
    acceleration = 15;
  }
  else {
    turbo = false;
    acceleration = 2;
  }

  if (getButton(DPAD_UP)) {
    drive_motor(5, 32 * (turbo + 1));
  }
  else if(getButton(DPAD_DOWN)) {
    drive_motor(5, -32 * (turbo + 1));
  }
  else {
    drive_motor(5, 0);
  }


  int press = getButton(SHOULDER_TOP_LEFT);
  if (press && press != mode_toggle) {
    mode_toggle = press;
    if(mode < 2) {
      mode++;
    }
    else {
      mode = 0;
    }
  }
  else if (!press) {
    mode_toggle = press;
  }
  
  
  int zeroed_lx = constrain(((int)(astate->stickLX) - 127), -127, 127);
  int zeroed_ly =  constrain(-1 * ((int)(astate->stickLY) - 127), -127, 127);
  int zeroed_rx = constrain(((int)(astate->stickRX) - 127), -127, 127);
  int zeroed_ry =  constrain(-1 * ((int)(astate->stickRY) - 127), -127, 127);

  // if statment is a safety check
  if (true) {  
    
    
    /* Drive modes - 
    Inputs:
      Range: -127, +127
      < 0 = left for x, down for y
      > 0 = right for x, up for y

      (int) zeroed_lx
      (int) zeroed_ly
      (int) zeroed_rx
      (int) zeroed_ry

    Outputs:
      Range: -127, +127
      > 0 = forward spin
      < 0 = reverse spin

      (int) target_fleft_power 
      (int) target_fright_power
      (int) target_bleft_power 
      (int) target_bright_power

    */
    // ARCADE DRIVE
    if (mode == ARCADE_DRIVE) {
      //double rawdriveangle = atan2(x, y);
      //double driveangle = rawdriveangle * 180 / 3.1415926;

      int x = zeroed_lx;
      int y = zeroed_ly;
      
      target_fleft_power = y;
      target_fright_power = y;
      target_bleft_power = y;
      target_bright_power = y;

      target_fleft_power += x;
      target_fright_power += -x;
      target_bleft_power += x;
      target_bright_power += -x;
    }

    // TANK DRIVE
    else if (mode == TANK_DRIVE) {
      int l = zeroed_ly;
      int r = zeroed_ry;
      target_fleft_power = l;
      target_bleft_power = l;
      target_fright_power = r;
      target_bright_power = r;
    }
    
    else if (mode == MECANUM_DRIVE) {
      int pivot = zeroed_rx;
      int vertical = zeroed_ly;
      int horizontal = -zeroed_lx; // inverted because wheels are swapped positions
      float denominator = max(abs(vertical) + abs(horizontal) + abs(pivot), 127);
      target_fleft_power  = (int) (127 * (( pivot+vertical+horizontal) / denominator));
      target_bleft_power  = (int) (127 * (( pivot+vertical-horizontal) / denominator));
      target_fright_power = (int) (127 * ((-pivot+vertical-horizontal) / denominator));
      target_bright_power = (int) (127 * ((-pivot+vertical+horizontal) / denominator));
    }



    // verify speeds are within limits
    target_fleft_power = constrain(target_fleft_power, -127, 127);
    target_bleft_power = constrain(target_bleft_power, -127, 127);
    target_fright_power = constrain(target_fright_power, -127, 127);
    target_bright_power = constrain(target_bright_power, -127, 127);


    // limit speed to 60% without turbo. otherwise, it will be up to 100%
    if(!turbo) {
      target_fleft_power = map(target_fleft_power, -127, 127, -25, 25);
      target_bleft_power = map(target_bleft_power, -127, 127, -25, 25);
      target_fright_power = map(target_fright_power, -127, 127, -25, 25);
      target_bright_power = map(target_bright_power, -127, 127, -25, 25);
      
    }
  }


  // reduce 
  if(cooldown > 0)
    cooldown --;
  
  
  // DEADZONE for the joystick
  if(abs(target_fleft_power) <= 4 && abs(target_bleft_power) <= 4 && // target power is low for all...
     abs(target_fright_power) <= 4 && abs(target_bright_power) <= 4 &&
    (abs(fleft_power) > 5 || abs(bleft_power) > 5 || // and at least one motor is spinning
     abs(fright_power) > 5 || abs(bright_power) > 5)) {
    fleft_power = 0;
    bleft_power = 0;
    fright_power = 0;
    bright_power = 0;
    cooldown = 2;
  }

  // These lines below control how the robot accelerates when you pull down on the stick 
  
  else if (cooldown == 0) {
    // // // FRONT LEFT MOTOR // // //
    if(target_fleft_power >= fleft_power + acceleration)
      fleft_power += acceleration;
    else if(target_fleft_power <= fleft_power - acceleration)
      fleft_power -= acceleration;
    else if(acceleration > target_fleft_power - fleft_power)
      fleft_power = target_fleft_power;
    else if(acceleration > fleft_power - target_fleft_power)
      fleft_power = target_fleft_power;

    // // // BACK LEFT MOTOR // // //
    if(target_bleft_power >= bleft_power + acceleration)
      bleft_power += acceleration;
    else if(target_bleft_power <= bleft_power - acceleration)
      bleft_power -= acceleration;
    else if(acceleration > target_bleft_power - bleft_power)
      bleft_power = target_bleft_power;
    else if(acceleration > bleft_power - target_bleft_power)
      bleft_power = target_bleft_power;

    // // // FRONT RIGHT MOTOR // // //
    if(target_fright_power >= fright_power + acceleration)
      fright_power += acceleration;
    else if(target_fright_power <= fright_power - acceleration)
      fright_power -= acceleration;
    else if(acceleration > target_fright_power - fright_power)
      fright_power = target_fright_power;
    else if(acceleration > fright_power - target_fright_power)
      fright_power = target_fright_power;

    // // // BACK RIGHT MOTOR // // //
    if(target_bright_power >= bright_power + acceleration)
      bright_power += acceleration;
    else if(target_bright_power <= bright_power - acceleration)
      bright_power -= acceleration;
    else if(acceleration > target_bright_power - bright_power)
      bright_power = target_bright_power;
    else if(acceleration > bright_power - target_bright_power)
      bright_power = target_bright_power;
  }

  

  int avg_speed = (abs(fright_power) + abs(fleft_power) + abs(bright_power) + abs(bleft_power))/4;


  drive_motor(1, fleft_power);
  drive_motor(2, -fright_power);
  drive_motor(3, bleft_power);
  drive_motor(4, -bright_power);
  Serial.println ("Mode: " + String(mode) + " FL: " + String(fleft_power) + " FR: " + String(fright_power) + " BL: " + String(bleft_power) + " BR: " + String(bright_power));
  SerComm.println("Mode: " + String(mode) + " AVG: " + String(avg_speed)); // + " FL: " + String(target_fleft_power) + " FR: " + String(target_fright_power) + " BL: " + String(target_bleft_power) + " BR: " + String(target_bright_power));

  //if(left_power != target_left_power || right_power != target_right_power)
  //print_status();
  delay(50);
  //DDYield();

}


// Loop to turn off and on with LED
void loop1() {
  //rp2040.wdt_reset();
  //drive_left(left_enabled, 255);
  digitalWrite(LED_BUILTIN, HIGH);  
  delay(50);
  //SerComm.println("update");
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
}