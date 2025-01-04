// StateMachine.cpp
// Contains implementations of functions declared in StateMachine.cpp
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
#define SETUP_STATE 0
#define CALIBRATION_STATE 1
#define IDLE_STATE 2
#define READY_STATE 3
#define DISPENSE_STATE 4
#define EVALUATE_STATE 5
#define ERRORID_STATE -1
#define RECOVERABLE_STATE -2
#define UNRECOVERABLE_STATE -3

// Include the header file
#include "StateMachine.h"

// Error tracker
// 0 = No error
// 1 = Scale response timed out (recoverable)
// 2 = Scale response includes out of range characters (unrecoverable, fix scale configuration)
// 3 = Scale response < -0.5 after bulk is completed
int error = 0;

// Calibration variables (in Steppers.cpp)
//float kernelWeight
//float grainsPerRev

// Idle state variables
int recalibrateStart = 0;
bool recalibrateFlag = false;
bool firstIdleUpdate = true;
int btnIncrements = 0;

// Ready state variables
bool firstReadyUpdate = true;

// Dispense state variables
double dispenseWeight = 0;
double targetWeight = 32.00; // Default target weight of 32.00gr
long startTime = 0;
long endTime = 0;
float secondBulkCalibration = 0.95;

// Evaluate state variables
double evaluateWeight = 0;
bool firstEvaluate = true;
bool evaluateUpdate = true;
long elapsedTime = 0;

