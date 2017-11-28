
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
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <DS3231.h>


extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];
extern uint8_t SevenSegNumFont[];

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
DS3231  DS3231_clock;

RTCDateTime dt;
//boolean isAlarm           = false;                               //

volatile bool alarm_synhro = false;                               //


//+++++++++++++++++++++++++++++ Внешняя память +++++++++++++++++++++++++++++++++++++++
int deviceaddress = 80;                                          // Адрес микросхемы памяти
int mem_start     = 24;                                          // признак первого включения прибора после сборки. Очистить память  
int adr_mem_start = 500;                                         // Адрес хранения признака первого включения прибора после сборки
byte hi;                                                         // Старший байт для преобразования числа
byte low;                                                        // Младший байт для преобразования числа


volatile int adr_count1_kn = 2;                                  // Адрес хранения уровня громкости 1 канала усилителя Sound Test Base 
volatile int adr_count2_kn = 4;                                  // Адрес хранения уровня громкости 2 канала усилителя Sound Test Base
volatile int adr_count3_kn = 6;                                  // Адрес хранения уровня громкости 1 канала трансмиттера 
volatile int adr_count4_kn = 8;                                  // Адрес хранения уровня громкости 2 канала трансмиттера 
int adr_set_timeZero       = 20;                                  // 4 байта Адрес хранения уровня задержки 0 расстояния
int adr_t_trigger          = 30;                                  // Адрес хранения уровня порога
int adr_volume1            = 40;                                  // Адрес хранения уровня порога
int adr_volume2            = 44;
int adr_volume3            = 48;
int adr_volume4            = 52;
int adr_mode               = 56;

int adr_start_unixtimetime = 100;                                // Адрес времени старта синхронизации модулей
int adr_start_year         = 104;                                // Адрес времени старта синхронизации модулей                                
int adr_start_month        = 105;                                // Адрес времени старта синхронизации модулей
int adr_start_day          = 106;                                // Адрес времени старта синхронизации модулей
int adr_start_hour         = 107;                                // Адрес времени старта синхронизации модулей
int adr_start_minute       = 108;                                // Адрес времени старта синхронизации модулей
int adr_start_second       = 109;                                // Адрес времени старта синхронизации модулей
int adr_ms_delay           = 110;                                // Адрес задержки синхронизации модулей (4 байта)


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
int x_osc, y_osc;
#define page_max  12
unsigned long Start_Power  = 0;                                  // Период контроля питания
volatile int count_ms      = 0;                                  // Временные метки
bool wiev_Start_Menu       = false;                              // Разрешение старта меню после синхронизации таймера часов
volatile bool start_synhro = false;                              // Старт синхро импульса
bool synhro_enable         = false;                              // Признак успешной синхронизации
unsigned long StartSample  = 0;
int Page_count             = 0;
int scale_strob            = 2;
int page                   = 0;
int xpos;
#define ms_info            0                                       // 
#define line_info          1                                       // 
int Sample_osc[240];
int Synhro_osc[240][2];
bool trig_sin = false;                                          // Есть срабатывание триггера
int page_trig = 0;
int dgvh;
int mode                   = 3;                                  //Время развертки  
int mode1                  = 0;                                  //Переключение чувствительности
int dTime                  = 2;
int x_dTime                = 276;
int tmode                  = 5;
int t_in_mode              = 0;
int Trigger                = 0;
float koeff_h              = 7.759 * 4;
volatile int kn            = 0;
byte counter_kn            = 0;
byte Chanal_volume         = 0;
float koeff_volume[]       = { 0.0, 1.0, 1.4, 2.0, 2.8, 4.0, 5.6, 8.0, 11.2, 15.87 };
const char* proc_volume[]  = { " 0%"," 6%"," 8%","12%","17%","25%","35%","50%","70%","99%" };
const char* proc_porog[]   = { "Off"," 20%"," 30%"," 40%"," 60%", " 70%"," 80%","100%"};
int volume_porog[]         = { 0, 511, 767, 1023, 1535, 1784, 2047,2550 };
float StartMeasure         = 0;
float EndMeasure           = 0;
unsigned long set_timeZero = 0;
int ypos_osc1_0;
int ypos_osc2_0;
int ypos_trig;
int ypos_trig_old;
int OldSample_osc[254][20];
const int hpos = 95; //set 0v on horizontal  grid
int Channel_x = 0;
volatile unsigned long StartSynhro = 0;
volatile bool alarm_enable = false;
//volatile bool Interrupt_enable = false;                    // Разрешить выполнение прерывания
volatile bool measure_enable = false;                        // Разрешить выполнение формирования временных меток





//***************** Назначение переменных для хранения текстов*****************************************************

char  txt_Start_Menu[] = "HA""CTPO""\x87""K""\x86";                                                     // "НАСТРОЙКИ"
char  txt_menu1_1[] = "\x86\x9C\xA1""ep.""\x9C""a""\x99""ep""\x9B\x9F\x9D";                             // "Измер.задержки"
char  txt_menu1_2[] = "KOHTPO""\x88\x92"" ""\x8E\x8A""MA";                                              // "КОНТРОЛЬ ШУМA"
char  txt_menu1_3[] = "C""\x86""HXPOH""\x86\x85""A""\x8C\x86\x95";                                      // "СИНХРОНИЗАЦИЯ"
char  txt_menu1_4[] = "KOHTPO""\x88\x92"" C""\x86""HXPO.";                                              // "КОНТРОЛЬ СИНХРО."
char  txt_menu1_5[] = "B\x91XO\x82";                                                                    // "ВЫХОД"       

char  txt_delay_measure1[] = "C""\x9D\xA2""xpo ""\xA3""o ""\xA4""a""\x9E\xA1""epy";                     // Синхро по таймеру
char  txt_delay_measure2[] = "";                              // Синхро по радио 
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

