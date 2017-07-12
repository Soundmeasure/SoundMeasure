#include <rtc_clock.h>

// Select the Slowclock source
//RTC_clock rtc_clock(RC);
RTC_clock rtc_clock(XTAL);

char* daynames[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

const int ledPin = 13;
const int ledPinA0 = 54;
const int ledPinA1 = 55;
const int ledPinA2 = 56;

int ledState = LOW;             // ledState used to set the LED

								// Generally, you should use "unsigned long" for variables that hold time
								// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

										 // constants won't change :
const long interval = 1000;           // interval at which to blink (milliseconds)
int thisByte = 33;

//#define Serial SerialUSB


void setup() {
	Serial.begin(9600);
	rtc_clock.init();
	rtc_clock.set_time(10, 55, 9);
	rtc_clock.set_date(12, 07, 2017);
	Serial1.begin(9600);
	Serial2.begin(9600);
	Serial3.begin(9600);

	pinMode(ledPin, OUTPUT);
	pinMode(ledPinA0, OUTPUT);
	pinMode(ledPinA1, OUTPUT);
	pinMode(ledPinA2, OUTPUT);

	// prints title with ending line break
	Serial.println("ASCII Table ~ Character Map");

}

void loop() 
{

	Serial.write(thisByte);

	Serial.print(", dec: ");
	// prints value as string as an ASCII-encoded decimal (base 10).
	// Decimal is the  default format for Serial.print() and Serial.println(),
	// so no modifier is needed:
	//Serial.print(thisByte);
	// But you can declare the modifier for decimal if you want to.
	//this also works if you uncomment it:

	Serial.print(thisByte, DEC);


	Serial.print(", hex: ");
	// prints value as string in hexadecimal (base 16):
	Serial.print(thisByte, HEX);

	Serial.print(", oct: ");
	// prints value as string in octal (base 8);
	Serial.print(thisByte, OCT);

	Serial.print(", bin: ");
	// prints value as string in binary (base 2)
	// also prints ending line break:
	Serial.println(thisByte, BIN);

	// if printed last visible character '~' or 126, stop:
	if (thisByte == 126) {    // you could also use if (thisByte == '~') {
							  // This loop loops forever and does nothing
							  /*	while (true) {
							  continue;
							  }
							  */
		thisByte = 33;
	}
	// go on to the next character
	thisByte++;

	// here is where you'd put code that needs to be running all the time.

	// check to see if it's time to blink the LED; that is, if the
	// difference between the current time and last time you blinked
	// the LED is bigger than the interval at which you want to
	// blink the LED.
	unsigned long currentMillis = millis();

	if (currentMillis - previousMillis >= interval) {
		// save the last time you blinked the LED
		previousMillis = currentMillis;

		// if the LED is off turn it on and vice-versa:
		if (ledState == LOW) {
			ledState = HIGH;
		}
		else {
			ledState = LOW;
		}

		// set the LED with the ledState of the variable:
		digitalWrite(ledPin, ledState);
		digitalWrite(ledPinA0, ledState);
		delay(50);
		digitalWrite(ledPinA1, ledState);
		delay(50);
		digitalWrite(ledPinA2, ledState);
		delay(50);

		Serial.println();
		Serial.print("At the third stroke, it will be ");
		Serial.print(rtc_clock.get_hours());
		Serial.print(":");
		Serial.print(rtc_clock.get_minutes());
		Serial.print(":");
		Serial.println(rtc_clock.get_seconds());
		Serial.print(daynames[rtc_clock.get_day_of_week() - 1]);
		Serial.print(": ");
		Serial.print(rtc_clock.get_days());
		Serial.print(".");
		Serial.print(rtc_clock.get_months());
		Serial.print(".");
		Serial.println(rtc_clock.get_years());
		Serial.println();



	}




	//delay(1000);
}
