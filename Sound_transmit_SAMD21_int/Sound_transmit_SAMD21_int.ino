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
#include <RTCZero.h>



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

//const int ledPin =  LED_BUILTIN;// the number of the LED pin
//const int ledPin = 28;// the number of the LED pin


// The type cast must fit with the selected timer Данный тип должен соответствовать выбранному таймеру
Tcc* TC = (Tcc*)TCC0;                    // get timer struct  получить структуру таймера



/* Create an rtc object */
RTCZero rtc;

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 17;
const byte hours = 13;

/* Change these values to set the current initial date */
const byte day = 4;
const byte month = 10;
const byte year = 17;


#define  ledPin  13                                 // Назначение светодиодов на плате
#define  sound_En  2                                // Управление звуком, разрешение включения усилителя
#define  ledLCD    8                                // Назначение led LCD
#define  synhro_pin 27                              // Назначение pin синхро
bool start_led = true;

int time_sound = 50;
int freq_sound = 1800;
byte volume1 = 254;
byte volume2 = 254;
float volume_Power = 0;

int pin_ovf_led = 13; // debug pin for overflow led 
//int pin_mc0_led = 28;  // debug pin for compare led 
unsigned int loop_count = 0;
unsigned int irq_ovf_count = 0;


//AH_AD9850(int CLK, int FQUP, int BitData, int RESET); 
AH_AD9850 AD9850(A4, A3, A2, A1);

volatile bool sound_start = false;
unsigned long PowerMillis = 0;
unsigned long StartSample = 0;
unsigned int Power_Interval = 1000;

bool intterrupt_enable = false;
//int seconds = 0; // счётчик секунд

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

//#define Serial SerialUSB                        // Native DUE
#define Serial SERIAL_PORT_USBVIRTUAL             // USB SAMD21G18A
//#define Serial SERIAL_PORT_USBVIRTUAL             // Подключаем USB порт в качестве COM порта

//void blink()
//{
//	startMillis = micros();
//	info_view = true;
//}


void info()
{
	resistor(1, volume1);                                 // Установить уровень сигнала
	resistor(2, volume2);                                 // Установить уровень сигнала
	float y_vol = volume2 / 254.0*100.0;
	int x_vol = y_vol;
	//myGLCD.setFont(SmallFont);                          // задаём размер шрифта

	myGLCD.print("    ", CENTER, 1);                      // Очистить строку 1
	myGLCD.print("Volume", LEFT, 1);                      // выводим в строке 1 
	myGLCD.print(String(x_vol), 35, 1);                    // выводим в строке1
	myGLCD.print(String("%"), 56, 1);                     // выводим в строке 1 
	myGLCD.print("      ", CENTER, 10);                   // Очистить строку 2
	myGLCD.print("    ", RIGHT, 20);                      // Очистить строку 3
	myGLCD.print("Time", LEFT, 10);                        // выводим в строке 2 
	myGLCD.print(String(time_sound), 40, 10);             // выводим в строке 2 
	myGLCD.print("ms", RIGHT, 10);                        // выводим в строке 2 
	myGLCD.print("Frequency", LEFT, 20);                  // выводим в строке 3 
	myGLCD.print(String(freq_sound), RIGHT, 20);          // выводим в строке 3 
	myGLCD.print("              ", CENTER, 30);           // Очистить строку 4
	myGLCD.print("    ", 20, 40);                     // Очистить строку 5
	//if (info_view)
	//{
	//	myGLCD.print(String(stopMillis - startMillis), CENTER, 30);         // выводим в строке 4
	//}
	myGLCD.print(String(data_in[0]), 20, 40);         // выводим в строке 5 
	myGLCD.update();
}


void TCC0_Handler()
{
//	Tcc* TC = (Tcc*)TCC0;                               // get timer struct
	if (TC->INTFLAG.bit.OVF == 1)
	{    
		sound_start = true;
		startMillis = millis();
		digitalWrite(pin_ovf_led, HIGH);    // for debug leds

		if (millis()- startMillis >= 10000)
		{
			digitalWrite(pin_ovf_led, LOW);    // for debug leds
			startMillis = millis();
		}



		/*
		if (irq_ovf_count % 2 == 1)
		{
			digitalWrite(pin_ovf_led, irq_ovf_count % 2);    // for debug leds
			//AD9850.set_frequency(0, 0, freq_sound);          //set power=UP, phase=0, 1kHz frequency
			//Serial.println(millis() - startMillis1);
			//startMillis1 = millis();
		}
	//	digitalWrite(pin_ovf_led, irq_ovf_count % 2);    // for debug leds
		//digitalWrite(pin_mc0_led, HIGH);               // for debug leds
		TC->INTFLAG.bit.OVF = 1;                         // writing a one clears the flag ovf flag
		irq_ovf_count++;                                 // for debug leds
		*/
	}

	if (TC->INTFLAG.bit.MC0 == 1) 
	{  // A compare to cc0 caused the interrupt
		//digitalWrite(pin_mc0_led, LOW);                 // for debug leds
		TC->INTFLAG.bit.MC0 = 1;                        // writing a one clears the flag ovf flag
	}
}

