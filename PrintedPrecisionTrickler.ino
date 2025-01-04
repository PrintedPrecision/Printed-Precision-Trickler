// External libraries
#include <Arduino.h> // Standard Arduino libraries

// Already included by and only used within Display.h
//#include <hd44780.h> // LCD screen
//#include <hd44780ioClass/hd44780_I2Cexp.h> // LCD screen i2c expansion header

/* Already included by and only used within Steppers.h
#include <MobaTools.h> // Stepper motor control
*/

//#include <SoftwareSerial.h> // Serial library for scale comms

// My own libraries
#include "Scale.h" // fx-120i control (weighing, zeroing, etc.)
#include "Steppers.h" // Stepper motor controls (triggering/aborting powder dispenses by kernel)
#include "Display.h" // Display controls
#include "StateMachine.h" // State machine operations

// State machine tracker (states described as below)
// 0 = Setup
// 1 = Calibration
// 2 = Idle
// 3 = Ready
// 4 = Dispense/Run
// 5 = Evaluate
// -1 = Error identification
// -2 = Recoverable Error (waiting for reset)
// -3 = Unrecoverable Error (waiting for power cycle)
static int currentState = 0;
#define SETUP_STATE 0
#define CALIBRATION_STATE 1
#define IDLE_STATE 2
#define READY_STATE 3
#define DISPENSE_STATE 4
#define EVALUATE_STATE 5
#define ERRORID_STATE -1
#define RECOVERABLE_STATE -2
#define UNRECOVERABLE_STATE -3
// The errors below are declared and stored within StateMachine.h, NOT here
// Error tracker
// 0 = No error
// 1 = Scale response timed out (recoverable)
// 2 = Scale response includes out of range characters (unrecoverable, fix scale configuration)

// Pin definitions
#define ENABLE_BTN 5
#define UP_BTN 9
#define DOWN_BTN 10


#define GREEN_LED 14
#define YELLOW_LED 15
#define RED_LED 16

int i = 0;

void setup() {
  // Open serial comms with PC
  Serial.begin(19200);
  Serial.println("Serial comms initialized\nSetup state entered");

  // Setup input/output pins
  pinMode(TRICKLE_ENABLE, OUTPUT);
  pinMode(BULK_ENABLE, OUTPUT);

  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  pinMode(YELLOW_LED, OUTPUT);
  digitalWrite(YELLOW_LED, LOW);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);

  pinMode(ENABLE_BTN, INPUT);
  pinMode(UP_BTN, INPUT_PULLUP);
  pinMode(DOWN_BTN, INPUT_PULLUP);

  // Disable the stepper motors
  digitalWrite(TRICKLE_ENABLE, HIGH);
  digitalWrite(BULK_ENABLE, HIGH);

  // Initialize the display
  LcdSetup();
  Serial.println("Display initialized");

  // Initialize the scale
  SetupScale();
  Serial.println("Scale initialized");

  // Initialize the stepper motors
  MotorSetup();
  Serial.println("Stepper motors initialized");

  // Advance state machine to the calibration state and move GUI to "Waiting to Calibrate"
  currentState = 1;
}

void loop() {
  // Temporary do-nothing loop
  if(!(i % 10000))
  {
    //Serial.print("Top of main loop, #");
    //Serial.print(i);
    //Serial.print(" currentState = ");
    //Serial.println(currentState);
  }

  switch(currentState)
  {
    case SETUP_STATE:
      // We should never get here, we don't ever go back to setup state
      break;
    case CALIBRATION_STATE:
      currentState = CalibrationState();
      break;
    case IDLE_STATE:
      currentState = IdleState();
      break;
    case READY_STATE:
      currentState = ReadyState();
      break;
    case DISPENSE_STATE:
      currentState = DispenseState();
      break;
    case EVALUATE_STATE:
      currentState = EvaluateState();
      break;
  }

  i++;
}