char  txt_info29[] = "Stop->PUSH Disp";
char  txt_info11[] = "ESC->PUSH Display";                                                              // "ESC->PUSH Display"





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
	//Interrupt_enable = true;

		//	myGLCD.clrScr();
		myGLCD.setColor(0, 0, 0);
		myGLCD.fillRoundRect(0, 15, 319, 239);
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
			pressed_button = myButtons.checkButtons();       // Если нажата - проверить что нажато
			if (pressed_button == butX)                      // Нажата вызов часы
			{
				myGLCD.setFont(BigFont);
				setClockRTC();
				myGLCD.clrScr();
				myButtons.drawButtons();                    // Восстановить кнопки
			}

			//*****************  Меню №1  **************

			if (pressed_button == but4)             // Работа с SD
			{
				Swich_Glav_Menu();
				myGLCD.clrScr();
				myButtons.drawButtons();
			}
		}
		if (kn == 1)
		{
			kn = 0;
			synhro_by_main();
			myGLCD.clrScr();
			myButtons.drawButtons();
		}

		dt = DS3231_clock.getDateTime();
		if (oldsec != dt.second)
		{
			myGLCD.setBackColor(0, 0, 0);                   //  
			myGLCD.setColor(255, 255, 255);
			myGLCD.setFont(SmallFont);
			myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s", dt), 10, 3);
			myGLCD.setFont(BigFont);
			oldsec = dt.second;
		}
		delay(100);
	}
}
void Draw_Glav_Menu()
{
	if (!alarm_enable)
	{
		myGLCD.clrScr();
		myGLCD.setFont(BigFont);
		myGLCD.setBackColor(0, 0, 255);
		for (int x = 0; x < 5; x++)
		{
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect(20, 15 + (45 * x), 300, 50 + (45 * x));
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect(20, 15 + (45 * x), 300, 50 + (45 * x));
		}

		myGLCD.print(txt_menu1_1, CENTER, 25);        // 
		myGLCD.print(txt_menu1_2, CENTER, 70);
		myGLCD.print(txt_menu1_3, CENTER, 115);
		myGLCD.print(txt_menu1_4, CENTER, 160);
		myGLCD.print(txt_menu1_5, CENTER, 205);
		myGLCD.setColor(255, 255, 255);
	}
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

			if ((x >= 20) && (x <= 3000))       // 
			{
				if ((y >= 15) && (y <= 50))    // Button: 1   
				{
					waitForIt(20, 15, 300, 50);
					myGLCD.clrScr();
					synhro_by_timer();                                    // Синхронизация по таймеру 
					Draw_Glav_Menu();
				}
				if ((y >= 60) && (y <= 100))   // Button: 2  
				{
					waitForIt(20, 60, 300, 100);
					oscilloscope();
					Draw_Glav_Menu();
				}
				if ((y >= 105) && (y <= 145))  // Button: 3  
				{
					waitForIt(20, 105, 300, 145);
					synhro_DS3231_clock();
					Draw_Glav_Menu();
				}
				if ((y >= 150) && (y <= 190))  // Button: 4  
				{
					waitForIt(20, 150, 300, 190);
					wiev_synhro();
					Draw_Glav_Menu();
				}
				if ((y >= 195) && (y <= 235))  // Button: 5 "EXIT" Выход
				{
					waitForIt(20, 195, 300, 235);
					break;
				}
			}
		}


		dt = DS3231_clock.getDateTime();
		if (oldsec != dt.second)
		{
			myGLCD.setBackColor(0, 0, 0);                   //  
			myGLCD.setColor(255, 255, 255);
			myGLCD.setFont(SmallFont);
			myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s", dt), 10, 3);
			myGLCD.setFont(BigFont);
			oldsec = dt.second;
		}
		delay(100);
	}
}
void Draw_menu_delay_measure()                                    // Отображение меню синхронизации
{
	if (!alarm_enable)
	{
		myGLCD.clrScr();
		myGLCD.setFont(BigFont);
		myGLCD.setBackColor(0, 0, 255);
		for (int x = 0; x < 4; x++)
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
	
					Draw_menu_delay_measure();
				}
				if ((y >= 70) && (y <= 110))                               // Button: 2 Синхронизация по радио
				{
					waitForIt(20, 70, 300, 110);
					myGLCD.clrScr();

					Draw_menu_delay_measure();
				}
				if ((y >= 120) && (y <= 160))                             // Button: 3  
				{
					waitForIt(20, 120, 300, 160);
					myGLCD.clrScr();

					Draw_menu_delay_measure();
				}
				if ((y >= 170) && (y <= 220))                             // Button: 4 "EXIT" Выход
				{
					waitForIt(20, 170, 300, 210);
					break;
				}
			}
		}
		delay(100);
	}
}
void Draw_menu_tuning()
{
	if (!alarm_enable)
	{
		myGLCD.clrScr();
		myGLCD.setFont(BigFont);
		myGLCD.setBackColor(0, 0, 255);
		for (int x = 0; x < 4; x++)
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
					oscilloscope();
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
	if (!alarm_enable)
	{
		myGLCD.clrScr();
		myGLCD.setFont(BigFont);
		myGLCD.setBackColor(0, 0, 255);
		for (int x = 0; x < 4; x++)
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
	alarm_enable = true;
	DS3231_clock.clearAlarm1();
	dt = DS3231_clock.getDateTime();
	if (alarm_synhro >1)
	{
		alarm_synhro = 0;
		start_synhro = true;
		StartSynhro = micros();                                            // Записать время
		if (measure_enable) Timer7.start(scale_strob * 1000);              // Включить формирование  временных меток на экране

	}
	alarm_synhro++;
	alarm_enable = false;
}
void sevenHandler()                                                        // Timer7 -  запись временных меток  в массив для вывода на экран
{
	count_ms += scale_strob;
	if(xpos >0) Synhro_osc[xpos-1][ms_info] = count_ms- scale_strob;       // Записать временные метки в массив для вывода на экран
	if(xpos >0) Synhro_osc[xpos-1][line_info] = 4095;                      // Записать временные метки в массив для вывода на экран
}

void DrawGrid()
{
	if (!alarm_enable)
	{
		myGLCD.setColor(VGA_GREEN);                                        // Цвет сетки                                 
		for (dgvh = 0; dgvh < 7; dgvh++)                                   // Нарисовать сетку
		{
			myGLCD.drawLine(dgvh * 40, 0, dgvh * 40, 159);
			if (dgvh < 5)
			{
				myGLCD.drawLine(0, (dgvh * 40), 239, (dgvh * 40));
			}
		}
	}
}
void DrawGrid1()
{
	if (!alarm_enable)
	{
		myGLCD.setColor(0, 200, 0);
		for (dgvh = 0; dgvh < 7; dgvh++)                                  // Нарисовать сетку
		{
			myGLCD.drawLine(dgvh * 40, 0 + 80, dgvh * 40, 159);
			if (dgvh < 3)
			{
				myGLCD.drawLine(0, (dgvh * 40) + 80, 239, (dgvh * 40) + 80);
			}
		}
		myGLCD.setColor(255, 255, 255);                                  // Белая
	}
}
void set_volume(int reg_module, byte count_vol)
{
	/*
	reg_module = 1  Установить уровень громкости 1 канала усилителя Sound Test Base
	reg_module = 2  Установить уровень громкости 2 канала усилителя Sound Test Base
	reg_module = 3  Установить уровень громкости 1 канала усилителя Трансмиттера
	reg_module = 4  Установить уровень громкости 2 канала усилителя Трансмиттера (не задействован)
	*/
	myGLCD.setFont(SmallFont);
	if (count_vol != 0)
	{
		byte b = count_vol;
		switch (reg_module)
		{
		case 1:
			resistor(1, 16 * koeff_volume[b]);
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print(proc_volume[b], 38, 185);
			break;
		case 2:
			resistor(2, 16 * koeff_volume[b]);
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print(proc_volume[b], 124, 185);
			break;
		case 3:
			volume1 = 16 * koeff_volume[b];
			data_out[2] = 6;
			radio_send_command();
			myGLCD.print(proc_volume[b], 208, 185);
			break;
		case 4:
			volume2 = 16 * koeff_volume[b];

			break;
		case 5:
			b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
			resistor(1, 16 * koeff_volume[b]);
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print(proc_volume[b], 38, 185);
			b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
			myGLCD.print(proc_volume[b], 124, 185);
			resistor(2, 16 * koeff_volume[b]);
			b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
			myGLCD.print(proc_volume[b], 208, 185);
			volume1 = 16 * koeff_volume[b];
			data_out[2] = 6;
			radio_send_command();
			break;
		case 6:
			b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
			resistor(1, 16 * koeff_volume[b]);
			b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
			resistor(2, 16 * koeff_volume[b]);
			b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
			volume1 = 16 * koeff_volume[b];
			data_out[2] = 6;
			radio_send_command();
			break;
		default:
			break;
		}
	}
}

void synhro_by_main()
{
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.clrScr();
	set_volume(6, 1);                                             // Отобразить уровень громкости  каналов усилителя на Базе
	myGLCD.setColor(255, 255, 255);
	tmode = i2c_eeprom_read_byte(deviceaddress, adr_t_trigger);    //Восстановить уровень триггера порога
	Trigger = volume_porog[tmode];                                // установить уровень триггера порога
	mode = i2c_eeprom_read_byte(deviceaddress, adr_mode);
	chench_mode(mode);
	count_ms = scale_strob;
	for (xpos = 0; xpos < 240; xpos++)                            // Стереть старые данные отображения и временных меток
	{
		Synhro_osc[xpos][ms_info] = 0;                            // Стереть старые данные временных меток
		Synhro_osc[xpos][line_info] = 0;                                          // Стереть старые данные временных меток
	}
	unsigned long Control_synhro = micros();                      // Переменная для контроля наличия синхроимпульса
	data_out[2] = 2;                                              // Формируем команду измерения задержки по синхроимпульсу
	radio_send_command();                                         // Отправляем команду измерения задержки по синхроимпульсу   
	DrawGrid1();                                                  // Нарисовать сетку
	measure_enable = true;
	while (1)
	{
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_osc = myTouch.getX();
			y_osc = myTouch.getY();

			if ((x_osc >= 2) && (x_osc <= 240))                 // Выход из программы. Нажать на область экрана
			{
				if ((y_osc >= 1) && (y_osc <= 160))             // 
				{
					Timer7.stop();                              //  Остановить синхронизацию вывода на экран
					measure_enable = false;
					break;                                      //
				}                                               //
			}                                                   //
			/*
			myGLCD.setBackColor(0, 0, 255);
			myGLCD.setFont(SmallFont);
			myGLCD.setColor(255, 255, 255);

			if ((x_osc >= 245) && (x_osc <= 277))               // Боковые кнопки
			{
				myGLCD.setColor(255, 255, 255);
				myGLCD.setFont(SmallFont);
				if ((y_osc >= 20) && (y_osc <= 45))              // Первая  период
				{
					waitForIt(245, 20, 277, 45);
					mode = i2c_eeprom_read_byte(deviceaddress, adr_mode);
					mode--;
					if (mode < 0) mode = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_mode, mode);
					chench_mode(mode);
					myGLCD.print("    ", 262, 3);
					myGLCD.printNumI(dTime, x_dTime, 3);
				}

				if ((y_osc >= 80) && (y_osc <= 105))             // Вторая - триггер
				{
					waitForIt(245, 80, 277, 105);
					tmode = i2c_eeprom_read_byte(deviceaddress, adr_t_trigger);
					tmode--;
					if (tmode < 0) tmode = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_t_trigger, tmode);
					trigger_volume(tmode);
				}
			}

			if ((x_osc >= 285) && (x_osc <= 317))              // Боковые кнопки
			{
				myGLCD.setColor(255, 255, 255);
				myGLCD.setFont(SmallFont);
				if ((y_osc >= 20) && (y_osc <= 45))             // Первая  период
				{

					waitForIt(285, 20, 317, 45);
					mode = i2c_eeprom_read_byte(deviceaddress, adr_mode);
					mode++;
					if (mode > 7) mode = 7;
					i2c_eeprom_write_byte(deviceaddress, adr_mode, mode);
					chench_mode(mode);
					myGLCD.print("    ", 262, 3);
					myGLCD.printNumI(dTime, x_dTime, 3);
				}

				if ((y_osc >= 80) && (y_osc <= 105))            // Вторая - триггер
				{
					waitForIt(285, 80, 317, 105);
					tmode = i2c_eeprom_read_byte(deviceaddress, adr_t_trigger);
					tmode++;
					if (tmode > 7) tmode = 7;
					i2c_eeprom_write_byte(deviceaddress, adr_t_trigger, tmode);
					trigger_volume(tmode);

				}
			}

			if ((x_osc >= 245) && (x_osc <= 318))               // Боковые кнопки
			{
				if ((y_osc >= 125) && (y_osc <= 160))           // Четвертая разрешение
				{
					waitForIt(245, 125, 318, 160);
					i2c_eeprom_ulong_write(adr_set_timeZero, EndMeasure - StartMeasure);  // Записать контрольное время при близком расположение блоков
				}
			}

			if ((y_osc >= 200) && (y_osc <= 225))                 // Нижние кнопки переключения 
			{
				if ((x_osc >= 10) && (x_osc <= 42))               //  Кнопка №1
				{
					waitForIt(10, 200, 42, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
					b++;
					if (b > 9) b = 9;
					i2c_eeprom_write_byte(deviceaddress, adr_count1_kn, b);
					set_volume(1, b);

				}
				if ((x_osc >= 52) && (x_osc <= 84))               //  Кнопка №2
				{
					waitForIt(52, 200, 84, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
					if (b > 0) b--;
					if (b <= 0) b = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_count1_kn, b);
					set_volume(1, b);
				}
				if ((x_osc >= 94) && (x_osc <= 126))               //  Кнопка №3
				{
					waitForIt(94, 200, 126, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
					b++;
					if (b > 9) b = 9;
					i2c_eeprom_write_byte(deviceaddress, adr_count2_kn, b);
					set_volume(2, b);
				}
				if ((x_osc >= 136) && (x_osc <= 168))               //  Кнопка №4
				{
					waitForIt(136, 200, 168, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
					if (b > 0) b--;
					if (b <= 0) b = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_count2_kn, b);
					set_volume(2, b);

				}
				if ((x_osc >= 178) && (x_osc <= 210))               //  Кнопка №5   Увеличить громкость динамика
				{
					waitForIt(178, 200, 210, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
					b++;
					if (b > 9) b = 9;
					i2c_eeprom_write_byte(deviceaddress, adr_count3_kn, b);
					set_volume(3, b);                               // Увеличить громкость динамика

				}
				if ((x_osc >= 220) && (x_osc <= 252))               //  Кнопка №6 Уменьшить громкость динамика
				{
					waitForIt(220, 200, 252, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
					if (b > 0) b--;
					if (b <= 0) b = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_count3_kn, b);
					set_volume(3, b);                               // Уменьшить громкость динамика

				}
			}
			*/
		}
		if (kn == 2)
		{
			kn = 0;
			Timer7.stop();                      //  Остановить синхронизацию вывода на экран
			measure_enable = false;
			myGLCD.clrScr();
			myButtons.drawButtons();
			break;                              //
		}

		trig_sin = false;

		if (micros() - Control_synhro > 3000000)                  // Ожидаем синхроимпульс в течении 2.5 секунд
		{
			myGLCD.setColor(255, 255, 255);
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print("He""\xA4", 160, 20);                    // Нет
			myGLCD.print("c""\x9D\xA2""xpo", 140, 45);            // синхро
			break;                                                // Завершить поиск синхроимпульса, не найден
		}
		if (start_synhro)                                         // Синхроимпульс получен
		{
			StartMeasure = micros();                           // Сохранить время получения синхроимпульса
			Control_synhro = micros();                            // Сохранить время получения синхроимпульса
			start_synhro = false;                               // Время начала синхроимпульса пришло. Запретить повторное измерение.
			Timer7.start(scale_strob * 1000.0);                   // Включить временные метки на экране
			while (!trig_sin)                                                        // Ожидание времени начала синхроимпульса.  Сигнал формируется таймером Timer5
			{
				if (micros() - StartMeasure > 2000000)                                    // Ожидаем синхроимпульс в течении 2 секунд
				{
					break;                                                               // Завершить ожидание синхроимпульса
				}
				for (xpos = 0; xpos < 240; xpos++)                                   // Блоки по 240 байтов
				{
					ADC_CR = ADC_START; 	                                         // Запустить преобразование
					while (!(ADC_ISR_DRDY));                                         // Ожидание завершения преобразования
					Sample_osc[xpos] = 0;
					Synhro_osc[xpos][ms_info] = 0;                                            // Стереть старые данные временных меток
					Synhro_osc[xpos][line_info] = 0;
					Sample_osc[xpos] = ADC->ADC_CDR[7];                        // Получить данные А0 и записать в массив
					if ((Sample_osc[xpos] > Trigger) && (trig_sin == false) && (xpos > 2))   // Поиск превышения уровня порога
					{
						EndMeasure = micros();                                        // Записать время срабатывания триггера порога
						trig_sin = true;                                             // установить флаг срабатывания триггера порога
																					 //page_trig = page;                                            // Записываем номер блока в котором сработал триггер
					}
					delayMicroseconds(dTime);                                        // dTime временной интервал (задержка) развертки 
				}
			}

			Timer7.stop();                                        // Остановить формирование временных меток
			count_ms = scale_strob;
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(1, 20, 240, 159);
			myGLCD.fillRoundRect(1, 162, 280, 178);
			myGLCD.setColor(255, 255, 255);
			myGLCD.setBackColor(0, 0, 0);
			set_timeZero = i2c_eeprom_ulong_read(adr_set_timeZero);
			myGLCD.setFont(BigFont);
			myGLCD.print("\x85""a""\x99""ep""\x9B\x9F""a c""\x9D\x98\xA2""a""\xA0""a", 2, 16); // "Задержка"
			myGLCD.print("MS", 152, 60);                                             // 
			DrawGrid1();
			if (trig_sin)                                                                     // Сигнал зарегистрирован                              
			{
				myGLCD.setColor(0, 0, 0);
				myGLCD.drawRoundRect(0, 30, 150, 79);
				myGLCD.setFont(SevenSegNumFont);
				long time_temp = EndMeasure - StartSynhro - set_timeZero;
				if (time_temp < 0) time_temp = 0;
				myGLCD.setColor(255, 255, 255);
				myGLCD.printNumI(time_temp / 1000.00, 10, 30);                               // вывод основной информации
			}
			else                                                                             // Сигнал не зарегистрирован        
			{
				myGLCD.setColor(0, 0, 0);
				myGLCD.drawRoundRect(0, 30, 318, 79);
			}

			ypos_trig = 255 - (Trigger / koeff_h) - hpos;                                    // Получить уровень порога для отображения
			if (ypos_trig != ypos_trig_old)                                                  // Стереть старый уровень порога при изменении
			{
				myGLCD.setColor(0, 0, 0);
				myGLCD.drawLine(1, ypos_trig_old, 240, ypos_trig_old);
				ypos_trig_old = ypos_trig;
				Serial.println(Trigger);
			}
			myGLCD.setColor(255, 0, 0);
			myGLCD.drawLine(1, ypos_trig, 240, ypos_trig);                             // Нарисовать новую линию уровня порога
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(0, 162, 242, 176);                                    // Очистить область экрана вывода меток

			for (int xpos = 0; xpos < 240; xpos++)                                     // вывод на экран
			{
				// Нарисовать линию  осциллограммы
				myGLCD.setFont(SmallFont);
				myGLCD.setColor(255, 255, 255);
				ypos_osc1_0 = 255 - (Sample_osc[xpos] / koeff_h) - hpos;
				ypos_osc2_0 = 255 - (Sample_osc[xpos + 1] / koeff_h) - hpos;
				if (ypos_osc1_0 < 0) ypos_osc1_0 = 0;
				if (ypos_osc2_0 < 0) ypos_osc2_0 = 0;
				if (ypos_osc1_0 > 220) ypos_osc1_0 = 220;
				if (ypos_osc2_0 > 220) ypos_osc2_0 = 220;
				myGLCD.drawLine(xpos, ypos_osc1_0, xpos + 1, ypos_osc2_0);
				myGLCD.setColor(VGA_YELLOW);

				if (Synhro_osc[xpos][line_info] == 4095)
				{
					myGLCD.drawLine(xpos, 80, xpos, 160);

				}
				if (Synhro_osc[xpos][ms_info] > 0)
				{
					myGLCD.printNumI(Synhro_osc[xpos][ms_info] - scale_strob, xpos, 165);
				}
			}
		}

		if (oldsec != dt.second)
		{
			myGLCD.setBackColor(0, 0, 0);                   //  
			myGLCD.setColor(255, 255, 255);
			myGLCD.setFont(SmallFont);
			myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s", dt), 10, 3);
			myGLCD.setFont(BigFont);
			oldsec = dt.second;
		}
	}
	while (myTouch.dataAvailable()) {}                  // Выход из программы
}
void synhro_by_timer()
{
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.clrScr();
	set_volume(5, 1);                                             // Отобразить уровень громкости  каналов усилителя на Базе
	buttons_right();                                              // Нарисовать кнопки справа экрана;
	button_down();                                                // Нарисовать кнопки внизу экрана;
	myGLCD.setBackColor(0, 0, 255);
	myGLCD.print("\x8A""CT.", 265, 130);                          // "УСТ."
	myGLCD.print("HO""\x88\x92", 265, 143);                       // "НОЛЬ"
	myGLCD.setColor(255, 0, 0);                                   // Красная окантовка кнопок справа
	myGLCD.drawRoundRect(242, 0, 319, 162);                       // Нарисовать кнопоки справа
	myGLCD.setColor(VGA_BLUE);                                    //  окантовка экрана
	myGLCD.drawRoundRect(0, 0, 241, 161);                         // окантовка экрана
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(0, 0, 0);
	tmode = i2c_eeprom_read_byte(deviceaddress, adr_t_trigger);   // Восстановить уровень триггера порога
	Trigger = volume_porog[tmode];                                // установить уровень триггера порога
	myGLCD.print(proc_porog[tmode], 264, 65);                     // Отобразить уровень триггера порога 
	mode = i2c_eeprom_read_byte(deviceaddress, adr_mode);
	chench_mode(mode);
	myGLCD.print("    ", 262, 3);
	myGLCD.printNumI(dTime, x_dTime, 3);
	count_ms = scale_strob;
	for (xpos = 0; xpos < 240; xpos++)                            // Стереть старые данные отображения и временных меток
	{
		Synhro_osc[xpos][ms_info] = 0;                            // Стереть старые данные временных меток
		Synhro_osc[xpos][line_info] = 0;                                          // Стереть старые данные временных меток
	}
	unsigned long Control_synhro = micros();                      // Переменная для контроля наличия синхроимпульса
	data_out[2] = 2;                                              // Формируем команду измерения задержки по синхроимпульсу
	radio_send_command();                                         // Отправляем команду измерения задержки по синхроимпульсу   
	DrawGrid1();                                                  // Нарисовать сетку
	measure_enable = true;
	while (1)
	{
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_osc = myTouch.getX();
			y_osc = myTouch.getY();

			if ((x_osc >= 2) && (x_osc <= 240))                 // Выход из программы. Нажать на область экрана
			{
				if ((y_osc >= 1) && (y_osc <= 160))         // 
				{
					Timer7.stop();                      //  Остановить синхронизацию вывода на экран
					measure_enable = false;
					break;                              //
				}                                       //
			}                                           //
			myGLCD.setBackColor(0, 0, 255);
			myGLCD.setFont(SmallFont);
			myGLCD.setColor(255, 255, 255);

			if ((x_osc >= 245) && (x_osc <= 277))               // Боковые кнопки
			{
				myGLCD.setColor(255, 255, 255);
				myGLCD.setBackColor(0, 0, 0);
				myGLCD.setFont(SmallFont);
				if ((y_osc >= 20) && (y_osc <= 45))              // Первая  период
				{
					waitForIt(245, 20, 277, 45);
					mode = i2c_eeprom_read_byte(deviceaddress, adr_mode);
					mode--;
					if (mode < 0) mode = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_mode, mode);
					chench_mode(mode);
					myGLCD.print("    ", 262, 3);
					myGLCD.printNumI(dTime, x_dTime, 3);
				}

				if ((y_osc >= 80) && (y_osc <= 105))             // Вторая - триггер
				{
					waitForIt(245, 80, 277, 105);
					tmode = i2c_eeprom_read_byte(deviceaddress, adr_t_trigger);
					tmode--;
					if (tmode < 0) tmode = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_t_trigger, tmode);
					Trigger = volume_porog[tmode];                                // установить уровень триггера порога
					myGLCD.print(proc_porog[tmode], 264, 65);                     // Отобразить уровень триггера порога 
				}
			}

			if ((x_osc >= 285) && (x_osc <= 317))              // Боковые кнопки
			{
				myGLCD.setColor(255, 255, 255);
				myGLCD.setBackColor(0, 0, 0);
				myGLCD.setFont(SmallFont);
				if ((y_osc >= 20) && (y_osc <= 45))             // Первая  период
				{

					waitForIt(285, 20, 317, 45);
					mode = i2c_eeprom_read_byte(deviceaddress, adr_mode);
					mode++;
					if (mode > 7) mode = 7;
					i2c_eeprom_write_byte(deviceaddress, adr_mode, mode);
					chench_mode(mode);   
					myGLCD.print("    ", 262, 3);
					myGLCD.printNumI(dTime, x_dTime, 3);
				}

				if ((y_osc >= 80) && (y_osc <= 105))                                  // Вторая - триггер
				{
					waitForIt(285, 80, 317, 105);
					tmode = i2c_eeprom_read_byte(deviceaddress, adr_t_trigger);
					tmode++;
					if (tmode > 7) tmode = 7;
					i2c_eeprom_write_byte(deviceaddress, adr_t_trigger, tmode);
					Trigger = volume_porog[tmode];                                        // установить уровень триггера порога
					myGLCD.print(proc_porog[tmode], 264, 65);                             // Отобразить уровень триггера порога 
				}
			}

			if ((x_osc >= 245) && (x_osc <= 318))                                         // Боковые кнопки
			{
				if ((y_osc >= 125) && (y_osc <= 160))                                     // Четвертая разрешение
				{
					waitForIt(245, 125, 318, 160);
					i2c_eeprom_ulong_write(adr_set_timeZero, EndMeasure - StartMeasure);  // Записать контрольное время при близком расположение блоков
				}
			}

			if ((y_osc >= 200) && (y_osc <= 225))                 // Нижние кнопки переключения 
			{
				if ((x_osc >= 10) && (x_osc <= 42))               //  Кнопка №1
				{
					waitForIt(10, 200, 42, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
					b++;
					if (b > 9) b = 9;
					i2c_eeprom_write_byte(deviceaddress, adr_count1_kn, b);
					set_volume(1, b);

				}
				if ((x_osc >= 52) && (x_osc <= 84))               //  Кнопка №2
				{
					waitForIt(52, 200, 84, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
					if (b > 0) b--;
					if (b <= 0) b = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_count1_kn, b);
					set_volume(1, b);
				}
				if ((x_osc >= 94) && (x_osc <= 126))               //  Кнопка №3
				{
					waitForIt(94, 200, 126, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
					b++;
					if (b > 9) b = 9;
					i2c_eeprom_write_byte(deviceaddress, adr_count2_kn, b);
					set_volume(2, b);
				}
				if ((x_osc >= 136) && (x_osc <= 168))               //  Кнопка №4
				{
					waitForIt(136, 200, 168, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
					if (b > 0) b--;
					if (b <= 0) b = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_count2_kn, b);
					set_volume(2, b);

				}
				if ((x_osc >= 178) && (x_osc <= 210))               //  Кнопка №5   Увеличить громкость динамика
				{
					waitForIt(178, 200, 210, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
					b++;
					if (b > 9) b = 9;
					i2c_eeprom_write_byte(deviceaddress, adr_count3_kn, b);
					set_volume(3, b);                               // Увеличить громкость динамика

				}
				if ((x_osc >= 220) && (x_osc <= 252))               //  Кнопка №6 Уменьшить громкость динамика
				{
					waitForIt(220, 200, 252, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
					if (b > 0) b--;
					if (b <= 0) b = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_count3_kn, b);
					set_volume(3, b);                               // Уменьшить громкость динамика

				}
			}
		}

		trig_sin = false;

		if (micros() - Control_synhro > 3000000)                  // Ожидаем синхроимпульс в течении 2.5 секунд
		{
			myGLCD.setColor(255, 255, 255);
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print("He""\xA4", 160, 20);                    // Нет
			myGLCD.print("c""\x9D\xA2""xpo", 140, 45);            // синхро
			break;                                                // Завершить поиск синхроимпульса, не найден
		}
		if (start_synhro)                                                                  // Синхроимпульс получен
		{
			StartMeasure    = micros();                                                    // Сохранить время получения синхроимпульса
			Control_synhro = micros();                                                     // Сохранить время получения синхроимпульса
			start_synhro   = false;                                                        // Время начала синхроимпульса пришло. Запретить повторное измерение.
			Timer7.start(scale_strob * 1000.0);                                            // Включить временные метки на экране
			while (!trig_sin)                                                              // Ожидание времени начала синхроимпульса.  Сигнал формируется таймером Timer5
			{
				if (micros() - StartMeasure > 2000000)                                     // Ожидаем синхроимпульс в течении 2 секунд
				{
					break;                                                                 // Завершить ожидание синхроимпульса
				}
				for (xpos = 0; xpos < 240; xpos++)                                         // Блоки по 240 байтов
				{
					ADC_CR = ADC_START; 	                                               // Запустить преобразование
					while (!(ADC_ISR_DRDY));                                               // Ожидание завершения преобразования
					Sample_osc[xpos] = 0;
					Synhro_osc[xpos][ms_info] = 0;                                         // Стереть старые данные временных меток
					Synhro_osc[xpos][line_info] = 0;
					Sample_osc[xpos] = ADC->ADC_CDR[7];                                    // Получить данные А0 и записать в массив
					if ((Sample_osc[xpos] > Trigger) && (trig_sin == false) && (xpos > 2)) // Поиск превышения уровня порога
					{
						EndMeasure = micros();                                             // Записать время срабатывания триггера порога
						trig_sin = true;                                                   // установить флаг срабатывания триггера порога
					}
					delayMicroseconds(dTime);                                              // dTime временной интервал (задержка) развертки 
				}
			}

			Timer7.stop();                                                                 // Остановить формирование временных меток
			count_ms = scale_strob;
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(0, 20, 240, 159);
			myGLCD.fillRoundRect(0, 162, 280, 178);
			myGLCD.setColor(255, 255, 255);
			myGLCD.setBackColor(0, 0, 0);
			set_timeZero = i2c_eeprom_ulong_read(adr_set_timeZero);
			myGLCD.setFont(BigFont);
			myGLCD.print("\x85""a""\x99""ep""\x9B\x9F""a", 2, 20);                         // "Задержка"
			myGLCD.print("c""\x9D\x98\xA2""a""\xA0""a", 2, 40);                            // "сигнала"
			myGLCD.print("   ", 160, 20);                                                  // Очистить надпись "Нет"
			myGLCD.print("      ", 140, 45);                                               // Очистить надпись "синхро"
			DrawGrid1();
			if (trig_sin)                                                                  // Сигнал зарегистрирован                              
			{
				float time_meas = ((EndMeasure - StartMeasure) - set_timeZero) / 1000.00;
				if (time_meas < 0)  time_meas = 0.0;
				myGLCD.printNumF(time_meas, 2, 5, 60);
				myGLCD.print(" ms", 88, 60);
			}
			else                                                                          // Сигнал не зарегистрирован        
			{
				myGLCD.print("      ", 5, 60);                                            // Очистить зону данных

			}

			ypos_trig = 255 - (Trigger / koeff_h) - hpos;                                 // Получить уровень порога для отображения
			if (ypos_trig != ypos_trig_old)                                               // Стереть старый уровень порога при изменении
			{
				myGLCD.setColor(0, 0, 0);
				myGLCD.drawLine(1, ypos_trig_old, 240, ypos_trig_old);
				ypos_trig_old = ypos_trig;
				Serial.println(Trigger);
			}
			myGLCD.setColor(255, 0, 0);
			myGLCD.drawLine(1, ypos_trig, 240, ypos_trig);                               // Нарисовать новую линию уровня порога
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(0, 162, 242, 176);                                    // Очистить область экрана вывода меток

			for (int xpos = 0; xpos < 240; xpos++)                                     // вывод на экран
			{
				// Нарисовать линию  осциллограммы
				myGLCD.setFont(SmallFont);
				myGLCD.setColor(255, 255, 255);
				ypos_osc1_0 = 255 - (Sample_osc[xpos] / koeff_h) - hpos;
				ypos_osc2_0 = 255 - (Sample_osc[xpos + 1] / koeff_h) - hpos;
				if (ypos_osc1_0 < 0) ypos_osc1_0 = 0;
				if (ypos_osc2_0 < 0) ypos_osc2_0 = 0;
				if (ypos_osc1_0 > 220) ypos_osc1_0 = 220;
				if (ypos_osc2_0 > 220) ypos_osc2_0 = 220;
				myGLCD.drawLine(xpos, ypos_osc1_0, xpos + 1, ypos_osc2_0);
				myGLCD.setColor(VGA_YELLOW);

				if (Synhro_osc[xpos][line_info] == 4095)
				{
					myGLCD.drawLine(xpos, 80, xpos, 160);

				}
				if (Synhro_osc[xpos][ms_info] > 0)
				{
					myGLCD.printNumI(Synhro_osc[xpos][ms_info] - scale_strob, xpos, 165);
				}
			}
		}
		dt = DS3231_clock.getDateTime();
		if (oldsec != dt.second)
		{
			myGLCD.setBackColor(0, 0, 0);                   //  
			myGLCD.setColor(255, 255, 255);
			myGLCD.setFont(SmallFont);
			myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s", dt), 10, 3);
			myGLCD.setFont(BigFont);
			oldsec = dt.second;
		}
	}
	while (myTouch.dataAvailable()) {}                  // Выход из программы
}
void oscilloscope()                                     // просмотр в реальном времени. Синхронизация по радиоимпульсу
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	set_volume(5, 1);                                             // Отобразить уровень громкости  каналов усилителя на Базе
	buttons_right();                                              // Нарисовать кнопки справа экрана;
	button_down();                                                // Нарисовать кнопки внизу экрана;
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(0, 0, 0);
	tmode = i2c_eeprom_read_byte(deviceaddress, adr_t_trigger);   // Восстановить уровень триггера порога
	Trigger = volume_porog[tmode];                                // установить уровень триггера порога
	myGLCD.print(proc_porog[tmode], 264, 65);                     // Отобразить уровень триггера порога                                     // Уровень триггера порога
	mode = i2c_eeprom_read_byte(deviceaddress, adr_mode);
	chench_mode(mode);
	myGLCD.print("    ", 262, 3);
	myGLCD.printNumI(dTime, x_dTime, 3);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor(255, 0, 0);                                   // Красная окантовка кнопок справа
	myGLCD.drawRoundRect(242, 0, 319, 162);                       // Нарисовать кнопоки справа

	myGLCD.setColor(VGA_BLUE);                                    // окантовка экрана
	myGLCD.drawRoundRect(0, 1, 241, 161);                         // окантовка экрана
	myGLCD.setColor(255, 255, 255);

	page = 0;                                            // Только 1 страница
	for (xpos = 0; xpos < 240; xpos++)                   // Стереть старые данные отображения и временных меток
	{
		OldSample_osc[xpos][page] = 0;                   // Стереть старые данные отображения
		Synhro_osc[xpos][ms_info] = 0;                          // Записать временные метки в массив для вывода на экран
		Synhro_osc[xpos][line_info] = 0;              // Записать временные метки в массив для вывода на экран;            // Стереть новые данные временных меток
	}

	while (1)                                            //                
	{
		ADC_CHER = 0;                                           // Включить аналоговый вход "0"          (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3
		for (xpos = 0; xpos < 240; xpos++)                                  // Блоки по 240 байтов
		{
			ADC_CR = ADC_START; 	                                        // Запустить преобразование
			while (!(ADC_ISR_DRDY));                                        // Ожидание завершения преобразования
			Sample_osc[xpos] = ADC->ADC_CDR[7];                              // Получить данные А0 и записать в массив
			Synhro_osc[xpos][ms_info] = 0;                                            // Стереть старые данные временных меток
			delayMicroseconds(dTime);                                       // dTime временной интервал (задержка) развертки 
		}

		ypos_trig = 255 - (Trigger / koeff_h) - hpos;                           // Получить уровень порога
		if (ypos_trig != ypos_trig_old)                                         // Стереть старый уровень порога при изменении
		{
			myGLCD.setColor(0, 0, 0);
			myGLCD.drawLine(1, ypos_trig_old, 240, ypos_trig_old);
			ypos_trig_old = ypos_trig;
		}
		myGLCD.setColor(255, 0, 0);
		myGLCD.drawLine(1, ypos_trig, 240, ypos_trig);                           // Нарисовать новую линию уровня порога

		for (int xpos = 0; xpos < 239; xpos++)                                   // вывод на экран
		{
			// Стереть предыдущий экран

			myGLCD.setColor(0, 0, 0);
			myGLCD.setBackColor(0, 0, 0);
			ypos_osc1_0 = 255 - (OldSample_osc[xpos][page] / koeff_h) - hpos;
			ypos_osc2_0 = 255 - (OldSample_osc[xpos + 1][page] / koeff_h) - hpos;
			if (ypos_osc1_0 < 0) ypos_osc1_0 = 0;
			if (ypos_osc2_0 < 0) ypos_osc2_0 = 0;
			if (ypos_osc1_0 > 220) ypos_osc1_0 = 220;
			if (ypos_osc2_0 > 220) ypos_osc2_0 = 220;
			myGLCD.drawLine(xpos, ypos_osc1_0, xpos + 1, ypos_osc2_0);
			myGLCD.drawLine(xpos + 1, ypos_osc1_0, xpos + 2, ypos_osc2_0);

			// Нарисовать линию  осциллограммы
			myGLCD.setColor(255, 255, 255);
			ypos_osc1_0 = 255 - (Sample_osc[xpos] / koeff_h) - hpos;
			ypos_osc2_0 = 255 - (Sample_osc[xpos + 1] / koeff_h) - hpos;
			if (ypos_osc1_0 < 0) ypos_osc1_0 = 0;
			if (ypos_osc2_0 < 0) ypos_osc2_0 = 0;
			if (ypos_osc1_0 > 220) ypos_osc1_0 = 220;
			if (ypos_osc2_0 > 220) ypos_osc2_0 = 220;
			myGLCD.drawLine(xpos, ypos_osc1_0, xpos + 1, ypos_osc2_0);
			OldSample_osc[xpos][page] = Sample_osc[xpos];
		}
		DrawGrid1();
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_osc = myTouch.getX();
			y_osc = myTouch.getY();

			if ((x_osc >= 2) && (x_osc <= 240))                 // Выход из программы. Нажать на область экрана
			{
				if ((y_osc >= 1) && (y_osc <= 160))         // 
				{
					Timer7.stop();                      //  Остановить синхронизацию вывода на экран
					measure_enable = false;
					break;                              //
				}                                       //
			}                                           //
			myGLCD.setBackColor(0, 0, 255);
			myGLCD.setFont(SmallFont);
			myGLCD.setColor(255, 255, 255);

			if ((x_osc >= 245) && (x_osc <= 277))               // Боковые кнопки
			{
				myGLCD.setColor(255, 255, 255);
				myGLCD.setBackColor(0, 0, 0);
				myGLCD.setFont(SmallFont);
				if ((y_osc >= 20) && (y_osc <= 45))              // Первая  период
				{
					waitForIt(245, 20, 277, 45);
					mode = i2c_eeprom_read_byte(deviceaddress, adr_mode);
					mode--;
					if (mode < 0) mode = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_mode, mode);
					chench_mode(mode);
					myGLCD.print("    ", 262, 3);
					myGLCD.printNumI(dTime, x_dTime, 3);
				}

				if ((y_osc >= 80) && (y_osc <= 105))             // Вторая - триггер
				{
					waitForIt(245, 80, 277, 105);
					tmode = i2c_eeprom_read_byte(deviceaddress, adr_t_trigger);
					tmode--;
					if (tmode < 0) tmode = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_t_trigger, tmode);
					Trigger = volume_porog[tmode];                                // установить уровень триггера порога
					myGLCD.print(proc_porog[tmode], 264, 65);                     // Отобразить уровень триггера порога 
				}
			}

			if ((x_osc >= 285) && (x_osc <= 317))              // Боковые кнопки
			{
				myGLCD.setColor(255, 255, 255);
				myGLCD.setBackColor(0, 0, 0);
				myGLCD.setFont(SmallFont);
				if ((y_osc >= 20) && (y_osc <= 45))             // Первая  период
				{
					waitForIt(285, 20, 317, 45);
					mode = i2c_eeprom_read_byte(deviceaddress, adr_mode);
					mode++;
					if (mode > 7) mode = 7;
					i2c_eeprom_write_byte(deviceaddress, adr_mode, mode);
					chench_mode(mode);
					myGLCD.print("    ", 262, 3);
					myGLCD.printNumI(dTime, x_dTime, 3);
				}

				if ((y_osc >= 80) && (y_osc <= 105))            // Вторая - триггер
				{
					waitForIt(285, 80, 317, 105);
					tmode = i2c_eeprom_read_byte(deviceaddress, adr_t_trigger);
					tmode++;
					if (tmode > 7) tmode = 7;
					i2c_eeprom_write_byte(deviceaddress, adr_t_trigger, tmode);
					Trigger = volume_porog[tmode];                                // установить уровень триггера порога
					myGLCD.print(proc_porog[tmode], 264, 65);                     // Отобразить уровень триггера порога 
				}
			}

			if ((y_osc >= 200) && (y_osc <= 225))                 // Нижние кнопки переключения 
			{
				if ((x_osc >= 10) && (x_osc <= 42))               //  Кнопка №1
				{
					waitForIt(10, 200, 42, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
					b++;
					if (b > 9) b = 9;
					i2c_eeprom_write_byte(deviceaddress, adr_count1_kn, b);
					set_volume(1, b);
				}
				if ((x_osc >= 52) && (x_osc <= 84))               //  Кнопка №2
				{
					waitForIt(52, 200, 84, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
					if (b > 0) b--;
					if (b <= 0) b = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_count1_kn, b);
					set_volume(1, b);
				}
				if ((x_osc >= 94) && (x_osc <= 126))               //  Кнопка №3
				{
					waitForIt(94, 200, 126, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
					b++;
					if (b > 9) b = 9;
					i2c_eeprom_write_byte(deviceaddress, adr_count2_kn, b);
					set_volume(2, b);
				}
				if ((x_osc >= 136) && (x_osc <= 168))               //  Кнопка №4
				{
					waitForIt(136, 200, 168, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
					if (b > 0) b--;
					if (b <= 0) b = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_count2_kn, b);
					set_volume(2, b);
				}
				if ((x_osc >= 178) && (x_osc <= 210))               //  Кнопка №5   Увеличить громкость динамика
				{
					waitForIt(178, 200, 210, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
					b++;
					if (b > 9) b = 9;
					i2c_eeprom_write_byte(deviceaddress, adr_count3_kn, b);
					set_volume(3, b);                               // Увеличить громкость динамика
				}
				if ((x_osc >= 220) && (x_osc <= 252))               //  Кнопка №6 Уменьшить громкость динамика
				{
					waitForIt(220, 200, 252, 225);
					byte b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
					if (b > 0) b--;
					if (b <= 0) b = 0;
					i2c_eeprom_write_byte(deviceaddress, adr_count3_kn, b);
					set_volume(3, b);                               // Уменьшить громкость динамика
				}
			}
		}
	}
	while (myTouch.dataAvailable()) {}
}

