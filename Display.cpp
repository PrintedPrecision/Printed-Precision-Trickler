// Display.cpp
// Contains implementations of functions declared in Display.h

// Include the header file
#include "Display.h"

// Declare lcd object: auto locate & auto config expander chip
hd44780_I2Cexp lcd; 

// Commonly used display lines
#define LINE1 (" Printed  Precision ")
#define LINE2 ("   Software v")


// LcdSetup()
// Initializes the LCD and prints the initial splash screen displayed during setup state
bool LcdSetup(char* response)
{
  int status;
  bool result = false;

  // Start the LCD and turn on the backlight
  lcd.begin(LCD_COLS, LCD_ROWS);

  // Print display lines 1 and 2
  lcd.setCursor(0,0);
  lcd.print(LINE1);
  
  lcd.setCursor(0,1);
  lcd.print(LINE2);
  lcd.print(VERSION_MAJOR);
  lcd.print(".");
  lcd.print(VERSION_MINOR);

  // Print display line 3
  //lcd.setCursor(0,2);
  //lcd.print("    Initializing    ");

  // Print display line 4
  lcd.setCursor(0,2);
  lcd.print("  Connecting Scale  ");

  // Print out the scale response
  lcd.setCursor(0,3);
  lcd.print("                    ");
  lcd.setCursor(0,3);
  lcd.print(response);


  result = true;

  return result;
}

// WaitingToCalibrate()
// Displayed during Setup state, while waiting for the enable toggle to be pressed before exiting Setup state
void WaitingToCalibrate()
{
  // Print display lines 1 and 2
  lcd.setCursor(0,0);
  lcd.print(LINE1);
  
  lcd.setCursor(0,1);
  lcd.print(LINE2);
  lcd.print(VERSION_MAJOR);
  lcd.print(".");
  lcd.print(VERSION_MINOR);

  // Print display line 3
  lcd.setCursor(0,2);
  lcd.print("    Press Enable    ");

  // Print display line 4
  lcd.setCursor(0,3);
  lcd.print("    To Calibrate    ");
}

// CalibrationScreen()
// Displayed during the calibration state
void CalibrationScreen()
{
  // Print display lines 1 and 2
  lcd.setCursor(0,0);
  lcd.print(LINE1);
  
  lcd.setCursor(0,1);
  lcd.print(LINE2);
  lcd.print(VERSION_MAJOR);
  lcd.print(".");
  lcd.print(VERSION_MINOR);

  // Print display line 3
  clearLine(2);

  // Print display line 4
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print(" Calibrating Powder ");
}

void StageOneBulk(float measured, float assigned)
{
  // Print display lines 1 and 2
  lcd.setCursor(0,0);
  lcd.print("   Stage One Bulk   ");

  clearLine(1);
  lcd.print("Expected = 20-150   ");

  // Print display line 3
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("Measured = ");
  lcd.print(measured, 4);
  
  // Print display line 4
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print("Assigned = ");
  lcd.print(assigned, 4);
}

void Trickle(float measured, float assigned)
{
  // Print display lines 1 and 2
  lcd.setCursor(0,0);
  lcd.print("      Trickler      ");

  clearLine(1);
  lcd.setCursor(0,1);
  lcd.print("Expected = 0.01-0.10");

  // Print display line 3
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("Measured = ");
  lcd.print(measured, 4);
  
  // Print display line 4
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print("Assigned = ");
  lcd.print(assigned, 4);
}

//CalibrationComplete()
// Displayed after completion of calibration to provide calibration values to the user
void CalibrationComplete(float bulk, float kernel)
{
  // Print display lines 1 and 2
  lcd.setCursor(0,0);
  lcd.print(" Calibration  Ended ");
  
  clearLine(1);
  lcd.setCursor(0,1);
  lcd.print(" Disable to Proceed ");

  // Print display line 3
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("    Bulk = ");
  lcd.print(bulk);

  // Print display line 4
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print("  Kernel = ");
  lcd.print(kernel, 3);
  lcd.setCursor(17,3);
  lcd.print("gr  ");
}

// IdleScreen()
// Displayed during the idle state
void IdleScreen(float targetWeight, float errorMargin)
{
  // Print display lines 1 and 2
  noErrorTopLines(errorMargin);

  // Print display line 3
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("   Target = ");
  lcd.print(targetWeight);

  // Print display line 4
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print("  Waiting to Start  ");
}