// CalibrationState()
// During this state the system will calibrate the trickler kernel weight
// isEnabled() must be true at all times to continue
//
// Entered from:
// - Setup state (when setup ends succesfully)
// - Idle state (when user requests re-calibration)
// Exits to:
// - Idle state (when calibration is successful or skipped by the user)
// - Error ID state (when calibration fails)
int CalibrationState()
{
  // Reset LEDs
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  float initialWeight;
  float finalWeight;

  Serial.println("Entering calibration state, waiting for enable toggle");

  // Change display to "Waiting for Calibration" display
  WaitingToCalibrate();
  
  // Wait for enable button press before advancing further
  while(!isEnabled())
  {
    // Do nothing while waiting here
  }
  delay(250); // Button debouncing time

  Serial.println("Beginning calibration, priming trickler and bulk");
  // Update the display to the calibration state
  CalibrationScreen();

  // Prime the trickler with more than 1/2 rotation (more than 32 kernels) to fill disk slots
  TrickleDispense(40);

  // Prime the bulk dispenser with more than 1/2 rotation to fill the bulk disk (dispense more than 50% of GetBulkWeight(), which returns grains per rev)
  if(!bulkThrow(0.5 * GetBulkWeight()))
  {
    Serial.println("Calibration failed during bulk prime");
    // Reset flags and return to idle state
    firstIdleUpdate = true;
    return IDLE_STATE;
  }

  // Wait for trickler priming movement to finish
  if(!waitForTrickle())
  {
    Serial.println("Calibration failed during trickle prime");
    // Reset flags and return to idle state
    firstIdleUpdate = true;
    return IDLE_STATE;
  }

  // ----------------
  // Bulk Calibration
  // ----------------
  Serial.println("Gathering initial weight for bulk calibration");
  // Gather initial stable weight
  initialWeight = StableWeight(2000);

  Serial.print("Bulk calibration initial weight = '");
  Serial.print(initialWeight, 6);
  Serial.println("'");

  for(int i = 0; i < 5; i++)
  {
    Serial.print("Starting #");
    Serial.print(i+1);
    Serial.println(" of 5 targetWeight bulk dispenses");

    if(!bulkThrow(targetWeight))
    {
      Serial.println("Calibration failed/cancelled during the bulk throws");
      // Reset flags and return to idle state b/c enable toggle was switched off
      firstIdleUpdate = true;
      return IDLE_STATE;
    }

    delay(375);
  }

  Serial.println("Gathering final weight for bulk calibration");
  // Gather final stable weight
  finalWeight = StableWeight(2000);

  Serial.print("Ending bulk calibration, final weight = '");
  Serial.print(finalWeight, 6);
  Serial.println("'");

  // Calculate the total revs dispensed and total weight dispensed
  float targetRevs = targetWeight / GetBulkWeight();
  float totalRevs = targetRevs * 5;

  float totalWeight = finalWeight - initialWeight;

  // Calculate the calibrated grainsPerRev based on these totals
  float newGrainsPerRev = totalWeight / totalRevs;

  Serial.print("totalRevs = ");
  Serial.println(totalRevs, 6);
  Serial.print("totalWeight = ");
  Serial.println(totalWeight, 6);
  Serial.print("newGrainsPerRev = ");
  Serial.println(newGrainsPerRev, 6);

  // Verify calibration value is within range
  if((30 < newGrainsPerRev) && (newGrainsPerRev < 100))
  {
    Serial.println("Bulk calibration value in range, updating grainsPerRev");
    SetBulkWeight(newGrainsPerRev);
  }
  // Out of range calibration value
  else
  {
    Serial.println("Bulk calibration out of spec, no parameter update");
  }

  // ----------------
  // Trickler Calibration
  // ----------------

  Serial.println("Gathering initial weight for trickler calibration");
  // Gather initial stable weight
  initialWeight = finalWeight;

  Serial.print("Starting calibration trickle of 300 kernels, initial weight = '");
  Serial.print(initialWeight, 6);
  Serial.println("'");

  // Request 300 kernels to be dropped
  for(int i = 0; i < 8; i++)
  {
    Serial.print("Starting #");
    Serial.print(i+1);
    Serial.println(" of eight 40 kernel trickle dispenses");

    TrickleDispense(40);
    if(!waitForTrickle())
    {
      Serial.println("Calibration failed/cancelled during the trickle throws");
      // Reset flags and return to idle state b/c enable toggle was switched off
      firstIdleUpdate = true;
      return IDLE_STATE;
    }

    delay(375);
  }

  if(!waitForTrickle())
  {
    Serial.println("Calibration failed/cancelled during the trickle throw");
    // Reset flags and return to idle state
    firstIdleUpdate = true;
    return IDLE_STATE;
  }

  
  Serial.println("Gathering final weight");
  // Gather final stable weight
  finalWeight = StableWeight(2000);

  Serial.print("Ending trickler calibration, final weight = '");
  Serial.print(finalWeight, 6);
  Serial.println("'");

  // Calculate average kernelWeight
  float weightDiff = finalWeight - initialWeight;
  float kernelAverage = weightDiff / 300;

  // Verify kernelAverage is within range
  if((0.01 < kernelAverage) && (0.1 > kernelAverage))
  {
    // If within range, update the kernelWeight and return
    SetKernelWeight(kernelAverage);
    Serial.print("New kernelWeight = '");
    Serial.print(kernelAverage, 6);
    Serial.println("'");

    firstIdleUpdate = true;
    return IDLE_STATE;
  }
  else
  {
    // Outside of range, don't change the kernelWeight
    Serial.print("No update to kernelWeight, kernelAverage was out of range = '");
    Serial.print(kernelAverage, 6);
    Serial.println("'");

    firstIdleUpdate = true;
    return IDLE_STATE; // TODO: THIS NEEDS TO BE UPDATED INTO AN ERROR ID STATE RETURN VALUE WITH ASSIGNMENT OF ERROR TYPE
  }

  // Just in case we haven't yet returned for some reason
  StopMotors();
  firstIdleUpdate = true;
  return IDLE_STATE;
}

