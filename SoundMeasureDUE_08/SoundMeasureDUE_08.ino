
/*

������ ����� 21.08.2017�.


�������������� 11.11.2017�.

��������� 14.11.2017�.

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

// ��������� ��������

UTFT myGLCD(TFT01_28, 38, 39, 40, 41);     // ��������� ��������
										   //UTFT myGLCD(ITDB28, 38, 39, 40, 41);     // ��������� ��������
UTouch        myTouch(6, 5, 4, 3, 2);          // ��������� ����������

UTFT_Buttons  myButtons(&myGLCD, &myTouch);
boolean default_colors = true;
uint8_t menu_redraw_required = 0;

//******************���������� ���������� ��� �������� � ����� ���� (������)****************************

int but1, but2, but3, but4, butX, pressed_button;


// +++++++++++++++ �������� pin +++++++++++++++++++++++++++++++++++++++++++++

#define intensityLCD      9                         // ���� ���������� �������� ������
#define synhro_pin       66                         // ���� ��� ������������� ���� �  ������������
#define alarm_pin         7                         // ���� ���������� �� �������  DS3231
#define kn_red           43                         // AD4 ������ ������� +
#define kn_blue          42                         // AD6 ������ ����� -
#define vibro            11                         // ����������
#define sounder          53                         // ������
#define LED_PIN13        13                         // ���������


// -------------------   ��������� �������� ������� DS3231 -------------------

int oldsec = 0;
//char* str_mon[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

DS3231 DS3231_clock;
RTCDateTime dt;
//char* daynames[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

boolean isAlarm           = false;                               //
//boolean alarmState        = false;                               //
int alarm_synhro          = 0;                                   //
unsigned long alarm_count = 0;                                   //




//+++++++++++++++++++++++++++++ ������� ������ +++++++++++++++++++++++++++++++++++++++
int deviceaddress = 80;                                          // ����� ���������� ������
int mem_start     = 24;                                          // ������� ������� ��������� ������� ����� ������. �������� ������  
int adr_mem_start = 1023;                                        // ����� �������� �������� ������� ��������� ������� ����� ������
byte hi;                                                         // ������� ���� ��� �������������� �����
byte low;                                                        // ������� ���� ��� �������������� �����

int adr_set_time = 100;                                          // ����� ������� ������������� �������




// ++++++++++++++++++++++ + ��������� ������������ ���������++++++++++++++++++++++++++++++++++++ +

#define address_AD5252   0x2C                                    // ����� ���������� AD5252  
#define control_word1    0x07                                    // ���� ���������� �������� �1
#define control_word2    0x87                                    // ���� ���������� �������� �2
byte resistance = 0x00;                                          // ������������� 0x00..0xFF - 0��..100���
//byte level_resist      = 0;                                    // ���� ��������� ������ �������� ���������

//+++++++++++++++++++++++++++++++

RF24 radio(48, 49);                                                        // DUE

unsigned long timeoutPeriod = 3000;                             // Set a user-defined timeout period. With auto-retransmit set to (15,15) retransmission will take up to 60ms and as little as 7.5ms with it set to (1,15).
										                        // With a timeout period of 1000, the radio will retry each payload for up to 1 second before giving up on the transmission and starting over
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };   // Radio pipe addresses for the 2 nodes to communicate.

byte data_in[24];                                               // ����� �������� �������� ������
byte data_out[24];                                              // ����� �������� ������ ��� ��������
volatile unsigned long counter;
unsigned long rxTimer, startTime, stopTime, payloads = 0;
bool TX = 1, RX = 0, role = 0, transferInProgress = 0;

typedef enum { role_ping_out = 1, role_pong_back } role_e;                  // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back" };  // The debug-friendly names of those roles
role_e role_test = role_pong_back;                                          // The role of the current running sketch
byte counter_test = 0;

const char str[] = "My very long string";
extern "C" char *sbrk(int i);
uint32_t message;                                                // ��� ���������� ��� ����� ��������� ��������� �� ���������;
unsigned long tim1 = 0;                                          // ����� ������ ��������������
unsigned long timeStartRadio = 0;                                // ����� ������ ����� ������ ��������
unsigned long timeStopRadio = 0;                                 // ����� ��������� ����� ������ ��������

// +++++++++++++++++++++++ ��������� �������� ������� ++++++++++++++++++++++++++

int time_sound = 50;                                             // ������������ �������� �������
int freq_sound = 1800;                                           // ������� �������� �������
byte volume1 = 100;                                              // 
byte volume2 = 100;                                              //
volatile byte volume_variant = 0;                                // ���������� ������������� ��������� ��. ����������. 

//++++++++++++++++++++++++++++++++ ��������� ���������� ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
unsigned long Start_Power = 0;                                  // ������ �������� �������
volatile int count_ms     = 0;                                  // ��������� �����
bool wiev_Start_Menu      = false;                              // ���������� ������ ���� ����� ������������� ������� �����
volatile bool start_enable = false;                             // ����� ������ ��������
bool synhro_enable = false;                                     // ������� �������� �������������



//***************** ���������� ���������� ��� �������� �������*****************************************************

char  txt_Start_Menu[] = "HA""CTPO""\x87""K""\x86";                                                     // "���������"

char  txt_menu1_1[] = "\x86\x9C\xA1""ep.""\x9C""a""\x99""ep""\x9B\x9F\x9D";                             // "�����.��������"
char  txt_menu1_2[] = "HA""CTPO""\x87""K""\x86";                                                        // "���������"
char  txt_menu1_3[] = "\x8D""AC""\x91"" C""\x86""HXPO.";                                                // "���� ������."
char  txt_menu1_4[] = "";                                                                               // "������ � SD"
char  txt_menu1_5[] = "B\x91XO\x82";                                                                    // "�����"       

char  txt_info11[]  = "ESC->PUSH Display";                                                              //

char  txt_delay_measure1[] = "C""\x9D\xA2""xpo ""\xA3""o ""\xA4""a""\x9E\xA1""epy";                     // ������ �� �������
char  txt_delay_measure2[] = "C""\x9D\xA2""xpo ""\xA3""o pa""\x99\x9D""o";                              // ������ �� ����� 
char  txt_delay_measure3[] = "";                                                                        //
char  txt_delay_measure4[] = "B\x91XO\x82";                                                             // �����    

char  txt_tuning_menu1[] = "Oc\xA6\x9D\xA0\xA0o\x98pa\xA5";                                             // "�����������"
char  txt_tuning_menu2[] = "C""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\xAF"" ";                        // "�������������"
char  txt_tuning_menu3[] = "PA""\x82\x86""O";                                                           // �����
char  txt_tuning_menu4[] = "B\x91XO\x82";                                                               // ����� 

char  txt_synhro_menu1[] = "C""\x9D\xA2""xpo pa""\x99\x9D""o";                                          // "������ �����"
char  txt_synhro_menu2[] = "C""\x9D\xA2""xpo ""\xA3""po""\x97""o""\x99";                                // "������ ������"
char  txt_synhro_menu3[] = "C""\xA4""o""\xA3"" c""\x9D\xA2""xpo.";                                      // "���� ������."
char  txt_synhro_menu4[] = "B\x91XO\x82";                                                               // �����      

//++++++++++++++++++++++++++++  ��������� ��� +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*
��������� ������� ������ � ���(����������� ����� ��������)
������ ����� ���������� ��������� ��������� ��� � �������������� �������� ADC_MR, ������ ��� ����� ����������� ��������������(8 ��� 10 ���),
����� ������(���������� / ������), ������� ������������ ���, ����� ������� � ������ ���������.
����� ���������� �������� ������, �� ������� ����� ����������� �������������� � ������� �������� ADC_CHER.
���� �������������� �������� � ��� � ������ ��� �����������, ���������� ��������� ���������� ���������� �� ��������� �������� ���������� �� ���,
������� ��������� ��������� ����������, � ����� �������� ������ �������� ���������� �� ������� ��� �� ����������� ��������(�������� �� ��������� ��������������).
����� ������������ ������ ��� � �������������� �������� ADC_CR.
���������� ���������� ��������������(������ �����) ������������ ���� �� �������� ���������� �������������� �� ������(��������, ADC_CDR4 ��� ������ 4),
���� �� �������� ���������� ���������� ��������������(ADC_LCDR).���������� ���������� ������ ����������� �� ����� ��������������!
������� ��� ������ � ��� � ������ ��� ���������� ����� ����������� ���������� ���������� ���������,
���� �� ����������� ��������������� ���� ���������� � �������� ADC_SR.���� ��������������� �������� �� ��������� ���������� �� ��������� ��������������,
��������� ����������� � ����������� ����������.
��������� ��������������, ���������� � ���� ������ ����� �� �������� �������� ���������� �� ����� ���.
��� ��������� ������ �������� ���������� ��������� ��������� �����������������.��� ����� ��������� �������������� ���������� �������� �� �������� ������ ��������������.
�������� ������ ������������ ���
q = Uref / 2n,
��� Uref � ������� ���������� ���, n � ����������� ��������������(������������ ����� LOW_RES �������� ADC_MR).
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
#define ADC_CHER * (volatile unsigned int *) (0x400C0010)           /*ADC Channel Enable Register  ������ ������*/
#define ADC_CHSR * (volatile unsigned int *) (0x400C0018)           /*ADC Channel Status Register  ������ ������ */
#define ADC_CDR0 * (volatile unsigned int *) (0x400C0050)           /*ADC Channel ������ ������ */
																								   //#define ADC_ISR_EOC0 0x00000001

