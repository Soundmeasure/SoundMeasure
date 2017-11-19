//---------------------------------------
//Модуль NRF24L01 Подключение
//+++++++++++++++++++++++++++++++++++++++
//DUE              NRF24L01
//GND              1 GND
//VCC +3,3V        2 VCC +3,3V
//D10              3 CE
//D9               4 SCN
//SCK       	   5 SCK
//MOSI      	   6 MOSI
//MISO             7 MISO
//8                8 LED                         
//
//------------------------------------------
// Библиотека RF24 для работы с радио модулем nRF24L01+
/*
// March 2014 - TMRh20 - Updated along with High Speed RF24 Library fork
// Parts derived from examples by J. Coliz <maniacbug@ymail.com>
*/
/**
* Example for efficient call-response using ack-payloads
*
* This example continues to make use of all the normal functionality of the radios including
* the auto-ack and auto-retry features, but allows ack-payloads to be written optionally as well.
* This allows very fast call-response communication, with the responding radio never having to
* switch out of Primary Receiver mode to send back a payload, but having the option to if wanting
* to initiate communication instead of respond to a commmunication.
*/


#define __SAM3X8E__

#include "Wire.h"
#include <SPI.h>
#include <LCD5110_Graph.h>    
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <AH_AD9850.h>
#include <DueTimer.h>
#include "printf.h"
#include <DS3231.h>



//#include <LCD5110_Basic.h> // подключаем библиотеку

LCD5110 myGLCD(7, 6, 5, 4, 3); // объявляем номера пинов LCD

extern uint8_t SmallFont[]; // малый шрифт (из библиотеки)
extern uint8_t MediumNumbers[]; // средний шрифт для цифр (из библиотеки)

DS3231 DS3231_clock;
RTCDateTime dt;
volatile boolean isAlarm = false;
volatile boolean alarmState = false;
int alarm_synhro = 0;
unsigned long alarm_count = 0;
#define alarm_pin 47                                // Порт прерывания по таймеру
int oldsec = 0;



#define  ledPin13  13                               // Назначение светодиодов на плате
#define  ledLCD    8                                // Назначение led LCD
#define  synhro_pin 11                              // Назначение pin синхро
bool start_led = true;

int time_sound = 50;
int freq_sound = 1800;
byte volume1 = 210;
byte volume2 = 210;
float volume_Power = 0;

unsigned int loop_count = 0;
unsigned int irq_ovf_count = 0;


//AH_AD9850(int CLK, int FQUP, int BitData, int RESET); 
AH_AD9850 AD9850(25, 26, 27, 28);

volatile bool sound_start = false;
unsigned long PowerMillis = 0;
unsigned long StartSample = 0;
unsigned int Power_Interval = 1000;
unsigned long TimePeriod = 3000000;



bool intterrupt_enable = false;
//int seconds = 0; // счётчик секунд

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 

RF24 radio(10, 9);             // DUE
		
							   // Topology
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };              // Radio pipe addresses for the 2 nodes to communicate.  Адреса радиоканалов для связи двух узлов.

																		   // Role management: Set up role.  This sketch uses the same software for all the nodes
																		   // in this system.  Doing so greatly simplifies testing.  

typedef enum { role_ping_out = 1, role_pong_back } role_e;                 // The various roles supported by this sketch  Различные роли, поддерживаемые этим эскизом
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back" }; // The debug-friendly names of those roles
role_e role = role_pong_back;                                              // The role of the current running sketch  Роль текущего эскиза

																		   // A single byte to keep track of the data being sent back and forth
byte counter = 1;                                                          // Один байт для отслеживания данных, отправляемых туда и обратно

const char str[] = "My very long string";
extern "C" char *sbrk(int i);

byte data_in[24];                                                         // Буфер хранения принятых данных
byte data_out[24];                                                        // Буфер хранения данных для отправки

int count = 0;
// constants won't change. Used here to set a pin number :
//const int ledPin =  LED_BUILTIN;// the number of the LED pin
//const int ledPin10 = 10;// the number of the LED pin
// Variables will change :
int ledState = LOW;                                  // ledState used to set the LED

								                     // Generally, you should use "unsigned long" for variables that hold time
								                     // The value will quickly become too large for an int to store
unsigned long previousMillis = 0;                    // will store last time LED was updated

										             // constants won't change :
const long interval = 50;                           // interval at which to blink (milliseconds)