// IdleState()
// During this state the system waits for user input
// User may depress the enable toggle to move to Ready state (isEnabled() must be false at all times)
// User may press up/down buttons to adjust target weight
// User may press and hold both up & down buttons to request re-calibration
//
// Entered from:
// - Calibration state (when successful or skipped by the user)
// - Ready/Dispense/Evaluate states (when user toggles the enable switch to off)
// - Recoverable Error state (when enable switch has been toggled on and back off again)
// Exits to
// - Ready state (when user toggles enable switch to on)
// - Calibration state (when user presses and holds both up & down buttons at the same time)
int IdleState()
{
  // Reset LEDs
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  // Change display to Idle state display
  if(firstIdleUpdate)
  {
    btnIncrements = 0;
    Serial.println("Entered Idle state for first time, updating display");
    IdleScreen(targetWeight);
    firstIdleUpdate = false;
  }

  // Advance to ready state if enable is pressed
  if(isEnabled())
  {
    Serial.println("Enable switch toggled to on in Idle state, advancing to Ready state");
    firstIdleUpdate = true;
    btnIncrements = 0;
    return READY_STATE;
  }

  if(upPressed() && downPressed())
  {
    btnIncrements = 0;
    // Check if it's the first press of each button
    if(!recalibrateFlag)
    {
      // Set the recalibrateStart time and flag
      recalibrateStart = millis();
      recalibrateFlag = true;
      return IDLE_STATE;
    }
    else
    {
      // Test if the buttons have been held long enough
      if(millis() > (recalibrateStart + 3000))
      {
        Serial.println("Up and Down buttons held for 3 seconds in IDLE state, exiting to Calibrate state");
        // Go back to calibration state after resetting flags
        recalibrateFlag = false;
        firstIdleUpdate = true;

        return CALIBRATION_STATE;
      }
    }
  }
  // Both were previously pressed but released prior to the start of recalibration
  else if(recalibrateFlag)
  {
    // Reset calibration flag and continue idle state
    recalibrateFlag = false;
    return IDLE_STATE;
  }
  // Only up btn pressed
  else if(upPressed() && !downPressed())
  {
    // Delay and re-measure for debounce
    delay(250);
    if(!upPressed() || downPressed())
    {
      // No increment if button states change
      btnIncrements = 0;
      return IDLE_STATE;
    }

    // Increment the btnIncrements and test to see if we should rollover to bigger value
    btnIncrements++;
    int targetHundrethsDigit = (int(targetWeight * 100) % 10);
    int targetTenthsDigit = (int(targetWeight * 10) % 10);

    // If the hundreths digit is 9 then we need to round up the tenths digit
    if(targetHundrethsDigit == 9)
    {
      // Roll over to 0 if the current 10ths digit is 9
      if(targetTenthsDigit == 9)
      {
        targetTenthsDigit = 0;
      }
      else
      {
        targetTenthsDigit++;
      }
      // Roll over 100ths to 0
      targetHundrethsDigit = 0;
    }
    // All other floating point rounding errors (which are indicated by an odd numbered 100ths digit) need to increment hundredths by one
    else if(targetHundrethsDigit % 2)
    {
      targetHundrethsDigit ++;
    }

    Serial.print("targetWeight = ");
    Serial.print(targetWeight, 6);
    Serial.print(", tenths = ");
    Serial.print(targetTenthsDigit);
    Serial.print(", hundreths = ");
    Serial.println(targetHundrethsDigit);

    if(btnIncrements >= 15 && targetTenthsDigit == 0)
    {
      changeTarget(1);
      IdleScreen(targetWeight);
      Serial.print("Incrementing target by 1gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }
    else if(btnIncrements >= 5 && targetHundrethsDigit == 0)
    {
      changeTarget(0.1);
      IdleScreen(targetWeight);
      Serial.print("Incrementing target by 0.1gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }
    else
    {
      changeTarget(0.02);
      IdleScreen(targetWeight);
      Serial.print("Incrementing target by 0.2gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }

    return IDLE_STATE;
  }
  // Only down btn pressed
  else if(downPressed() && !upPressed())
  {
    // Delay and re-measure for debounce
    delay(250);
    if(upPressed() || !downPressed())
    {
      // No decrement if button states change
      btnIncrements = 0;
      return IDLE_STATE;
    }

    // Increment the btnIncrements and test to see if we should rollover to bigger value
    btnIncrements++;
    int targetHundrethsDigit = (int(targetWeight * 100) % 10);
    int targetTenthsDigit = (int(targetWeight * 10) % 10);

    // If the hundreths digit is 9 then we need to round up the tenths digit
    if(targetHundrethsDigit == 9)
    {
      // Roll over to 0 if the current 10ths digit is 9
      if(targetTenthsDigit == 9)
      {
        targetTenthsDigit = 0;
      }
      else
      {
        targetTenthsDigit++;
      }
      // Roll over 100ths to 0
      targetHundrethsDigit = 0;
    }
    // All other floating point rounding errors (which are indicated by an odd numbered 100ths digit) need to increment hundredths by one
    else if(targetHundrethsDigit % 2)
    {
      targetHundrethsDigit ++;
    }

    if(btnIncrements >= 15 && targetTenthsDigit == 0)
    {
      changeTarget(-1);
      IdleScreen(targetWeight);
      Serial.print("Decrementing target by 1gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }
    else if(btnIncrements >= 5 && targetHundrethsDigit == 0)
    {
      changeTarget(-0.1);
      IdleScreen(targetWeight);
      Serial.print("Decrementing target by 0.1gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }
    else
    {
      changeTarget(-0.02);
      IdleScreen(targetWeight);
      Serial.print("Decrementing target by 0.2gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }

    return IDLE_STATE;
  }
  // No buttons pressed, continue idle state after resetting btnIncrements
  else
  {
    btnIncrements = 0;
    return IDLE_STATE;
  }
}

// ReadyState()
// During this state the enable toggle must be pressed
// The state machine will wait until conditions are met to advance to the dispense state
// 
// Entered from:
// - Idle state (when user toggles the enable switch to on)
// - Evaluate state (when user has removed the dispensed charge from the scale)
// Exits to:
// - Idle state (when user toggles the enable switch to off)
// - Dispense state (when conditions to begin dispensing are met)
// - Error state (when scale cannot be communicated with)
int ReadyState()
{

  // Reset LEDs
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  // Test whether the enable switch is off
  if(!isEnabled())
  {
    Serial.println("Enable switch toggled to off in Ready state, returning to Idle state");
    // Clear flags and return to idle state
    firstReadyUpdate = true;
    firstIdleUpdate = true;
    return IDLE_STATE;
  }

  // Change display to ready state
  if(firstReadyUpdate)
  {
    Serial.println("Entered Ready state for first time, updating display");
    ReadyScreen(targetWeight);
    firstReadyUpdate = false;
  }

  // Measure stable weight from the scale
  float currentWeight = StableWeight(750);

  // Scale response timed out
  if(currentWeight == -5000)
  {
    Serial.println("Scale response timed out, advancing to ErrorID state");
    // Clear flags, update error state, and advance to Error ID state
    firstReadyUpdate = true;
    error = 1;
    return READY_STATE;
  }
  // Scale returned characters out of range
  if(currentWeight == -6000)
  {
    Serial.println("Scale returned out of range weight characters, advancing to ErrorID state");
    // Clear flags, update error state, and advance to Error ID state
    firstReadyUpdate = true;
    error = 2;
    return READY_STATE;
  }

  // Check if we should exit to dispense state
  if((currentWeight > -0.5) && (currentWeight < 0.5))
  {
    Serial.println("Weight close enough to zero, advancing to Dispense state");
    // Clear flags, re-zero scale, and advance to Dispense state
    firstReadyUpdate = true;
    firstIdleUpdate = true;
    zeroScale();
    return DISPENSE_STATE;
  }
  
  return READY_STATE;
}

// DispenseState()
// Handles the logic for bulk dispensing of powder
// Enable switch must be toggled on at all times
//
// Enters from:
// - Ready state (when conditions to begin dispensing are met)
// - Evaluate state (if the weight settles back to less than targetWeight during evaluation)
// Exits to:
// - Idle state (when user toggles enable switch to off)
// - Evaluate state (when targetWeight is achieved or exceeded)
// - Error state (if scale stops responding or the bulk dispense times out)
int DispenseState()
{
  bool waitVal = false;

  Serial.println("Entering Dispense state");

  // Reset LEDs and illuminate yellow one
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, HIGH);
  digitalWrite(RED_LED, LOW);

  // Test whether the enable switch is off
  if(!isEnabled())
  {
    Serial.println("Enable switch toggled to off in Dispense state, returning to Idle state");
    // Clear flags/variables and return to idle state
    firstIdleUpdate = true;

    return IDLE_STATE;
  }

  // Gather the current weight and calculate our weight difference stuff
  dispenseWeight = StableWeight(LONG);
  float weightDiff = targetWeight - dispenseWeight;

  startTime = millis();

  // Do bulk dispense if required
  if(weightDiff > 1)
  {
    BulkScreen(targetWeight);
    // Dispense 97% of the required amount
    // BulkDispense(weightDiff * 0.97);
    
    // Dispense 97% of the required amount and wait for bulk to finish while monitoring enable button
    if(!bulkThrow(weightDiff * 0.97))
    {
      Serial.println("Enable toggled off during first bulk pulse, exiting to idle");
      firstIdleUpdate = true;

      return IDLE_STATE;
    }
    endTime = millis();

    // Collect weight again to evaluate next steps
    dispenseWeight = StableWeight(SHORT);
    weightDiff = targetWeight - dispenseWeight;

    // Overthrow case, adjust calibration
    if(weightDiff < -0.03)
    {
      Serial.println("First bulk pulse overthrow, exiting to evaluate");
      increaseBulkCalibration();
      
      return EVALUATE_STATE;
    }
    // Perfect throw case
    else if(weightDiff < 0.01)
    {
      // Go to evaluate state
      Serial.println("1st bulk pulse hit exact targetWeight, exiting to evaluate");

      return EVALUATE_STATE;
    }
    // Underthrow, need a second bulk pulse
    else if(weightDiff > 1)
    {
      // Handle extreme underthrow case (return to ready state or go to error state in this instance)
      if(weightDiff > targetWeight * 0.5)
      {
        Serial.println("Extreme underthrow error during first bulk pulse");

        return READY_STATE;
      }
      // Adjust calibration only for large underthrow (10% or more)
      if(weightDiff > targetWeight * 0.1)
      {
        decreaseBulkCalibration();
      }

      // Dispense a portion of the required amount and wait for bulk to finish while monitoring enable button
      if(!bulkThrow(weightDiff * secondBulkCalibration))
      {
        Serial.println("Enable toggled off during second bulk pulse, exiting to idle");
        firstIdleUpdate = true;

        return IDLE_STATE;
      }
      endTime = millis();
      
      // TODO : Replace the long measurement with a short one + adelay based on kernel drop time/scale settling time

      dispenseWeight = StableWeight(SHORT);
      weightDiff = targetWeight - dispenseWeight;

      // Overthrow case
      if(weightDiff < -0.03)
      {
        Serial.println("Second bulk pulse overthrow, exiting to evaluate");
        secondBulkCalibration = secondBulkCalibration - 0.02;
        Serial.print("secondBulkCalibration reduced by 0.02, new value = ");
        Serial.println(secondBulkCalibration);
      
        return EVALUATE_STATE;
      }
      // Perfect throw case
      else if(weightDiff < 0.01)
      {
        // Go to evaluate state
        Serial.println("2nd bulk pulse hit exact targetWeight, exiting to evaluate");

        return EVALUATE_STATE;
      }
      // Handle extreme underthrow first
      else if(weightDiff > targetWeight * 0.5)
      {
        Serial.println("Extreme underthrow error during second bulk pulse");
        
        return READY_STATE;
      }
      // Handle normal underthrow second
      else if (weightDiff > 0.7)
      {
        Serial.println("Second bulk pulse underthrow");
        secondBulkCalibration = secondBulkCalibration + 0.01;
        Serial.print("secondBulkCalibration increased by 0.01, new value = ");
        Serial.println(secondBulkCalibration);

        // Do not exit dispense state in this instance
      }
    }
  }
  // Now do a final trickle, on first entry we already have a current weight and weightDiff from above sections of code
  while(isEnabled())
  {
    // If weightDiff is 0.02 or less, re-measure the weight just in case
    if(weightDiff < 0.03)
    {
      dispenseWeight = StableWeight(LONG);
      weightDiff = targetWeight - dispenseWeight;

      // If target weight has been reached, go to Evaluate state
      if(weightDiff < 0.01)
      {
        return EVALUATE_STATE;
      }
    }

    // Calculate kernels
    float kernelWeight = GetKernelWeight();
    int kernels = weightDiff / kernelWeight;
    float remainder = ((weightDiff / kernelWeight) - kernels) * kernelWeight;

    Serial.print("Remainder after kernels have been dispensed = ");
    Serial.println(remainder, 6);

    // Check the remainder to see if we need to add an extra kernel
    if(remainder >= 0.01)
    {
      Serial.print("Adding kernel because remainder = ");
      Serial.println(remainder, 6);
      kernels = kernels + 1;
    }
    
    // If weightDiff is non-zero but kernels rounds to 0 (shouldn't happen anymore with the remainder check)
    if(!kernels && (weightDiff > 0.01))
    {
      Serial.println("Setting kernels to 1 because of zero kernels but a postiive weight diff, this shouldn't happen anymore");
      kernels = 1;
    }

    Serial.print("Fine trickling '");
    Serial.print(kernels);
    Serial.print("' kernels, with weight difference of ");
    Serial.println(weightDiff, 6);

    // Dispense appropriate number of kernels
    TrickleDispense(kernels);

    if(!waitForTrickle())
    {
      Serial.println("Trickle in Dispense state failed");
      firstIdleUpdate = true;

      return IDLE_STATE;
    }
    endTime = millis();

    // Calculate the settling delay based on the time spent trickling and the maximum delay time
    // We can drop ~20 kernels per second (18.7 revs/minute and 64 kernels/rev)
    // LONG is the duration of our stable measurement window in ms
    int delayStart = millis();

    // Loop for a max duration of settlingDelayMs looking for a change in the scale weight, polling every 50ms
    while(StableWeight(50) <= dispenseWeight)
    {
      // Return to idle if no longer enabled
      if(!isEnabled())
      {
        Serial.println("Enable button toggled to off while waiting for scale to register a change in weight");
        firstIdleUpdate = true;

        return IDLE_STATE;
      }

      // Check if we have reached settlingDelayMs and break from the polling loop if so
      int curTime = millis();
      if((curTime - delayStart) > MAX_DELAY)
      {
        break;
      }
    }
    // Weight has changed from the last time dispenseWeight was measured or it has timed out waiting for it to change

    // Get new dispenseWeight and weightDiff
    dispenseWeight = StableWeight(LONG);
    weightDiff = targetWeight - dispenseWeight;

    // Overthrow case
    if(weightDiff < -0.03)
    {
      // Increase trickler calibration before going to evaluate
      Serial.println("Trickler overthrow, exiting to evaluate");
      increaseTrickleCalibration();

      return EVALUATE_STATE;
    }
    // Perfect throw case
    else if(weightDiff < 0.01)
    {
      // Go to evaluate state
      Serial.println("Trickled to correct targetWeight, exiting to evaluate");

      return EVALUATE_STATE;
    }
    // Underthrow case
    else
    {
      // Handle extreme underthrow
      if(weightDiff > targetWeight * 0.5)
      {
        Serial.println("Extreme underthrow error during trickle");

        return READY_STATE;
      }
      Serial.println("Trickle did not reach targetWeight, restarting the trickle");

      if(weightDiff > 0.061)
      {
        decreaseTrickleCalibration();
      }
    }
  }
  // Return idle because reaching this point means enable button is no longer pressed (we exited the isEnabled() while loop that runs the trickler)

  return IDLE_STATE;
}

int EvaluateState()
{
  float weightDiff;

  // Test if we are enabled or not
  if(!isEnabled())
  {
    Serial.println("Enable switch toggled to off in Evaluate state before weight measurements, returning to Idle state");
    // Reset evaluate and idle flags
    firstEvaluate = true;
    evaluateUpdate = true;
    firstIdleUpdate = true;

    // Reset LEDs before exiting evaluate
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);

    // Return to idle state
    return IDLE_STATE;
  }

  // Update evaluateWeight and weightDiff as necessary TODO: might want to eliminate this portion and use a single shared currentWeight that is managed in the main function?
  // First entry to Evaluate state
  if(firstEvaluate)
  {
    evaluateUpdate = true;
    firstEvaluate = false;

    evaluateWeight = dispenseWeight;
    weightDiff = targetWeight - evaluateWeight;

    elapsedTime = endTime - startTime;

    Serial.print("----- Entered Evaluate state after ");
    Serial.print(elapsedTime);
    Serial.print("ms, evaluateWeight = ");
    Serial.print(evaluateWeight, 6);
    Serial.print("gr and weightDiff = ");
    Serial.print(weightDiff, 6);
    Serial.println(" -----");
  }
  // Repeat loops in Evaluate state
  else
  {
    // Take new weight reading
    float tmpWeight = StableWeight(2 * LONG);

    // Case 1, weight has changed
    // Test if new weight reading is different from the existing one
    if(tmpWeight != evaluateWeight)
    {
      // Case 1.1 - tmpWeight indicates user has removed the shot glass
      // Verify weight is less than -500, since shot glass will weigh at least that much
      if(tmpWeight > 500)
      {
        // Reset the evaluation flags
        firstEvaluate = true;
        evaluateUpdate = true;

        // Reset LEDs before exiting evaluate
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(RED_LED, LOW);

        // Return to Ready state
        return READY_STATE;
      }
      // Case 1.2 - Weight has changed without user removing the shot glass
      // Verify this changed weight with a second measurement
      else if(StableWeight(2 * LONG) == tmpWeight)
      {
        // Set evaluate update flag before updating evaluateWeight and weightDiff
        evaluateUpdate = true;

        evaluateWeight = tmpWeight;
        weightDiff = targetWeight - evaluateWeight;

        // Report this weight change to serial comms
        Serial.print("Weight changed during Evaluate state, new weight = ");
        Serial.print(evaluateWeight, 6);
        Serial.print("gr and weightDiff = ");
        Serial.println(weightDiff, 6);
      }
      // Case 1.3 - Weight change cannot be confirmed
      else
      {
        // No further action required, return to the top of Evaluate state
        return EVALUATE_STATE;
      }
    }
    // Case 2, weight has not changed
    else
    {
      // No further action required, return to the top of Evaluate state
      return EVALUATE_STATE;
    }
  }
  // End of the update to evaluateWeight and weightDiff

  // Make sure that we are still enabled after the weight measurements complete
  if(!isEnabled())
  {
    Serial.println("Enable switch toggled to off in Evaluate state after weight measurements, returning to Idle state");
    // Reset evaluation and idle flags
    firstEvaluate = true;
    evaluateUpdate = true;
    firstIdleUpdate = true;

    // Reset LEDs before exiting evaluate
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);

    // Return to idle state
    return IDLE_STATE;
  }

  // Determine evaluation display update based on weightDiff
  // Case 1 - overthrow
  if(weightDiff < -0.03)
  {
    // Change the display
    Serial.println("Overthrow detected in Evaluate State");
    OverthrowScreen(targetWeight, evaluateWeight, elapsedTime);

    // Illuminate the Red LED after turning the others off
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  }
  // Case 2 - correct weight
  else if(weightDiff < 0.01)
  {
    // Change the display
    Serial.println("Acceptable charge detected in Evaluate State");
    GoodChargeScreen(targetWeight, evaluateWeight, elapsedTime);

    // Illuminate the Green LED after turning the others off
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
  }
  // Case 3 - underthrow
  else
  {
    // Reset evaluation flags since we will exit Evaluate state
    firstEvaluate = true;
    evaluateUpdate = true;

    // Handle extreme underthrow
    if(weightDiff > targetWeight * 0.5)
    {
      // Reset LEDs before exiting evaluate
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(RED_LED, LOW);

      Serial.println("Extreme underthrow error during evaluate");
      return READY_STATE;
    }
    else
    {
      // Reset LEDs before exiting evaluate
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(RED_LED, LOW);

      Serial.println("Underthrow detected in Evaluate state, returning to Dispense state");
      return DISPENSE_STATE;
    }
  }

  // Set evaluate flags to false since it's not our first evaluation and any updates have been completed
  firstEvaluate = false;
  evaluateUpdate = false;

  // Return to top of Evaluate state
  return EVALUATE_STATE;
}

int ErrorIdState()
{

}
int RecoverableErrorState()
{

}
void UnrecoverableErrorState()
{

}

// changeTarget(float weightDiff)
void changeTarget(float weightDiff)
{
  targetWeight = targetWeight + weightDiff;
}

// isEnabled()
// Returns whether or not the enable toggle is currently pressed
bool isEnabled()
{
  if(digitalRead(ENABLE_BTN))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool upPressed()
{
  if(!digitalRead(UP_BTN))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool downPressed()
{
  if(!digitalRead(DOWN_BTN))
  {
    return true;
  }
  else
  {
    return false;
  }
}

// Does a bulk throw, including the retraction at the end
bool bulkThrow(float grains)
{
  // Bulk dispense the requested number of grains of powder
  BulkDispense(grains, (1.5 * RECOVERY_STEPS));

  if(!waitForBulk())
  {
    Serial.println("Bulk throw cancelled during the dispense phase");
    // Return false if enable is toggled to off during the bulk dispense
    return false;
  }

  // Retract the bulk trickler by the specified retraction distance
  BulkRetract(RETRACT_STEPS);

  if(!waitForBulk())
  {
    Serial.println("Bulk throw cancelled during the 1st retract phase");
    // Return false if enable is toggled to off during the retraction
    return false;
  }

  // Go forwards the recovery distance
  BulkRetract(-1 * RECOVERY_STEPS);
  if(!waitForBulk())
  {
    Serial.println("Bulk throw cancelled during the shimmy phase");
    // Return false if enable is toggled to off during the retraction
    return false;
  }

  // Return true if the dispense plus retraction completes successfully
  return true;
}

bool waitForBulk()
{
  // Wait for initial bulk to complete
  while(IsBulking())
  {
    // Exit to Idle state if enable switch is toggled off at any time
    if(!isEnabled())
    {
      EndBulk();
      EndTrickle();
      return false;
    }
  }
  /*
  // Delay an extra 100ms
  delay(100);

  // Perform EndBulk() retraction
  EndBulk();
  */

  return true;

}


bool waitForTrickle()
{
  // Wait for initial bulk to complete
  while(IsTrickling())
  {
    // Exit to Idle state if enable switch is toggled off at any time
    if(!isEnabled())
    {
      EndTrickle();
      EndBulk();
      return false;
    }
  }

  return true;
}

void increaseBulkCalibration()
{
  float curRevs = GetBulkWeight();
  SetBulkWeight(curRevs * 1.03);

  Serial.print("Adjusting calibration up by 3%, grainsPerRev = ");
  Serial.println(GetBulkWeight(), 6);
}

void decreaseBulkCalibration()
{
  float curRevs = GetBulkWeight();
  SetBulkWeight(curRevs * 0.98);

  Serial.print("Adjusting calibration down by 2%, grainsPerRev = ");
  Serial.println(GetBulkWeight(), 6);
}

void increaseTrickleCalibration()
{
  // Adjust kernel weight to be slightly larger
  float curKernel = GetKernelWeight();
  SetKernelWeight(curKernel * 1.03);

  Serial.print("Adjusting calibration up by 3%, kernelWeight = ");
  Serial.println(GetKernelWeight(), 6);
}

void decreaseTrickleCalibration()
{
  // Adjust kernel weight to be slightly smaller
  float curKernel = GetKernelWeight();
  SetKernelWeight(curKernel * 0.98);

  Serial.print("Adjusting calibration down by 2%, kernelWeight = ");
  Serial.println(GetKernelWeight(), 6);
}