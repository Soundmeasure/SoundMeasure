
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
#include "printf.h"


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

//******************Назначение переменных для хранения № опций меню (клавиш)****************************

int but1, but2, but3, but4, butX, pressed_button;


// +++++++++++++++ Настроки pin +++++++++++++++++++++++++++++++++++++++++++++

#define intensityLCD      9                         // Порт управления яркостью экрана
#define synhro_pin       66                         // Вход для синхронизации Базы и  Трансмиттера
#define alarm_pin         7                         // Порт прерывания по таймеру  DS3231
#define kn_red           43                         // AD4 Кнопка красная +
#define kn_blue          42                         // AD6 Кнопка синяя -
#define vibro            11                         // Вибромотор
#define sounder          53                         // Зуммер
#define LED_PIN13        13                         // Индикация


// -------------------   Настройка часового таймера DS3231 -------------------

int oldsec = 0;
//char* str_mon[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

DS3231 DS3231_clock;
RTCDateTime dt;
//char* daynames[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

boolean isAlarm           = false;                               //
//boolean alarmState        = false;                               //
int alarm_synhro          = 0;                                   //
unsigned long alarm_count = 0;                                   //




//+++++++++++++++++++++++++++++ Внешняя память +++++++++++++++++++++++++++++++++++++++
int deviceaddress = 80;                                          // Адрес микросхемы памяти
int mem_start     = 24;                                          // признак первого включения прибора после сборки. Очистить память  
int adr_mem_start = 1023;                                        // Адрес хранения признака первого включения прибора после сборки
byte hi;                                                         // Старший байт для преобразования числа
byte low;                                                        // Младший байт для преобразования числа

int adr_set_time = 100;                                          // Адрес времени синхронизации модулей




// ++++++++++++++++++++++ + Настройка электронного резистора++++++++++++++++++++++++++++++++++++ +

#define address_AD5252   0x2C                                    // Адрес микросхемы AD5252  
#define control_word1    0x07                                    // Байт инструкции резистор №1
#define control_word2    0x87                                    // Байт инструкции резистор №2
byte resistance = 0x00;                                          // Сопротивление 0x00..0xFF - 0Ом..100кОм
//byte level_resist      = 0;                                    // Байт считанных данных величины резистора

//+++++++++++++++++++++++++++++++

RF24 radio(48, 49);                                                        // DUE

unsigned long timeoutPeriod = 3000;                             // Set a user-defined timeout period. With auto-retransmit set to (15,15) retransmission will take up to 60ms and as little as 7.5ms with it set to (1,15).
										                        // With a timeout period of 1000, the radio will retry each payload for up to 1 second before giving up on the transmission and starting over
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };   // Radio pipe addresses for the 2 nodes to communicate.

byte data_in[24];                                               // Буфер хранения принятых данных
byte data_out[24];                                              // Буфер хранения данных для отправки
volatile unsigned long counter;
unsigned long rxTimer, startTime, stopTime, payloads = 0;
bool TX = 1, RX = 0, role = 0, transferInProgress = 0;

typedef enum { role_ping_out = 1, role_pong_back } role_e;                  // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back" };  // The debug-friendly names of those roles
role_e role_test = role_pong_back;                                          // The role of the current running sketch
byte counter_test = 0;

const char str[] = "My very long string";
extern "C" char *sbrk(int i);
uint32_t message;                                                // Эта переменная для сбора обратного сообщения от приемника;
unsigned long tim1 = 0;                                          // время ответа синхроимпульса
unsigned long timeStartRadio = 0;                                // Время старта радио синхро импульса
unsigned long timeStopRadio = 0;                                 // Время окончания радио синхро импульса

// +++++++++++++++++++++++ Настройки звуковых посылок ++++++++++++++++++++++++++

int time_sound = 50;                                             // Длительность звуковой посылки
int freq_sound = 1800;                                           // Частота звуковой посылки
byte volume1 = 100;                                              // 
byte volume2 = 100;                                              //
volatile byte volume_variant = 0;                                // Управление переключением настройки эл. резисторов. 

//++++++++++++++++++++++++++++++++ Системные переменные ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
unsigned long Start_Power = 0;                                  // Период контроля питания
volatile int count_ms     = 0;                                  // Временные метки
bool wiev_Start_Menu      = false;                              // Разрешение старта меню после синхронизации таймера часов
volatile bool start_enable = false;                             // Старт синхро импульса
bool synhro_enable = false;                                     // Признак успешной синхронизации



//***************** Назначение переменных для хранения текстов*****************************************************

char  txt_Start_Menu[] = "HA""CTPO""\x87""K""\x86";                                                     // "НАСТРОЙКИ"

char  txt_menu1_1[] = "\x86\x9C\xA1""ep.""\x9C""a""\x99""ep""\x9B\x9F\x9D";                             // "Измер.задержки"
char  txt_menu1_2[] = "HA""CTPO""\x87""K""\x86";                                                        // "НАСТРОЙКИ"
char  txt_menu1_3[] = "\x8D""AC""\x91"" C""\x86""HXPO.";                                                // "ЧАСЫ СИНХРО."
char  txt_menu1_4[] = "";                                                                               // "РАБОТА с SD"
char  txt_menu1_5[] = "B\x91XO\x82";                                                                    // "ВЫХОД"       

char  txt_info11[]  = "ESC->PUSH Display";                                                              //