//-------------  ��������� ���������� ������  ------------------------
#define Analog_pinA0 ADC_CHER_CH7    // ���� A0
#define Analog_pinA1 ADC_CHER_CH6    // ���� A1
#define Analog_pinA2 ADC_CHER_CH5    // ���� A2
#define Analog_pinA3 ADC_CHER_CH4    // ���� A3
#define Analog_pinA4 ADC_CHER_CH3    // ���� A4
#define Analog_pinA5 ADC_CHER_CH2    // ���� A5
#define Analog_pinA6 ADC_CHER_CH1    // ���� A6
#define Analog_pinA7 ADC_CHER_CH0    // ���� A7



//+++++++++++++++++++++++  ��������� EEPROM +++++++++++++++++++++++++++++++++++++
unsigned long i2c_eeprom_ulong_read(int addr)
{
	byte raw[4];
	for (byte i = 0; i < 4; i++) raw[i] = i2c_eeprom_read_byte(deviceaddress, addr + i);
	unsigned long &num = (unsigned long&)raw;
	return num;
}
// ������
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

// ******************* ������� ���� ********************************
void draw_Start_Menu()
{
//	myGLCD.clrScr();
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRoundRect(0,15, 319, 239);
	but4 = myButtons.addButton(10, 200, 250, 35, txt_Start_Menu);
	butX = myButtons.addButton(279, 199, 40, 40, "W", BUTTON_SYMBOL); // ������ ���� 
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
		myButtons.setTextFont(BigFont);                      // ���������� ������� ����� ������  
		measure_power();
		if (myTouch.dataAvailable() == true)                 // ��������� ������� ������
		{
		//	sound1();
			pressed_button = myButtons.checkButtons();       // ���� ������ - ��������� ��� ������
			if (pressed_button == butX)                      // ������ ����� ����
			{
				//sound1();
				myGLCD.setFont(BigFont);
				setClockRTC();
				myGLCD.clrScr();
				myButtons.drawButtons();                    // ������������ ������
			}

			//*****************  ���� �1  **************

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
			if (pressed_button == but4)             // ������ � SD
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
				if ((y >= 195) && (y <= 235))  // Button: 5 "EXIT" �����
				{
					waitForIt(10, 195, 250, 235);
					break;
				}
			}
		}
	}
}
void Draw_menu_delay_measure()                                    // ����������� ���� �������������
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
void menu_delay_measure()                                                // ���� ��������� �������������
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
				if ((y >= 20) && (y <= 60))                                // Button: 1  ������������� �� �������
				{
					waitForIt(20, 20, 300, 60);
					myGLCD.clrScr();
//					synhro_by_timer();
					Draw_menu_delay_measure();
				}
				if ((y >= 70) && (y <= 110))                               // Button: 2 ������������� �� �����
				{
					waitForIt(20, 70, 300, 110);
					myGLCD.clrScr();
//					synhro_by_radio();                                    //  "������������� �� �����"
					Draw_menu_delay_measure();
				}
				if ((y >= 120) && (y <= 160))                             // Button: 3  
				{
					waitForIt(20, 120, 300, 160);
					myGLCD.clrScr();
					//Draw_menu_ADC_RF();                                   // ���������� ���� ������������
					//menu_ADC_RF();                                        // ������� � ���� ������������ � ������ RF
					Draw_menu_delay_measure();
				}
				if ((y >= 170) && (y <= 220))                             // Button: 4 "EXIT" �����
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
void menu_tuning()   // ���� "���������", ���������� �� �������� ���� 
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
				if ((y >= 70) && (y <= 110))   // Button: 2 "������������� ���."
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
				if ((y >= 170) && (y <= 220))  // Button: 4 "EXIT" �����
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
void menu_synhro()                                                        // ���� "���������", ���������� �� �������� ���� 
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
				if ((y >= 20) && (y <= 60))                                 // Button: 1  "������������� �� �����"
				{
					waitForIt(20, 20, 300, 60);
					myGLCD.clrScr();
					data_out[2] = 2;                                        // ��������� ������� ������������� �������(�������� ������ ����������)
			//		tuning_mod();                                           // ������������� �� �����
					Draw_menu_synhro();
				}
				if ((y >= 70) && (y <= 110))                               // Button: 2 "������������� ���������"
				{
					waitForIt(20, 70, 300, 110);
					myGLCD.clrScr();
					data_out[2] = 3;                                        // ��������� ������� ������������� �������(�������� ������ ����������)
					radio_send_command();
			//		synhro_ware();
					Draw_menu_synhro();
				}
				if ((y >= 120) && (y <= 160))  // Button: 3  
				{
					waitForIt(20, 120, 300, 160);
					myGLCD.clrScr();
					data_out[2] = 4;                                        // �������� ������� ����
//					setup_radio();                                          // ��������� ����������
//					delayMicroseconds(500);
					radio_send_command();                                   //  ��������� �� ����� ������� ����
					//Timer4.stop();
					//Timer5.stop();
					//Timer6.stop();
					Draw_menu_synhro();
				}
				if ((y >= 170) && (y <= 220))  // Button: 4 "EXIT" �����
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
//++++++++++++++++++++++++++ ����� ���� ������� ++++++++++++++++++++++++



