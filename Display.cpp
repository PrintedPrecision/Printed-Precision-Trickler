// Display.cpp
// Contains implementations of functions declared in Display.h

// Include the header file
#include "Display.h"

// Declare lcd object: auto locate & auto config expander chip
hd44780_I2Cexp lcd; 

// Commonly used display lines
#define LINE1 (" Printed Precision  ")
#define LINE2 ("Software Version ")


// LcdSetup()
// Initializes the LCD and prints the initial splash screen displayed during setup state
bool LcdSetup()
{
  int status;
  bool result = false;

  // Start the LCD and turn on the backlight
  lcd.begin(LCD_COLS, LCD_ROWS);

  // Print display lines 1 and 2
  noErrorTopLines();

  // Print display line 3
  lcd.setCursor(0,2);
  lcd.print("    Initializing    ");

  // Print display line 4
  lcd.setCursor(0,3);
  lcd.print("  Connecting Scale  ");

  result = true;

  return result;
}

// WaitingToCalibrate()
// Displayed during Setup state, while waiting for the enable toggle to be pressed before exiting Setup state
void WaitingToCalibrate()
{
  // Print display lines 1 and 2
  noErrorTopLines();

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
  noErrorTopLines();

  // Print display line 3
  clearLine(2);

  // Print display line 4
  lcd.setCursor(0,3);
  lcd.print(" Calibrating Powder ");
}

// IdleScreen()
// Displayed during the idle state
void IdleScreen(float targetWeight)
{
  // Print display lines 1 and 2
  noErrorTopLines();

  // Print display line 3
  clearLine(2);
  lcd.print("   Target = ");
  lcd.print(targetWeight);

  // Print display line 4
  lcd.setCursor(0,3);
  lcd.print("  Waiting to Start  ");
}

// ReadyScreen()
// Displayed during the ready state
void ReadyScreen(float targetWeight)
{
  // Print display lines 1 and 2
  noErrorTopLines();

  // Print display line 3
  clearLine(2);
  lcd.print("   Target = ");
  lcd.print(targetWeight);

  // Print display line 4
  lcd.setCursor(0,3);
  lcd.print(" Ready for Dispense ");
}

// BulkScreen()
// Displayed during the dispense state when bulk dispensing
void BulkScreen(float targetWeight)
{
  // Print display lines 1 and 2
  noErrorTopLines();

  // Print display line 3
  clearLine(2);
  lcd.print("   Target = ");
  lcd.print(targetWeight);

  // Print display line 4
  lcd.setCursor(0,3);
  lcd.print("  Rapid Dispensing  ");
}

// TrickleScreen()
// Displayed during the dispense state when trickling
void TrickleScreen(float targetWeight)
{
  // Print display lines 1 and 2
  noErrorTopLines();

  // Print display line 3
  clearLine(2);
  lcd.print("   Target = ");
  lcd.print(targetWeight);

  // Print display line 4
  lcd.setCursor(0,3);
  lcd.print("    Fine Trickle    ");
}

void GoodChargeScreen(float targetWeight, float finalWeight, int duration)
{
  // Print the 2nd display line
  lcd.setCursor(0,1);
  lcd.print(" Trickle Completed! ");

  // Clear and print 3rd display line
  clearLine(2);
  lcd.print("  Dispensed = ");
  lcd.print(finalWeight);

  // Clear and then print the 4th display line
  clearLine(3);
  lcd.print(" Throw Time = ");
  lcd.print(duration);
}

void OverthrowScreen(float targetWeight, float finalWeight, int duration)
{
  // Print 2nd display line
  lcd.setCursor(0,1);
  lcd.print(" OVERTHROW WARNING! ");

  // Clear and print 3rd display line
  clearLine(2);
  lcd.print("  Dispensed = ");
  lcd.print(finalWeight);

  // Clear and then print the 4th display line
  clearLine(3);
  lcd.print(" Throw Time = ");
  lcd.print(duration);
}

// noErrorTopLines()
// Prints the top two lines for no-error screens (Brand + version info)
void noErrorTopLines()
{
  lcd.setCursor(0,0);
  lcd.print(LINE1);
  
  lcd.setCursor(0,1);
  lcd.print(LINE2);
  lcd.print(VERSION_MAJOR);
  lcd.print(".");
  lcd.print(VERSION_MINOR);
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