char  txt_delay_measure1[] = "C""\x9D\xA2""xpo ""\xA3""o ""\xA4""a""\x9E\xA1""epy";                     // Синхро по таймеру
char  txt_delay_measure2[] = "C""\x9D\xA2""xpo ""\xA3""o pa""\x99\x9D""o";                              // Синхро по радио 
char  txt_delay_measure3[] = "";                                                                        //
char  txt_delay_measure4[] = "B\x91XO\x82";                                                             // Выход    

char  txt_tuning_menu1[] = "Oc\xA6\x9D\xA0\xA0o\x98pa\xA5";                                             // "Осциллограф"
char  txt_tuning_menu2[] = "C""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\xAF"" ";                        // "Синхронизация"
char  txt_tuning_menu3[] = "PA""\x82\x86""O";                                                           // Радио
char  txt_tuning_menu4[] = "B\x91XO\x82";                                                               // Выход 

char  txt_synhro_menu1[] = "C""\x9D\xA2""xpo pa""\x99\x9D""o";                                          // "Синхро радио"
char  txt_synhro_menu2[] = "C""\x9D\xA2""xpo ""\xA3""po""\x97""o""\x99";                                // "Синхро провод"
char  txt_synhro_menu3[] = "C""\xA4""o""\xA3"" c""\x9D\xA2""xpo.";                                      // "Стоп синхро."
char  txt_synhro_menu4[] = "B\x91XO\x82";                                                               // Выход      

//++++++++++++++++++++++++++++  Настройки АЦП +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*
Примерный порядок работы с АЦП(минимальный набор действий)
Прежде всего необходимо выполнить настройку АЦП с использованием регистра ADC_MR, указав тем самым разрядность преобразования(8 или 10 бит),
режим работы(нормальный / спящий), частоту тактирования АЦП, время выборки и другие параметры.
Также необходимо включить каналы, по которым будет выполняться преобразование с помощью регистра ADC_CHER.
Если предполагается работать с АЦП в режиме «по прерываниям», необходимо настроить контроллер прерываний на обработку сигналов прерываний от АЦП,
указать процедуру обработки прерывания, а также включить выдачу сигналов прерываний на стороне АЦП по определённым событиям(например по окончании преобразования).
Далее производится запуск АЦП с использованием регистра ADC_CR.
Считывание результата преобразования(целого числа) производится либо из регистра результата преобразования по каналу(например, ADC_CDR4 для канала 4),
либо из регистра последнего результата преобразования(ADC_LCDR).Считывание результата нельзя производить во время преобразования!
Поэтому при работе с АЦП в режиме «по готовности» перед считыванием результата необходимо подождать,
пока не установится соответствующий флаг готовности в регистре ADC_SR.Если микроконтроллер настроен на генерацию прерываний по окончании преобразования,
результат считывается в обработчике прерывания.
Результат преобразования, полученный в виде целого числа не отражает значения напряжения на входе АЦП.
Для получения данной величины необходимо выполнить процедуру «масштабирования».Для этого результат преобразования необходимо умножить на величину кванта преобразования.
Величина кванта определяется как
q = Uref / 2n,
где Uref – опорное напряжение АЦП, n – разрядность преобразования(определяется полем LOW_RES регистра ADC_MR).
*/




// ADC speed one channel 480,000 samples/sec (no enable per read)
//           one channel 288,000 samples/sec (enable per read)

#define ADC_MR * (volatile unsigned int *) (0x400C0004)              /*adc mode word*/
#define ADC_CR * (volatile unsigned int *) (0x400C0000)              /*write a 2 to start convertion*/
#define ADC_ISR * (volatile unsigned int *) (0x400C0030)             /*status reg -- bit 24 is data ready*/
#define ADC_ISR_DRDY 0x01000000
																								   //
#define ADC_START 2
#define ADC_LCDR * (volatile unsigned int *) (0x400C0020)            /*last converted low 12 bits*/
#define ADC_DATA 0x00000FFF 
#define ADC_STARTUP_FAST 12
																								   //
#define ADC_CHER * (volatile unsigned int *) (0x400C0010)           /*ADC Channel Enable Register  Только запись*/
#define ADC_CHSR * (volatile unsigned int *) (0x400C0018)           /*ADC Channel Status Register  Только чтение */
#define ADC_CDR0 * (volatile unsigned int *) (0x400C0050)           /*ADC Channel Только чтение */
																								   //#define ADC_ISR_EOC0 0x00000001

//-------------  Настройки аналоговых входов  ------------------------
#define Analog_pinA0 ADC_CHER_CH7    // Вход A0
#define Analog_pinA1 ADC_CHER_CH6    // Вход A1
#define Analog_pinA2 ADC_CHER_CH5    // Вход A2
#define Analog_pinA3 ADC_CHER_CH4    // Вход A3
#define Analog_pinA4 ADC_CHER_CH3    // Вход A4
#define Analog_pinA5 ADC_CHER_CH2    // Вход A5
#define Analog_pinA6 ADC_CHER_CH1    // Вход A6
#define Analog_pinA7 ADC_CHER_CH0    // Вход A7



//+++++++++++++++++++++++  Настройки EEPROM +++++++++++++++++++++++++++++++++++++
unsigned long i2c_eeprom_ulong_read(int addr)
{
	byte raw[4];
	for (byte i = 0; i < 4; i++) raw[i] = i2c_eeprom_read_byte(deviceaddress, addr + i);
	unsigned long &num = (unsigned long&)raw;
	return num;
}
// запись
void i2c_eeprom_ulong_write(int addr, unsigned long num)
{
	byte raw[4];
	(unsigned long&)raw = num;
	for (byte i = 0; i < 4; i++) i2c_eeprom_write_byte(deviceaddress, addr + i, raw[i]);
}

