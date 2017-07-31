
/*
Программа тестирования звукового канала
Заказчик "Децима"
Начало работ 01.06.2017г.

Тестируемые параметры:
Измерение задержки прохождения звукового сигнала.
Регистрация помех в тестируемом канале.


*/


#define __SAM3X8E__
#include <UTFT.h>
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include "AnalogBinLogger.h"
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <DueTimer.h>
#include "Wire.h"
#include <rtc_clock.h>


extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

UTFT myGLCD(TFT01_28, 38, 39, 40, 41);

UTouch        myTouch(6, 5, 4, 3, 2);
#define TOUCH_ORIENTATION  PORTRAIT
//
//// Finally we set up UTFT_Buttons :)
UTFT_Buttons  myButtons(&myGLCD, &myTouch);
boolean default_colors = true;
uint8_t menu_redraw_required = 0;

StdioStream csvStream;
// serial output steam
ArduinoOutStream cout(Serial);

//----------------------Конец  Настройки дисплея --------------------------------
RTC_clock rtc_clock(XTAL);

char* str[] = { "MON","TUE","WED","THU","FRI","SAT","SUN" };
char* str_mon[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
char* daynames[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
uint8_t sec = 0;       //Initialization time
uint8_t min = 0;
uint8_t hour = 0;
uint8_t dow1 = 1;
uint8_t date = 1;
uint8_t mon1 = 1;
uint16_t year = 14;
unsigned long timeF;
int flag_time = 0;

int hh, mm, ss, dow, dd, mon, yyyy;

const int clockCenterX = 119;
const int clockCenterY = 119;
int oldsec = 0;
//------------------------------------------------------------

//******************Назначение переменных для хранения № опций меню (клавиш)****************************

int but1, but2, but3, but4, but5, but6, but7, but8, but9, but10, butX, butY, but_m1, but_m2, but_m3, but_m4, but_m5, pressed_button;
int m2 = 1; // Переменная номера меню

			//=====================================================================================




//+++++++++++++++++++++++ SD info ++++++++++++++++++++++++++
SdFile file;

File root;

SdFat sd;

SdBaseFile binFile;

Sd2Card card;

// SD chip select pin.

const uint8_t SD_CS_PIN = 10;

//----------------------------------------------------------------------
uint32_t cx, cy;
uint32_t rx[10], ry[10];
uint32_t clx, crx, cty, cby;
float px, py;
int dispx, dispy, text_y_center;
uint32_t calx, caly, cals;
char buf[13];


//***************** Назначение переменных для хранения текстов*****************************************************

char  txt_menu1_1[] = "PE\x81\x86""CTPATOP";                                                       // "РЕГИСТРАТОР"
char  txt_menu1_2[] = "CAMO\x89\x86""CE\x8C";                                                      // "САМОПИСЕЦ"
char  txt_menu1_3[] = "PE\x81\x86""CT.+ CAMO\x89.";                                                // "РЕГИСТ. + САМОП."
char  txt_menu1_4[] = "PA\x80OTA c SD";                                                            // "РАБОТА с SD"


char  txt_info11[] = "ESC->PUSH Display";




void dateTime(uint16_t* date, uint16_t* time) // Программа записи времени и даты файла
{
	rtc_clock.get_time(&hh, &mm, &ss);
	rtc_clock.get_date(&dow, &dd, &mon, &yyyy);

	// return date using FAT_DATE macro to format fields
	*date = FAT_DATE(yyyy, mon, dd);

	// return time using FAT_TIME macro to format fields
	*time = FAT_TIME(hh, mm, ss);
}

void drawDisplay()
{
	// Clear screen
	myGLCD.clrScr();

	// Draw Clockface
	myGLCD.setColor(0, 0, 255);
	myGLCD.setBackColor(0, 0, 0);
	for (int i = 0; i<5; i++)
	{
		myGLCD.drawCircle(clockCenterX, clockCenterY, 119 - i);
	}
	for (int i = 0; i<5; i++)
	{
		myGLCD.drawCircle(clockCenterX, clockCenterY, i);
	}

	myGLCD.setColor(192, 192, 255);
	myGLCD.print("3", clockCenterX + 92, clockCenterY - 8);
	myGLCD.print("6", clockCenterX - 8, clockCenterY + 95);
	myGLCD.print("9", clockCenterX - 109, clockCenterY - 8);
	myGLCD.print("12", clockCenterX - 16, clockCenterY - 109);
	for (int i = 0; i<12; i++)
	{
		if ((i % 3) != 0)
			drawMark(i);
	}

	rtc_clock.get_time(&hh, &mm, &ss);
	rtc_clock.get_date(&dow, &dd, &mon, &yyyy);

	// clock_read();
	drawMin(mm);
	drawHour(hh, mm);
	drawSec(ss);
	oldsec = ss;

	// Draw calendar
	myGLCD.setColor(255, 255, 255);
	myGLCD.fillRoundRect(240, 0, 319, 85);
	myGLCD.setColor(0, 0, 0);
	for (int i = 0; i<7; i++)
	{
		myGLCD.drawLine(249 + (i * 10), 0, 248 + (i * 10), 3);
		myGLCD.drawLine(250 + (i * 10), 0, 249 + (i * 10), 3);
		myGLCD.drawLine(251 + (i * 10), 0, 250 + (i * 10), 3);
	}

	// Draw SET button
	myGLCD.setColor(64, 64, 128);
	myGLCD.fillRoundRect(260, 200, 319, 239);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(260, 200, 319, 239);
	myGLCD.setBackColor(64, 64, 128);
	myGLCD.print("SET", 266, 212);
	myGLCD.setBackColor(0, 0, 0);

	myGLCD.setColor(64, 64, 128);
	myGLCD.fillRoundRect(260, 140, 319, 180);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(260, 140, 319, 180);
	myGLCD.setBackColor(64, 64, 128);
	myGLCD.print("RET", 266, 150);
	myGLCD.setBackColor(0, 0, 0);

}
void drawMark(int h)
{
	float x1, y1, x2, y2;

	h = h * 30;
	h = h + 270;

	x1 = 110 * cos(h*0.0175);
	y1 = 110 * sin(h*0.0175);
	x2 = 100 * cos(h*0.0175);
	y2 = 100 * sin(h*0.0175);

	myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
}
void drawSec(int s)
{
	float x1, y1, x2, y2;
	int ps = s - 1;

	myGLCD.setColor(0, 0, 0);
	if (ps == -1)
		ps = 59;
	ps = ps * 6;
	ps = ps + 270;

	x1 = 95 * cos(ps*0.0175);
	y1 = 95 * sin(ps*0.0175);
	x2 = 80 * cos(ps*0.0175);
	y2 = 80 * sin(ps*0.0175);

	myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);

	myGLCD.setColor(255, 0, 0);
	s = s * 6;
	s = s + 270;

	x1 = 95 * cos(s*0.0175);
	y1 = 95 * sin(s*0.0175);
	x2 = 80 * cos(s*0.0175);
	y2 = 80 * sin(s*0.0175);

	myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
}
void drawMin(int m)
{
	float x1, y1, x2, y2, x3, y3, x4, y4;
	int pm = m - 1;

	myGLCD.setColor(0, 0, 0);
	if (pm == -1)
		pm = 59;
	pm = pm * 6;
	pm = pm + 270;

	x1 = 80 * cos(pm*0.0175);
	y1 = 80 * sin(pm*0.0175);
	x2 = 5 * cos(pm*0.0175);
	y2 = 5 * sin(pm*0.0175);
	x3 = 30 * cos((pm + 4)*0.0175);
	y3 = 30 * sin((pm + 4)*0.0175);
	x4 = 30 * cos((pm - 4)*0.0175);
	y4 = 30 * sin((pm - 4)*0.0175);

	myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY);
	myGLCD.drawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
	myGLCD.drawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY);
	myGLCD.drawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY);

	myGLCD.setColor(0, 255, 0);
	m = m * 6;
	m = m + 270;

	x1 = 80 * cos(m*0.0175);
	y1 = 80 * sin(m*0.0175);
	x2 = 5 * cos(m*0.0175);
	y2 = 5 * sin(m*0.0175);
	x3 = 30 * cos((m + 4)*0.0175);
	y3 = 30 * sin((m + 4)*0.0175);
	x4 = 30 * cos((m - 4)*0.0175);
	y4 = 30 * sin((m - 4)*0.0175);

	myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY);
	myGLCD.drawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
	myGLCD.drawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY);
	myGLCD.drawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY);
}
void drawHour(int h, int m)
{
	float x1, y1, x2, y2, x3, y3, x4, y4;
	int ph = h;

	myGLCD.setColor(0, 0, 0);
	if (m == 0)
	{
		ph = ((ph - 1) * 30) + ((m + 59) / 2);
	}
	else
	{
		ph = (ph * 30) + ((m - 1) / 2);
	}
	ph = ph + 270;

	x1 = 60 * cos(ph*0.0175);
	y1 = 60 * sin(ph*0.0175);
	x2 = 5 * cos(ph*0.0175);
	y2 = 5 * sin(ph*0.0175);
	x3 = 20 * cos((ph + 5)*0.0175);
	y3 = 20 * sin((ph + 5)*0.0175);
	x4 = 20 * cos((ph - 5)*0.0175);
	y4 = 20 * sin((ph - 5)*0.0175);

	myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY);
	myGLCD.drawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
	myGLCD.drawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY);
	myGLCD.drawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY);

	myGLCD.setColor(255, 255, 0);
	h = (h * 30) + (m / 2);
	h = h + 270;

	x1 = 60 * cos(h*0.0175);
	y1 = 60 * sin(h*0.0175);
	x2 = 5 * cos(h*0.0175);
	y2 = 5 * sin(h*0.0175);
	x3 = 20 * cos((h + 5)*0.0175);
	y3 = 20 * sin((h + 5)*0.0175);
	x4 = 20 * cos((h - 5)*0.0175);
	y4 = 20 * sin((h - 5)*0.0175);

	myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY);
	myGLCD.drawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
	myGLCD.drawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY);
	myGLCD.drawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY);
}
void printDate()
{
	rtc_clock.get_time(&hh, &mm, &ss);
	rtc_clock.get_date(&dow, &dd, &mon, &yyyy);
	myGLCD.setFont(BigFont);
	myGLCD.setColor(0, 0, 0);
	myGLCD.setBackColor(255, 255, 255);
	myGLCD.print(str[dow - 1], 256, 8);
	if (dd<10)
		myGLCD.printNumI(dd, 272, 28);
	else
		myGLCD.printNumI(dd, 264, 28);

	myGLCD.print(str_mon[mon - 1], 256, 48);
	myGLCD.printNumI(yyyy, 248, 65);
}
void clearDate()
{
	myGLCD.setColor(255, 255, 255);
	myGLCD.fillRect(248, 8, 312, 81);
}
void AnalogClock()