volatile unsigned long startMillis = 0;              //  
volatile unsigned long startMillis1 = 0;
volatile unsigned long synhro_count = 0;              //  
unsigned long stopMillis           = 0;              //  
volatile bool info_view = false;
byte pipeNo;                                         // 
byte gotByte;                                        // Dump the payloads until we've gotten everything Сбрасывайте полезную нагрузку, пока мы не получим все
uint32_t message = 1;

//+++++++++++++++++++++++ Настройка электронного резистора +++++++++++++++++++++++++++++++++++++
#define address_AD5252   0x2F                       // Адрес микросхемы AD5252  
#define control_word1    0x07                       // Байт инструкции резистор №1
#define control_word2    0x87                       // Байт инструкции резистор №2
//byte resistance = 0x00;                           // Сопротивление 0x00..0xFF - 0Ом..100кОм
//byte level_resist      = 0;                       // Байт считанных данных величины резистора
//-----------------------------------------------------------------------------------------------

//+++++++++++++++++++++++++++++ Внешняя память +++++++++++++++++++++++++++++++++++++++
int deviceaddress = 80;                             // Адрес микросхемы памяти
byte hi;                                            // Старший байт для преобразования числа
byte low;                                           // Младший байт для преобразования числа

int adr_time_period = 10;


void firstHandler() 
{
	delayMicroseconds(5000);
	synhro_count++;
	AD9850.set_frequency(0, 0, freq_sound);                  //set power=UP, phase=0, 1kHz frequency
	delayMicroseconds(890);
	digitalWrite(synhro_pin, HIGH);
	delayMicroseconds(100);
	digitalWrite(synhro_pin, LOW);
	delayMicroseconds(43000);
	myGLCD.print(String(synhro_count), 1, 30);         // выводим в строке 5 
	myGLCD.update();
	AD9850.powerDown();
}



void resistor(int resist, int valresist)
{
	byte resistance = valresist;
	switch (resist)
	{
	case 1:
		Wire.beginTransmission(address_AD5252);     // transmit to device
		Wire.write(byte(control_word1));            // sends instruction byte  
		Wire.write(resistance);                     // sends potentiometer value byte  
		Wire.endTransmission();                     // stop transmitting
		break;
	case 2:
		Wire.beginTransmission(address_AD5252);     // transmit to device
		Wire.write(byte(control_word2));            // sends instruction byte  
		Wire.write(resistance);                     // sends potentiometer value byte  
		Wire.endTransmission();                     // stop transmitting
		break;
	}
	//Wire.requestFrom(address_AD5252, 1, true);  // Считать состояние движка резистора 
	//level_resist = Wire.read();                 // sends potentiometer value byte  
}
void setup_resistor()
{
	Wire.beginTransmission(address_AD5252);      // transmit to device
	Wire.write(byte(control_word1));             // sends instruction byte  
	Wire.write(0);                               // sends potentiometer value byte  
	Wire.endTransmission();                      // stop transmitting
	Wire.beginTransmission(address_AD5252);      // transmit to device
	Wire.write(byte(control_word2));             // sends instruction byte  
	Wire.write(0);                               // sends potentiometer value byte  
	Wire.endTransmission();                      // stop transmitting
}


void sound_run(unsigned int time, unsigned int frequency)
{
	AD9850.powerDown();                              //set signal output to LOW
	//delay(100);
	AD9850.set_frequency(0, 0, frequency);                   //set power=UP, phase=0, 1kHz frequency

	delay(time);
	AD9850.powerDown();
	sound_start = false;
}


