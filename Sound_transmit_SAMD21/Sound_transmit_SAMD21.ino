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



#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
//#include <LCD5110_Basic.h> // подключаем библиотеку
#include <LCD5110_Graph.h>

LCD5110 myGLCD(7, 6, 5, 4, 3); // объявляем номера пинов LCD

extern uint8_t SmallFont[]; // малый шрифт (из библиотеки)
extern uint8_t MediumNumbers[]; // средний шрифт для цифр (из библиотеки)

int seconds = 0; // счётчик секунд





// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 

RF24 radio(10, 9);             // SAMD21G18A
		
							   // Topology
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };              // Radio pipe addresses for the 2 nodes to communicate.

																		   // Role management: Set up role.  This sketch uses the same software for all the nodes
																		   // in this system.  Doing so greatly simplifies testing.  

typedef enum { role_ping_out = 1, role_pong_back } role_e;                 // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back" };  // The debug-friendly names of those roles
role_e role = role_pong_back;                                              // The role of the current running sketch

																		   // A single byte to keep track of the data being sent back and forth
byte counter = 1;

const char str[] = "My very long string";
extern "C" char *sbrk(int i);



int count = 0;
// constants won't change. Used here to set a pin number :
//const int ledPin =  LED_BUILTIN;// the number of the LED pin
//const int ledPin10 = 10;// the number of the LED pin
// Variables will change :
int ledState = LOW;             // ledState used to set the LED

								// Generally, you should use "unsigned long" for variables that hold time
								// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

										 // constants won't change :
const long interval = 200;           // interval at which to blink (milliseconds)

int FreeRam() {
	char stack_dummy = 0;
	return &stack_dummy - sbrk(0);
}


//#define Serial SerialUSB               // Native DUE
#define Serial SERIAL_PORT_USBVIRTUAL  // USB SAMD21G18A

void setup() {


	Serial.begin(115200);
	delay(1000);
	//while (!Serial);
	// pinMode (ledPin, OUTPUT);
	Serial.print("FreeRam");
	Serial.println(FreeRam());
	Serial.print("Address of str $"); Serial.println((int)&str, HEX);
	Serial.println("\r\n==================");
	printf_begin();
	Serial.print(F("\n\rRF24/examples/pingpair_ack/\n\rROLE: "));
	Serial.println(role_friendly_name[role]);
	Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));

	myGLCD.InitLCD(); // инициализация LCD дисплея

	// Setup and configure rf radio

	radio.begin();
	radio.setAutoAck(1);                    // Ensure autoACK is enabled
	radio.enableAckPayload();               // Allow optional ack payloads
	radio.setRetries(0, 15);                 // Smallest time between retries, max no. of retries
	radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed
	radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();                 // Start listening
	radio.printDetails();                   // Dump the configuration of the rf unit for debugging

	role = role_pong_back;                  // Become the primary receiver (pong back)
	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();


	myGLCD.clrScr(); // очистка экрана
	myGLCD.setFont(SmallFont); // задаём размер шрифта
	//myGLCD.print("Hello,", LEFT, 0); // выводим на строке 0, равнение по левому краю  
	//myGLCD.print("SolTau.ru :-)", CENTER, 16); // выводим в строке 16

	//myGLCD.setFont(MediumNumbers); // задаём размер шрифта
}

void loop(void) {

	//
	//  unsigned long currentMillis = millis();
	//
	//  if (currentMillis - previousMillis >= interval) {
	//    // save the last time you blinked the LED
	//    previousMillis = currentMillis;
	////   Serial.print( "count = ");
	////     Serial.println(count);
	//// 
	////  count++;
	//
	//
	//
	//    // if the LED is off turn it on and vice-versa:
	//    if (ledState == LOW) {
	//      ledState = HIGH;
	//    } else {
	//      ledState = LOW;
	//    }
	//
	//    // set the LED with the ledState of the variable:
	//    digitalWrite(ledPin, ledState);
	//  }

	// Pong back role.  Receive each packet, dump it out, and send it back

	if (role == role_pong_back) {
		byte pipeNo;
		byte gotByte;                                       // Dump the payloads until we've gotten everything Сбрасывайте полезную нагрузку, пока мы не получим все
		while (radio.available(&pipeNo)) {
			radio.read(&gotByte, 1);
			radio.writeAckPayload(pipeNo, &gotByte, 1);
		}

		//myGLCD.setFont(SmallFont); // задаём размер шрифта
		//myGLCD.print("    ", CENTER, 14); // выводим в строке 34 
		myGLCD.print("    ", CENTER, 34); // выводим в строке 34 

		//myGLCD.print(String(pipeNo), CENTER, 14); // выводим в строке 34 
		myGLCD.print(String(gotByte), CENTER, 34); // выводим в строке 34 
		myGLCD.update();
	}
}