{
	int x, y;

	drawDisplay();
	printDate();
	rtc_clock.get_time(&hh, &mm, &ss);
	rtc_clock.get_date(&dow, &dd, &mon, &yyyy);
	//clock_read();

	while (true)
	{

		if (oldsec != ss)
		{
			if ((ss == 0) && (mm == 0) && (hh == 0))
			{
				clearDate();
				printDate();
			}
			if (ss == 0)
			{
				drawMin(mm);
				drawHour(hh, mm);
			}
			drawSec(ss);
			oldsec = ss;
		}

		if (myTouch.dataAvailable())
		{
			myTouch.read();
			x = myTouch.getX();
			y = myTouch.getY();
			Serial.print(x);
			Serial.print(" - ");
			Serial.println(x);
			if (((y >= 200) && (y <= 239)) && ((x >= 260) && (x <= 319))) //установка часов
			{
				myGLCD.setColor(255, 0, 0);
				myGLCD.drawRoundRect(260, 200, 319, 239);
				setClockRTC();
			}

			if (((y >= 140) && (y <= 180)) && ((x >= 260) && (x <= 319))) //Возврат
			{
				myGLCD.setColor(255, 0, 0);
				myGLCD.drawRoundRect(260, 140, 319, 180);
				myGLCD.clrScr();
				myGLCD.setFont(BigFont);
				break;
			}
		}

		delay(10);
		rtc_clock.get_time(&hh, &mm, &ss);
		rtc_clock.get_date(&dow, &dd, &mon, &yyyy);
	}

}

