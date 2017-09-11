//---------------------------------------
//Модуль NRF24L01 Подключение
//+++++++++++++++++++++++++++++++++++++++
//SAMD21G18A      NRF24L01
//GND              1 GND
//VCC +3,3V        2 VCC +3,3V
//D10  (27)        3 CE
//D9   (12)        4 SCN
//SCK  (20) 	   5 SCK
//MOSI (19)  	   6 MOSI
//MISO (21)	       7 MISO
//
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
#include "Wire.h"
#include <SPI.h>
#include <LCD5110_Graph.h>    
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <AH_AD9850.h>
//#include <LCD5110_Basic.h> // подключаем библиотеку

LCD5110 myGLCD(7, 6, 5, 4, 3); // объявляем номера пинов LCD

extern uint8_t SmallFont[]; // малый шрифт (из библиотеки)
extern uint8_t MediumNumbers[]; // средний шрифт для цифр (из библиотеки)

#define  ledPin13  13                               // Назначение светодиодов на плате
#define  sound_En  2                                // Управление звуком, разрешение включения усилителя

int time_sound = 200;
int freq_sound = 1850;
byte volume1 = 0;
byte volume2 = 0;
//AH_AD9850(int CLK, int FQUP, int BitData, int RESET); 
AH_AD9850 AD9850(A4, A3, A2, A1);

bool sound_start = false;

int seconds = 0; // счётчик секунд

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 

RF24 radio(10, 9);             // SAMD21G18A
		
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
const long interval = 1000;                          // interval at which to blink (milliseconds)

byte pipeNo;                                         // 
byte gotByte;                                        // Dump the payloads until we've gotten everything Сбрасывайте полезную нагрузку, пока мы не получим все
uint32_t message = 1;

//+++++++++++++++++++++++ Настройка электронного резистора +++++++++++++++++++++++++++++++++++++
#define address_AD5252   0x2F                       // Адрес микросхемы AD5252  
#define control_word1    0x07                       // Байт инструкции резистор №1
#define control_word2    0x87                       // Байт инструкции резистор №2
byte resistance = 0x00;                             // Сопротивление 0x00..0xFF - 0Ом..100кОм
//byte level_resist      = 0;                       // Байт считанных данных величины резистора
//-----------------------------------------------------------------------------------------------


void resistor(int resist, int valresist)
{
	resistance = valresist;
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


//------------------------------------------------------------------------------

int FreeRam() {
	char stack_dummy = 0;
	return &stack_dummy - sbrk(0);
}

//#define Serial SerialUSB               // Native DUE
#define Serial SERIAL_PORT_USBVIRTUAL             // USB SAMD21G18A

void setup() 
{
	Serial.begin(115200);
	delay(1000);
	//while (!Serial){};
	Serial.print("FreeRam");
	Serial.println(FreeRam());
	Serial.print("Address of str $"); Serial.println((int)&str, HEX);
	Serial.println("\r\n==================");
	printf_begin();
	Serial.print(F("\n\rRF24/examples/pingpair_ack/\n\rROLE: "));
	Serial.println(role_friendly_name[role]);
	Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));
	Wire.begin();
	myGLCD.InitLCD();                            // инициализация LCD дисплея

											     // Настройка радиочастотного радиоканала
	radio.begin();                               // Старт работы;
	radio.setAutoAck(1);                         // Убедитесь, что включена функция автозагрузки
	radio.enableAckPayload();                    // Разрешение отправки нетипового ответа передатчику;
	//radio.setPALevel(RF24_PA_MAX);
	radio.setPALevel(RF24_PA_HIGH);
	//radio.setPALevel(RF24_PA_LOW);           // Set PA LOW for this demonstration. We want the radio to be as lossy as possible for this example.
	radio.setDataRate(RF24_1MBPS);
	//radio.setDataRate(RF24_250KBPS);
	radio.setRetries(0, 2);                     // Самое короткое время между попытками, max no. повторений
	//radio.setPayloadSize(1);                   // Здесь мы отправляем 1-байтовую полезную нагрузку для проверки скорости ответа на вызов
	radio.setPayloadSize(sizeof(data_out));      // Здесь мы отправляем 32-байтовую полезную нагрузку
	radio.openWritingPipe(pipes[1]);             // Обе радиостанции прослушиваются по тем же самым каналам по умолчанию и переключаются при записи
	radio.openReadingPipe(1, pipes[0]);          // Открываем трубу приема
	radio.startListening();                      // Начать прослушивание
	radio.printDetails();                        // Dump the configuration of the rf unit for debugging

	role = role_pong_back;                       // Станьте основным приемником (pong back)
	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();
	radio.powerUp();
	myGLCD.clrScr();                             // очистка экрана
	myGLCD.setFont(SmallFont);                   // задаём размер шрифта
	//myGLCD.setFont(MediumNumbers);             // задаём размер шрифта

	AD9850.reset();                              //reset module
	delay(500);
	AD9850.powerDown();                          //set signal output to LOW
	delay(100);
	AD9850.set_frequency(0,0,1850);              //set power=UP, phase=0, 1kHz frequency
	setup_resistor();                            // Начальные установки резистора
	resistor(1, volume1);                              // Установить уровень сигнала
	resistor(2, volume2);                              // Установить уровень сигнала
	pinMode(ledPin13, OUTPUT);
	pinMode(sound_En, OUTPUT);
	digitalWrite(ledPin13, HIGH);
	//digitalWrite(sound_En, LOW);
	digitalWrite(sound_En, HIGH);
	AD9850.powerDown();                          //set signal output to LOW

}