void buttons_right()  //  Правые кнопки  oscilloscope
{
	drawDownButton(245, 20);                                      // Нарисовать кнопки регулировки усиления 1 каскада усиления;
	drawUpButton(285, 20);                                        // Нарисовать кнопки регулировки усиления 1 каскада усиления;
	drawDownButton(245, 80);                                      // Нарисовать кнопки регулировки усиления 1 каскада усиления;
	drawUpButton(285, 80);                                        // Нарисовать кнопки регулировки усиления 1 каскада усиления;

	myGLCD.setColor(0, 0, 255);
	myGLCD.setBackColor(0, 0, 255);
	myGLCD.fillRoundRect(245, 125, 318, 160);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(245, 125, 318, 160);

	myGLCD.setFont(SmallFont);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("DELAY", 265, 48);
	myGLCD.print("\x89""OPO""\x81", 265, 108);                     //  "ПОРОГ"
}
void button_down()
{
	drawUpButton(10, 200);                                        // Нарисовать кнопки регулировки усиления 1 каскада усиления;
	drawDownButton(52, 200);                                      // Нарисовать кнопки регулировки усиления 1 каскада усиления;
	drawUpButton(94, 200);                                        // Нарисовать кнопки регулировки усиления 2 каскада усиления;
	drawDownButton(136, 200);                                     // Нарисовать кнопки регулировки усиления 2 каскада усиления;
	drawUpButton(178, 200);                                       // Нарисовать кнопки громкости справа экрана;
	drawDownButton(220, 200);                                     // Нарисовать кнопки громкости справа экрана;

	myGLCD.setFont(SmallFont);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("\x8A""H""\x8D""1 -M""\x86""KPO""\x8B""OH- ""\x8A""H""\x8D""2 ", 11, 227);     // "УНЧ1 -МИКРОФОН- УНЧ2 "
	myGLCD.print("\x82\x86""HAM""\x86""K", 190, 227);              // "ДИНАМИК"
	myGLCD.setColor(255, 0, 0);                                           // Белая окантовка кнопок справа
	myGLCD.drawRoundRect(1,180, 260, 239);                               // Нарисовать кнопоки справа
	myGLCD.setColor(255, 255, 255);
}
void drawUpButVolume(int x, int y)
{
	myGLCD.setColor(64, 64, 128);
	myGLCD.fillRoundRect(x, y, x + 32, y + 25);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(x, y, x + 32, y + 25);
	myGLCD.setColor(128, 128, 255);
	for (int i = 0; i<15; i++)
		myGLCD.drawLine(x + 6 + (i / 1.5), y + 20 - i, x + 27 - (i / 1.5), y + 20 - i);
}
void drawDownButVolume(int x, int y)
{
	myGLCD.setColor(64, 64, 128);
	myGLCD.fillRoundRect(x, y, x + 32, y + 25);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(x, y, x + 32, y + 25);
	myGLCD.setColor(128, 128, 255);
	for (int i = 0; i<15; i++)
		myGLCD.drawLine(x + 6 + (i / 1.5), y + 5 + i, x + 27 - (i / 1.5), y + 5 + i);
}

