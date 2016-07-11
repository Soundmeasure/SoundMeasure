/*
  SerialInput.h - Simple library for Arduino.
  Handles input of numeric data in text form via serial interface.
  Copyleft (c) 2009 Giordano Bruno.
  Mailto: GiordanoFilippoBruno on server gmail.com

  This library is totally free software; you can do anything with it.
*/

#ifndef SERIAL_INPUT_h
#define SERIAL_INPUT_h

class CSerialInput
{
  public:
    CSerialInput();          // default constructor
    bool NumberEntered;      // becomes true when number is entered
    long int DefaultNumber;  // default value when nothing entered
    bool EchoOn;             // if true, then echo is on
    long int InputNumber();  // inputs decimal number
};

extern CSerialInput SerialInput;

#endif  // #ifndef SERIAL_INPUT_h