void i2c_eeprom_write_byte(int deviceaddress, unsigned int eeaddress, byte data)
{
	int rdata = data;
	Wire.beginTransmission(deviceaddress);
	Wire.write((int)(eeaddress >> 8)); // MSB
	Wire.write((int)(eeaddress & 0xFF)); // LSB
	Wire.write(rdata);
	Wire.endTransmission();
	delay(10);
}
byte i2c_eeprom_read_byte(int deviceaddress, unsigned int eeaddress) {
	byte rdata = 0xFF;
	Wire.beginTransmission(deviceaddress);
	Wire.write((int)(eeaddress >> 8)); // MSB
	Wire.write((int)(eeaddress & 0xFF)); // LSB
	Wire.endTransmission();
	Wire.requestFrom(deviceaddress, 1);
	if (Wire.available()) rdata = Wire.read();
	return rdata;
}
void i2c_eeprom_read_buffer(int deviceaddress, unsigned int eeaddress, byte *buffer, int length)
{

	Wire.beginTransmission(deviceaddress);
	Wire.write((int)(eeaddress >> 8)); // MSB
	Wire.write((int)(eeaddress & 0xFF)); // LSB
	Wire.endTransmission();
	Wire.requestFrom(deviceaddress, length);
	int c = 0;
	for (c = 0; c < length; c++)
		if (Wire.available()) buffer[c] = Wire.read();

}
void i2c_eeprom_write_page(int deviceaddress, unsigned int eeaddresspage, byte* data, byte length)
{
	Wire.beginTransmission(deviceaddress);
	Wire.write((int)(eeaddresspage >> 8)); // MSB
	Wire.write((int)(eeaddresspage & 0xFF)); // LSB
	byte c;
	for (c = 0; c < length; c++)
		Wire.write(data[c]);
	Wire.endTransmission();

}
void i2c_test()
{
	Serial.println("--------  EEPROM Test  ---------");
	char somedata[] = "this data from the eeprom i2c"; // data to write
	i2c_eeprom_write_page(deviceaddress, 0, (byte *)somedata, sizeof(somedata)); // write to EEPROM
	delay(100); //add a small delay
	Serial.println("Written Done");
	delay(10);
	Serial.print("Read EERPOM:");
	byte b = i2c_eeprom_read_byte(deviceaddress, 0); // access the first address from the memory
	char addr = 0; //first address

	while (b != 0)
	{
		Serial.print((char)b); //print content to serial port
		if (b != somedata[addr])
		{
			break;
		}
		addr++; //increase address
		b = i2c_eeprom_read_byte(0x50, addr); //access an address from the memory
	}
	Serial.println();
	Serial.println();
}

