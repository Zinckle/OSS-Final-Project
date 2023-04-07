#pragma once
#include <Arduino.h>

void init_shift(byte dataPin, byte clockPin, byte latchPin);
void myShiftOut(byte dataPin, byte clockPin, byte bitOrder, byte val);