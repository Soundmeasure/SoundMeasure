// UTFT_Analog_Clock (C)2010-2012 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program was made simply because I was bored.
//
// Hardware requirements:
//  - Arduino Mega
//  - Display module supported by UTFT
//  - DS1307 RTC
//  - Some method of connecting the display module to the Arduino
//    like the old ITDB02 Mega Shield v1.1 as it already contains
//    the DS1307 or the All in One Super Screw Shield from
//    AquaLEDSource which also has everything you need on the
//    shield.
//
// Software requirements:
//  - UTFT library    http://www.henningkarlsen.com/electronics/library.php?id=51
//  - UTouch library  http://www.henningkarlsen.com/electronics/library.php?id=55
//  - DS1307 library  http://www.henningkarlsen.com/electronics/library.php?id=34
//
// This program should work on all the supported display modules
// from the 2.4" ones and up to the 5.0" ones, but keep in mind 
// that this program was made to work on a 320x240 module, so it 
// may look a little silly on larger displays.
// It will not work on the 2.2" modules without modification as 
// the resolution on those modules are too low.
//
// Default serial speed for this sketch is 115200 baud.
//
// You can buy the AquaLEDSource All in One Super Screw Shield here:
// English:    http://www.aqualedsource.com/prods/screwshield.php
// Portuguese: http://www.aqualed-light.com/?sec=home&id=1818
//
/**************************************************************************/

// Enable (1) or disable (0) VT100 terminal mode
// Enable this if your terminal program supports VT100 control sequences.
// The Serial Monitor in the Arduino IDE does not support VT100 control sequences. 
// If using the Serial Monitor the line ending should be set to 'No line ending'.
#define VT100_MODE  1

// Enable or disable the use of the AquaLEDSource All in One Super Screw Shield
// Uncomment the following line if you are using this shield
//#define AQUALED_SHIELD 1

#include <UTFT.h>
#include <DS1307.h>
#include <UTouch.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];

UTFT    myGLCD(ITDB32S,38,39,40,41);   // Remember to change the model parameter to suit your display module!
#ifndef AQUALED_SHIELD
  UTouch  myTouch(6,5,4,3,2);
#else
  UTouch  myTouch(62,63,64,65,66);
#endif

// Init the DS1307
DS1307 rtc(20, 21);

// Init a Time-data structure
Time  t;

int clockCenterX=119;
int clockCenterY=119;
int oldsec=0;

void setup()
{
  myGLCD.InitLCD();
  myGLCD.setFont(BigFont);

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_HI);

  // Set the clock to run-mode
  rtc.halt(false);
  
  Serial.begin(115200);
  Serial.println("Send any character to enter serial mode...");
  Serial.println();
}

void drawDisplay()
{
  // Clear screen
  myGLCD.clrScr();
  
  // Draw Clockface
  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(0, 0, 0);
  for (int i=0; i<5; i++)
  {
    myGLCD.drawCircle(clockCenterX, clockCenterY, 119-i);
  }
  for (int i=0; i<5; i++)
  {
    myGLCD.drawCircle(clockCenterX, clockCenterY, i);
  }
  
  myGLCD.setColor(192, 192, 255);
  myGLCD.print("3", clockCenterX+92, clockCenterY-8);
  myGLCD.print("6", clockCenterX-8, clockCenterY+95);
  myGLCD.print("9", clockCenterX-109, clockCenterY-8);
  myGLCD.print("12", clockCenterX-16, clockCenterY-109);
  for (int i=0; i<12; i++)
  {
    if ((i % 3)!=0)
      drawMark(i);
  }  
  t = rtc.getTime();
  drawMin(t.min);
  drawHour(t.hour, t.min);
  drawSec(t.sec);
  oldsec=t.sec;

  // Draw calendar
  myGLCD.setColor(255, 255, 255);
  myGLCD.fillRoundRect(240, 0, 319, 85);
  myGLCD.setColor(0, 0, 0);
  for (int i=0; i<7; i++)
  {
    myGLCD.drawLine(249+(i*10), 0, 248+(i*10), 3);
    myGLCD.drawLine(250+(i*10), 0, 249+(i*10), 3);
    myGLCD.drawLine(251+(i*10), 0, 250+(i*10), 3);
  }

  // Draw SET button
  myGLCD.setColor(64, 64, 128);
  myGLCD.fillRoundRect(260, 200, 319, 239);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(260, 200, 319, 239);
  myGLCD.setBackColor(64, 64, 128);
  myGLCD.print("SET", 266, 212);
  myGLCD.setBackColor(0, 0, 0);
}

void drawMark(int h)
{
  float x1, y1, x2, y2;
  
  h=h*30;
  h=h+270;
  
  x1=110*cos(h*0.0175);
  y1=110*sin(h*0.0175);
  x2=100*cos(h*0.0175);
  y2=100*sin(h*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x2+clockCenterX, y2+clockCenterY);
}