// ******************* Главное меню ********************************
void draw_Start_Menu()
{
//	myGLCD.clrScr();
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRoundRect(0,15, 319, 239);
	but4 = myButtons.addButton(10, 200, 250, 35, txt_Start_Menu);
	butX = myButtons.addButton(279, 199, 40, 40, "W", BUTTON_SYMBOL); // кнопка Часы 
	myGLCD.setColor(VGA_BLACK);
	myGLCD.setBackColor(VGA_WHITE);
	myGLCD.setColor(0, 255, 0);
	myGLCD.setBackColor(0, 0, 0);
	myButtons.drawButtons();
}
void Start_Menu()                    // 
{
	draw_Start_Menu();
	while (1)
	{
		myButtons.setTextFont(BigFont);                      // Установить Большой шрифт кнопок  
		measure_power();
		if (myTouch.dataAvailable() == true)                 // Проверить нажатие кнопок
		{
		//	sound1();
			pressed_button = myButtons.checkButtons();       // Если нажата - проверить что нажато
			if (pressed_button == butX)                      // Нажата вызов часы
			{
				//sound1();
				myGLCD.setFont(BigFont);
				setClockRTC();
				myGLCD.clrScr();
				myButtons.drawButtons();                    // Восстановить кнопки
			}

			//*****************  Меню №1  **************

			//if (pressed_button == but1)                 //  
			//{

			//	myGLCD.clrScr();
			//	myButtons.drawButtons();
			//}

			//if (pressed_button == but2)              / 
			//{

			//	myGLCD.clrScr();
			//	myButtons.drawButtons();
			//}

			//if (pressed_button == but3)             // 
			//{

			//	myGLCD.clrScr();
			//	myButtons.drawButtons();
			//}
			if (pressed_button == but4)             // Работа с SD
			{
				//	draw_Glav_Menu();
				Swich_Glav_Menu();
				myGLCD.clrScr();
				myButtons.drawButtons();
			}
		}
	}
}
void Draw_Glav_Menu()
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x = 0; x<5; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(10, 15 + (45 * x), 250, 50 + (45 * x));
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(10, 15 + (45 * x), 250, 50 + (45 * x));
	}

	myGLCD.print(txt_menu1_1, 20, 25);        // 
	myGLCD.print(txt_menu1_2, 55, 70);
	myGLCD.print(txt_menu1_3, 45, 115);
	myGLCD.print(txt_menu1_4, 45, 160);
	myGLCD.print(txt_menu1_5, 90, 205);
	myGLCD.setColor(255, 255, 255);

}
void Swich_Glav_Menu()
{
	Draw_Glav_Menu();
	while (true)
	{
		delay(10);
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			int	x = myTouch.getX();
			int	y = myTouch.getY();

			if ((x >= 10) && (x <= 250))       // 
			{
				if ((y >= 15) && (y <= 50))    // Button: 1   
				{
					waitForIt(10, 15, 250, 50);
					myGLCD.clrScr();
					menu_delay_measure();
					Draw_Glav_Menu();
				}
				if ((y >= 60) && (y <= 100))   // Button: 2  
				{
					waitForIt(10, 60, 250, 100);
				//	Draw_menu_tuning();
				//	menu_tuning();
					Draw_Glav_Menu();
				}
				if ((y >= 105) && (y <= 145))  // Button: 3  
				{
					waitForIt(10, 105, 250, 145);
					synhro_DS3231_clock();
					Draw_Glav_Menu();
				}
				if ((y >= 150) && (y <= 190))  // Button: 4  
				{
					waitForIt(10, 150, 250, 190);

					Draw_Glav_Menu();
				}
				if ((y >= 195) && (y <= 235))  // Button: 5 "EXIT" Выход
				{
					waitForIt(10, 195, 250, 235);
					break;
				}
			}
		}
	}
}
void Draw_menu_delay_measure()                                    // Отображение меню синхронизации
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x = 0; x<4; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(20, 20 + (50 * x), 300, 60 + (50 * x));
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(20, 20 + (50 * x), 300, 60 + (50 * x));
	}
	myGLCD.print(txt_delay_measure1, CENTER, 30);     // 
	myGLCD.print(txt_delay_measure2, CENTER, 80);
	myGLCD.print(txt_delay_measure3, CENTER, 130);
	myGLCD.print(txt_delay_measure4, CENTER, 180);

}
void menu_delay_measure()                                                // Меню вариантов синхронизации
{
	Draw_menu_delay_measure();
	while (true)
	{
		delay(10);
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			int	x = myTouch.getX();
			int	y = myTouch.getY();

			if ((x >= 20) && (x <= 3000))                                  // 
			{
				if ((y >= 20) && (y <= 60))                                // Button: 1  Синхронизация по таймеру
				{
					waitForIt(20, 20, 300, 60);
					myGLCD.clrScr();
//					synhro_by_timer();
					Draw_menu_delay_measure();
				}
				if ((y >= 70) && (y <= 110))                               // Button: 2 Синхронизация по радио
				{
					waitForIt(20, 70, 300, 110);
					myGLCD.clrScr();
//					synhro_by_radio();                                    //  "Синхронизация по радио"
					Draw_menu_delay_measure();
				}
				if ((y >= 120) && (y <= 160))                             // Button: 3  
				{
					waitForIt(20, 120, 300, 160);
					myGLCD.clrScr();
					//Draw_menu_ADC_RF();                                   // Нарисовать меню регистратора
					//menu_ADC_RF();                                        // Перейти в меню регистратора с синхро RF
					Draw_menu_delay_measure();
				}
				if ((y >= 170) && (y <= 220))                             // Button: 4 "EXIT" Выход
				{
					waitForIt(20, 170, 300, 210);
					break;
				}
			}
		}
	}
}
void Draw_menu_tuning()
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x = 0; x<4; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(30, 20 + (50 * x), 290, 60 + (50 * x));
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(30, 20 + (50 * x), 290, 60 + (50 * x));
	}
	myGLCD.print(txt_tuning_menu1, CENTER, 30);     // 
	myGLCD.print(txt_tuning_menu2, CENTER, 80);
	myGLCD.print(txt_tuning_menu3, CENTER, 130);
	myGLCD.print(txt_tuning_menu4, CENTER, 180);
}
void menu_tuning()   // Меню "Настройка", вызывается из главного меню 
{
	while (true)
	{
		delay(10);
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			int	x = myTouch.getX();
			int	y = myTouch.getY();

			if ((x >= 30) && (x <= 290))       // 
			{
				if ((y >= 20) && (y <= 60))    // Button: 1  "Oscilloscope"
				{
					waitForIt(30, 20, 290, 60);
					myGLCD.clrScr();
				//	oscilloscope();
					Draw_menu_tuning();
				}
				if ((y >= 70) && (y <= 110))   // Button: 2 "Синхронизация мод."
				{
					waitForIt(30, 70, 290, 110);
					myGLCD.clrScr();
					Draw_menu_synhro();
					menu_synhro();
					Draw_menu_tuning();
				}
				if ((y >= 120) && (y <= 160))  // Button: 3  
				{
					waitForIt(30, 120, 290, 160);
					myGLCD.clrScr();
				//	menu_radio();
					Draw_menu_tuning();
				}
				if ((y >= 170) && (y <= 220))  // Button: 4 "EXIT" Выход
				{
					waitForIt(30, 170, 290, 210);
					break;
				}
			}
		}
	}

}

