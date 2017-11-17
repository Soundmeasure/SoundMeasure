
/*

Начало работ 21.08.2017г.


Восстановление 11.11.2017г.

Изменение 14.11.2017г.

*/

#define __SAM3X8E__


#include <SPI.h>
#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <DueTimer.h>
#include "Wire.h"
#include <DS3231.h>
#include "nRF24L01.h"
#include "RF24.h"



extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

// Настройка монитора

UTFT myGLCD(TFT01_28, 38, 39, 40, 41);     // Настройка монитора
										   //UTFT myGLCD(ITDB28, 38, 39, 40, 41);     // Настройка монитора
UTouch        myTouch(6, 5, 4, 3, 2);          // Настройка клавиатуры

UTFT_Buttons  myButtons(&myGLCD, &myTouch);
boolean default_colors = true;
uint8_t menu_redraw_required = 0;


#define intensityLCD      9                         // Порт управления яркостью экрана
#define synhro_pin       66                         // Порт синхронизации модулей
#define alarm_pin         7                         // Порт прерывания по таймеру  DS3231
#define kn_red           43                         // AD4 Кнопка красная +
#define kn_blue          42                         // AD6 Кнопка синяя -
#define vibro            11                         // Вибромотор
#define sounder          53                         // Зуммер
#define LED_PIN13        13                         // Индикация
#define strob_pin        A4                         // Вход для синхронизации Базы и  Трансмиттера





// -------------------   Настройка часового таймера DS3231 -------------------

int oldsec = 0;
//char* str_mon[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

DS3231 DS3231_clock;
RTCDateTime dt;
//char* daynames[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };



boolean isAlarm = false;
boolean alarmState = false;
int alarm_synhro = 0;
unsigned long alarm_count = 0;



void sound1()
{
	digitalWrite(sounder, HIGH);
	delay(100);
	digitalWrite(sounder, LOW);
	delay(100);
}


 void alarmFunction()
{
	isAlarm = false;
	DS3231_clock.clearAlarm1();
	dt = DS3231_clock.getDateTime();
	myGLCD.setBackColor(0, 0, 0);                   // Синий фон кнопок
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(SmallFont);
	myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s - ", dt), 10, 0);
	alarm_synhro++;
	if (alarm_synhro >4)
	{
		alarm_synhro = 0;
		alarm_count++;
		myGLCD.print(DS3231_clock.dateFormat("s", dt), 190, 0);
	}
	isAlarm = true;
}








void setup()
{
	Serial.begin(115200);

	Serial.println(F("Setup start!"));

	pinMode(kn_red, INPUT);
	pinMode(kn_blue, INPUT);
	pinMode(intensityLCD, OUTPUT);
	pinMode(synhro_pin, INPUT);
	pinMode(LED_PIN13, OUTPUT);
	digitalWrite(LED_PIN13, LOW);
	digitalWrite(intensityLCD, LOW);
	pinMode(strob_pin, INPUT);
	digitalWrite(strob_pin, HIGH);
	digitalWrite(synhro_pin, HIGH);
	pinMode(alarm_pin, INPUT);
	digitalWrite(alarm_pin, HIGH);
	pinMode(vibro, OUTPUT);
	pinMode(sounder, OUTPUT);
	digitalWrite(vibro, LOW);
	digitalWrite(sounder, LOW);
	pinMode(kn_red, OUTPUT);
	pinMode(kn_blue, OUTPUT);
	digitalWrite(kn_red, HIGH);
	digitalWrite(kn_blue, HIGH);

	Wire.begin();
	myGLCD.InitLCD();
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);                   // Синий фон кнопок
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(BigFont);

	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);
	//myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
	DS3231_clock.begin();
	// Disarm alarms and clear alarms for this example, because alarms is battery backed.
	// Under normal conditions, the settings should be reset after power and restart microcontroller.
	DS3231_clock.armAlarm1(false);
	DS3231_clock.armAlarm2(false);
	DS3231_clock.clearAlarm1();
	DS3231_clock.clearAlarm2();

	DS3231_clock.setDateTime(__DATE__, __TIME__);
	// Enable output
	DS3231_clock.enableOutput(false);

	// Check config

	if (DS3231_clock.isOutput())
	{
		Serial.println("Oscilator is enabled");
	}
	else
	{
		Serial.println("Oscilator is disabled");
	}

	switch (DS3231_clock.getOutput())
	{
	case DS3231_1HZ:     Serial.println("SQW = 1Hz"); break;
	case DS3231_4096HZ:  Serial.println("SQW = 4096Hz"); break;
	case DS3231_8192HZ:  Serial.println("SQW = 8192Hz"); break;
	case DS3231_32768HZ: Serial.println("SQW = 32768Hz"); break;
	default: Serial.println("SQW = Unknown"); break;
	}

	DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);   //DS3231_EVERY_SECOND //Каждую секунду
	attachInterrupt(alarm_pin, alarmFunction, FALLING);





	Serial.println(F("Setup Ok!"));
}

void loop()
{

  /* add main program code here */

}