void alarmFunction()
{
	DS3231_clock.clearAlarm1();
	dt = DS3231_clock.getDateTime();
//	Serial.println(DS3231_clock.dateFormat("H:i:s", dt));
	myGLCD.setBackColor(0, 0, 0);                   // ����� ��� ������
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
		myGLCD.print("C""\x9D\xA2""xpo ""\xA2""e ""\xA3""o""\x99\x9F\xA0\xAE\xA7""e""\xA2", CENTER, 80);   // ������ �� ���������
		delay(2000);
	}
	else
	{
		//adr_set_time = 100;                                          // ����� ������� ������������� �������
		//unsigned long Start_unixtime = dt.unixtime;


		dt = DS3231_clock.getDateTime();
		data_out[2] = 8;                                      // ��������� ������� ������������� �����
		data_out[12] = dt.year - 2000;                      // 
		data_out[13] = dt.month;                            // 
		data_out[14] = dt.day;                              // 
		data_out[15] = dt.hour;                             //
		data_out[16] = dt.minute;                           // 

		radio_send_command();

		unsigned long StartSynhro = micros();               // �������� �����
	
		while (!synhro_enable) 
		{
			if (micros() - StartSynhro >= 20000000)
			{
				myGLCD.setFont(BigFont);
				myGLCD.setColor(VGA_RED);
				myGLCD.print("He""\xA4"" c""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\x9D", CENTER, 80);   // "��� �������������"
				delay(2000);
				break;
			}
			if (digitalRead(synhro_pin) == HIGH)
			{
				do {} while (digitalRead(synhro_pin));                                       // ����� ��������� ��������������
				DS3231_clock.setDateTime(dt.year, dt.month, dt.day, dt.hour, dt.minute, 00); // ���������� ����� ������������������ �����
				alarm_synhro = 0;                                                            // ���������� ������� ���������������
				DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);                     // DS3231_EVERY_SECOND //���������� ������ �������
				while (digitalRead(alarm_pin) != HIGH){}                                     // �������������� �������� ����������
				while (digitalRead(alarm_pin) == HIGH){}                                     // �������������� �������� ����������

				while (true)                                                                 // ������������� ����� ������ ������ �������������
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
				if(!synhro_enable) attachInterrupt(alarm_pin, alarmFunction, FALLING);      // ���������� ���������� ������ ��� ����� �������� �� ����� � LOW �� HIGH
				myGLCD.setFont(BigFont);
				myGLCD.setColor(VGA_LIME);
				myGLCD.print("C""\x9D\xA2""xpo OK!", CENTER, 80);                            // ������ ��!
				myGLCD.setColor(255, 255, 255);
				delay(1000);
				synhro_enable = true;
			}
		}  // ������������� ���������, ��������� ���������
	}
}