void Draw_menu_synhro()
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x = 0; x<4; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(20, 20 + (50 * x), 300, 60 + (50 * x));
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(20, 20 + (50 * x), 300, 60 + (50 * x));
	}
	myGLCD.print(txt_synhro_menu1, CENTER, 30);     // 
	myGLCD.print(txt_synhro_menu2, CENTER, 80);
	myGLCD.print(txt_synhro_menu3, CENTER, 130);
	myGLCD.print(txt_synhro_menu4, CENTER, 180);
}
void menu_synhro()                                                        // Меню "Настройка", вызывается из главного меню 
{
	while (true)
	{
		delay(10);
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			int	x = myTouch.getX();
			int	y = myTouch.getY();

			if ((x >= 20) && (x <= 300))       // 
			{
				if ((y >= 20) && (y <= 60))                                 // Button: 1  "Синхронизация по радио"
				{
					waitForIt(20, 20, 300, 60);
					myGLCD.clrScr();
					data_out[2] = 2;                                        // Отправить команду синхронизации модулей(включить таймер прерываний)
			//		tuning_mod();                                           // Синхронизация по радио
					Draw_menu_synhro();
				}
				if ((y >= 70) && (y <= 110))                               // Button: 2 "Синхронизация проводная"
				{
					waitForIt(20, 70, 300, 110);
					myGLCD.clrScr();
					data_out[2] = 3;                                        // Отправить команду синхронизации модулей(включить таймер прерываний)
					radio_send_command();
			//		synhro_ware();
					Draw_menu_synhro();
				}
				if ((y >= 120) && (y <= 160))  // Button: 3  
				{
					waitForIt(20, 120, 300, 160);
					myGLCD.clrScr();
					data_out[2] = 4;                                        // Записать команду стоп
//					setup_radio();                                          // Настроить радиоканал
//					delayMicroseconds(500);
					radio_send_command();                                   //  Отправить по радио команду стоп
					//Timer4.stop();
					//Timer5.stop();
					//Timer6.stop();
					Draw_menu_synhro();
				}
				if ((y >= 170) && (y <= 220))  // Button: 4 "EXIT" Выход
				{
					waitForIt(20, 170, 300, 210);
					break;
				}
			}
		}
	}
}







void waitForIt(int x1, int y1, int x2, int y2)
{
	myGLCD.setColor(255, 0, 0);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
	sound1();
	while (myTouch.dataAvailable())
		myTouch.read();
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
}
//++++++++++++++++++++++++++ Конец меню прибора ++++++++++++++++++++++++



void alarmFunction()
{
	DS3231_clock.clearAlarm1();
	dt = DS3231_clock.getDateTime();
//	Serial.println(DS3231_clock.dateFormat("H:i:s", dt));
	myGLCD.setBackColor(0, 0, 0);                   // Синий фон кнопок
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(SmallFont);
	if (alarm_synhro >4)
	{
		alarm_synhro = 0;
		alarm_count++;
		myGLCD.print(DS3231_clock.dateFormat("s", dt), 190, 0);
		wiev_Start_Menu = true;
		start_enable = true;
	}
	alarm_synhro++;
	myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s - ", dt), 5, 0);
	myGLCD.printNumI(alarm_synhro, 300, 0);
	myGLCD.setFont(BigFont);
}


void synhro_DS3231_clock()
{
	detachInterrupt(alarm_pin);
	DS3231_clock.clearAlarm1();
	DS3231_clock.clearAlarm2();
	pinMode(alarm_pin, INPUT);
	digitalWrite(alarm_pin, HIGH);
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(BigFont);
	count_ms = 0;
	synhro_enable = false;
	alarm_count = 0;
	wiev_Start_Menu = false;
	start_enable = false;

	if (digitalRead(synhro_pin) != LOW)
	{
		myGLCD.setColor(VGA_RED);
		myGLCD.print("C""\x9D\xA2""xpo ""\xA2""e ""\xA3""o""\x99\x9F\xA0\xAE\xA7""e""\xA2", CENTER, 80);   // Синхро не подключен
		delay(2000);
	}
	else
	{
		//adr_set_time = 100;                                          // Адрес времени синхронизации модулей
		//unsigned long Start_unixtime = dt.unixtime;


		dt = DS3231_clock.getDateTime();
		data_out[2] = 8;                                      // Отправить команду синхронизации часов
		data_out[12] = dt.year - 2000;                      // 
		data_out[13] = dt.month;                            // 
		data_out[14] = dt.day;                              // 
		data_out[15] = dt.hour;                             //
		data_out[16] = dt.minute;                           // 

		radio_send_command();

		unsigned long StartSynhro = micros();               // Записать время
	
		while (!synhro_enable) 
		{
			if (micros() - StartSynhro >= 20000000)
			{
				myGLCD.setFont(BigFont);
				myGLCD.setColor(VGA_RED);
				myGLCD.print("He""\xA4"" c""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\x9D", CENTER, 80);   // "Нет синхронизации"
				delay(2000);
				break;
			}
			if (digitalRead(synhro_pin) == HIGH)
			{
				do {} while (digitalRead(synhro_pin));                                       // Поиск окончания синхроимпульса
				DS3231_clock.setDateTime(dt.year, dt.month, dt.day, dt.hour, dt.minute, 00); // Записываем новое синхронизированное время
				alarm_synhro = 0;                                                            // Сбрасываем счетчик синхроимпульсов
				DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);                     // DS3231_EVERY_SECOND //Прерывание каждую секунду
				while (digitalRead(alarm_pin) != HIGH){}                                     // Синхронизируем импульсы прерывания
				while (digitalRead(alarm_pin) == HIGH){}                                     // Синхронизируем импульсы прерывания

				while (true)                                                                 // Устанавливаем время нового старта синхронизации
				{
					dt = DS3231_clock.getDateTime();
					if (oldsec != dt.second)
					{
						myGLCD.print(DS3231_clock.dateFormat("H:i:s", dt), 5, 0);
						oldsec = dt.second;
					}
					if (dt.second == 10)
					{
						break;
					}
				}
				if(!synhro_enable) attachInterrupt(alarm_pin, alarmFunction, FALLING);      // прерывание вызывается только при смене значения на порту с LOW на HIGH
				myGLCD.setFont(BigFont);
				myGLCD.setColor(VGA_LIME);
				myGLCD.print("C""\x9D\xA2""xpo OK!", CENTER, 80);                            // Синхро ОК!
				myGLCD.setColor(255, 255, 255);
				delay(1000);
				synhro_enable = true;
			}
		}  // Синхронизация выполнена, завершить программу
	}
}