void setTimer(long period) 
{
	//Tcc* TC = (Tcc*)TCC0; // get timer struct

	TC->CTRLA.reg &= ~TCC_CTRLA_ENABLE;                 // Disable TC
	while (TC->SYNCBUSY.bit.ENABLE == 1);               // wait for sync 

	TC->PER.reg = period;                               // Set counter Top using the PER register  
	while (TC->SYNCBUSY.bit.PER == 1);                  // wait for sync 

	TC->CTRLA.reg |= TCC_CTRLA_ENABLE;                  // Enable TC
	while (TC->SYNCBUSY.bit.ENABLE == 1);               // wait for sync 
}

void interrupts_start()
{
	// Enable clock for TC  Включить часы для TC
	REG_GCLK_CLKCTRL = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC0_TCC1);
	while (GCLK->STATUS.bit.SYNCBUSY == 1);         // wait for sync ждать синхронизацию


													//										         // The type cast must fit with the selected timer Данный тип должен соответствовать выбранному таймеру
	Tcc* TC = (Tcc*)TCC0;                            // get timer struct  получить структуру таймера

	TC->CTRLA.reg &= ~TCC_CTRLA_ENABLE;              // Disable TC
	while (TC->SYNCBUSY.bit.ENABLE == 1);            // wait for sync 


	TC->CTRLA.reg |= TCC_CTRLA_PRESCALER_DIV256;     // Set perscaler


	TC->WAVE.reg |= TCC_WAVE_WAVEGEN_NFRQ;           // Set wave form configuration 
	while (TC->SYNCBUSY.bit.WAVE == 1);              // wait for sync 

	TC->PER.reg = 0xFFFF;              // Set counter Top using the PER register  Установить счетчик сверху с использованием регистра PER
	while (TC->SYNCBUSY.bit.PER == 1); // wait for sync 

	TC->CC[0].reg = 0xFFF;
	while (TC->SYNCBUSY.bit.CC0 == 1); // wait for sync 

									   // Interrupts 
	TC->INTENSET.reg = 0;                 // disable all interrupts
	TC->INTENSET.bit.OVF = 1;             // enable overfollow включить переполнение
	TC->INTENSET.bit.MC0 = 1;             // enable compare match to CC0 включить сравнение соответствия с CC0
										  //	setTimer(300000);
	//setTimer(281250-120);
	setTimer(562380);

	// Enable InterruptVector
	NVIC_EnableIRQ(TCC0_IRQn);

	// Enable TC
	TC->CTRLA.reg |= TCC_CTRLA_ENABLE;
	while (TC->SYNCBUSY.bit.ENABLE == 1); // wait for sync 

}


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
	resistor(2, volume2);                            // Установить уровень сигнала
	pinMode(ledPin, OUTPUT);
	pinMode(ledLCD, OUTPUT);
	pinMode(synhro_pin, INPUT);
	digitalWrite(synhro_pin, HIGH);
	digitalWrite(ledLCD, LOW);
	pinMode(sound_En, OUTPUT);
	digitalWrite(ledPin, HIGH);
	delay(100);
	digitalWrite(ledPin, LOW);
	delay(100);
	digitalWrite(ledPin, HIGH);
	delay(100);
	digitalWrite(ledPin, LOW);
	//digitalWrite(sound_En, LOW);
	digitalWrite(sound_En, HIGH);
	AD9850.powerDown();                              //set signal output to LOW
	//attachInterrupt(2, blink, HIGH);
	rtc.begin(); // initialize RTC

				 // Set the time
				 //  rtc.setHours(hours);
				 //  rtc.setMinutes(minutes);
				 //  rtc.setSeconds(seconds);
				 //
				 //  // Set the date
				 //  rtc.setDay(day);
				 //  rtc.setMonth(month);
				 //  rtc.setYear(year);

				 // you can use also
				// rtc.setTime(hours, minutes, seconds);
				// rtc.setDate(day, month, year);
	info();

	pinMode(pin_ovf_led, OUTPUT);   // for debug leds
	digitalWrite(pin_ovf_led, LOW); // for debug leds
	//pinMode(pin_mc0_led, OUTPUT);   // for debug leds
	//digitalWrite(pin_mc0_led, LOW); // for debug leds



	//volume_Power = analogRead(0)*(3.2 / 1024 * 2);
	//myGLCD.print(String(volume_Power), RIGHT, 1);          // выводим в строке 1
	//setTimer(300000);
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
			digitalWrite(pin_ovf_led, LOW);    // for debug leds
			sound_start = false;
		}
	}

	if (millis() - PowerMillis >= Power_Interval)
	{
		volume_Power = analogRead(0)*(3.2 / 1024 * 2);
		myGLCD.printNumF(volume_Power,1, RIGHT, 1);          // выводим в строке 1
		//myGLCD.print(String(volume_Power), RIGHT, 1);          // выводим в строке 1
		myGLCD.print(String("       "), RIGHT, 40);          // выводим в строке 1
		myGLCD.print(String(rtc.getHours()), 35, 40);          // выводим в строке 1
		myGLCD.print(String("/"), 48, 40);          // выводим в строке 1
		myGLCD.print(String(rtc.getMinutes()), 55, 40);          // выводим в строке 1
		myGLCD.print(String(":"), 67, 40);          // выводим в строке 1
		myGLCD.print(String(rtc.getSeconds()), RIGHT, 40);          // выводим в строке 1
		myGLCD.update();
		PowerMillis = millis();
	
	}


	while (radio.available(&pipeNo))                       // 
	{
		radio.read(&data_in, sizeof(data_in));
		data_out[0] = data_in[0];
		if (data_in[2]==1)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);   // Грузим сообщение 2 байта для автоотправки;
			stopMillis = micros();
			delayMicroseconds(1000);                       // Задержка для получения ответа и завершения процессов на Базе
			sound_run(time_sound, freq_sound);
			info();
			//info_view = false;
		}
		else if (data_in[2] == 2)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);    // Грузим сообщение 2 байта для автоотправки;
			delayMicroseconds(1000);
			//NVIC_EnableIRQ(TCC0_IRQn);                      // Включаем прерывание
		}
		else if (data_in[2] == 3)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);    // Грузим сообщение 2 байта для автоотправки;
			delayMicroseconds(1000);
		//	NVIC_DisableIRQ(TCC0_IRQn);                     // Отключаем прерывание
			TC->CTRLA.reg &= ~TCC_CTRLA_ENABLE;             // Disable TC
			while (TC->SYNCBUSY.bit.ENABLE == 1);           // wait for sync 
			NVIC_DisableIRQ(TCC0_IRQn);                     // Отключаем прерывание
			intterrupt_enable = false;
		}
		else if (data_in[2] == 4)
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);    // Грузим сообщение 2 байта для автоотправки;

			if (digitalRead(synhro_pin) == LOW)
			{
				StartSample = micros();                     // Записать время
				while (true)
				{
					if (micros() - StartSample >= 3000000)
					{
						//myGLCD.setFont(BigFont);
						//myGLCD.print("He""\xA4"" c""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\x9D", CENTER, 80);   // "Нет синхронизации"
						delay(2000);
						break;
					}

					if (digitalRead(synhro_pin) == HIGH)
					{
						while (true)
						{
							if (digitalRead(synhro_pin) == LOW)
							{
								StartSample = micros();
								while (true)
								{
									if (micros() - StartSample >= 3000000-966)
									{
										digitalWrite(ledPin, HIGH);
										StartSample = micros();
									}
									if (micros() - StartSample >= 10000)
									{
										digitalWrite(ledPin, LOW);
									}
								}



								//if (!intterrupt_enable)
								//{
								//	interrupts_start();
								//	intterrupt_enable = true;
								//}
								//break;
							}
						}
						break;
					}
				}
			}


			//delayMicroseconds(20000);
