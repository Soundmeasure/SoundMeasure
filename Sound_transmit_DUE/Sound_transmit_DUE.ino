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
//#include <rtc_clock.h>
#include "printf.h"

//#  define CONF_CLOCK_XOSC_ENABLE                  true
//#  define CONF_CLOCK_XOSC_EXTERNAL_CRYSTAL        SYSTEM_CLOCK_EXTERNAL_CRYSTAL
//#  define CONF_CLOCK_XOSC_EXTERNAL_FREQUENCY      12000000UL


// SYSTEM_CLOCK_SOURCE_XOSC configuration - External clock/oscillator 
//#  define CONF_CLOCK_XOSC_ENABLE                  true
//#  define CONF_CLOCK_XOSC_EXTERNAL_CRYSTAL        SYSTEM_CLOCK_EXTERNAL_CRYSTAL
//#  define CONF_CLOCK_XOSC_EXTERNAL_FREQUENCY      16000000UL
//#  define CONF_CLOCK_XOSC_STARTUP_TIME            SYSTEM_XOSC_STARTUP_32768
//#  define CONF_CLOCK_XOSC_AUTO_GAIN_CONTROL       true
//#  define CONF_CLOCK_XOSC_ON_DEMAND               true
//#  define CONF_CLOCK_XOSC_RUN_IN_STANDBY          false
//


//#include <LCD5110_Basic.h> // подключаем библиотеку

LCD5110 myGLCD(7, 6, 5, 4, 3); // объявляем номера пинов LCD

extern uint8_t SmallFont[]; // малый шрифт (из библиотеки)
extern uint8_t MediumNumbers[]; // средний шрифт для цифр (из библиотеки)

//RTC_clock rtc_clock(XTAL);

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 17;
const byte hours = 13;

/* Change these values to set the current initial date */
const byte day = 4;
const byte month = 10;
const byte year = 17;


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

byte data_in[16];                                                         // Буфер хранения принятых данных
byte data_out[16];                                                        // Буфер хранения данных для отправки

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



void firstHandler() 
{
	digitalWrite(ledPin13, HIGH);
	delayMicroseconds(2000);
	AD9850.set_frequency(0, 0, 1850);                  //set power=UP, phase=0, 1kHz frequency
	delayMicroseconds(50000);
	AD9850.powerDown();
	digitalWrite(synhro_pin, HIGH);
	delayMicroseconds(1000);
	digitalWrite(synhro_pin, LOW);
	digitalWrite(ledPin13, LOW);
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
	for (int i = 0; i < time; i++)
	{
		delay(1);
	}
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

	myGLCD.print("    ", CENTER, 1);                      // Очистить строку 1
	myGLCD.print("Volume", LEFT, 1);                      // выводим в строке 1 
	myGLCD.print(String(x_vol), 38, 1);                   // выводим в строке1
	myGLCD.print(String("%"), 56, 1);                     // выводим в строке 1 
	myGLCD.print("      ", CENTER, 10);                   // Очистить строку 2
	myGLCD.print("    ", RIGHT, 20);                      // Очистить строку 3
	myGLCD.print("Time", LEFT, 10);                       // выводим в строке 2 
	myGLCD.print(String(time_sound), 40, 10);             // выводим в строке 2 
	myGLCD.print("ms", RIGHT, 10);                        // выводим в строке 2 
	myGLCD.print("Frequency", LEFT, 20);                  // выводим в строке 3 
	myGLCD.print(String(freq_sound), RIGHT, 20);          // выводим в строке 3 
	myGLCD.print("              ", CENTER, 30);           // Очистить строку 4
	myGLCD.print("    ", 20, 40);                         // Очистить строку 5
	//if (info_view)
	//{
	//	myGLCD.print(String(stopMillis - startMillis), CENTER, 30);         // выводим в строке 4
	//}
	myGLCD.print(String(data_in[0]), 20, 40);         // выводим в строке 5 
	myGLCD.update();
}


void setup() 
{
	Serial.begin(115200);
	delay(1000);
	//while (!Serial){};
	Serial.print("FreeRam");
//	Serial.println(FreeRam());
	Serial.println("\r\n==================");
	printf_begin();

	Wire.begin();
	myGLCD.InitLCD();                                // инициализация LCD дисплея

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
	digitalWrite(synhro_pin,LOW);
	//digitalWrite(ledLCD, LOW);
	digitalWrite(ledPin13, HIGH);
	delay(100);
	digitalWrite(ledPin13, LOW);
	delay(100);
	digitalWrite(ledPin13, HIGH);
	delay(100);
	digitalWrite(ledPin13, LOW);
	AD9850.powerDown();                                       //set signal output to LOW

	//analogReference(AR_DEFAULT);

	info();

	//volume_Power = analogRead(0)*(3.2 / 1024 * 2);
	//myGLCD.print(String(volume_Power), RIGHT, 1);          // выводим в строке 1
	Timer6.setPeriod(2000000);
	Timer6.attachInterrupt(firstHandler);                    // Every 3 sec.
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
		myGLCD.printNumF(volume_Power,1, RIGHT, 1);          // выводим в строке 1
		//myGLCD.print(String("       "), RIGHT, 40);          // выводим в строке 40
		//myGLCD.print(String(rtc.getHours()), 35, 40);          // выводим в строке 1
		//myGLCD.print(String("/"), 48, 40);          // выводим в строке 1
		//myGLCD.print(String(rtc.getMinutes()), 55, 40);          // выводим в строке 1
		//myGLCD.print(String(":"), 67, 40);          // выводим в строке 1
		//myGLCD.print(String(rtc.getSeconds()), RIGHT, 40);          // выводим в строке 1
		myGLCD.update();
		PowerMillis = millis();
	}


	while (radio.available(&pipeNo))                                // 
	{
		radio.read(&data_in, sizeof(data_in));
		data_out[0] = data_in[0];
		if (data_in[2]==1)                                         // Прием синхроимпульса по радио и генерирование одиночного сигнала.
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);           // Грузим сообщение 2 байта для автоотправки;
			stopMillis = micros();
			delayMicroseconds(1000);                               // Задержка для получения ответа и завершения процессов на Базе
			sound_run(time_sound, freq_sound);
			info();
		}
		else if (data_in[2] == 2)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);           // Грузим сообщение 2 байта для автоотправки;
			delayMicroseconds(1000);

		}
		else if (data_in[2] == 3)                                  // Выполнить синхронизацию по проводу
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);           // Грузим сообщение 2 байта для автоотправки;
			delayMicroseconds(20000);
			digitalWrite(synhro_pin, HIGH);
			delayMicroseconds(1000);
			digitalWrite(synhro_pin, LOW);
			Timer6.start();

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