void draw_Glav_Menu()
{
	but1 = myButtons.addButton(10, 20, 250, 35, txt_menu1_1);
	but2 = myButtons.addButton(10, 65, 250, 35, txt_menu1_2);
	but3 = myButtons.addButton(10, 110, 250, 35, txt_menu1_3);
	but4 = myButtons.addButton(10, 155, 250, 35, txt_menu1_4);
	butX = myButtons.addButton(279, 199, 40, 40, "W", BUTTON_SYMBOL); // кнопка Часы 
	myGLCD.setColor(VGA_BLACK);
	myGLCD.setBackColor(VGA_WHITE);
	myGLCD.setColor(0, 255, 0);
	myGLCD.setBackColor(0, 0, 0);
	myButtons.drawButtons();
}
void swichMenu() // Тексты меню в строках "txt....."

{
	while (1)
	{
		myButtons.setTextFont(BigFont);                      // Установить Большой шрифт кнопок  
		//measure_power();
		if (myTouch.dataAvailable() == true)              // Проверить нажатие кнопок
		{
			pressed_button = myButtons.checkButtons();    // Если нажата - проверить что нажато
			if (pressed_button == butX)                // Нажата вызов часы
			{
				myGLCD.setFont(BigFont);
				AnalogClock();
				myGLCD.clrScr();
				myButtons.drawButtons();         // Восстановить кнопки
			}

			//*****************  Меню №1  **************

			if (pressed_button == but1)
			{
				//Draw_menu_ADC1();
				//menu_ADC();
				myGLCD.clrScr();
				myButtons.drawButtons();;
			}

			if (pressed_button == but2)
			{
				//Draw_menu_Osc();
				//menu_Oscilloscope();
				myGLCD.clrScr();
				myButtons.drawButtons();
			}

			if (pressed_button == but3)
			{
				//oscilloscope_file();
				myGLCD.clrScr();
				myButtons.drawButtons();
			}
			if (pressed_button == but4)
			{
				//Draw_menu_SD();
				//menu_SD();
				myGLCD.clrScr();
				myButtons.drawButtons();
			}

		}
	}
}
void waitForIt(int x1, int y1, int x2, int y2)
{
	myGLCD.setColor(255, 0, 0);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
	while (myTouch.dataAvailable())
	myTouch.read();
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
}