void radio_send_command()                                   // Синронизация модулей
{
	//detachInterrupt(kn_red);
	//detachInterrupt(kn_blue);
	tim1 = 0;                                                 // время ответа синхроимпульса
//	volume_variant = 3;                                       // Установить уровень громкости 1 канала усилителя Трансмиттера
	radio.stopListening();                                    // Во-первых, перестаньте слушать, чтобы мы могли поговорить.
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setFont(SmallFont);
//	if (kn != 0) set_volume(volume_variant, kn);
	data_out[0] = counter_test;
	data_out[1] = 1;                                          //  
	//data_out[2] = 2;                                        // Команда задана ранее при вызове программы
	data_out[3] = 1;                                          //
	data_out[4] = highByte(time_sound);                       // Старший байт Отправить длительность звуковой посылки
	data_out[5] = lowByte(time_sound);                        // Младший байт Отправить длительность звуковой посылки
	data_out[6] = highByte(freq_sound);                       // Старший байт Отправить частоту звуковой посылки
	data_out[7] = lowByte(freq_sound);                        // Младший байт Отправить частоту звуковой посылки 
	data_out[8] = volume1;                                    // 
	data_out[9] = volume2;                                    // 

	timeStartRadio = micros();                                // Записать время старта радио синхро импульса

	if (!radio.write(&data_out, sizeof(data_out)))
	{
		myGLCD.setColor(VGA_LIME);                            // Вывести на дисплей время ответа синхроимпульса
		myGLCD.print("     ", 90 - 40, 178);                  // Вывести на дисплей время ответа синхроимпульса
		myGLCD.printNumI(0, 90 - 32, 178);                    // Вывести на дисплей время ответа синхроимпульса
		myGLCD.setColor(255, 255, 255);
	}
	else
	{
		if (!radio.available())
		{
			//	Serial.println(F("Blank Payload Received."));
		}
		else
		{
			while (radio.isAckPayloadAvailable())
			{
				timeStopRadio = micros();                     // Записать время получения ответа.  
				radio.read(&data_in, sizeof(data_in));
				tim1 = timeStopRadio - timeStartRadio;        // Получить время ответа синхро импульса
				myGLCD.print("Delay: ", 5, 178);              // Вывести на дисплей время ответа синхроимпульса
				myGLCD.print("     ", 90 - 40, 178);          // Вывести на дисплей время ответа синхроимпульса
				myGLCD.setColor(VGA_LIME);                    // Вывести на дисплей время ответа синхроимпульса
				if (tim1 < 999)                               // Подравнять вывод чисел на дисплей 
				{
					myGLCD.printNumI(tim1, 90 - 32, 178);     // Вывести на дисплей время ответа синхроимпульса
				}
				else
				{
					myGLCD.printNumI(tim1, 90 - 40, 178);     // Вывести на дисплей время ответа синхроимпульса
				}
				myGLCD.setColor(255, 255, 255);
				myGLCD.print("microsec", 90, 178);            // Вывести на дисплей время ответа синхроимпульса
				counter_test++;
			}
		}
	}
}
void radio_test_ping()
{
	detachInterrupt(kn_red);
	detachInterrupt(kn_blue);
	myGLCD.clrScr();
	Serial.println(F("Test ping."));
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print("TEST PING", CENTER, 10);
	myGLCD.print(txt_info11, CENTER, 221);            // Кнопка "ESC -> PUSH"
	myGLCD.setBackColor(0, 0, 0);
	setup_radio();
	role_test = role_ping_out;
	tim1 = 0;
	volume_variant = 3;                                       //  Установить уровень громкости 1 канала усилителя Трансмиттера
	int x_touch, y_touch;

	while (1)
	{
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_touch = myTouch.getX();
			y_touch = myTouch.getY();

			if ((x_touch >= 2) && (x_touch <= 319))               //  Область экрана
			{
				if ((y_touch >= 1) && (y_touch <= 239))           // Delay row
				{
					sound1();
					break;
				}
			}
		}
		if (role_test == role_ping_out)
		{
			radio.stopListening();                                  // Во-первых, перестаньте слушать, чтобы мы могли поговорить.
			myGLCD.print("Sending", 1, 40);
			myGLCD.print("    ", 125, 40);
			myGLCD.setColor(VGA_LIME);
			if (counter_test<10)
			{
				myGLCD.printNumI(counter_test, 155, 40);
			}
			else if (counter_test>9 && counter_test<100)
			{
				myGLCD.printNumI(counter_test, 155 - 16, 40);
			}
			else if (counter_test > 99)
			{
				myGLCD.printNumI(counter_test, 155 - 32, 40);
			}
			myGLCD.setColor(255, 255, 255);
			myGLCD.print("payload", 178, 40);
	//		if (kn != 0) set_volume(volume_variant, kn);

			data_out[0] = counter_test;
			data_out[1] = 1;                                       //1= Отправить команду ping звуковую посылку 
			data_out[2] = 1;                    //
			data_out[3] = 1;                    //
			data_out[4] = highByte(time_sound);                     //1= Отправить звуковую посылку 
			data_out[5] = lowByte(time_sound);                    //1= Отправить звуковую посылку 
			data_out[6] = highByte(freq_sound);                    //1= Отправить звуковую посылку 
			data_out[7] = lowByte(freq_sound);                   //1= Отправить звуковую посылку 
			data_out[8] = volume1;
			data_out[9] = volume2;


			//int value = 3000;
			//// разбираем
			//byte hi = highByte(value);
			//byte low = lowByte(value);

			//// тут мы эти hi,low можем сохраить, прочитать из eePROM

			//int value2 = (hi << 8) | low; // собираем как "настоящие программеры"
			//int value3 = word(hi, low); // или собираем как "ардуинщики"

			//int time_sound = 200;
			//int freq_sound = 1850;



			unsigned long time = micros();                          // Take the time, and send it.  This will block until complete   

			if (!radio.write(&data_out, sizeof(data_out)))
				//if (!radio.write(&data_out, 2)) 
			{
				Serial.println(F("failed."));
			}
			else
			{
				if (!radio.available())
				{
					Serial.println(F("Blank Payload Received."));
				}
				else
				{
					while (radio.isAckPayloadAvailable())
					{
						unsigned long tim = micros();
						radio.read(&data_in, sizeof(data_in));

						//	radio.read(&data_in, sizeof(data_in));
						tim1 = tim - time;
						myGLCD.print("Respons", 1, 60);
						myGLCD.print("    ", 125, 60);
						myGLCD.setColor(VGA_LIME);
						if (data_in[0]<10)
						{
							myGLCD.printNumI(data_in[0], 155, 60);
						}
						else if (data_in[0]>9 && data_in[0]<100)
						{
							myGLCD.printNumI(data_in[0], 155 - 16, 60);
						}
						else if (data_in[0] > 99)
						{
							myGLCD.printNumI(data_in[0], 155 - 32, 60);
						}
						myGLCD.setColor(255, 255, 255);
						myGLCD.print("round", 178, 60);
						myGLCD.print("Delay: ", 1, 80);
						myGLCD.print("     ", 100, 80);
						myGLCD.setColor(VGA_LIME);
						if (tim1<999)
						{
							myGLCD.printNumI(tim1, 155 - 32, 80);
						}
						else
						{
							myGLCD.printNumI(tim1, 155 - 48, 80);
						}
						myGLCD.setColor(255, 255, 255);
						myGLCD.print("microsec", 178, 80);
						counter_test++;
					}
				}
			}
			//attachInterrupt(kn_red, volume_up, FALLING);
			//attachInterrupt(kn_blue, volume_down, FALLING);
			delay(1000);
		}
	}
	while (!myTouch.dataAvailable()) {}
	delay(50);
	while (myTouch.dataAvailable()) {}
}


