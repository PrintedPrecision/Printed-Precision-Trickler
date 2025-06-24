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

// Calibrate state variables
bool recalibrateFlag = false;

// Idle state variables
int recalibrateStart = 0;
int curTime = 0;
bool firstIdleUpdate = true;
int btnIncrements = 0;

// Ready state variables
bool firstReadyUpdate = true;

// Dispense state variables
double dispenseWeight = 0;
double targetWeight = 32.00; // Default target weight of 32.00gr
long startTime = 0;
long endTime = 0;
float secondBulkCalibration = STAGE_TWO_DEFAULT;
double errorMargin = 0.02;

// Evaluate state variables
double evaluateWeight = 0;
bool firstEvaluate = true;
bool evaluateUpdate = false;
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

  float tempTarget;

  // Change display to "Waiting for Calibration" display
  WaitingToCalibrate();

  // Check if we are recalibrating and avoid replacing targetWeight if so
  if(recalibrateFlag)
  {
    Serial.println("Recalibrating, no saved targetWeight read required");
    recalibrateFlag = false;
  }
  else
  {
    Serial.println("First calibration, reading saved targetWeight from EEPROM");

    EEPROM.get(MEMORY_ADDR, tempTarget);

    // Make sure the read value is within range
    if((0 <= tempTarget) && (tempTarget <= 250))
    {
      // Set targetWeight to the read value
      targetWeight = tempTarget;
      Serial.print("Read target value is: '");
      Serial.print(tempTarget, 6);
      Serial.println("', setting targetWeight to match");
    }
    // Read value is out of range
    else
    {
      Serial.print("Read value out of range: '");
      Serial.print(tempTarget, 6);
      Serial.println("', leaving targetWeight as default of 32.00gr");
    }
  }

  Serial.println("Beginning calibration, waiting for enable toggle");
  
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

  // Indicate calibration start with LEDs
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, HIGH);
  digitalWrite(RED_LED, LOW);

  // ----------------
  // Stage 1 Bulk Calibration
  // ----------------
  Serial.println("Gathering initial weight for stage 1 bulk calibration");
  // Gather initial stable weight
  initialWeight = StableWeight(2000);

  Serial.print("Stage 1 Bulk calibration initial weight = '");
  Serial.print(initialWeight, 6);
  Serial.println("'");

  for(int i = 0; i < 4; i++)
  {
    Serial.print("Starting #");
    Serial.print(i+1);
    Serial.println(" of 4 targetWeight bulk dispenses");

    if(!bulkThrow(targetWeight))
    {
      Serial.println("Calibration failed/cancelled during the bulk throws");
      // Reset flags and return to idle state b/c enable toggle was switched off
      firstIdleUpdate = true;
      return IDLE_STATE;
    }

    delay(500);
  }

  Serial.println("Gathering final weight for stage 1 bulk calibration");
  // Gather final stable weight
  finalWeight = StableWeight(2000);

  Serial.print("Ending stage 1 bulk calibration, final weight = '");
  Serial.print(finalWeight, 6);
  Serial.println("'");

  // Calculate the total revs dispensed and total weight dispensed
  float targetRevs = targetWeight / GetBulkWeight();
  float totalRevs = targetRevs * 4;

  float totalWeight = finalWeight - initialWeight;

  // Calculate the calibrated grainsPerRev based on these totals (and adjust up by 5% to avoid early overthrows)
  float newGrainsPerRev = (totalWeight / totalRevs) * 1.05;

  Serial.print("totalRevs = ");
  Serial.println(totalRevs, 6);
  Serial.print("totalWeight = ");
  Serial.println(totalWeight, 6);
  Serial.print("newGrainsPerRev = ");
  Serial.println(newGrainsPerRev, 6);

  // Verify calibration value is within range
  if((20 < newGrainsPerRev) && (newGrainsPerRev < 150))
  {
    Serial.println("Stage 1 Bulk calibration value in range, updating grainsPerRev");
    SetBulkWeight(newGrainsPerRev);

    // Indicate calibration success with the LEDs
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);

    StageOneBulk(newGrainsPerRev,GetBulkWeight());

    delay(1000);
  }
  // Out of range calibration value
  else
  {
    Serial.println("Bulk calibration out of spec, no parameter update");

    // Indicate calibration failure with the LEDs
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, HIGH);

    StageOneBulk(newGrainsPerRev,GetBulkWeight());

    delay(2000);
  }

  // ----------------
  // Trickler Calibration
  // ----------------
  // Correct initial stable weight
  initialWeight = finalWeight;

  Serial.print("Starting calibration trickle of 100 kernels, initial weight = '");
  Serial.print(initialWeight, 6);
  Serial.println("'");

  // Indicate calibration start with LEDs
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, HIGH);
  digitalWrite(RED_LED, LOW);

  // Request 200 kernels to be dropped
  /*
  for(int i = 0; i < 4; i++)
  {
    Serial.print("Starting #");
    Serial.print(i+1);
    Serial.println(" of four 25 kernel trickle dispenses");

    TrickleDispense(25);
    if(!waitForTrickle())
    {
      Serial.println("Calibration failed/cancelled during the trickle throws");
      // Reset flags and return to idle state b/c enable toggle was switched off
      firstIdleUpdate = true;
      return IDLE_STATE;
    }

    delay(375);
  }
  */
  TrickleDispense(200);
  if(!waitForTrickle())
  {
    Serial.println("Calibration failed/cancelled during the trickle throws");
    // Reset flags and return to idle state b/c enable toggle was switched off
    firstIdleUpdate = true;
    return IDLE_STATE;
  }
  
  Serial.println("Gathering final weight");
  // Gather final stable weight
  finalWeight = StableWeight(2000);

  Serial.print("Ending trickler calibration, final weight = '");
  Serial.print(finalWeight, 6);
  Serial.println("'");

  // Calculate average kernelWeight (and adjust it up by 5% to avoid early overtrickles)
  float weightDiff = finalWeight - initialWeight;
  float kernelAverage = (weightDiff / 200) * 1.05;

  // Verify kernelAverage is within range
  if((0.01 < kernelAverage) && (0.10 > kernelAverage))
  {
    // If within range, update the kernelWeight and return
    SetKernelWeight(kernelAverage);
    Serial.print("New kernelWeight = '");
    Serial.print(kernelAverage, 6);
    Serial.println("'");

    // Adjust the errorMargin in case of chonky kernels
    if(kernelAverage > 0.03 && kernelAverage <= 0.05)
    {
      Serial.println("Setting errorMargin to 0.04 because of kernelWeight");
      errorMargin = 0.04;
    }
    else if(kernelAverage > 0.05)
    {
      Serial.println("Setting errorMargin to 0.06 because of kernelWeight");
      errorMargin = 0.06;
    }

    // Indicate calibration success with LEDs
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);

    Trickle(kernelAverage, GetKernelWeight());

    delay(2500);

    // DO NOT RETURN TO IDLE (need to display results)
  }
  else
  {
    // Outside of range, don't change the kernelWeight
    Serial.print("No update to kernelWeight, kernelAverage was out of range = '");
    Serial.print(kernelAverage, 6);
    Serial.println("'");

    // Indicate calibration fail with LEDs
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);

    Trickle(kernelAverage, GetKernelWeight());

    delay(2000);

    // DO NOT RETURN TO IDLE (need to display results)
  }

  // Turn all LEDs off
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  bool firstScreenUpdate = true;
  // Infinite loop to display calibration results until enable button is toggled off
  while(isEnabled())
  {
    // Only update the display once
    if(firstScreenUpdate)
    {
      Serial.println("Displaying calibration results and awaiting toggle before advancting to idle state");

      CalibrationComplete(GetBulkWeight(), GetKernelWeight());
      firstScreenUpdate = false;
    }
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

  // Reset recalibration flag, just in case
  recalibrateFlag = false;

  // Change display to Idle state display
  if(firstIdleUpdate)
  {
    btnIncrements = 0;
    Serial.println("Entered Idle state for first time, updating display");
    IdleScreen(targetWeight, errorMargin);
    firstIdleUpdate = false;
  }

  // Advance to ready state if enable is pressed
  if(isEnabled())
  {
    Serial.println("Enable switch toggled to on in Idle state");

    // Check if the stored targetWeight in EEPROM has changed
    float tempTarget;
    EEPROM.get(MEMORY_ADDR, tempTarget);

    // targetWeight has changed, need to update saved value
    if(tempTarget != targetWeight)
    {
      Serial.println("Writing new targetWeight value to EEPROM");
      Serial.print("Old value = '");
      Serial.print(tempTarget, 6);
      Serial.print("', new value = '");
      Serial.print(targetWeight, 6);
      Serial.println("'");

      EEPROM.put(MEMORY_ADDR, targetWeight);
    }
    // targetWeight has not changed, do not update saved value
    else
    {
      Serial.println("Saved targetWeight value matches current, no update necessary");
    }

    Serial.println("Advancing to Ready state");

    firstIdleUpdate = true;
    btnIncrements = 0;
    return READY_STATE;
  }

  if(upPressed() && downPressed())
  {
    Serial.println("Up and Down buttons both pressed for the first time");
    btnIncrements = 0;
    // Establish the starting time they were both pressed
    recalibrateStart = millis();

    // Check the button states on a loop until the elapsed time reaches 2 seconds
    for(curTime = millis(); curTime < (recalibrateStart + 2000); curTime = millis())
    {
      if(!upPressed() || !downPressed())
      {
        Serial.println("A button was released early, returning to Idle state");
        return IDLE_STATE;
      }
    }
    // Proceed to recalibration
    Serial.println("Both buttons held for 2000ms, setting recalibrateFlag and proceeding to Calibrate state");
    recalibrateFlag = true; 

    return CALIBRATION_STATE;
  }
  // Only up btn pressed
  else if(upPressed() && !downPressed())
  {
    Serial.println("Only up pressed");
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
      IdleScreen(targetWeight, errorMargin);
      Serial.print("Incrementing target by 1gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }
    else if(btnIncrements >= 5 && targetHundrethsDigit == 0)
    {
      changeTarget(0.1);
      IdleScreen(targetWeight, errorMargin);
      Serial.print("Incrementing target by 0.1gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }
    else
    {
      changeTarget(0.02);
      IdleScreen(targetWeight, errorMargin);
      Serial.print("Incrementing target by 0.2gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }

    return IDLE_STATE;
  }
  // Only down btn pressed
  else if(downPressed() && !upPressed())
  {
    Serial.println("Only down pressed");
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
      IdleScreen(targetWeight, errorMargin);
      Serial.print("Decrementing target by 1gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }
    else if(btnIncrements >= 5 && targetHundrethsDigit == 0)
    {
      changeTarget(-0.1);
      IdleScreen(targetWeight, errorMargin);
      Serial.print("Decrementing target by 0.1gr, targetWeight = ");
      Serial.println(targetWeight, 6);
    }
    else
    {
      changeTarget(-0.02);
      IdleScreen(targetWeight, errorMargin);
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
    ReadyScreen(targetWeight, errorMargin);
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
  if((currentWeight > -0.5) && (currentWeight < (targetWeight + 0.5)))
  {
    Serial.println("Weight within range of -0.5gr and (targetWeight + 0.5gr), advancing to dispense state");
    // Clear flags, re-zero scale, and advance to Dispense state
    firstReadyUpdate = true;
    firstIdleUpdate = true;
    
    // Re-zero the scale if we are close enough to zero when the cup is replaced
    if((currentWeight > -0.5) && (currentWeight < 0.5))
    {
      zeroScale();
    }
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
  float startingWeightDiff = weightDiff;

  startTime = millis();

  // First skip straight to evaluate if weightDiff is negative
  if(weightDiff <= 0)
  {
    return EVALUATE_STATE;
  }

  // If the weightDiff is greater than 250gr, skip straight to evaluate because something is wrong
  if(weightDiff > 250)
  {
    return EVALUATE_STATE;
  }

  // Do first stage bulk dispense if we need 10+ grains
  if(weightDiff > 10)
  {
    BulkScreen(targetWeight, errorMargin);
    
    // Dispense 92% of the required amount and wait for bulk to finish while monitoring enable button
    if(!bulkThrow(weightDiff * 0.92))
    {
      Serial.println("Enable toggled off during first bulk pulse, exiting to idle");
      firstIdleUpdate = true;

      return IDLE_STATE;
    }
    endTime = millis();

    // Collect weight again to evaluate next steps
    dispenseWeight = StableWeight(SHORT);
    weightDiff = targetWeight - dispenseWeight;
    float dispenseTotal = startingWeightDiff - weightDiff;

    // Less than 1gr was dispensed, skip straight to eval state
    if(dispenseTotal < 1)
    {
      return EVALUATE_STATE;
    }

    // Getting too close to target case, small calibration adjustment
    else if(weightDiff < (0.02 * targetWeight))
    {
      Serial.println("First bulk pulse too close to target, making slight calibration adjustment");
      smallIncreaseBulkCalibration();
    }
    // Overthrow case, adjust calibration
    else if(weightDiff < (-1.2 * errorMargin))
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
      // ADD SMALL BULK CALIBRATION INCREASE HERE
      return EVALUATE_STATE;
    }
    // Advance to trickle if our weightDiff <= 1, but get a second short weight measurement first
    else if(weightDiff <= 1)
    {
      Serial.println("Good 1st bulk, take short weight measurement and advance to trickle");
      dispenseWeight = StableWeight(SHORT);
      weightDiff = targetWeight - dispenseWeight;
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
      // Adjust calibration for large underthrow (15% or more)
      if(weightDiff > targetWeight * 0.15)
      {
        Serial.println("Large first bulk underthrow, decreasing calibration value");
        decreaseBulkCalibration();
      }
      // Adjust calibration for smaller underthrow (10% or more)
      else if(weightDiff > targetWeight * 0.1)
      {
        Serial.println("Small first bulk underthrow, slightly decreasing calibration value");
        smallDecreaseBulkCalibration();
      }

      // Dispense a portion of the required amount and wait for bulk to finish while monitoring enable button
      if(!bulkThrow(weightDiff * secondBulkCalibration))
      {
        Serial.println("Enable toggled off during second bulk pulse, exiting to idle");
        firstIdleUpdate = true;

        return IDLE_STATE;
      }
      endTime = millis();

      dispenseWeight = StableWeight(SHORT);
      weightDiff = targetWeight - dispenseWeight;

      // Take a long measurement if the weightDiff is small
      if(weightDiff < 0.1 && weightDiff > -0.1)
      {
        dispenseWeight = StableWeight(LONG);
        weightDiff = targetWeight - dispenseWeight;
      }

      // Overthrow case
      if(weightDiff < (-1.2 * errorMargin))
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
        // Go to evaluate state after adjusting calibration
        Serial.println("2nd bulk pulse hit exact targetWeight, exiting to evaluate");
        secondBulkCalibration = secondBulkCalibration - 0.005;
        Serial.print("secondBulkCalibration reduced by 0.005, new value = ");
        Serial.println(secondBulkCalibration);

        return EVALUATE_STATE;
      }
      // Fine tune calibration on close calls to avoid overthrows
      else if(weightDiff < 0.2)
      {
        Serial.println("Second bulk pulse too close to target, adjusting calibration");
        secondBulkCalibration = secondBulkCalibration - 0.005;
        Serial.print("secondBulkCalibration reduced by 0.005, new value = ");
        Serial.println(secondBulkCalibration);

        // Do not exit dispense state in this instance
      }
      // Handle extreme underthrow case (relative to starting point, not relative to target weight now that re-trickle was added)
      else if(weightDiff > startingWeightDiff * 0.5)
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
  // Start with a second stage bulk dispense if we have < 15 kernels but > 2 kernels left to dispense
  else if(weightDiff > 2)
  {
    // Dispense a portion of the required amount and wait for bulk to finish while monitoring enable button
    if(!bulkThrow(weightDiff * secondBulkCalibration))
    {
      Serial.println("Enable toggled off during second bulk pulse, exiting to idle");
      firstIdleUpdate = true;

      return IDLE_STATE;
    }
    endTime = millis();

    dispenseWeight = StableWeight(SHORT);
    weightDiff = targetWeight - dispenseWeight;

    // Take a long measurement if the weightDiff is small
    if(weightDiff < 0.1 && weightDiff > -0.1)
    {
      dispenseWeight = StableWeight(LONG);
      weightDiff = targetWeight - dispenseWeight;
    }

    // Overthrow case
    if(weightDiff < (-1.2 * errorMargin))
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
      // Go to evaluate state after adjusting calibration
      Serial.println("2nd bulk pulse hit exact targetWeight, exiting to evaluate");
      secondBulkCalibration = secondBulkCalibration - 0.005;
      Serial.print("secondBulkCalibration reduced by 0.005, new value = ");
      Serial.println(secondBulkCalibration);

      return EVALUATE_STATE;
    }
    // Fine tune calibration on close calls to avoid overthrows
    else if(weightDiff < 0.1)
    {
      Serial.println("Second bulk pulse too close to target, adjusting calibration");
      secondBulkCalibration = secondBulkCalibration - 0.005;
      Serial.print("secondBulkCalibration reduced by 0.005, new value = ");
      Serial.println(secondBulkCalibration);

      // Do not exit dispense state in this instance
    }
    // Handle extreme underthrow case (relative to starting point, not relative to target weight now that re-trickle was added)
    else if(weightDiff > startingWeightDiff * 0.5)
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
  // Update the screen to indicate we are moving on to the trickle
  TrickleScreen(targetWeight, errorMargin);

  // Now do a final trickle, on first entry we already have a current weight and weightDiff from above sections of code
  while(isEnabled())
  {
    // Do not allow it to trickle more than 5gr of powder
    if(weightDiff > 5)
    {
      return DISPENSE_STATE;
    }
    
    // If weightDiff is one kernel or less, re-measure the weight just in case
    if(weightDiff < (1.2 * errorMargin))
    {
      dispenseWeight = StableWeight(LONG);
      weightDiff = targetWeight - dispenseWeight;

      // If target weight has been reached, go directly to Evaluate state
      if(weightDiff <= 0.01)
      {
        return EVALUATE_STATE;
      }
    }

    // Calculate kernels
    float kernelWeight = GetKernelWeight();
    int kernels = weightDiff / kernelWeight;
    float remainder = ((weightDiff / kernelWeight) - kernels) * kernelWeight;

    Serial.print("Predicted remainder after dispening kernels = ");
    Serial.println(remainder, 6);

    // Check the remainder to see if we need to add an extra kernel
    if(remainder >= (0.49 * errorMargin))
    {
      Serial.print("Adding kernel because remainder = ");
      Serial.println(remainder, 6);
      kernels = kernels + 1;
    }
    
    // Handle single kernel dispense cases (positive weightDiff and calculated kernels <= 1)
    if((weightDiff > 0) && (kernels <= 1))
    {
      Serial.println("Trickle dispensing one more kernel before immediately proceeding to evaluate state");
      kernels = 1;

      // Dispense the one kernel
      TrickleDispense(1);

      endTime = millis();

      // Immediately proceed to evaluate state
      return EVALUATE_STATE;
    }

    // Adjust kernel target for longer trickles
    if(weightDiff > 0.75)
    {
      kernels = kernels - 3;
    }
    else if(weightDiff > 0.4)
    {
      kernels = kernels - 1;
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
    // Can only exit early if we disable dispensing or see the weight increase by at least 75% of the weightDiff to target
    while(StableWeight(50) <= (dispenseWeight + (0.75 * weightDiff)))
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
    if(weightDiff < (-1.2 * errorMargin))
    {
      // Handle case of overthrow only 1 tick past errorMargin (0.02 over) on a long throw
      if(kernels > 15 && (weightDiff > ((-1.2 * errorMargin) - 0.02)))
      {
        Serial.println("Tiny overthrow on a long trickle, exiting to evaluate");
        smallIncreaseTrickleCalibration();

        return EVALUATE_STATE;
      }
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
      if(weightDiff > 2)
      {
        Serial.println("Extreme underthrow error during trickle");

        return READY_STATE;
      }
      Serial.println("Trickle did not reach targetWeight, restarting the trickle");

      // Adjust calibration for underthrow of at least 4 kernels
      if(weightDiff > (3 * errorMargin))
      {
        decreaseTrickleCalibration();
      }
      // Do a tiny calibration change for underthrow of only 2 kernels
      else if(weightDiff > (1.2 * errorMargin))
      {
        smallDecreaseTrickleCalibration();
      }
    }
  }
  // Return idle because reaching this point means enable button is no longer pressed (we exited the isEnabled() while loop that runs the trickler)

  return IDLE_STATE;
}

int EvaluateState()
{
  float weightDiff;
  float kernelWeight = GetKernelWeight();

  // Test if we are enabled or not
  if(!isEnabled())
  {
    Serial.println("Enable switch toggled to off in Evaluate state before weight measurements, returning to Idle state");
    // Reset evaluate and idle flags
    firstEvaluate = true;
    evaluateUpdate = false;
    firstIdleUpdate = true;

    // Reset LEDs before exiting evaluate
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);

    // Return to idle state
    return IDLE_STATE;
  }

  // Check if user has option to add a kernel
  if(evaluateUpdate)
  {
    // Check if up button has been pressed
    if(upPressed() && !downPressed())
    {
      // Delay and re-measure for debounce
      delay(100);
      if(!upPressed() || downPressed())
      {
        // If button state changes go back to top of evaluate
        return EVALUATE_STATE;
      }

      Serial.println("User has requested one additional kernel during stale evaluate state");

      // Remove evaluateUpdate flag
      evaluateUpdate = false;

      // Add one kernel
      TrickleDispense(1);

      Serial.println("Kernel dispensed, returning to top of Evaluate state");
      // Return to top of Evaluate state to assess status after kernel was added
      return EVALUATE_STATE;
    }
  }

  // Update evaluateWeight and weightDiff as necessary TODO: might want to eliminate this portion and use a single shared currentWeight that is managed in the main function?
  // First entry to Evaluate state
  if(firstEvaluate)
  {
    evaluateUpdate = false;
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
    float tmpWeight = StableWeight(LONG);

    // Case 1, weight has changed
    // Test if new weight reading is different from the existing one
    if(tmpWeight != evaluateWeight)
    {
      // Case 1.1 - tmpWeight indicates user has removed the shot glass
      // Verify weight is less than -200 or greater than 500 (overflow error), since shot glass will weigh at least that much
      if(tmpWeight > 500 || tmpWeight < -200)
      {
        // Reset the evaluation flags
        firstEvaluate = true;
        evaluateUpdate = false;

        // Reset LEDs before exiting evaluate
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(RED_LED, LOW);

        // Return to Ready state
        return READY_STATE;
      }
      // Case 1.2 - Weight has changed without user removing the shot glass
      // Verify this changed weight with a second measurement
      else if(StableWeight(LONG) == tmpWeight)
      {
        evaluateUpdate = false;

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
    evaluateUpdate = false;
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
  if(weightDiff < (-1.2 * errorMargin))
  {
    // Change the display
    Serial.println("Overthrow detected in Evaluate State");
    OverthrowScreen(targetWeight, evaluateWeight, elapsedTime, errorMargin);

    // Illuminate the Red LED after turning the others off
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  }
  // Case 2 - correct weight (0.02 under is acceptable as a good throw)
  else if(weightDiff < 0.021)
  {
    // Change the display
    Serial.println("Acceptable charge detected in Evaluate State");
    GoodChargeScreen(targetWeight, evaluateWeight, elapsedTime, errorMargin);

    // Illuminate the Green LED after turning the others off
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
  }
  // Case 3 - underthrow
  else
  {
    // Handle extreme underthrow (this should never happen)
    if(weightDiff > 1)
    {
      // Reset LEDs before exiting evaluate (yellow plus red for extreme underthrow)
      LowChargeScreen(targetWeight, evaluateWeight, elapsedTime, errorMargin);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(YELLOW_LED, HIGH);
      digitalWrite(RED_LED, HIGH);

      // Set evaluateUpdate flag
      evaluateUpdate = true;

      Serial.println("Extreme underthrow error during evaluate");
      return EVALUATE_STATE;
    }
    // Underthrow by more than 0.02gr (which tests as a good throw above), but less than the calibrated kernel weight
    else if(weightDiff <= (1.2 * kernelWeight))
    {
      // Reset LEDS before exiting evaluate (both green and yellow illuminated for this case)
      StaleChargeScreen(targetWeight, evaluateWeight, elapsedTime, errorMargin);
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(YELLOW_LED, HIGH);
      digitalWrite(RED_LED, LOW);

      // Set evaluateUpdate flag
      evaluateUpdate = true;

      Serial.println("Sub-kernel underthrow detected in Evaluate state, changing to light to indicate caution but remaining in Evaluate state");
      return EVALUATE_STATE;
    }
    else
    {
      // Reset LEDs before exiting evaluate (yellow only for true underthrow)
      LowChargeScreen(targetWeight, evaluateWeight, elapsedTime, errorMargin);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(YELLOW_LED, HIGH);
      digitalWrite(RED_LED, LOW);

      // Set evaluateUpdate flag
      evaluateUpdate = true;

      Serial.println("Underthrow detected in Evaluate state, changing to light to indicate caution but remaining in Evaluate state");
      return EVALUATE_STATE;
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
  // Do not allow dispensing of more than 250 grains of powder
  if(grains > 250)
  {
    return false;
  }

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
      Serial.println("Enable toggled off during bulk, stopping motors and ending their movement");
      StopMotors();
      
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
      Serial.println("Enable toggled off during trickle, stopping motors and ending their movement");
      StopMotors();

      EndTrickle();
      EndBulk();
      return false;
    }
    // Stop the trickle if the weight goes below zero at any time
    if(StableWeight(50) < 0)
    {
      Serial.println("Cup removed during trickle, stopping motors and ending their movement");
      StopMotors();

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

void smallIncreaseBulkCalibration()
{
  float curRevs = GetBulkWeight();
  SetBulkWeight(curRevs * 1.01);

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

void smallDecreaseBulkCalibration()
{
  float curRevs = GetBulkWeight();
  SetBulkWeight(curRevs * 0.99);

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

void smallIncreaseTrickleCalibration()
{
  // Adjust kernel weight to be slightly larger
  float curKernel = GetKernelWeight();
  SetKernelWeight(curKernel * 1.01);

  Serial.print("Adjusting calibration up by 1%, kernelWeight = ");
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

void smallDecreaseTrickleCalibration()
{
  // Adjust kernel weight to be slightly smaller
  float curKernel = GetKernelWeight();
  SetKernelWeight(curKernel * 0.99);

  Serial.print("Adjusting calibration down by 1%, kernelWeight = ");
  Serial.println(GetKernelWeight(), 6);
}