void test_TFT()
{
	int buf[318];
	int x, x2;
	int y, y2;
	int r;

	// Clear the screen and draw the frame
	myGLCD.clrScr();

	myGLCD.setColor(255, 0, 0);
	myGLCD.fillRect(0, 0, 319, 13);
	myGLCD.setColor(64, 64, 64);
	myGLCD.fillRect(0, 226, 319, 239);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(255, 0, 0);
	myGLCD.print("* Universal Color TFT Display Library *", CENTER, 1);
	myGLCD.setBackColor(64, 64, 64);
	myGLCD.setColor(255, 255, 0);
	myGLCD.print("<http://www.RinkyDinkElectronics.com/>", CENTER, 227);

	myGLCD.setColor(0, 0, 255);
	myGLCD.drawRect(0, 14, 319, 225);

	// Draw crosshairs
	myGLCD.setColor(0, 0, 255);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.drawLine(159, 15, 159, 224);
	myGLCD.drawLine(1, 119, 318, 119);
	for (int i = 9; i<310; i += 10)
		myGLCD.drawLine(i, 117, i, 121);
	for (int i = 19; i<220; i += 10)
		myGLCD.drawLine(157, i, 161, i);

	// Draw sin-, cos- and tan-lines  
	myGLCD.setColor(0, 255, 255);
	myGLCD.print("Sin", 5, 15);
	for (int i = 1; i<318; i++)
	{
		myGLCD.drawPixel(i, 119 + (sin(((i*1.13)*3.14) / 180) * 95));
	}

	myGLCD.setColor(255, 0, 0);
	myGLCD.print("Cos", 5, 27);
	for (int i = 1; i<318; i++)
	{
		myGLCD.drawPixel(i, 119 + (cos(((i*1.13)*3.14) / 180) * 95));
	}

	myGLCD.setColor(255, 255, 0);
	myGLCD.print("Tan", 5, 39);
	for (int i = 1; i<318; i++)
	{
		myGLCD.drawPixel(i, 119 + (tan(((i*1.13)*3.14) / 180)));
	}

	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);
	myGLCD.setColor(0, 0, 255);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.drawLine(159, 15, 159, 224);
	myGLCD.drawLine(1, 119, 318, 119);

	// Draw a moving sinewave
	x = 1;
	for (int i = 1; i<(318 * 20); i++)
	{
		x++;
		if (x == 319)
			x = 1;
		if (i>319)
		{
			if ((x == 159) || (buf[x - 1] == 119))
				myGLCD.setColor(0, 0, 255);
			else
				myGLCD.setColor(0, 0, 0);
			myGLCD.drawPixel(x, buf[x - 1]);
		}
		myGLCD.setColor(0, 255, 255);
		y = 119 + (sin(((i*1.1)*3.14) / 180)*(90 - (i / 100)));
		myGLCD.drawPixel(x, y);
		buf[x - 1] = y;
	}
	/*
	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);

	// Draw some filled rectangles
	for (int i = 1; i<6; i++)
	{
		switch (i)
		{
		case 1:
			myGLCD.setColor(255, 0, 255);
			break;
		case 2:
			myGLCD.setColor(255, 0, 0);
			break;
		case 3:
			myGLCD.setColor(0, 255, 0);
			break;
		case 4:
			myGLCD.setColor(0, 0, 255);
			break;
		case 5:
			myGLCD.setColor(255, 255, 0);
			break;
		}
		myGLCD.fillRect(70 + (i * 20), 30 + (i * 20), 130 + (i * 20), 90 + (i * 20));
	}

	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);

	// Draw some filled, rounded rectangles
	for (int i = 1; i<6; i++)
	{
		switch (i)
		{
		case 1:
			myGLCD.setColor(255, 0, 255);
			break;
		case 2:
			myGLCD.setColor(255, 0, 0);
			break;
		case 3:
			myGLCD.setColor(0, 255, 0);
			break;
		case 4:
			myGLCD.setColor(0, 0, 255);
			break;
		case 5:
			myGLCD.setColor(255, 255, 0);
			break;
		}
		myGLCD.fillRoundRect(190 - (i * 20), 30 + (i * 20), 250 - (i * 20), 90 + (i * 20));
	}

	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);

	// Draw some filled circles
	for (int i = 1; i<6; i++)
	{
		switch (i)
		{
		case 1:
			myGLCD.setColor(255, 0, 255);
			break;
		case 2:
			myGLCD.setColor(255, 0, 0);
			break;
		case 3:
			myGLCD.setColor(0, 255, 0);
			break;
		case 4:
			myGLCD.setColor(0, 0, 255);
			break;
		case 5:
			myGLCD.setColor(255, 255, 0);
			break;
		}
		myGLCD.fillCircle(100 + (i * 20), 60 + (i * 20), 30);
	}

	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);

	// Draw some lines in a pattern
	myGLCD.setColor(255, 0, 0);
	for (int i = 15; i<224; i += 5)
	{
		myGLCD.drawLine(1, i, (i*1.44) - 10, 224);
	}
	myGLCD.setColor(255, 0, 0);
	for (int i = 224; i>15; i -= 5)
	{
		myGLCD.drawLine(318, i, (i*1.44) - 11, 15);
	}
	myGLCD.setColor(0, 255, 255);
	for (int i = 224; i>15; i -= 5)
	{
		myGLCD.drawLine(1, i, 331 - (i*1.44), 15);
	}
	myGLCD.setColor(0, 255, 255);
	for (int i = 15; i<224; i += 5)
	{
		myGLCD.drawLine(318, i, 330 - (i*1.44), 224);
	}

	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);

	// Draw some random circles
	for (int i = 0; i<100; i++)
	{
		myGLCD.setColor(random(255), random(255), random(255));
		x = 32 + random(256);
		y = 45 + random(146);
		r = random(30);
		myGLCD.drawCircle(x, y, r);
	}

	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);

	// Draw some random rectangles
	for (int i = 0; i<100; i++)
	{
		myGLCD.setColor(random(255), random(255), random(255));
		x = 2 + random(316);
		y = 16 + random(207);
		x2 = 2 + random(316);
		y2 = 16 + random(207);
		myGLCD.drawRect(x, y, x2, y2);
	}

	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);

	// Draw some random rounded rectangles
	for (int i = 0; i<100; i++)
	{
		myGLCD.setColor(random(255), random(255), random(255));
		x = 2 + random(316);
		y = 16 + random(207);
		x2 = 2 + random(316);
		y2 = 16 + random(207);
		myGLCD.drawRoundRect(x, y, x2, y2);
	}

	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);

	for (int i = 0; i<100; i++)
	{
		myGLCD.setColor(random(255), random(255), random(255));
		x = 2 + random(316);
		y = 16 + random(209);
		x2 = 2 + random(316);
		y2 = 16 + random(209);
		myGLCD.drawLine(x, y, x2, y2);
	}

	delay(2000);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 318, 224);

	for (int i = 0; i<10000; i++)
	{
		myGLCD.setColor(random(255), random(255), random(255));
		myGLCD.drawPixel(2 + random(316), 16 + random(209));
	}

	delay(2000);

	myGLCD.fillScr(0, 0, 255);
	myGLCD.setColor(255, 0, 0);
	myGLCD.fillRoundRect(80, 70, 239, 169);

	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(255, 0, 0);
	myGLCD.print("That's it!", CENTER, 93);
	myGLCD.print("Restarting in a", CENTER, 119);
	myGLCD.print("few seconds...", CENTER, 132);

	myGLCD.setColor(0, 255, 0);
	myGLCD.setBackColor(0, 0, 255);
	myGLCD.print("Runtime: (msecs)", CENTER, 210);
	myGLCD.printNumI(millis(), CENTER, 225);



	*/


}
void clock_print_serial()
{
	rtc_clock.get_time(&hh, &mm, &ss);
	rtc_clock.get_date(&dow, &dd, &mon, &yyyy);

	Serial.print(dd, DEC);
	Serial.print('/');
	Serial.print(mon, DEC);
	Serial.print('/');
	Serial.print(yyyy, DEC);//Serial display time
	Serial.print(' ');
	Serial.print(hh, DEC);
	Serial.print(':');
	Serial.print(mm, DEC);
	Serial.print(':');
	Serial.print(ss, DEC);
	Serial.println();
	Serial.print(" week: ");
	//Serial.print(dow, DEC);
	Serial.print(str[dow-1]);
	Serial.println();
	
}