/*
			digitalWrite(synhro_pin, HIGH);
			delayMicroseconds(1000);
			digitalWrite(synhro_pin, LOW);
			delayMicroseconds(20000);
			StartSample = micros();                         // Записать время
*/
	/*		if (!intterrupt_enable)
			{
				interrupts_start();
				intterrupt_enable = true;
				
			}*/
	
	
			/*
			while (true)
			{
				if (micros() - StartSample >= 3000000-990)
				{
					digitalWrite(ledPin, HIGH);
					StartSample = micros();
				}
				if (micros() - StartSample >= 20000)
				{
					digitalWrite(ledPin, LOW);
				}

				//while (radio.available(&pipeNo))                       // 
				//{
					radio.read(&data_in, sizeof(data_in));
				//	data_out[0] = data_in[0];
					if (data_in[2] == 3)
					{
						//radio.writeAckPayload(pipeNo, &data_out, 2);   // Грузим сообщение 2 байта для автоотправки;
						break;
					}
				//}

			}
			*/
		}
		else
		{
			radio.writeAckPayload(pipeNo, &data_out, 2);    // Грузим сообщение 2 байта для автоотправки;
		}

	//	digitalWrite(synhro_pin, LOW);
		time_sound = (data_in[4] << 8) | data_in[5];        // Длительность посылки. Собираем как "настоящие программеры"
		freq_sound = (data_in[6] << 8) | data_in[7];        // Частота генератора. Собираем как "настоящие программеры"
		volume1 = data_in[8];                               // 
		volume2 = data_in[9];                               // Громкость звучания. 

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