void drawSec(int s)
{
  float x1, y1, x2, y2;
  int ps = s-1;
  
  myGLCD.setColor(0, 0, 0);
  if (ps==-1)
    ps=59;
  ps=ps*6;
  ps=ps+270;
  
  x1=95*cos(ps*0.0175);
  y1=95*sin(ps*0.0175);
  x2=80*cos(ps*0.0175);
  y2=80*sin(ps*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x2+clockCenterX, y2+clockCenterY);

  myGLCD.setColor(255, 0, 0);
  s=s*6;
  s=s+270;
  
  x1=95*cos(s*0.0175);
  y1=95*sin(s*0.0175);
  x2=80*cos(s*0.0175);
  y2=80*sin(s*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x2+clockCenterX, y2+clockCenterY);
}

void drawMin(int m)
{
  float x1, y1, x2, y2, x3, y3, x4, y4;
  int pm = m-1;
  
  myGLCD.setColor(0, 0, 0);
  if (pm==-1)
    pm=59;
  pm=pm*6;
  pm=pm+270;
  
  x1=80*cos(pm*0.0175);
  y1=80*sin(pm*0.0175);
  x2=5*cos(pm*0.0175);
  y2=5*sin(pm*0.0175);
  x3=30*cos((pm+4)*0.0175);
  y3=30*sin((pm+4)*0.0175);
  x4=30*cos((pm-4)*0.0175);
  y4=30*sin((pm-4)*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  myGLCD.drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  myGLCD.drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  myGLCD.drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);

  myGLCD.setColor(0, 255, 0);
  m=m*6;
  m=m+270;
  
  x1=80*cos(m*0.0175);
  y1=80*sin(m*0.0175);
  x2=5*cos(m*0.0175);
  y2=5*sin(m*0.0175);
  x3=30*cos((m+4)*0.0175);
  y3=30*sin((m+4)*0.0175);
  x4=30*cos((m-4)*0.0175);
  y4=30*sin((m-4)*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  myGLCD.drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  myGLCD.drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  myGLCD.drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);
}

void drawHour(int h, int m)
{
  float x1, y1, x2, y2, x3, y3, x4, y4;
  int ph = h;
  
  myGLCD.setColor(0, 0, 0);
  if (m==0)
  {
    ph=((ph-1)*30)+((m+59)/2);
  }
  else
  {
    ph=(ph*30)+((m-1)/2);
  }
  ph=ph+270;
  
  x1=60*cos(ph*0.0175);
  y1=60*sin(ph*0.0175);
  x2=5*cos(ph*0.0175);
  y2=5*sin(ph*0.0175);
  x3=20*cos((ph+5)*0.0175);
  y3=20*sin((ph+5)*0.0175);
  x4=20*cos((ph-5)*0.0175);
  y4=20*sin((ph-5)*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  myGLCD.drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  myGLCD.drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  myGLCD.drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);

  myGLCD.setColor(255, 255, 0);
  h=(h*30)+(m/2);
  h=h+270;
  
  x1=60*cos(h*0.0175);
  y1=60*sin(h*0.0175);
  x2=5*cos(h*0.0175);
  y2=5*sin(h*0.0175);
  x3=20*cos((h+5)*0.0175);
  y3=20*sin((h+5)*0.0175);
  x4=20*cos((h-5)*0.0175);
  y4=20*sin((h-5)*0.0175);
  
  myGLCD.drawLine(x1+clockCenterX, y1+clockCenterY, x3+clockCenterX, y3+clockCenterY);
  myGLCD.drawLine(x3+clockCenterX, y3+clockCenterY, x2+clockCenterX, y2+clockCenterY);
  myGLCD.drawLine(x2+clockCenterX, y2+clockCenterY, x4+clockCenterX, y4+clockCenterY);
  myGLCD.drawLine(x4+clockCenterX, y4+clockCenterY, x1+clockCenterX, y1+clockCenterY);
}

void printDate()
{
  Time t_temp;
  
  t_temp = rtc.getTime();
  myGLCD.setFont(BigFont);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(255, 255, 255);
  myGLCD.print(rtc.getDOWStr(FORMAT_SHORT), 256, 8);
  if (t_temp.date<10)
    myGLCD.printNumI(t_temp.date, 272, 28);
  else
    myGLCD.printNumI(t_temp.date, 264, 28);
  myGLCD.print(rtc.getMonthStr(FORMAT_SHORT), 256, 48);
  myGLCD.printNumI(t_temp.year, 248, 65);
}

void clearDate()
{
  myGLCD.setColor(255, 255, 255);
  myGLCD.fillRect(248, 8, 312, 81);
}

void loop()
{
  int x, y;
  
  t = rtc.getTime();
  if (t.date==0)
  {
    setClock();
  }
  else
  {
    drawDisplay();
    printDate();
  }
  
  t = rtc.getTime();
  
  while (true)
  {
    if (Serial.available()>0)
      serialMode();
    
    if (oldsec!=t.sec)
    {
      if ((t.sec==0) and (t.min==0) and (t.hour==0))
      {
        clearDate();
        printDate();
      }
      if (t.sec==0)
      {
        drawMin(t.min);
        drawHour(t.hour, t.min);
      }
      drawSec(t.sec);
      oldsec=t.sec;
    }

    if (myTouch.dataAvailable())
    {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      if (((y>=200) && (y<=239)) && ((x>=260) && (x<=319)))
      {
        myGLCD.setColor (255, 0, 0);
        myGLCD.drawRoundRect(260, 200, 319, 239);
        setClock();
      }
    }
    
    delay(10);
    t = rtc.getTime();
  }
}