void drawCrossHair(int x, int y)
{
	myGLCD.drawRect(x - 10, y - 10, x + 10, y + 10);
	myGLCD.drawLine(x - 5, y, x + 5, y);
	myGLCD.drawLine(x, y - 5, x, y + 5);
}
void readCoordinates()
{
	int iter = 2000;
	int cnt = 0;
	uint32_t tx = 0;
	uint32_t ty = 0;
	boolean OK = false;

	while (OK == false)
	{
		myGLCD.setColor(255, 255, 255);
		myGLCD.print("*  PRESS  *", CENTER, text_y_center);
		while (myTouch.dataAvailable() == false) {}
		myGLCD.print("*  HOLD!  *", CENTER, text_y_center);
		while ((myTouch.dataAvailable() == true) && (cnt<iter))
		{
			myTouch.read();
			if (!((myTouch.TP_X == 65535) || (myTouch.TP_Y == 65535)))
			{
				tx += myTouch.TP_X;
				ty += myTouch.TP_Y;
				cnt++;
			}
		}
		if (cnt >= iter)
		{
			OK = true;
		}
		else
		{
			tx = 0;
			ty = 0;
			cnt = 0;
		}
	}

	cx = tx / iter;
	cy = ty / iter;


}
void calibrate(int x, int y, int i)
{
	myGLCD.setColor(255, 255, 255);
	drawCrossHair(x, y);
	myGLCD.setBackColor(255, 0, 0);
	readCoordinates();
	myGLCD.setColor(255, 255, 255);
	myGLCD.print("* RELEASE *", CENTER, text_y_center);
	myGLCD.setColor(80, 80, 80);
	drawCrossHair(x, y);
	rx[i] = cx;
	ry[i] = cy;
	while (myTouch.dataAvailable() == true)
	{
		myTouch.read();
	}
}
void waitForTouch()
{
	while (myTouch.dataAvailable() == true)
	{
		myTouch.read();
	}
	while (myTouch.dataAvailable() == false) {}
	while (myTouch.dataAvailable() == true)
	{
		myTouch.read();
	}
}
void toHex(uint32_t num)
{
	buf[0] = '0';
	buf[1] = 'x';
	buf[10] = 'U';
	buf[11] = 'L';
	buf[12] = 0;
	for (int zz = 9; zz>1; zz--)
	{
		if ((num & 0xF) > 9)
			buf[zz] = (num & 0xF) + 55;
		else
			buf[zz] = (num & 0xF) + 48;
		num = num >> 4;
	}
}
void startup()
{
	myGLCD.setColor(255, 0, 0);
	myGLCD.fillRect(0, 0, dispx - 1, 13);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(255, 0, 0);
	myGLCD.drawLine(0, 14, dispx - 1, 14);
	myGLCD.print("UTouch Calibration", CENTER, 1);
	myGLCD.setBackColor(0, 0, 0);

	if (dispx == 220)
	{
		myGLCD.print("Use a stylus or something", LEFT, 30);
		myGLCD.print("similar to touch as close", LEFT, 42);
		myGLCD.print("to the center of the", LEFT, 54);
		myGLCD.print("highlighted crosshair as", LEFT, 66);
		myGLCD.print("possible. Keep as still as", LEFT, 78);
		myGLCD.print("possible and keep holding", LEFT, 90);
		myGLCD.print("until the highlight is", LEFT, 102);
		myGLCD.print("removed. Repeat for all", LEFT, 114);
		myGLCD.print("crosshairs in sequence.", LEFT, 126);
		myGLCD.print("Touch screen to continue", CENTER, 162);
	}
	else
	{
		myGLCD.print("INSTRUCTIONS", CENTER, 30);
		myGLCD.print("Use a stylus or something similar to", LEFT, 50);
		myGLCD.print("touch as close to the center of the", LEFT, 62);
		myGLCD.print("highlighted crosshair as possible. Keep", LEFT, 74);
		myGLCD.print("as still as possible and keep holding", LEFT, 86);
		myGLCD.print("until the highlight is removed. Repeat", LEFT, 98);
		myGLCD.print("for all crosshairs in sequence.", LEFT, 110);

		myGLCD.print("Further instructions will be displayed", LEFT, 134);
		myGLCD.print("when the calibration is complete.", LEFT, 146);

		myGLCD.print("Do NOT use your finger as a calibration", LEFT, 170);
		myGLCD.print("stylus or the result WILL BE imprecise.", LEFT, 182);

		myGLCD.print("Touch screen to continue", CENTER, 226);
	}

	waitForTouch();
	myGLCD.clrScr();
}
void done()
{
	myGLCD.clrScr();
	myGLCD.setColor(255, 0, 0);
	myGLCD.fillRect(0, 0, dispx - 1, 13);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(255, 0, 0);
	myGLCD.drawLine(0, 14, dispx - 1, 14);
	myGLCD.print("UTouch Calibration", CENTER, 1);
	myGLCD.setBackColor(0, 0, 0);

	if (dispx == 220)
	{
		myGLCD.print("To use the new calibration", LEFT, 30);
		myGLCD.print("settings you must edit the", LEFT, 42);
		myGLCD.setColor(160, 160, 255);
		myGLCD.print("UTouchCD.h", LEFT, 54);
		myGLCD.setColor(255, 255, 255);
		myGLCD.print("file and change", 88, 54);
		myGLCD.print("the following values. The", LEFT, 66);
		myGLCD.print("values are located right", LEFT, 78);
		myGLCD.print("below the opening comment.", LEFT, 90);
		myGLCD.print("CAL_X", LEFT, 110);
		myGLCD.print("CAL_Y", LEFT, 122);
		myGLCD.print("CAL_S", LEFT, 134);
		toHex(calx);
		myGLCD.print(buf, 75, 110);
		toHex(caly);
		myGLCD.print(buf, 75, 122);
		toHex(cals);
		myGLCD.print(buf, 75, 134);
	}
	else
	{
		myGLCD.print("CALIBRATION COMPLETE", CENTER, 30);
		myGLCD.print("To use the new calibration", LEFT, 50);
		myGLCD.print("settings you must edit the", LEFT, 62);
		myGLCD.setColor(160, 160, 255);
		myGLCD.print("UTouchCD.h", LEFT, 74);
		myGLCD.setColor(255, 255, 255);
		myGLCD.print("file and change", 88, 74);
		myGLCD.print("the following values.", LEFT, 86);
		myGLCD.print("The values are located right", LEFT, 98);
		myGLCD.print("below the opening comment in", LEFT, 110);
		myGLCD.print("the file.", LEFT, 122);
		myGLCD.print("CAL_X", LEFT, 150);
		myGLCD.print("CAL_Y", LEFT, 162);
		myGLCD.print("CAL_S", LEFT, 174);

		toHex(calx);
		myGLCD.print(buf, 75, 150);
		toHex(caly);
		myGLCD.print(buf, 75, 162);
		toHex(cals);
		myGLCD.print(buf, 75, 174);
	}

}
void test_Touch()
{
	myTouch.InitTouch(TOUCH_ORIENTATION);
	myTouch.setPrecision(PREC_LOW);
	dispx = myGLCD.getDisplayXSize();
	dispy = myGLCD.getDisplayYSize();
	text_y_center = (dispy / 2) - 6; 

	startup();

	myGLCD.setColor(80, 80, 80);
	drawCrossHair(dispx - 11, 10);
	drawCrossHair(dispx / 2, 10);
	drawCrossHair(10, 10);
	drawCrossHair(dispx - 11, dispy / 2);
	drawCrossHair(10, dispy / 2);
	drawCrossHair(dispx - 11, dispy - 11);
	drawCrossHair(dispx / 2, dispy - 11);
	drawCrossHair(10, dispy - 11);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(255, 0, 0);
	myGLCD.print("***********", CENTER, text_y_center - 12);
	myGLCD.print("***********", CENTER, text_y_center + 12);

	calibrate(10, 10, 0);
	calibrate(10, dispy / 2, 1);
	calibrate(10, dispy - 11, 2);
	calibrate(dispx / 2, 10, 3);
	calibrate(dispx / 2, dispy - 11, 4);
	calibrate(dispx - 11, 10, 5);
	calibrate(dispx - 11, dispy / 2, 6);
	calibrate(dispx - 11, dispy - 11, 7);

	if (TOUCH_ORIENTATION == LANDSCAPE)
		cals = (long(dispx - 1) << 12) + (dispy - 1);
	else
		cals = (long(dispy - 1) << 12) + (dispx - 1);

	if (TOUCH_ORIENTATION == PORTRAIT)
		px = abs(((float(rx[2] + rx[4] + rx[7]) / 3) - (float(rx[0] + rx[3] + rx[5]) / 3)) / (dispy - 20));  // PORTRAIT
	else
		px = abs(((float(rx[5] + rx[6] + rx[7]) / 3) - (float(rx[0] + rx[1] + rx[2]) / 3)) / (dispy - 20));  // LANDSCAPE

	if (TOUCH_ORIENTATION == PORTRAIT)
	{
		clx = (((rx[0] + rx[3] + rx[5]) / 3));  // PORTRAIT
		crx = (((rx[2] + rx[4] + rx[7]) / 3));  // PORTRAIT
	}
	else
	{
		clx = (((rx[0] + rx[1] + rx[2]) / 3));  // LANDSCAPE
		crx = (((rx[5] + rx[6] + rx[7]) / 3));  // LANDSCAPE
	}
	if (clx<crx)
	{
		clx = clx - (px * 10);
		crx = crx + (px * 10);
	}
	else
	{
		clx = clx + (px * 10);
		crx = crx - (px * 10);
	}

	if (TOUCH_ORIENTATION == PORTRAIT)
		py = abs(((float(ry[5] + ry[6] + ry[7]) / 3) - (float(ry[0] + ry[1] + ry[2]) / 3)) / (dispx - 20));  // PORTRAIT
	else
		py = abs(((float(ry[0] + ry[3] + ry[5]) / 3) - (float(ry[2] + ry[4] + ry[7]) / 3)) / (dispx - 20));  // LANDSCAPE

	if (TOUCH_ORIENTATION == PORTRAIT)
	{
		cty = (((ry[5] + ry[6] + ry[7]) / 3));  // PORTRAIT
		cby = (((ry[0] + ry[1] + ry[2]) / 3));  // PORTRAIT
	}
	else
	{
		cty = (((ry[0] + ry[3] + ry[5]) / 3));  // LANDSCAPE
		cby = (((ry[2] + ry[4] + ry[7]) / 3));  // LANDSCAPE
	}
	if (cty<cby)
	{
		cty = cty - (py * 10);
		cby = cby + (py * 10);
	}
	else
	{
		cty = cty + (py * 10);
		cby = cby - (py * 10);
	}

	calx = (long(clx) << 14) + long(crx);
	caly = (long(cty) << 14) + long(cby);
	if (TOUCH_ORIENTATION == LANDSCAPE)
		cals = cals + (1L << 31);

	done();
	while (myTouch.dataAvailable()) {}
	delay(50);
	while (!myTouch.dataAvailable()) {}
	delay(50);

}