void loop(void) 
{

	while (radio.available(&pipeNo))                       // 
	{
		radio.read(&data_in, sizeof(data_in));
		data_out[0] = data_in[0];
		if (data_in[2]==1)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);   // Грузим сообщение 2 байта для автоотправки;
		//	delayMicroseconds(2000);                        // Задержка, для завершения процессов на Базе
			sound_run(time_sound, freq_sound);
		}
		else
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);  // Грузим сообщение 2 байта для автоотправки;
		}

		time_sound = (data_in[4] << 8) | data_in[5];      // Длительность посылки. Собираем как "настоящие программеры"
		freq_sound = (data_in[6] << 8) | data_in[7];      // Частота генератора. Собираем как "настоящие программеры"
		volume1 = data_in[8];                             // 
		volume2 = data_in[9];                             // Громкость звучания. 
		//sound_start = true;
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

	resistor(1, volume1);                              // Установить уровень сигнала
	resistor(2, volume2);                              // Установить уровень сигнала
	float y_vol = volume2 / 254.0*100.0;
	int x_vol = y_vol;
	//myGLCD.setFont(SmallFont); // задаём размер шрифта

	myGLCD.print("    ", CENTER, 1); // выводим в строке 34 
	myGLCD.print("Volume", LEFT, 1); // выводим в строке 34 
	myGLCD.print(String(x_vol),40, 1); // выводим в строке 34 
	myGLCD.print(String("%"), 56, 1); // выводим в строке 34 
	myGLCD.print("    ", CENTER, 10); // выводим в строке 34 
	myGLCD.print("    ", RIGHT, 20); // выводим в строке 34 
	myGLCD.print("Time",LEFT, 10); // выводим в строке 34 
	myGLCD.print(String(time_sound), 40, 10); // выводим в строке 34 
	myGLCD.print("ms", RIGHT, 10); // выводим в строке 34 
	myGLCD.print("Frequency", LEFT, 20); // выводим в строке 34 
	myGLCD.print(String(freq_sound), RIGHT, 20); // выводим в строке 34 

		myGLCD.print("    ", CENTER, 30); // выводим в строке 34 
		myGLCD.print("    ", CENTER, 40); // выводим в строке 34 

		myGLCD.print(String(data_in[2]), CENTER, 30); // выводим в строке 34 
		//myGLCD.print(String(gotByte), CENTER, 34); // выводим в строке 34 
		myGLCD.print(String(data_in[0]), CENTER, 40); // выводим в строке 34 
		myGLCD.update();
}