// Steppers.h
// This library will provide interfaces to control the bulk and trickler stepper motors
// Allows the State Machine to worry about desired weight, not kernels or steps

#ifndef STEPPERS_H
#define STEPPERS_H

// External libraries
#include <Arduino.h> // Standard Arduino libraries
#include <MobaTools.h> // Stepper motor control

// Conversion and calibration constants
#define STEPS_PER_REV 6400 // Using 1/32 microstepping we have 200 * 32 steps per revolution
#define KERNELS_PER_REV 64 // There are 64 slots in the disk so 64 kernels to be dropped in a full turn

#define DEFAULT_KERNEL_WEIGHT 0.021 // Default kernel weight of 0.021 is a conservative estimate for Varget (actual is usually 0.18-0.2)
#define DEFAULT_BULK_WEIGHT 65.00 // Default weight of 65gr of powder dumped by bulk dispense in one full revolution

// Stepper control pin definitions
#define TRICKLE_ENABLE 4 // Pin 4
#define TRICKLE_DIR 2 // Pin 2
#define TRICKLE_STEP 3 // Pin 3
#define TRICKLE_SPEED 187 // Max speed of 18.7 rotations per minute, or ~1,995 steps per second
#define TRICKLE_RAMP 10 // Ramp length of 10 steps for any speed changes (short, targeting 0.02s or less)

#define BULK_ENABLE 8 // Pin 8
#define BULK_DIR 6 // Pin 6
#define BULK_STEP 7 // Pin 7
#define BULK_SPEED 187 // Max speed of 18.7 rotations per minute, or ~1,995 steps per second
#define BULK_RAMP 10 // Ramp length of 10 steps for any speed changes (short, targeting ~0.02s or less)


void MotorSetup();

// Trigger the trickle of requested number of kernels
int TrickleDispense(int kernels);
// Immediately end the current trickle
void EndTrickle();
// Determine if trickle is dispensing
bool IsTrickling();

// Begin running bulk dispense motor
void BulkDispense(float targetWeight, int recover);
void BulkRetract(int steps);
// Stop bulk and back up slightly for bump safety
void EndBulk();
// Determine if bulk is dispensing
bool IsBulking();

float GetKernelWeight();
void SetKernelWeight(float newValue);

float GetBulkWeight();
void SetBulkWeight(float newValue);

void StopMotors();

#endif // STEPPERS_H