void info()
{
	byte _volume1 = volume1;
	//resistor(1, _volume1);                                 // Установить уровень сигнала
	//resistor(2, volume2);                                 // Установить уровень сигнала
	float y_vol = _volume1 / 254.0*100.0;
	int x_vol = y_vol;
	//myGLCD.setFont(SmallFont);                          // задаём размер шрифта

	myGLCD.print("    ", CENTER, 10);                      // Очистить строку 1
	myGLCD.print("Volume", LEFT, 10);                      // выводим в строке 1 
	myGLCD.print(String(x_vol), 38, 10);                   // выводим в строке1
	myGLCD.print(String("%"), 56, 10);                     // выводим в строке 1 
	myGLCD.print("      ", CENTER, 20);                   // Очистить строку 2
	myGLCD.print("    ", RIGHT, 30);                      // Очистить строку 3
	myGLCD.print("Time", LEFT, 20);                       // выводим в строке 2 
	myGLCD.print(String(time_sound), 40, 20);             // выводим в строке 2 
	myGLCD.print("ms", RIGHT, 20);                        // выводим в строке 2 
	myGLCD.print("Frequency", LEFT, 30);                  // выводим в строке 3 
	myGLCD.print(String(freq_sound), RIGHT, 30);          // выводим в строке 3 
//	myGLCD.print("              ", CENTER, 30);           // Очистить строку 4
	//myGLCD.print("    ", 20, 40);                         // Очистить строку 5
	//myGLCD.print(String(data_in[0]), 20, 40);         // выводим в строке 5 
//	myGLCD.drawCircle(5, 42, 4);
	myGLCD.update();
}

void alarmFunction()
{
//	isAlarm = false;
	DS3231_clock.clearAlarm1();
	dt = DS3231_clock.getDateTime();
	myGLCD.print(DS3231_clock.dateFormat("H:i:s -", dt), 0, 0);
	if (alarm_synhro > 4)
	{
		alarm_synhro = 0;
		delayMicroseconds(16000);
		delayMicroseconds(3000);
		//delayMicroseconds(900);
		//delayMicroseconds(6000);
		digitalWrite(synhro_pin, HIGH);
		delayMicroseconds(50);
		alarm_count++;
		myGLCD.print(DS3231_clock.dateFormat("s", dt), 63, 0);
		digitalWrite(synhro_pin, LOW);
	}
	alarm_synhro++;
	myGLCD.print(String(alarm_synhro), 78, 0);
	myGLCD.update();
}


void setup() 
{
	Serial.begin(115200);
	delay(1000);
	Serial.print("FreeRam");
//	Serial.println(FreeRam());
	Serial.println("\r\n==================");
	printf_begin();

	Wire.begin();
	myGLCD.InitLCD();                                // инициализация LCD дисплея
	DS3231_clock.begin();
											         // Настройка радиочастотного радиоканала
	radio.begin();                                   // Старт работы;
	radio.setAutoAck(1);                             // Убедитесь, что включена функция автозагрузки
	radio.enableAckPayload();                        // Разрешение отправки нетипового ответа передатчику;
	//radio.setPALevel(RF24_PA_MAX);
	radio.setPALevel(RF24_PA_HIGH);
	//radio.setPALevel(RF24_PA_LOW);                 // Set PA LOW for this demonstration. We want the radio to be as lossy as possible for this example.
	radio.setDataRate(RF24_1MBPS);
	//radio.setDataRate(RF24_250KBPS);
	radio.setRetries(0, 2);                          // Самое короткое время между попытками, max no. повторений
	//radio.setPayloadSize(1);                       // Здесь мы отправляем 1-байтовую полезную нагрузку для проверки скорости ответа на вызов
	radio.setPayloadSize(sizeof(data_out));          // Здесь мы отправляем 32-байтовую полезную нагрузку
	radio.openWritingPipe(pipes[1]);                 // Обе радиостанции прослушиваются по тем же самым каналам по умолчанию и переключаются при записи
	radio.openReadingPipe(1, pipes[0]);              // Открываем трубу приема
	radio.startListening();                          // Начать прослушивание
	radio.printDetails();                            // Dump the configuration of the rf unit for debugging

	role = role_pong_back;                           // Станьте основным приемником (pong back)
	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();
	radio.powerUp();
	myGLCD.clrScr();                                 // очистка экрана
	myGLCD.setFont(SmallFont);                       // задаём размер шрифта
	//myGLCD.setFont(MediumNumbers);                 // задаём размер шрифта

	AD9850.reset();                                  //reset module
	delay(500);
	AD9850.powerDown();                              //set signal output to LOW
	delay(100);
	AD9850.set_frequency(0,0,1850);                  //set power=UP, phase=0, 1kHz frequency
	setup_resistor();                                // Начальные установки резистора
	resistor(1, volume1);                            // Установить уровень сигнала
//	resistor(2, volume2);                            // Установить уровень сигнала
	pinMode(ledPin13, OUTPUT);
	pinMode(ledLCD, OUTPUT);
	analogWrite(ledLCD, 80);
	pinMode(synhro_pin, OUTPUT);
	digitalWrite(synhro_pin, LOW);
	pinMode(alarm_pin, INPUT);
	digitalWrite(alarm_pin, HIGH);
	
	digitalWrite(ledPin13, HIGH);
	delay(100);
	digitalWrite(ledPin13, LOW);
	delay(100);
	Timer6.setPeriod(TimePeriod);
	Timer6.attachInterrupt(firstHandler);                    // Every 3 sec.
	AD9850.powerDown();                                       //set signal output to LOW

															  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
															  // Under normal conditions, the settings should be reset after power and restart microcontroller.
	DS3231_clock.armAlarm1(false);
	DS3231_clock.armAlarm2(false);
	DS3231_clock.clearAlarm1();
	DS3231_clock.clearAlarm2();

	//DS3231_clock.setDateTime(__DATE__, __TIME__);
	//analogReference(AR_DEFAULT);

	//volume_Power = analogRead(0)*(3.2 / 1024 * 2);
	//myGLCD.print(String(volume_Power), RIGHT, 1);          // выводим в строке 1
	DS3231_clock.setOutput(DS3231_1HZ);
	DS3231_clock.enableOutput(true);

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
	DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);         //DS3231_EVERY_SECOND //Каждую секунду
	while (digitalRead(alarm_pin) != HIGH)
	{

	}

	while (digitalRead(alarm_pin) == HIGH)
	{

	}


	while (true)
	{
		dt = DS3231_clock.getDateTime();
		if (oldsec != dt.second)
		{
			myGLCD.print(DS3231_clock.dateFormat("H:i:s",  dt), 0, 0);
			myGLCD.update();
			oldsec = dt.second;
	    }
		if (dt.second == 0 || dt.second == 10 || dt.second == 20 || dt.second == 30 || dt.second == 40 || dt.second == 50)
		{
			break;
		}
	}

	attachInterrupt(alarm_pin, alarmFunction, FALLING);    // прерывание вызывается только при смене значения на порту с LOW на HIGH