void setup()
{
	Serial.begin(115200);
	Serial.print(F("FreeRam: "));
	Serial.println(FreeRam());
	// Setup the LCD
	myGLCD.InitLCD();
	myGLCD.setFont(SmallFont);
	pinMode(9, OUTPUT);
	digitalWrite(9, LOW);
	//test_TFT();

	myTouch.InitTouch(1);
	//myTouch.InitTouch(TOUCH_ORIENTATION);
	myTouch.setPrecision(PREC_MEDIUM);
	//myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);

	// initialize file system.
	if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED))
	{
		sd.initErrorPrint();
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.setColor(255, 100, 0);
		myGLCD.print("Can't access SD card", CENTER, 40);
		myGLCD.print("Do not reformat", CENTER, 70);
		myGLCD.print("SD card problem?", CENTER, 100);
		myGLCD.setColor(VGA_LIME);
		myGLCD.print(txt_info11, CENTER, 200);
		myGLCD.setColor(255, 255, 255);
		while (myTouch.dataAvailable()) {}
		delay(50);
		while (!myTouch.dataAvailable()) {}
		delay(50);
		myGLCD.clrScr();
		myGLCD.print("Run Setup", CENTER, 120);
	}

	rtc_clock.init();
	rtc_clock.set_time(__TIME__);
	rtc_clock.set_date(__DATE__);
	SdFile::dateTimeCallback(dateTime);
	////++++++++++++++++ SD info ++++++++++++++++++++++++++++++

	//// use uppercase in hex and use 0X base prefix
	//cout << uppercase << showbase << endl;
	//// pstr stores strings in flash to save RAM

	cout << pstr("SdFat version: ") << SD_FAT_VERSION << endl;

	clock_print_serial();
	myGLCD.clrScr();
	myGLCD.setFont(SmallFont);


	//test_Touch();
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("Setup Ok!", CENTER, 120);


	Serial.println(F("Setup Ok!"));

}

void loop()
{

	draw_Glav_Menu();
	//AnalogClock();
	swichMenu();
}
