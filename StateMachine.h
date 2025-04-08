// StateMachine.h
// Implements the state machine for the trickler
// Each state will have its own function for the main program to call
// The return value of each state will be the updated state machine tracker value
//
// State machine tracker values
// 0 = Setup
// 1 = Calibration
// 2 = Idle
// 3 = Ready
// 4 = Dispense/Run
// 5 = Evaluate
// -1 = Error identification
// -2 = Recoverable Error (waiting for reset)
// -3 = Unrecoverable Error (waiting for power cycle)

#ifndef STATES_H
#define STATES_H

// External libraries
#include <Arduino.h> // Standard Arduino libraries
#include <EEPROM.h> // Arduino EEPROM libraries

// Internal libraries
#include "Display.h" // LCD controls
#include "Scale.h" // Scale controls
#include "Steppers.h" // Motor controls

// Definitions
#define ENABLE_BTN 5
#define UP_BTN 9
#define DOWN_BTN 10

#define LONG 500
#define SHORT 350

#define MAX_DELAY 1500

#define RETRACT_STEPS 250
#define RECOVERY_STEPS 50

#define STAGE_TWO_DEFAULT 0.7

#define GREEN_LED 14
#define YELLOW_LED 15
#define RED_LED 16

#define MEMORY_ADDR 0

// Error tracker values
// 0 = No error
// 1 = Scale response timed out (recoverable)
// 2 = Scale response includes out of range characters (unrecoverable, fix scale configuration)
//int error = 0;

// State functions
int CalibrationState();
int IdleState();
int ReadyState();
int DispenseState();
int EvaluateState();

int ErrorIdState();
int RecoverableErrorState();
void UnrecoverableErrorState();

void changeTarget(float weightDiff);

bool isEnabled();
bool upPressed();
bool downPressed();

bool bulkThrow(float grains);

bool waitForBulk();
bool waitForTrickle();

void increaseBulkCalibration();
void smallIncreaseBulkCalibration();
void decreaseBulkCalibration();
void smallDecreaseBulkCalibration();
void increaseTrickleCalibration();
void smallIncreaseTrickleCalibration();
void decreaseTrickleCalibration();
void smallDecreaseTrickleCalibration();


#endif // STATES_H