void measure_power()
{                                                     // Программа измерения напряжения питания с делителем 1/2 
												      // Установить резистивный делитель +15к общ 10к на разъем питания
	uint32_t logTime1 = 0;
	logTime1 = millis();
	if (logTime1 - Start_Power > 500)                 //  индикация 
	{
		Start_Power = millis();
		int m_power = 0;
		float ind_power = 0;
		ADC_CHER = Analog_pinA3;                      // Подключить канал А3, разрядность 12
		ADC_CR = ADC_START; 	                      // Запустить преобразование
		while (!(ADC_ISR_DRDY));                      // Ожидание конца преобразования
		m_power = ADC->ADC_CDR[4];                    // Считать данные с канала A3
		ind_power = m_power *(3.2 / 4096 * 2);        // Получить напряжение в вольтах
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.setFont(SmallSymbolFont);
		if (ind_power > 4.1)
		{
			myGLCD.setColor(VGA_LIME);
			myGLCD.drawRoundRect(279, 149, 319, 189);
			myGLCD.setColor(VGA_WHITE);
			myGLCD.print("\x20", 295, 155);
		}
		else if (ind_power > 4.0 && ind_power < 4.1)
		{
			myGLCD.setColor(VGA_LIME);
			myGLCD.drawRoundRect(279, 149, 319, 189);
			myGLCD.setColor(VGA_WHITE);
			myGLCD.print("\x21", 295, 155);
		}
		else if (ind_power > 3.9 && ind_power < 4.0)
		{
			myGLCD.setColor(VGA_WHITE);
			myGLCD.drawRoundRect(279, 149, 319, 189);
			myGLCD.setColor(VGA_WHITE);
			myGLCD.print("\x22", 295, 155);
		}
		else if (ind_power > 3.8 && ind_power < 3.9)
		{
			myGLCD.setColor(VGA_YELLOW);
			myGLCD.drawRoundRect(279, 149, 319, 189);
			myGLCD.setColor(VGA_WHITE);
			myGLCD.print("\x23", 295, 155);
		}
		else if (ind_power < 3.8)
		{
			myGLCD.setColor(VGA_RED);
			myGLCD.drawRoundRect(279, 149, 319, 189);
			myGLCD.setColor(VGA_WHITE);
			myGLCD.print("\x24", 295, 155);
		}
		myGLCD.setFont(SmallFont);
		myGLCD.setColor(VGA_WHITE);
		myGLCD.printNumF(ind_power, 1, 289, 172);
	}
}

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
}
void setup_resistor()
{
	Wire.beginTransmission(address_AD5252);        // transmit to device
	Wire.write(byte(control_word1));               // sends instruction byte  
	Wire.write(0);                                 // sends potentiometer value byte  
	Wire.endTransmission();                        // stop transmitting
	Wire.beginTransmission(address_AD5252);        // transmit to device
	Wire.write(byte(control_word2));               // sends instruction byte  
	Wire.write(0);                                 // sends potentiometer value byte  
	Wire.endTransmission();                        // stop transmitting
}







