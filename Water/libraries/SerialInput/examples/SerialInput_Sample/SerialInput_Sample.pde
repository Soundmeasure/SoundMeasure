#include <SerialInput.h>

void setup() {
  Serial.begin(9600);
}

void loop()
{
  long int Number;

  // 1 - simple example
  Serial.print("Enter number: ");
  Number = SerialInput.InputNumber();	// Here we enter somthing

  Serial.print("You entered: ");
  Serial.println(Number, DEC);

  // 2 - here we check, if something entered
  Serial.print("Enter number: ");
  Number = SerialInput.InputNumber();

  if (SerialInput.NumberEntered) {
    Serial.print("You entered: ");
    Serial.println(Number, DEC);
  } else {
    Serial.println("You didn't entered anything");
  }

  // 3 - you can turn echo off, and change default value (it's
  // returned when nothing entered)
  SerialInput.EchoOn = false;
  SerialInput.DefaultNumber = -1;
  Serial.print("Enter number: ");
  Number = SerialInput.InputNumber();
  Serial.println();
  Serial.print("You entered: ");
  Serial.println(Number, DEC);
}
