// Scale.h
// Includes functions for commanding and reading the fx-120i

#ifndef SCALE_H
#define SCALE_H

// External libraries
#include <Arduino.h> // Standard Arduino libraries

// Define the scale serial pins
#define TX_PIN 11
#define RX_PIN 12

void SetupScale();
float StableWeight(int durationMillis);
float ReadScale();

void flushSerial();
void zeroScale();
#endif // SCALE_H