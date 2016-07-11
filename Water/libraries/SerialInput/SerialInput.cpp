/*
  SerialInput.h - Simple library for Arduino.
  Handles input of numeric data in text form via serial interface.
  Copyleft (c) 2009 Giordano Bruno.
  Mailto: GiordanoFilippoBruno on server gmail.com

  This library is totally free software; you can do anything with it.
*/

#include "HardwareSerial.h"
#include "SerialInput.h"

CSerialInput::CSerialInput() {
  // set default values
  NumberEntered = false;
  DefaultNumber = 0;
  EchoOn = true;
}

long int CSerialInput::InputNumber() {
  char c;
  long int n = 0;
  bool bMinus = false;

  NumberEntered = false;
  do {
    while (!Serial.available());
    c = Serial.read();
    if (EchoOn) Serial.print(c);
    if ( (c>='0') && (c<='9') ) {
      n = n*10 + c - '0';
      NumberEntered = true;
    }
    if ((c=='-') && (!NumberEntered)) bMinus = true;
  } while (c != 13);
//  if (EchoOn) Serial.print(10);
  if (bMinus) n = 0 - n;
  if (!NumberEntered) n = DefaultNumber;
  return n;
}

CSerialInput SerialInput;