//	attachInterrupt(alarm_pin, alarmFunction, RISING);     // ппрерывание вызывается только при смене значения на порту с HIGH на LOW
	PowerMillis = millis();

}

void loop(void) 
{

	unsigned long currentMillis = millis();
	if (sound_start)
	{
		if (millis() - startMillis >= interval)
		{
			Serial.println(currentMillis - startMillis);
			//previousMillis = currentMillis;
			AD9850.powerDown();                                 //set signal output to LOW
			sound_start = false;
		}
	}

	if (millis() - PowerMillis >= Power_Interval)
	{
		volume_Power = analogRead(0)*(2.50 / 1024 * 2);
		myGLCD.printNumF(volume_Power,1, RIGHT, 10);          // выводим в строке 1
		//myGLCD.print(String("       "), RIGHT, 40);          // выводим в строке 40
		//myGLCD.print(String(rtc.getHours()), 35, 40);          // выводим в строке 1
		//myGLCD.print(String("/"), 48, 40);          // выводим в строке 1
		//myGLCD.print(String(rtc.getMinutes()), 55, 40);          // выводим в строке 1
		//myGLCD.print(String(":"), 67, 40);          // выводим в строке 1
		//myGLCD.print(String(rtc.getSeconds()), RIGHT, 40);          // выводим в строке 1
		info();
	//	myGLCD.update();
		PowerMillis = millis();
	}
	
	while (radio.available(&pipeNo))                               // 
	{
		radio.read(&data_in, sizeof(data_in));
		data_out[0] = data_in[0];
		if (data_in[2]==1)                                         // Прием синхроимпульса по радио и генерирование одиночного сигнала.
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);           // Грузим сообщение 2 байта для автоотправки;
			stopMillis = micros();
			delayMicroseconds(17000);                               // Задержка для получения ответа и завершения процессов на Базе
			sound_run(time_sound, freq_sound);
			info();
		}
		else if (data_in[2] == 2)                                  // Синхронизация по таймеру 
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);           // Грузим сообщение 2 байта для автоотправки;
			delayMicroseconds(130000);
			synhro_count = 0;
			Timer6.start(TimePeriod);
		}
		else if (data_in[2] == 3)                                  // Выполнить синхронизацию по проводу
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);           // Грузим сообщение 2 байта для автоотправки;
			delayMicroseconds(100000);
			digitalWrite(synhro_pin, HIGH);
			delayMicroseconds(100000);
			digitalWrite(synhro_pin, LOW);
			Timer6.start(TimePeriod);
		}
		else if (data_in[2] == 4)                               // Остановить синхронизацию модулей
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);        // Грузим сообщение 2 байта для автоотправки;
			delayMicroseconds(1000);
			Timer6.stop();                                      // Отключаем прерывание
			intterrupt_enable = false;
		}
		else if (data_in[2] == 5)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);        // Грузим сообщение 2 байта для автоотправки;

		}
		else if (data_in[2] == 6)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);        // Грузим сообщение 2 байта для автоотправки;
			volume1 = data_in[8];                               // 
			volume2 = data_in[9];                               // Громкость звучания. 
		}
		else if (data_in[2] == 7)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);        // Грузим сообщение 2 байта для автоотправки;
			TimePeriod = (data_in[10] << 8) | data_in[11];        // Длительность посылки. Собираем как "настоящие программеры"
		}
		else if (data_in[2] == 8)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);        // Грузим сообщение 2 байта для автоотправки;
			detachInterrupt(alarm_pin);
			pinMode(alarm_pin, INPUT);
			digitalWrite(alarm_pin, HIGH);
			DS3231_clock.clearAlarm1();
			DS3231_clock.clearAlarm2();
			alarm_count = 0;
			delayMicroseconds(10000);
			digitalWrite(synhro_pin, HIGH);
			delayMicroseconds(100000);
			digitalWrite(synhro_pin, LOW);
			DS3231_clock.setDateTime(data_in[12]+2000, data_in[13], data_in[14], data_in[15], data_in[16], 00);
			alarm_synhro = 0;
			DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);         //DS3231_EVERY_SECOND //Каждую секунду
			while (digitalRead(alarm_pin) != HIGH)
			{

			}

			while (digitalRead(alarm_pin) == HIGH)
			{

			}
																			 //	dt = DS3231_clock.getDateTime();
			while (true)
			{
				dt = DS3231_clock.getDateTime();
				if (oldsec != dt.second)
				{
					myGLCD.print(DS3231_clock.dateFormat("H:i:s", dt), 0, 0);
					myGLCD.update();
					oldsec = dt.second;
				}
				if (dt.second == 10)
				{
					break;
				}
			}
	////		Serial.println(DS3231_clock.dateFormat("d-m-Y H:i:s - l", dt));
			attachInterrupt(alarm_pin, alarmFunction, FALLING);    // прерывание вызывается только при смене значения на порту с LOW на HIGH
		//	attachInterrupt(alarm_pin, alarmFunction, RISING);     // прерывание вызывается только при смене значения на порту с HIGH на LOW
		}
		else
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);    // Грузим сообщение 2 байта для автоотправки;
		}

		time_sound = (data_in[4] << 8) | data_in[5];        // Длительность посылки. Собираем как "настоящие программеры"
		freq_sound = (data_in[6] << 8) | data_in[7];        // Частота генератора. Собираем как "настоящие программеры"
		volume1 = data_in[8];                               // 
		volume2 = data_in[9];                               // Громкость звучания. 
    	resistor(1, volume1);                               // Установить уровень сигнала
	    resistor(2, volume2);                               // Установить уровень сигнала
		info();
	}
}






//int value = 3000;
//// разбираем
//byte hi = highByte(value);
//byte low = lowByte(value);

//// тут мы эти hi,low можем сохраить, прочитать из eePROM

//int value2 = (hi << 8) | low; // собираем как "настоящие программеры"
//int value3 = word(hi, low); // или собираем как "ардуинщики"



//unsigned long currentMillis = millis();

//if (currentMillis - previousMillis >= interval) 
//{
//	// save the last time you blinked the LED
//	previousMillis = currentMillis;
//	sound_start = true;
//}

//if (sound_start == true) sound_run(time_sound, freq_sound);



//for (int i = 0; i<sizeof(data_in); i++) 
//{
//	Serial.print(data_in[i]);               //Load the buffer with random data
//	Serial.print(",");
//}
//Serial.println();
//int value2 = (data_in[3] << 8) | data_in[4]; // собираем как "настоящие программеры"
//Serial.print(value2);               //Load the buffer with random data
//Serial.print(",");
//int value3 = (data_in[5] << 8) | data_in[6]; // собираем как "настоящие программеры"
//Serial.print(value3);               //Load the buffer with random data
//Serial.println();
