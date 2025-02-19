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
#define VERSION_MINOR 8
// Display constants
#define LCD_COLS 20
#define LCD_ROWS 4

// LcdSetup()
// Initializes the LCD and prints the initial splash screen displayed during setup state
// Returns true on success, or false on failure
bool LcdSetup();
void WaitingToCalibrate();
void CalibrationScreen();

void IdleScreen(float targetWeight, float errorMargin);
void ReadyScreen(float targetWeight, float errorMargin);

void BulkScreen(float targetWeight, float errorMargin);
void TrickleScreen(float targetWeight, float errorMargin);

void GoodChargeScreen(float targetWeight, float finalWeight, int duration, float errorMargin);
void OverthrowScreen(float targetWeight, float finalWeight, int duration, float errorMargin);
void StaleChargeScreen(float targetWeight, float finalWeight, int duration, float errorMargin);

void noErrorTopLines(float errorMargin);
void eraseTopLines();

void clearLine(int line);

#endif // DISPLAY_H