void clean_mem()
{
	byte b = i2c_eeprom_read_byte(deviceaddress, adr_mem_start);                                  // Получить признак первого включения прибора после сборки
	if (b != mem_start)
	{
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print("O""\xA7\x9D""c""\xA4\x9F""a ""\xA3""a""\xA1\xAF\xA4\x9D", CENTER, 60);      // "Очистка памяти"
		for (int i = 0; i < 1024; i++)
		{
			i2c_eeprom_write_byte(deviceaddress, i, 0x00);
			delay(10);
		}
		i2c_eeprom_write_byte(deviceaddress, adr_mem_start, mem_start);
//		i2c_eeprom_ulong_write(adr_set_timeSynhro, set_timeSynhro);                              // Записать  время 
		myGLCD.print("                 ", CENTER, 60);                                           // "Очистка памяти"
	}
}
void sound1()
{
	digitalWrite(sounder, HIGH);
	delay(100);
	digitalWrite(sounder, LOW);
	delay(100);
}
void vibro1()
{
	digitalWrite(vibro, HIGH);
	delay(100);
	digitalWrite(vibro, LOW);
	delay(100);
}

void setup_pin()
{
	pinMode(kn_red, INPUT);
	pinMode(kn_blue, INPUT);
	pinMode(intensityLCD, OUTPUT);
	pinMode(synhro_pin, INPUT);
	pinMode(LED_PIN13, OUTPUT);
	digitalWrite(LED_PIN13, LOW);
	digitalWrite(intensityLCD, LOW);
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
}

void setup_radio()
{
	radio.begin();
	radio.setAutoAck(1);                    // Ensure autoACK is enabled
											//radio.setPALevel(RF24_PA_MAX);
	radio.setPALevel(RF24_PA_HIGH);
	//radio.setPALevel(RF24_PA_LOW);           // Set PA LOW for this demonstration. We want the radio to be as lossy as possible for this example.
	radio.setDataRate(RF24_1MBPS);
	//	radio.setDataRate(RF24_250KBPS);
	radio.enableAckPayload();               // Allow optional ack payloads
	radio.setRetries(0, 1);                 // Smallest time between retries, max no. of retries
	radio.setPayloadSize(sizeof(data_out));                // Here we are sending 1-byte payloads to test the call-response speed
														   //radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed
	radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();                 // Start listening
	radio.printDetails();                   // Dump the configuration of the rf unit for debugging

	role = role_ping_out;                  // Become the primary transmitter (ping out)
	radio.openWritingPipe(pipes[0]);
	radio.openReadingPipe(1, pipes[1]);
}


void setup()
{
	Serial.begin(115200);
	printf_begin();
	Serial.println(F("Setup start!"));
	setup_pin();
	Wire.begin();
	DS3231_clock.begin();
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
	setup_radio();                                // Настройка радио канала

	DS3231_clock.begin();
	// Disarm alarms and clear alarms for this example, because alarms is battery backed.
	// Under normal conditions, the settings should be reset after power and restart microcontroller.
	DS3231_clock.armAlarm1(false);
	DS3231_clock.armAlarm2(false);
	DS3231_clock.clearAlarm1();
	DS3231_clock.clearAlarm2();

	//DS3231_clock.setDateTime(__DATE__, __TIME__);
	// Enable output

	DS3231_clock.setOutput(DS3231_1HZ);
	DS3231_clock.enableOutput(true);
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

// Звуковая сигнализация окончания setup 
	sound1();
	delay(100);
	sound1();
	delay(100);
	vibro1();
	delay(100);
	vibro1();


	clean_mem();                                                     // Очистка памяти при первом включении прибора  
	alarm_synhro = 0;

	myGLCD.setFont(SmallFont);
	
	while (digitalRead(alarm_pin) != HIGH)
	{

	}

	while (digitalRead(alarm_pin) == HIGH)
	{

	}

	DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);         //DS3231_EVERY_SECOND //Каждую секунду
	while (true)
	{
		dt = DS3231_clock.getDateTime();
		if (oldsec != dt.second)
		{
			myGLCD.print(DS3231_clock.dateFormat("H:i:s", dt), 5, 0);
			oldsec = dt.second;
		}
		if (dt.second == 0 || dt.second == 10 || dt.second == 20 || dt.second == 30 || dt.second == 40 || dt.second == 50)
		{
			break;
		}
	} 
	//delayMicroseconds(100000);
	attachInterrupt(alarm_pin, alarmFunction, FALLING);      // прерывание вызывается только при смене значения на порту с LOW на HIGH
  //  attachInterrupt(alarm_pin, alarmFunction, RISING);     // прерывание вызывается только при смене значения на порту с HIGH на LOW
	Serial.println(F("Setup Ok!"));
}

void loop()
{
	if (wiev_Start_Menu)
	{
		Start_Menu();
	}
}
