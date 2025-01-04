// Steppers.cpp
// Contains implementations of functions declared in Steppers.h

// Include the header file
#include "Steppers.h"

// MoToStepper objects
MoToStepper trickler(STEPS_PER_REV, STEPDIR);
MoToStepper bulk(STEPS_PER_REV, STEPDIR);

// Calibration parameters
static float kernelWeight = 0.021; // Weight of single kernel in grains for trickler
static float grainsPerRev = 65.00; // Weight of powder dumped by bulk in one full revolution

// MotorSetup()
// Attachs motor pins and enable pins for MoToStepper objects
void MotorSetup()
{ 
  // Set up trickler motor parameters
  trickler.attach(TRICKLE_STEP, TRICKLE_DIR);
  trickler.attachEnable(TRICKLE_ENABLE, 5, false);
  trickler.setSpeed(TRICKLE_SPEED);
  trickler.setRampLen(TRICKLE_RAMP);

  // Set up bulk motor parameters
  bulk.attach(BULK_STEP, BULK_DIR);
  bulk.attachEnable(BULK_ENABLE, 5, false);
  bulk.setSpeed(BULK_SPEED);
  bulk.setRampLen(BULK_RAMP);
}

// TrickleDispense(float weightTarget)
// Dispenses the requested weight using the trickler
// Returns the step target that was given to the trickler stepper
int TrickleDispense(int kernels)
{
  // Calculate required steps
  int steps = (STEPS_PER_REV / KERNELS_PER_REV) * kernels;

  // Command motor to begin moving the calculated number of steps
  trickler.move(steps);

  return steps;
}

// EndTrickle()
// Immediately end the current trickle
void EndTrickle()
{
  trickler.move(0);
}

// IsTrickling()
// Returns true if trickle motor is currently moving, false if it isn't
bool IsTrickling()
{
  long moving = trickler.stepsToDo();

  if(moving > 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

// BulkDispense()
// Starts the bulk dispenser rotating in positive direction to dispense a targeted weight
void BulkDispense(float targetWeight, int recover)
{
  // Calculate the number of revs based on targetWeight and grainsPerRev
  float targetRevs = targetWeight / grainsPerRev;
  float targetSteps = targetRevs * STEPS_PER_REV;

  // Adjust the targetSteps based on retraction distance (must add the retraction distance back onto the next dispense)
  targetSteps = targetSteps + recover;

  // Trigger bulk to move that many steps
  bulk.move(targetSteps);
}

// BulkRetract()
// Starts the bulk dispenser rotating in negative direction to retract the bulk dispenser by a given number of steps
void BulkRetract(int steps)
{
  // Turn the steps value negative before moving the desired number of steps
  steps = steps * -1;
  bulk.move(steps);
}

// EndBulk()
// Moves the bulk dispense disk 150 steps backwards from current position (to avoid bumps dumping extra kernels)
void EndBulk()
{
  bulk.move(0);
}

float GetKernelWeight()
{
  return kernelWeight;
}
void SetKernelWeight(float newValue)
{
  kernelWeight = newValue;
}

// IsBulk()
// Returns true if bulk motor is currently moving, false if it isn't
bool IsBulking()
{
  long moving = bulk.stepsToDo();

  if(moving > 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

float GetBulkWeight()
{
  return grainsPerRev;
}
void SetBulkWeight(float newValue)
{
  grainsPerRev = newValue;
}

void StopMotors()
{
  trickler.rotate(0);
  bulk.rotate(0);
}