void chench_mode(int mod)   // Переключение развертки
{
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(SmallFont);

	if (mod < 0) mod = 0;
	if (mod > 9) mod = 9;
	// Select delay times you can change values to suite your needs
	if (mod == 0) { dTime = 1;    x_dTime = 278; scale_strob = 1; }
	if (mod == 1) { dTime = 10;   x_dTime = 274; scale_strob = 1; }
	if (mod == 2) { dTime = 20;   x_dTime = 274; scale_strob = 2; }
	if (mod == 3) { dTime = 50;   x_dTime = 274; scale_strob = 2; }
	if (mod == 4) { dTime = 100;  x_dTime = 272; scale_strob = 5; }
	if (mod == 5) { dTime = 200;  x_dTime = 272; scale_strob = 10; }
	if (mod == 6) { dTime = 300;  x_dTime = 272; scale_strob = 20; }
	if (mod == 7) { dTime = 500;  x_dTime = 272; scale_strob = 30; }
	if (mod == 8) { dTime = 1000; x_dTime = 267; scale_strob = 50; }
	if (mod == 9) { dTime = 5000; x_dTime = 267; scale_strob = 100; }
}
void chench_mode1(bool mod)  // Разрешение экрана
{
	if (mod)
	{
		mode1++;
	}
	else
	{
		mode1--;
	}
	if (mode1 < 0) mode1 = 0;
	if (mode1 > 3) mode1 = 3;
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRoundRect(1, 1, 240, 160);
	DrawGrid();
	myGLCD.setColor(255, 255, 255);
	if (mode1 == 0) { koeff_h = 7.759 * 4; myGLCD.print("  1 ", 262, 110); }
	if (mode1 == 1) { koeff_h = 3.879 * 4; myGLCD.print(" 0.5", 262, 110); }
	if (mode1 == 2) { koeff_h = 1.939 * 4; myGLCD.print("0.25", 264, 110); }
	if (mode1 == 3) { koeff_h = 0.969 * 4; myGLCD.print(" 0.1", 262, 110); }
} 
// Подтверждение команды синхронизации
void synhro_DS3231_clock()
{
	detachInterrupt(alarm_pin);
	//DS3231_clock.clearAlarm1();
	//DS3231_clock.clearAlarm2();
	pinMode(alarm_pin, INPUT);
	digitalWrite(alarm_pin, HIGH);
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(BigFont);
	count_ms = scale_strob;                                      // Время развертки
	draw_synhro_clock_run();

	int x, y;
	while (1)
	{
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			x = myTouch.getX();
			y = myTouch.getY();

			if ((y >= 160) && (y <= 200)) // Buttons: 
			{
				if ((x >= 165) && (x <= 319))
				{
					waitForIt(165, 160, 319, 200);
					break;
				}
				if ((x >= 0) && (x <= 154))
				{
					waitForIt(0, 160, 154, 200);
					synhro_clock_run();
					draw_synhro_clock_run();
					//break;
				}
			}
		}
	}
}
void draw_synhro_clock_run()
{
	//myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(BigFont);
	count_ms = scale_strob;                                      // Время развертки
																					// Draw Save button
	myGLCD.setColor(64, 64, 128);
	myGLCD.fillRoundRect(165, 160, 319, 200);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(165, 160, 319, 200);
	myGLCD.setBackColor(64, 64, 128);
	myGLCD.setBackColor(0, 0, 0);
	// Draw Cancel button
	myGLCD.setColor(64, 64, 128);
	myGLCD.fillRoundRect(0, 160, 154, 200);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(0, 160, 154, 200);
	myGLCD.setBackColor(64, 64, 128);
	myGLCD.print("B""\x91\x89""O""\x88""H""\x86""T""\x92", 5, 172);   // "ВЫПОЛНИТЬ"
	myGLCD.print("B""\x91""XO""\x82", 200, 172);                      // "ВЫХОД"
	myGLCD.setBackColor(0, 0, 0);

} 
// Выполнение  команды синхронизации
void synhro_clock_run()
{
	myGLCD.clrScr();                                                //
	myGLCD.setColor(255, 255, 255);
	bool _synhro_enable = false;                                    // Переменная для контроля наличия синхроимпльса в определенный промежток времени
	synhro_enable = false;                                          // Переменная для подтверждения наличия синхроимпльса
	unsigned long Control_Synhro = micros();                        // Контроль наличия синхроимпульса

	if (digitalRead(synhro_pin) != LOW)                             // Проверить подключения кабеля синхронизации                
	{
		myGLCD.setColor(VGA_RED);
		myGLCD.print("C""\x9D\xA2""xpo ""\xA2""e ""\xA3""o""\x99\x9F\xA0\xAE\xA7""e""\xA2", CENTER, 80);   // Синхро не подключен
		delay(2000);
	}
	else
	{
		dt = DS3231_clock.getDateTime();                        // Получить текщее время
		data_out[2] = 8;                                    // Записать команду № 8 (синхронизация часов)
		data_out[12] = dt.year - 2000;                      // Записать год
		data_out[13] = dt.month;                              // Записать месяц
		data_out[14] = dt.day;                              // Записать день
		data_out[15] = dt.hour;                             // Записать час
		data_out[16] = dt.minute;                           // Записать минуту 
		radio_send_command();                               // Отправить команду синхронизации часов

		unsigned long StartMeasure = micros();               // Записать время

		while (!_synhro_enable) 
		{
			if (micros() - StartMeasure >= 3000000)
			{
				myGLCD.setFont(BigFont);
				myGLCD.setColor(VGA_RED);
				myGLCD.print("He""\xA4"" c""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\x9D", CENTER, 80);   // "Нет синхронизации"
				delay(2000);
				break;
			}
			if (digitalRead(synhro_pin) == HIGH)
			{
				while (digitalRead(synhro_pin)) {}                                           // Поиск окончания синхроимпульса
				do {} while (digitalRead(synhro_pin));                                       // Поиск окончания синхроимпульса

				DS3231_clock.setDateTime(dt.year, dt.month, dt.day, dt.hour, dt.minute, 8);  // Записываем новое синхронизированное время
				alarm_synhro = 0;                                                            // Сбрасываем счетчик синхроимпульсов
				DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);                     // DS3231_EVERY_SECOND //Прерывание каждую секунду
				myGLCD.setFont(BigFont);
				myGLCD.setColor(255, 255, 255);
				myGLCD.print("HACTPO""\x87""KA", CENTER, 40);                                // НАСТРОЙКА СИНХРОНИЗАЦИИ
				myGLCD.print("C""\x86""HXPOH""\x86\x85""A""\x8C\x86\x86", CENTER, 60);       // НАСТРОЙКА СИНХРОНИЗАЦИИ
				myGLCD.setFont(SmallFont);
				while (true)                                                                 // Устанавливаем время нового старта синхронизации
				{
					dt = DS3231_clock.getDateTime();
					if (oldsec != dt.second)
					{
						myGLCD.setBackColor(0, 0, 0);                   //  
						myGLCD.setColor(255, 255, 255);
						myGLCD.setFont(SmallFont);
						myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s", dt), 10, 3);
						myGLCD.setFont(BigFont);
						oldsec = dt.second;
					}

					if (dt.second == 10)
					{
						break;
					}
				}
				if(!synhro_enable) attachInterrupt(alarm_pin, alarmFunction, FALLING);      // прерывание вызывается только при смене значения на порту с LOW на HIGH
				unsigned long Start_unixtime = dt.unixtime;
				i2c_eeprom_ulong_write(adr_start_unixtimetime, Start_unixtime);             // Записать время старта синхронизации модулей
				i2c_eeprom_write_byte(deviceaddress, adr_start_year, dt.year - 2000);       // Записать время старта синхронизации модулей
				i2c_eeprom_write_byte(deviceaddress, adr_start_month, dt.month);            // Записать время старта синхронизации модулей
				i2c_eeprom_write_byte(deviceaddress, adr_start_day, dt.day);                // Записать время старта синхронизации модулей
				i2c_eeprom_write_byte(deviceaddress, adr_start_hour, dt.hour);              // Записать время старта синхронизации модулей
				i2c_eeprom_write_byte(deviceaddress, adr_start_minute, dt.minute);          // Записать время старта синхронизации модулей
				i2c_eeprom_write_byte(deviceaddress, adr_start_second, dt.second);          // Записать время старта синхронизации модулей

				start_synhro = false;                                                       // Время начала синхроимпульса пришло. Выставляем флаг старта в исходное для ожидания следующего синхро импульса
				trig_sin = false;                                                           // установить флаг срабатывания триггера порога
				Control_Synhro = micros();
				while (1)                                                                   // Ожидание времени начала синхроимпульса.  Сигнал формируется таймером Timer5
				{
					if (micros() - Control_Synhro > 3000000)                                // Ожидаем синхроимпульс в течении 3 секунд
					{
						break;                                                              // Завершить ожидание синхроимпульса
					}

					for (xpos = 0; xpos < 240; xpos++)                                      // Блоки по 240 байтов
					{
						Sample_osc[xpos] = 0;
						Synhro_osc[xpos][ms_info] = 0;                                      // Стереть старые данные временных меток
						Synhro_osc[xpos][line_info] = 0;
						Sample_osc[xpos] = digitalRead(synhro_pin);                         // Получить данные synhro_pin и записать в массив
						if ((Sample_osc[xpos] > Trigger) && (trig_sin == false))            // Поиск превышения уровня порога. Записать при первом обнаружении импульса.
						{
							EndMeasure = micros();                                          // Записать время срабатывания триггера порога
							trig_sin = true;                                                // установить флаг срабатывания триггера порога
							Control_Synhro = micros();
						}
						delayMicroseconds(24);                                              // временной интервал (задержка) развертки 
					}
					if (trig_sin == true)
					{
						break;                                                              // Завершить сканирование на блоке в котором сработал порог
					}
				}

				myGLCD.setFont(BigFont);
				int ms_delayS = ((EndMeasure - StartSynhro) / 100);
				if (trig_sin)
				{
					myGLCD.setColor(VGA_LIME);
					myGLCD.print("C""\x9D\xA2""xpo OK!", CENTER, 80);                        // Синхро ОК!
					myGLCD.setColor(255, 255, 255);
					i2c_eeprom_ulong_write(adr_ms_delay, ms_delayS);                         // Записать время старта синхронизации модулей
					delay(1000);
					synhro_enable = true;                                                    // Сигнал для контроля записи времени синхронизации в память
				}
				else
				{
					myGLCD.setColor(VGA_RED);
					myGLCD.print("C""\x9D\xA2""xpo NO!", CENTER, 80);                        // Синхро NO!
					myGLCD.setColor(255, 255, 255);
					i2c_eeprom_ulong_write(adr_ms_delay, 0);                                 // Записать время старта синхронизации модулей
					delay(1000);
					synhro_enable = false;                                                   //
				}
				_synhro_enable = true;
			}
		}  // Синхронизация выполнена, завершить программу
	}
	_synhro_enable = false;
}