// ReadyScreen()
// Displayed during the ready state
void ReadyScreen(float targetWeight, float errorMargin)
{
  // Print display lines 1 and 2
  noErrorTopLines(errorMargin);

  // Print display line 3
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("   Target = ");
  lcd.print(targetWeight);

  // Print display line 4
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print(" Ready for Dispense ");
}

// BulkScreen()
// Displayed during the dispense state when bulk dispensing
void BulkScreen(float targetWeight, float errorMargin)
{
  // Print display lines 1 and 2
  noErrorTopLines(errorMargin);

  // Print display line 3
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("   Target = ");
  lcd.print(targetWeight);

  // Print display line 4
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print("  Rapid Dispensing  ");
}

// TrickleScreen()
// Displayed during the dispense state when trickling
void TrickleScreen(float targetWeight, float errorMargin)
{
  // Print display lines 1 and 2
  noErrorTopLines(errorMargin);

  // Print display line 3
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("   Target = ");
  lcd.print(targetWeight);

  // Print display line 4
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print("    Fine Trickle    ");
}

void GoodChargeScreen(float targetWeight, float finalWeight, int duration, float errorMargin)
{
  // Print the 1st display line
  lcd.setCursor(0,0);
  lcd.print(LINE1);

  // Print the 2nd display line
  lcd.setCursor(0,1);
  lcd.print(" Trickle Completed! ");

  // Clear and print 3rd display line
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("  Dispensed = ");
  lcd.print(finalWeight);

  // Clear and then print the 4th display line
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print(" Throw Time = ");
  lcd.print(duration);
}

void OverthrowScreen(float targetWeight, float finalWeight, int duration, float errorMargin)
{
  // Print the 1st display line
  lcd.setCursor(0,0);
  lcd.print(LINE1);

  // Print 2nd display line
  lcd.setCursor(0,1);
  lcd.print(" OVERTHROW WARNING! ");

  // Clear and print 3rd display line
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("  Dispensed = ");
  lcd.print(finalWeight);

  // Clear and then print the 4th display line
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print(" Throw Time = ");
  lcd.print(duration);
}

void StaleChargeScreen(float targetWeight, float finalWeight, int duration, float errorMargin)
{
  // Print the 1st display line
  lcd.setCursor(0,0);
  lcd.print(LINE1);

  // Print 2nd display line
  lcd.setCursor(0,1);
  lcd.print("  CHANGE DETECTED!  ");

  // Clear and print 3rd display line
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("  Dispensed = ");
  lcd.print(finalWeight);

  // Clear and then print the 4th display line
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print("Push ^ To Add Kernel");
}

void LowChargeScreen(float targetWeight, float finalWeight, int duration, float errorMargin)
{
  // Print the 1st display line
  lcd.setCursor(0,0);
  lcd.print(LINE1);

  // Print 2nd display line
  lcd.setCursor(0,1);
  lcd.print("UNDERTHROW DETECTED!");

  // Clear and print 3rd display line
  clearLine(2);
  lcd.setCursor(0,2);
  lcd.print("  Dispensed = ");
  lcd.print(finalWeight);

  // Clear and then print the 4th display line
  clearLine(3);
  lcd.setCursor(0,3);
  lcd.print("Push ^ To Add Kernel");
}

// noErrorTopLines()
// Prints the top two lines for no-error screens (Brand + version info)
void noErrorTopLines(float errorMargin)
{
  lcd.setCursor(0,0);
  lcd.print(LINE1);
  
  lcd.setCursor(0,1);
  lcd.print("v");
  lcd.print(VERSION_MAJOR);
  lcd.print(".");
  lcd.print(VERSION_MINOR);
  lcd.print("  Error = ");
  lcd.print(errorMargin);
  lcd.print("gr");
}

void eraseTopLines()
{
  lcd.setCursor(0,0);
  lcd.print("                    ");
  lcd.setCursor(0,1);
  lcd.print("                    ");
}

// Clears the selected display line and resets the cursor to the start of that line
void clearLine(int line)
{
  lcd.setCursor(0, line);
  lcd.print("                    ");
  lcd.setCursor(0, line);
}