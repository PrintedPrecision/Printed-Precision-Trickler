// Display.h
// This library provides convenient functions for controlling the display
// Includes functions for one-step writing of static display layouts (such as during setup/error states)
// as well as functions to update values in dynamic display layouts (such as during idle/ready/run/evaluate states)

#ifndef DISPLAY_H
#define DISPLAY_H

// External libraries
#include <Arduino.h> // Standard Arduino libraries
#include <Wire.h> // Needed by LCD library
#include <hd44780.h> // LCD screen
#include <hd44780ioClass/hd44780_I2Cexp.h> // LCD screen i2c expansion header

// Internal libraries
//#include "Steppers.h"

// Current software version
#define VERSION_MAJOR 0
#define VERSION_MINOR 5
// Display constants
#define LCD_COLS 20
#define LCD_ROWS 4

// LcdSetup()
// Initializes the LCD and prints the initial splash screen displayed during setup state
// Returns true on success, or false on failure
bool LcdSetup();
void WaitingToCalibrate();
void CalibrationScreen();

void IdleScreen(float targetWeight);
void ReadyScreen(float targetWeight);

void BulkScreen(float targetWeight);
void TrickleScreen(float targetWeight);

void GoodChargeScreen(float targetWeight, float finalWeight, int duration);
void OverthrowScreen(float targetWeight, float finalWeight, int duration);

void noErrorTopLines();
void eraseTopLines();

void clearLine(int line);

#endif // DISPLAY_H