void wiev_synhro()
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	start_synhro                   = false;
	int count_synhro               = 0;                                                // Счетчик количества синхроимпульсов ()
	long time_temp                 = 0;                                                // Для правильного отображения результатов измерения
	count_ms                       = scale_strob;                                      // Время развертки
	unsigned long Current_unixtime = 0;                                                // 
	unsigned long Control_Synhro   = 0;                                                // Контроль наличия синхроимпульса

	for (xpos = 0; xpos < 240; xpos++)                                                 // Стереть старые данные отображения и временных меток
	{
		Sample_osc[xpos]            = 0;
		Synhro_osc[xpos][ms_info]   = 0;                                               // Стереть старые данные временных меток
		Synhro_osc[xpos][line_info] = 0;                                               // Стереть старые данные временных меток
	}

	//unsigned long Start_unixtime = i2c_eeprom_ulong_read(adr_start_unixtimetime);      // Получить время старта синхронизации модулей
	//int start_year     = i2c_eeprom_read_byte(deviceaddress, adr_start_year);          // Время старта синхронизации модулей
	//int start_month    = i2c_eeprom_read_byte(deviceaddress, adr_start_month);         // Время старта синхронизации модулей
	//int start_day      = i2c_eeprom_read_byte(deviceaddress, adr_start_day);           // Время старта синхронизации модулей
	//int start_hour     = i2c_eeprom_read_byte(deviceaddress, adr_start_hour);          // Время старта синхронизации модулей
	//int start_minute   = i2c_eeprom_read_byte(deviceaddress, adr_start_minute);        // Время старта синхронизации модулей
	//int start_second   = i2c_eeprom_read_byte(deviceaddress, adr_start_second);        // Время старта синхронизации модулей
	//float ms_delayS    = i2c_eeprom_ulong_read( adr_ms_delay)/10.0;                    // Величина задержки измерения при старте

	//myGLCD.setFont(SmallFont);

	//myGLCD.print("Start", 250, 10);                                                    //  
	//myGLCD.print("H:", 250, 25);                                                       //  
	//myGLCD.printNumI(start_hour, 280, 25, 2);                                          // Вывести на экран время час
	//myGLCD.print("M:", 250, 40);                                                       //  
	//myGLCD.printNumI(start_minute, 280, 40, 2);                                        // Вывести на экран время мин
	//myGLCD.print("S:", 250, 55);                                                       //  
	//myGLCD.printNumI(start_second, 280, 55, 2);                                        // Вывести на экран время сек
	//myGLCD.print("ms:", 250, 70);                                                      //  
	//myGLCD.printNumF(ms_delayS,1, 280, 70);                                            // Вывести на экран время задержки ms

	//myGLCD.print("Current", 250, 85);                                        //  

	//myGLCD.print("Duration", 250, 150);                                      //  
	//myGLCD.print("of time", 259, 165);
	//myGLCD.print("Bpe""\xA1\xAF"" o""\xA4"" c""\xA4""ap""\xA4""a c""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\x9D", 0, 180);  //"Время от старта синхронизации"
	//myGLCD.print("min", 295, 180);
	//myGLCD.print("C""\xA7""e""\xA4\xA7\x9D\x9F"" c""\x9D\xA2""xpo""\x9D\xA1\xA3""y""\xA0\xAC""co""\x97", 50, 200); //  "Счетчик синхроимпульсов"


	measure_enable = true;

	while (1)
	{
		/*
    	if (start_synhro)                                                             // Синхроимпульс от прерывания обнаружен
		{
			Control_Synhro = micros();
			// Все нормально. Измеряем !! 
			start_synhro = false;                                                     // Время начала синхроимпульса пришло. Выставляем флаг старта в исходное для ожидания следующего синхро импульса
			trig_sin = false;                                                         // установить флаг срабатывания триггера порога
			//while (!trig_sin)                                                         // Ожидание превышения порога, поиск синхроимпульса.  
			//{
			//	if (micros() - Control_Synhro > 3000000)                              // Ожидаем синхроимпульс в течении 4 секунд
			//	{
			//		break;                                                            // Завершить ожидание синхроимпульса
			//	}

			//	for (xpos = 0; xpos < 240; xpos++)                                    // Блоки по 240 байтов
			//	{
			//		Sample_osc[xpos]            = 0;
			//		Synhro_osc[xpos][ms_info]   = 0;                                  // Стереть старые данные временных меток
			//		Synhro_osc[xpos][line_info] = 0;
			//		Sample_osc[xpos]            = digitalRead(synhro_pin);            // Получить данные synhro_pin и записать в массив
			//		if ((Sample_osc[xpos] == true) && (trig_sin == false))          // Поиск превышения уровня порога. Записать при первом обнаружении импульса.
			//		{
			//			EndMeasure = micros();                                        // Записать время срабатывания триггера порога
			//			trig_sin = true;                                              // установить флаг срабатывания триггера порога
			//			Control_Synhro = micros();
			//		}
			//		delayMicroseconds(24);                                            // временной интервал (задержка) развертки 
			//	}
			//}

	        Timer7.stop();
			count_ms = scale_strob;                                                  // Время развертки
			count_synhro++;
			DrawGrid1();
			//if (myTouch.dataAvailable())
			//{
			//	measure_enable = false;
			//	break;                                                               //Остановить вывод на экран
			//}
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(0, 60, 240, 176);                                   // Очистить область экрана для вывода временных меток. 
			DrawGrid1();
			myGLCD.setColor(255, 255, 255);

			myGLCD.setFont(SmallFont);
			myGLCD.printNumI(count_synhro, 250, 200);                                // Вывести на экран счетчик синхроимпульсов
			dt = DS3231_clock.getDateTime();
			myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s", dt), 10, 3);
			myGLCD.print("H:", 250, 100);                                            //  
			myGLCD.printNumI(dt.hour, 280, 100, 2);                                  // Вывести на экран время час
			myGLCD.print("M:", 250, 115);                                            //  
			myGLCD.printNumI(dt.minute, 280, 115, 2);                                // Вывести на экран время мин
			myGLCD.print("S:", 250, 130);                                            //  
			myGLCD.printNumI(dt.second, 280, 130, 2);                                // Вывести на экран время сек
			Current_unixtime = dt.unixtime;
			myGLCD.printNumI((Current_unixtime - Start_unixtime) / 60, 250, 180);     // Вывести на экран время мин
			myGLCD.setFont(BigFont);

			if (trig_sin)
			{
				myGLCD.printNumF(((EndMeasure - StartSynhro) / 1000), 1, LEFT, 30);    // Вывести на экран задержку появления синхроимпульса
			}
			else
			{
				myGLCD.print("                 ", LEFT, 30);                         // Стереть информаию о задержке
			}


			myGLCD.print("ms", 100,30);
			for (int xpos = 0; xpos < 239; xpos++)                                   // вывод на экран
			{
				// Нарисовать линию  осциллограммы
				if (xpos == 119)
				{
					myGLCD.setColor(VGA_YELLOW);
					myGLCD.drawLine(xpos-1, 80, xpos-1, 160);
					myGLCD.drawLine(xpos, 80, xpos, 160);
					myGLCD.drawLine(xpos + 1, 80, xpos + 1, 160);
					myGLCD.setColor(255, 255, 255);
				}

				if (Sample_osc[xpos] == HIGH)
				{
					myGLCD.setColor(255, 255, 255);
					myGLCD.drawLine(xpos, 100, xpos, 160);
					myGLCD.drawLine(xpos + 1, 100, xpos + 1, 160);
				}

				myGLCD.setFont(SmallFont);
				myGLCD.setColor(VGA_WHITE);


				if (Synhro_osc[xpos][line_info] == 4095)
				{
					myGLCD.drawLine(xpos, 120, xpos, 160);

				}
				if (Synhro_osc[xpos][ms_info] > 0)
				{
					myGLCD.printNumI(Synhro_osc[xpos][ms_info], xpos, 165);
				}
			}
		}
		*/
		if (myTouch.dataAvailable())
		{
			measure_enable = false;
			break;                                    //Остановить вывод на экран
		}

		dt = DS3231_clock.getDateTime();
		if (oldsec != dt.second)
		{
			myGLCD.setBackColor(0, 0, 0);                   //  
			myGLCD.setColor(255, 255, 255);
			myGLCD.setFont(SmallFont);
			myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s", dt), 10, 3);
			myGLCD.setFont(BigFont);
			oldsec = dt.second;
		}
	}
	while (myTouch.dataAvailable()) {}               // Ждать когда экран будет освобожден от прикосновения
	delay(400);
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
		//myGLCD.setColor(VGA_LIME);                            // Вывести на дисплей время ответа синхроимпульса
		//myGLCD.print("     ", 90 - 40, 178);                  // Вывести на дисплей время ответа синхроимпульса
		//myGLCD.printNumI(0, 90 - 32, 178);                    // Вывести на дисплей время ответа синхроимпульса
		//myGLCD.setColor(255, 255, 255);
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
				//myGLCD.print("Delay: ", 5, 178);              // Вывести на дисплей время ответа синхроимпульса
				//myGLCD.print("     ", 90 - 40, 178);          // Вывести на дисплей время ответа синхроимпульса
				//myGLCD.setColor(VGA_LIME);                    // Вывести на дисплей время ответа синхроимпульса
				if (tim1 < 999)                               // Подравнять вывод чисел на дисплей 
				{
					//myGLCD.printNumI(tim1, 90 - 32, 178);     // Вывести на дисплей время ответа синхроимпульса
				}
				else
				{
					//myGLCD.printNumI(tim1, 90 - 40, 178);     // Вывести на дисплей время ответа синхроимпульса
				}
				//myGLCD.setColor(255, 255, 255);
				//myGLCD.print("microsec", 90, 178);            // Вывести на дисплей время ответа синхроимпульса
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
	if (logTime1 - Start_Power > 2000)                 //  индикация 
	{
		Start_Power = millis();
		int m_power = 0;
		float ind_power = 0;
		ADC_CHER = Analog_pinA3;                      // Подключить канал А3, разрядность 12
		ADC_CR = ADC_START; 	                      // Запустить преобразование
		while (!(ADC_ISR_DRDY));                      // Ожидание конца преобразования
		m_power = ADC->ADC_CDR[4];                    // Считать данные с канала A3
		ind_power = m_power *(3.25 / 4096 * 1.93);        // Получить напряжение в вольтах
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.setFont(SmallSymbolFont);
		if (ind_power > 4.0)
		{
			myGLCD.setColor(VGA_LIME);
			myGLCD.drawRoundRect(279, 149, 319, 189);
			myGLCD.setColor(VGA_WHITE);
			myGLCD.print("\x20", 295, 155);
		}
		else if (ind_power > 3.8 && ind_power < 4.0)
		{
			myGLCD.setColor(VGA_LIME);
			myGLCD.drawRoundRect(279, 149, 319, 189);
			myGLCD.setColor(VGA_WHITE);
			myGLCD.print("\x21", 295, 155);
		}
		else if (ind_power > 3.6 && ind_power < 3.8)
		{
			myGLCD.setColor(VGA_WHITE);
			myGLCD.drawRoundRect(279, 149, 319, 189);
			myGLCD.setColor(VGA_WHITE);
			myGLCD.print("\x22", 295, 155);
		}
		else if (ind_power > 3.4 && ind_power < 3.6)
		{
			myGLCD.setColor(VGA_YELLOW);
			myGLCD.drawRoundRect(279, 149, 319, 189);
			myGLCD.setColor(VGA_WHITE);
			myGLCD.print("\x23", 295, 155);
		}
		else if (ind_power < 3.4)
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
void chench_Channel()
{
	//Подготовка номера аналогового сигнала, количества каналов и кода настройки АЦП
	Channel_x = 0;                                  // Аналоговый вход А0
//	count_pin = 1;                                  // Количество входов
	Channel_x |= 0x80;                              // Сформировать код установки входов
	ADC_CHER = Channel_x;                           // Записать код настройки входов в АЦП
	//SAMPLES_PER_BLOCK = DATA_DIM16 / count_pin;     // Размер выделенного буфера
}

void clean_mem()
{
	byte b = i2c_eeprom_read_byte(deviceaddress, adr_mem_start);                                  // Получить признак первого включения прибора после сборки
	Serial.println(b);
	if (b != mem_start)
	{
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print("O""\xA7\x9D""c""\xA4\x9F""a ""\xA3""a""\xA1\xAF\xA4\x9D", CENTER, 60);      // "Очистка памяти"
		for (int i = 0; i < 512; i++)
		{
			i2c_eeprom_write_byte(deviceaddress, i, 0);
			//delay(10);
		}
		i2c_eeprom_write_byte(deviceaddress, adr_mem_start, mem_start);
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

void Start_main_up()
{

		detachInterrupt(kn_red);
		detachInterrupt(kn_blue);
		kn = 1;
		//Serial.println("Up");
		attachInterrupt(kn_red, Start_main_up, FALLING);
		attachInterrupt(kn_blue, Start_main_down, FALLING);

}
void Start_main_down()
{

		detachInterrupt(kn_red);
		detachInterrupt(kn_blue);
		kn = 2;
		//Serial.println("Down");
		attachInterrupt(kn_red, Start_main_up, FALLING);
		attachInterrupt(kn_blue, Start_main_down, FALLING);
	
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
	digitalWrite(vibro, LOW);
	pinMode(sounder, OUTPUT);
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
	Serial.println(F("Version - SoundMeasureDUE_10"));

	Wire.begin();
	setup_pin();
	myGLCD.InitLCD();
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);                   // Синий фон кнопок
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(SmallFont);
	myGLCD.print("Version - Sound Measure DUE_10", 10, 220);
	myGLCD.setFont(BigFont);
	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);
	//myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
	setup_radio();                                    // Настройка радио канала
	ADC_MR |= 0x00000100;                                           // ADC full speed
	chench_Channel();
	clean_mem();                                                     // Очистка памяти при первом включении прибора  
	alarm_synhro = 0;

	DS3231_clock.begin();

	myGLCD.print("HACTPO""\x87""KA", CENTER, 40);  // НАСТРОЙКА
	myGLCD.print("C""\x86""HXPOH""\x86\x85""A""\x8C\x86\x86", CENTER, 60);  // СИНХРОНИЗАЦИИ
	myGLCD.setFont(SmallFont);

	// Disarm alarms and clear alarms for this example, because alarms is battery backed.
	// Under normal conditions, the settings should be reset after power and restart microcontroller.
	DS3231_clock.armAlarm1(false);
	DS3231_clock.armAlarm2(false);
	DS3231_clock.clearAlarm1();
	DS3231_clock.clearAlarm2();

	dt = DS3231_clock.getDateTime();
	if (dt.year == 2000 || dt.year == 2165)
	{
		DS3231_clock.setDateTime(__DATE__, __TIME__);
		delay(200);
		dt = DS3231_clock.getDateTime();
		Serial.println(DS3231_clock.dateFormat("d-m-Y H:i:s", dt));
	}
	// Enable output
	DS3231_clock.setOutput(DS3231_1HZ);
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

	DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);         //DS3231_EVERY_SECOND //Каждую секунду
	while (true)
	{
		dt = DS3231_clock.getDateTime();
		if (oldsec != dt.second)
		{
			myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s", dt), 10, 3);
			oldsec = dt.second;
		}
		if (dt.second == 0 || dt.second == 10 || dt.second == 20 || dt.second == 30 || dt.second == 40 || dt.second == 50)
		{
			break;
		}
	}

//	attachInterrupt(alarm_pin, alarmFunction, FALLING);        // прерывание вызывается только при смене значения на порту с LOW на HIGH
	attachInterrupt(kn_red, Start_main_up, FALLING);
	attachInterrupt(kn_blue, Start_main_down, FALLING);
	Timer7.attachInterrupt(sevenHandler);                      // Timer7 - запись временных меток  в массив для вывода на экран
	// Звуковая сигнализация окончания setup 
	sound1();
	delay(100);
	sound1();
	delay(100);
	vibro1();
	delay(100);
	vibro1();
	Serial.println(F("Setup Ok!"));
}

void loop()
{
   Start_Menu();
}
