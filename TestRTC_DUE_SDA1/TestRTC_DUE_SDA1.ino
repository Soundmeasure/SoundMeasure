// Date, Time and Alarm functions using a DS3231 RTC connected via I2C and Wire lib
// The RTC can wake up the Arduino, if the Arduino has been put to sleep (to save power). 
// This can be a handy function for battery operated units.
//
// Hardware: connect RTC SQW/INT pin to Arduino pin2
// during an alarm the RTC SQW/INT pin is pulled low

#include <Wire.h>
#include <RTClib.h>

RTC_DS3231  DS3231_clock;

DateTime dt;


int alarm_pin = 2;
volatile int state = LOW;

volatile bool alarm_enable = false;
volatile int alarm_synhro = 0;
volatile bool start_synhro = false;


void alarmFunction()
{
	alarm_enable = true;
	//DS3231_clock.clearAlarm1();
	dt = DS3231_clock.now();
	if (alarm_synhro >1)
	{
		alarm_synhro = 0;
		start_synhro = true;
	//	StartSynhro = micros();                                            // Записать время
	//	if (measure_enable) Timer7.start(scale_strob * 1000);              // Включить формирование  временных меток на экране

	}
	alarm_synhro++;
	alarm_enable = false;
}





void setup() 
{
	pinMode(alarm_pin, INPUT_PULLUP); //internal pullup on the Arduino ensures that we can detect the LOW from the SQW/INT pin of the RTC

	Serial.begin(115200);

	if (!DS3231_clock.begin()) 
	{
		Serial.println("Couldn't find RTC");
		while (1);   // sketch halts in an endless loop
	}


	if (!DS3231_clock.isrunning()) {
		Serial.println("RTC is NOT running!");
		//set the RTC date & time - to the time this sketch was compiled
		DS3231_clock.adjust(DateTime(__DATE__, __TIME__));
		// set the RTC date & time - to any time you choose example 
		// rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));      // would set to January 21, 2014 at 3:00:00 (3am)
		// rtc.adjust(DateTime(2015, 2, 28, 14, 50, 00));   // would set to February 28, 2015 at 14:50:00 (2:50pm)
	}


	dt = DS3231_clock.now();
	DS3231_clock.setAlarm1Simple(dt.hour(), dt.minute() + 1);
//	DS3231_clock.setA1Time(dt.minute());
	DS3231_clock.turnOnAlarm(1);
	if (DS3231_clock.checkAlarmEnabled(1))
	{
	    Serial.println("Alarm Enabled in 1 minutes");
	}
	Serial.print(dt.hour(), DEC);
	Serial.print(':');
	Serial.print(dt.minute(), DEC);
	Serial.print(':');
	Serial.println(dt.second(), DEC);

	//get temperature 
	float c = DS3231_clock.getTemperature();

	//print temperature
	Serial.print(F("(chip) temperature is "));
	Serial.println(c);
	attachInterrupt(alarm_pin, alarmFunction, FALLING);        // прерывание вызывается только при смене значения на порту с LOW на HIGH
}

void loop() 
{
	dt = DS3231_clock.now();
	DS3231_clock.setAlarm1Simple(dt.hour(), dt.minute() + 1);
	DS3231_clock.turnOnAlarm(1);
	//if (DS3231_clock.checkAlarmEnabled(1))
	//{
	//	Serial.println("Alarm Enabled in 1 minutes");
	//}
	Serial.print(dt.hour(), DEC);
	Serial.print(':');
	Serial.print(dt.minute(), DEC);
	Serial.print(':');
	Serial.println(dt.second(), DEC);
	//if (DS3231_clock.checkAlarmEnabled(1)) Serial.println("A1 Enabled"); else  Serial.println("A1 Disbled");

	//Serial.flush(); // empty the send buffer, before continue with; going to sleep

	// check (in software) to see if Alarm 1 has been triggered
	if (DS3231_clock.checkIfAlarm(1)) 
	{
		Serial.println("Alarm 1 Triggered");
	}

	if (start_synhro)
	{
		dt = DS3231_clock.now();
		Serial.print(dt.hour(), DEC);
		Serial.print(':');
		Serial.print(dt.minute(), DEC);
		Serial.print(':');
		Serial.println(dt.second(), DEC);
		start_synhro = false;
	}


	delay(1000);
}
//
//void sleepNow() {
//  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
//  sleep_enable();
//  attachInterrupt(0,logData, LOW); //make sure we can be made to wake up :-) 
//  sleep_mode();
//  //CONTINUE HERE AFTER WAKING UP
//  sleep_disable();
//  detachInterrupt(0); //avoid random interrupts
//  
//}