void radio_send_command()                                   // ������������ �������
{
	//detachInterrupt(kn_red);
	//detachInterrupt(kn_blue);
	tim1 = 0;                                                 // ����� ������ ��������������
//	volume_variant = 3;                                       // ���������� ������� ��������� 1 ������ ��������� ������������
	radio.stopListening();                                    // ��-������, ����������� �������, ����� �� ����� ����������.
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setFont(SmallFont);
//	if (kn != 0) set_volume(volume_variant, kn);
	data_out[0] = counter_test;
	data_out[1] = 1;                                          //  
	//data_out[2] = 2;                                        // ������� ������ ����� ��� ������ ���������
	data_out[3] = 1;                                          //
	data_out[4] = highByte(time_sound);                       // ������� ���� ��������� ������������ �������� �������
	data_out[5] = lowByte(time_sound);                        // ������� ���� ��������� ������������ �������� �������
	data_out[6] = highByte(freq_sound);                       // ������� ���� ��������� ������� �������� �������
	data_out[7] = lowByte(freq_sound);                        // ������� ���� ��������� ������� �������� ������� 
	data_out[8] = volume1;                                    // 
	data_out[9] = volume2;                                    // 

	timeStartRadio = micros();                                // �������� ����� ������ ����� ������ ��������

	if (!radio.write(&data_out, sizeof(data_out)))
	{
		myGLCD.setColor(VGA_LIME);                            // ������� �� ������� ����� ������ ��������������
		myGLCD.print("     ", 90 - 40, 178);                  // ������� �� ������� ����� ������ ��������������
		myGLCD.printNumI(0, 90 - 32, 178);                    // ������� �� ������� ����� ������ ��������������
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
				timeStopRadio = micros();                     // �������� ����� ��������� ������.  
				radio.read(&data_in, sizeof(data_in));
				tim1 = timeStopRadio - timeStartRadio;        // �������� ����� ������ ������ ��������
				myGLCD.print("Delay: ", 5, 178);              // ������� �� ������� ����� ������ ��������������
				myGLCD.print("     ", 90 - 40, 178);          // ������� �� ������� ����� ������ ��������������
				myGLCD.setColor(VGA_LIME);                    // ������� �� ������� ����� ������ ��������������
				if (tim1 < 999)                               // ���������� ����� ����� �� ������� 
				{
					myGLCD.printNumI(tim1, 90 - 32, 178);     // ������� �� ������� ����� ������ ��������������
				}
				else
				{
					myGLCD.printNumI(tim1, 90 - 40, 178);     // ������� �� ������� ����� ������ ��������������
				}
				myGLCD.setColor(255, 255, 255);
				myGLCD.print("microsec", 90, 178);            // ������� �� ������� ����� ������ ��������������
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
	myGLCD.print(txt_info11, CENTER, 221);            // ������ "ESC -> PUSH"
	myGLCD.setBackColor(0, 0, 0);
	setup_radio();
	role_test = role_ping_out;
	tim1 = 0;
	volume_variant = 3;                                       //  ���������� ������� ��������� 1 ������ ��������� ������������
	int x_touch, y_touch;

	while (1)
	{
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_touch = myTouch.getX();
			y_touch = myTouch.getY();

			if ((x_touch >= 2) && (x_touch <= 319))               //  ������� ������
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
			radio.stopListening();                                  // ��-������, ����������� �������, ����� �� ����� ����������.
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
			data_out[1] = 1;                                       //1= ��������� ������� ping �������� ������� 
			data_out[2] = 1;                    //
			data_out[3] = 1;                    //
			data_out[4] = highByte(time_sound);                     //1= ��������� �������� ������� 
			data_out[5] = lowByte(time_sound);                    //1= ��������� �������� ������� 
			data_out[6] = highByte(freq_sound);                    //1= ��������� �������� ������� 
			data_out[7] = lowByte(freq_sound);                   //1= ��������� �������� ������� 
			data_out[8] = volume1;
			data_out[9] = volume2;


			//int value = 3000;
			//// ���������
			//byte hi = highByte(value);
			//byte low = lowByte(value);

			//// ��� �� ��� hi,low ����� ��������, ��������� �� eePROM

			//int value2 = (hi << 8) | low; // �������� ��� "��������� �����������"
			//int value3 = word(hi, low); // ��� �������� ��� "����������"

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
{                                                     // ��������� ��������� ���������� ������� � ��������� 1/2 
												      // ���������� ����������� �������� +15� ��� 10� �� ������ �������
	uint32_t logTime1 = 0;
	logTime1 = millis();
	if (logTime1 - Start_Power > 500)                 //  ��������� 
	{
		Start_Power = millis();
		int m_power = 0;
		float ind_power = 0;
		ADC_CHER = Analog_pinA3;                      // ���������� ����� �3, ����������� 12
		ADC_CR = ADC_START; 	                      // ��������� ��������������
		while (!(ADC_ISR_DRDY));                      // �������� ����� ��������������
		m_power = ADC->ADC_CDR[4];                    // ������� ������ � ������ A3
		ind_power = m_power *(3.2 / 4096 * 2);        // �������� ���������� � �������
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
	byte b = i2c_eeprom_read_byte(deviceaddress, adr_mem_start);                                  // �������� ������� ������� ��������� ������� ����� ������
	if (b != mem_start)
	{
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print("O""\xA7\x9D""c""\xA4\x9F""a ""\xA3""a""\xA1\xAF\xA4\x9D", CENTER, 60);      // "������� ������"
		for (int i = 0; i < 1024; i++)
		{
			i2c_eeprom_write_byte(deviceaddress, i, 0x00);
			delay(10);
		}
		i2c_eeprom_write_byte(deviceaddress, adr_mem_start, mem_start);
//		i2c_eeprom_ulong_write(adr_set_timeSynhro, set_timeSynhro);                              // ��������  ����� 
		myGLCD.print("                 ", CENTER, 60);                                           // "������� ������"
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
	myGLCD.setBackColor(0, 0, 0);                   // ����� ��� ������
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(BigFont);

	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);
	//myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
	setup_radio();                                // ��������� ����� ������

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

// �������� ������������ ��������� setup 
	sound1();
	delay(100);
	sound1();
	delay(100);
	vibro1();
	delay(100);
	vibro1();


	clean_mem();                                                     // ������� ������ ��� ������ ��������� �������  
	alarm_synhro = 0;

	myGLCD.setFont(SmallFont);
	
	while (digitalRead(alarm_pin) != HIGH)
	{

	}

	while (digitalRead(alarm_pin) == HIGH)
	{

	}

	DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);         //DS3231_EVERY_SECOND //������ �������
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
	attachInterrupt(alarm_pin, alarmFunction, FALLING);      // ���������� ���������� ������ ��� ����� �������� �� ����� � LOW �� HIGH
  //  attachInterrupt(alarm_pin, alarmFunction, RISING);     // ���������� ���������� ������ ��� ����� �������� �� ����� � HIGH �� LOW
	Serial.println(F("Setup Ok!"));
}

void loop()
{
	if (wiev_Start_Menu)
	{
		Start_Menu();
	}
}
