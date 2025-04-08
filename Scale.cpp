// Scale.cpp
// Contains implementations of functions declared in Scale.cpp

// Include the header file
#include "Scale.h"

// Persistent weight variables
float latestWeight;

// SetupScale()
// Begins serial communications with the scale
// Waits until serial number is received from the scale before returning
// The received scale serial number is printed to the serial monitor
void SetupScale()
{
  // Begin serial comms with scale at selected baudrate
  Serial1.begin(19200);

  // Try to request the serial number of scale until it eventually responds, flushing old or partial commands from its memory
  do
  {
    Serial1.write("?ID\r");

    delay(50);
  } while(!Serial1.available());

  Serial.print("Scale serial number is '");
  while(Serial1.available())
  {
    Serial.print(Serial1.read());
  }
  Serial.println("'");
}

// StableWeight(int millis)
// Waits until the weight has been stable across the given timespan and then returns the stabilized weight value in float format
// Error values:
float StableWeight(int durationMillis)
{
  bool success = false;
  float tempWeight;
  long tempTime;

  float retVal = ReadScale();
  long lastTime = millis();

  float newVal = retVal;
  long newTime = lastTime;


  // While loop to sit in until weight reading stabilizes
  while(!success)
  {
    // Capture current weight into tempWeight and current time into tempTime
    tempWeight = ReadScale();
    tempTime = millis();

    // Scale response timed out
    if(tempWeight == -5000)
    {
      return tempWeight;
    }
    // Scale response contains out of range characters
    if(tempWeight == -6000)
    {
      return tempWeight;
    }

    // Compare tempWeight to retVal to see if it's changed
    if(tempWeight != retVal)
    {
      // Weight has changed, check if it's the same as newVal
      if(tempWeight != newVal)
      {
        // Weight is an entirely new value, rotate newVal/newTime into retVal/lastTime and shift tempWeight/tempTime into newVal/newTime
        retVal =  newVal;
        lastTime = newTime;

        newVal = tempWeight;
        newTime = tempTime;
      }
      else
      {
        // Weight is same as newVal, check if duration has elapsed (comparing temp to new)
        if((tempTime - newTime) > durationMillis)
        {
          // Duration has elapsed, newVal is the stabilized weight to be returned
          return newVal;
        }
      }
    }
    else
    {
      // Weight is the same as retVal, check if duration has elapsed (comparing temp to last)
      if((tempTime - lastTime) > durationMillis)
      {
        // Duration has elapsed, retVal is the stabilized weight to be returned
        return retVal;
      }
    }
  } // This while loop never ends
}

// readScale()
// Reads the current weight from the scale and returns the weight value in float format
// Error values:
// -5000 = Scale response timeout
// -6000 = Scale response error (out of range characters, likely a formatting issue)
float ReadScale()
{
  String asciiNum;
  char byteReceived;
  int i = 0;
  bool isNegative = false;

  flushSerial();

  // Command the scale to report the current weight WITHOUT blinking the display 
  Serial1.print("PRT\r");

  int startTime = millis();
  int curTime = startTime;

  // Loop until we have received the full scale response
  while(i < 10 && (curTime - startTime) < 500)
  {
    i = Serial1.available();
    curTime = millis();
  }

  // Handle timeout of scale response
  if((curTime - startTime) >= 500)
  {
    Serial.println("Scale response timed out during readScale()");
    return -5000;
  }
  
  // Collect the available response from the scale
  for(int j = 0; j < i; j++)
  {
    byteReceived = Serial1.read();
    //Serial.print(byteReceived);
    //Serial.print(" -- ");
    //Serial.print(byte(byteReceived));
    //Serial.print("\n");

    // Test first received character to see if number is negative
    if(j == 0)
    {
      byte test = byte(byteReceived);
      if(test == 45)
      {
        isNegative = true;
      }
    }
    // Store the portion that is the number into a char array
    if(j > 0 && j < 9)
    {
      asciiNum += byteReceived;

      // Verify that each of these are a number or decimal point, return false early if they aren't
      byte test = byte(byteReceived);
      if(test < 46 || test > 57)
      {
        Serial.println("Error receiving weight from scale, response includes out of range characters");
        return -6000;
      }
    }
  }

  latestWeight = asciiNum.toFloat();

  if(isNegative)
  {
    latestWeight = latestWeight * -1;
  }

  return latestWeight;
}

// flushSerial()
// Reads from the RX buffer for the scale until nothing is left in the buffer
void flushSerial()
{
  while(Serial1.available())
  {
    Serial.print(Serial1.available());
    Serial1.read();
  }
  return;
}

// zeroScale()
// Commands the scale to re-zero
void zeroScale()
{
  Serial1.print("R\r");

  delay(50);
}