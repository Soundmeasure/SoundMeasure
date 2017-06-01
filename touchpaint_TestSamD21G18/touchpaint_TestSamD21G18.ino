/***************************************************
  This is our touchscreen painting example for the Adafruit ILI9341 Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_ILI9341.h>
#include <UTouch.h>

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// The STMPE610 uses hardware SPI on the shield, and #8
//#define STMPE_CS 8
//Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);


// For the Adafruit shield, these are the default.
#define TFT_RST  8
#define TFT_DC 9
#define TFT_CS 10

//#define TFT_MOSI 51
//#define TFT_CLK 52

#define TFT_MOSI MOSI
#define TFT_MISO MISO
#define TFT_CLK  SCK

//#define TFT_MISO 50
// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
// If using the breakout, change pins as desired
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);


  #define Serial SERIAL_PORT_USBVIRTUAL

// Size of the color selection boxes and the paintbrush size
#define BOXSIZE 40
#define PENRADIUS 3
int oldcolor, currentcolor;
UTouch      myTouch(6,5,4,3,2);

int x, y;
char stCurrent[20]="";
int stCurrentLen=0;
char stLast[20]="";

void setup(void) {
 // while (!Serial);     // used for leonardo debugging
 
  Serial.begin(9600);
   while (!Serial);
  Serial.println(F("Touch Paint!"));
  
  tft.begin();
//
//  if (!ts.begin()) {
//    Serial.println("Couldn't start touchscreen controller");
//    while (1);
//  }
//  Serial.println("Touchscreen started");
  
  tft.fillScreen(ILI9341_BLACK);
  
  // make the color selection boxes

  for (x=0; x<5; x++)
  {
  //  tft.setColor(0, 0, 255);
    tft.fillRect(5+((x*BOXSIZE)+(5*x)), 5, BOXSIZE, BOXSIZE, ILI9341_BLUE);
    tft.drawRect(5+((x*BOXSIZE)+(5*x)), 5, BOXSIZE, BOXSIZE, ILI9341_WHITE);
   // Serial.println(5+((x*BOXSIZE)+(5*x)));
	tft.fillRect(5 + ((x*BOXSIZE) + (5 * x)), 280, BOXSIZE, BOXSIZE, ILI9341_BLUE);
	tft.drawRect(5 + ((x*BOXSIZE) + (5 * x)), 280, BOXSIZE, BOXSIZE, ILI9341_WHITE);

    
  //  tft.setColor(255, 255, 255);
    //tft.drawRoundRect (10+(x*60), 10, 60+(x*60), 60);
   // tft.printNumI(x+1, 27+(x*60), 27);
  }

  
//  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);
//  tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_YELLOW);
//  tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, ILI9341_GREEN);
//  tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, ILI9341_CYAN);
//  tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, ILI9341_BLUE);
//  tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, ILI9341_MAGENTA);
// 
//  // select the current color 'red'
//  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
  currentcolor = ILI9341_RED;
  
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
}

void waitForIt(int x1, int y1, int x2, int y2)
{
//  myGLCD.setColor(255, 0, 0);
  tft.drawRect(x1, y1, x2, y2,ILI9341_RED);
  while (myTouch.dataAvailable())
    myTouch.read();
//  myGLCD.setColor(255, 255, 255);
tft.drawRect(x1, y1, x2, y2, ILI9341_WHITE);
}


void loop()
{

  while (true)
  {
    if (myTouch.dataAvailable())
    {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();

      if ((y>=5) && (y<=45))  // Upper row
      {
        if ((x>=5) && (x<=45))  // Button: 1
        {
          waitForIt(5, 5, 40, 40);
          Serial.println('1');
        }
        if ((x>=50) && (x<=90))  // Button: 2
        {
          waitForIt(50, 5, 40, 40);
          Serial.println('2');
        }
        if ((x>=95) && (x<=135))  // Button: 3
        {
          waitForIt(95, 5, 40, 40);
          Serial.println('3');
        }
        if ((x>=140) && (x<=180))  // Button: 4
        {
          waitForIt(140, 5, 40, 40);
           Serial.println('4');
        }
        if ((x>=185) && (x<=235))  // Button: 5
        {
          waitForIt(185, 5, 40, 40);
           Serial.println('5');
        }
      }

	  if ((y >= 280) && (y <= 319))  // Upper row
	  {
		  if ((x >= 5) && (x <= 45))  // Button: 1
		  {
			  waitForIt(5, 280, 40, 40);
			  Serial.println('6');
		  }
		  if ((x >= 50) && (x <= 90))  // Button: 2
		  {
			  waitForIt(50, 280, 40, 40);
			  Serial.println('7');
		  }
		  if ((x >= 95) && (x <= 135))  // Button: 3
		  {
			  waitForIt(95, 280, 40, 40);
			  Serial.println('8');
		  }
		  if ((x >= 140) && (x <= 180))  // Button: 4
		  {
			  waitForIt(140, 280, 40, 40);
			  Serial.println('9');
		  }
		  if ((x >= 185) && (x <= 235))  // Button: 5
		  {
			  waitForIt(185, 280, 40, 40);
			  Serial.println('0');
		  }
	  }




      

//Serial.println("Touch Ok");
      
    }
  }




  
  /*
  // See if there's any  touch data for us
  if (ts.bufferEmpty()) {
    return;
  }
  */
  /*
  // You can also wait for a touch
  if (! ts.touched()) {
    return;
  }
  */

  // Retrieve a point  
 // TS_Point p = ts.getPoint();
  
 /*
  Serial.print("X = "); Serial.print(p.x);
  Serial.print("\tY = "); Serial.print(p.y);
  Serial.print("\tPressure = "); Serial.println(p.z);  
 */
 
  // Scale from ~0->4000 to tft.width using the calibration #'s
 // p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
//  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  /*
  Serial.print("("); Serial.print(p.x);
  Serial.print(", "); Serial.print(p.y);
  Serial.println(")");
  */
/*
  if (p.y < BOXSIZE) {
     oldcolor = currentcolor;

     if (p.x < BOXSIZE) { 
       currentcolor = ILI9341_RED; 
       tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
     } else if (p.x < BOXSIZE*2) {
       currentcolor = ILI9341_YELLOW;
       tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
     } else if (p.x < BOXSIZE*3) {
       currentcolor = ILI9341_GREEN;
       tft.drawRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
     } else if (p.x < BOXSIZE*4) {
       currentcolor = ILI9341_CYAN;
       tft.drawRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
     } else if (p.x < BOXSIZE*5) {
       currentcolor = ILI9341_BLUE;
       tft.drawRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
     } else if (p.x < BOXSIZE*6) {
       currentcolor = ILI9341_MAGENTA;
       tft.drawRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
     }

     if (oldcolor != currentcolor) {
        if (oldcolor == ILI9341_RED) 
          tft.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);
        if (oldcolor == ILI9341_YELLOW) 
          tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_YELLOW);
        if (oldcolor == ILI9341_GREEN) 
          tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, ILI9341_GREEN);
        if (oldcolor == ILI9341_CYAN) 
          tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, ILI9341_CYAN);
        if (oldcolor == ILI9341_BLUE) 
          tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, ILI9341_BLUE);
        if (oldcolor == ILI9341_MAGENTA) 
          tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, ILI9341_MAGENTA);
     }
  }
  if (((p.y-PENRADIUS) > BOXSIZE) && ((p.y+PENRADIUS) < tft.height())) {
    tft.fillCircle(p.x, p.y, PENRADIUS, currentcolor);
  }
  */
}
