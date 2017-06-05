

#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_ILI9341.h>
#include <UTouch.h>


#define TFT_RST  8
#define TFT_DC 9
#define TFT_CS 10
#define TFT_MOSI MOSI
#define TFT_MISO MISO
#define TFT_CLK  SCK
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);




// Define the orientation of the touch screen. Further 
// information can be found in the instructions.
#define TOUCH_ORIENTATION  PORTRAIT

// Declare which fonts we will be using
//extern uint8_t SmallFont[];


UTouch      myTouch(7,6,5,4,3);


#define Serial SERIAL_PORT_USBVIRTUAL




// ************************************
// DO NOT EDIT ANYTHING BELOW THIS LINE
// ************************************
uint32_t cx, cy;
uint32_t rx[10], ry[10];
uint32_t clx, crx, cty, cby;
float px, py;
int dispx, dispy, text_y_center;
uint32_t calx, caly, cals;
char buf[13];

void setup()
{

	Serial.begin(9600);
	delay(3000);
//	while (!Serial);
	Serial.println(F("Touch Paint!"));

	tft.begin();

  myTouch.InitTouch(TOUCH_ORIENTATION);
  myTouch.setPrecision(PREC_LOW);
  dispx = 240;
  dispy = 320;
  tft.fillScreen(ILI9341_BLACK);

 /* dispx=myGLCD.getDisplayXSize();
  dispy=myGLCD.getDisplayYSize();*/
  text_y_center=(dispy/2)-6;
 // drawCrossHair(120, 160);
}

void drawCrossHair(int x, int y)
{
	tft.drawRect(x-10, y-10, 20, 20, ILI9341_WHITE);
	tft.drawLine(x-5, y, x+5, y, ILI9341_WHITE);
	tft.drawLine(x, y-5, x, y+5, ILI9341_WHITE);
}
void drawCrossHair1(int x, int y)
{
	tft.drawRect(x - 10, y - 10, 20, 20, ILI9341_RED);
	tft.drawLine(x - 5, y, x + 5, y, ILI9341_RED);
	tft.drawLine(x, y - 5, x, y + 5, ILI9341_RED);
}

void readCoordinates()
{
  int iter = 2000;
  int cnt = 0;
  uint32_t tx=0;
  uint32_t ty=0;
  boolean OK = false;
  
  while (OK == false)
  {
	//tft.fillScreen(ILI9341_BLACK);

	tft.setCursor(120, 160);
	tft.fillRect(100, 150, 20, 100, ILI9341_BLACK);
	tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
    tft.print("*  PRESS  *");
    while (myTouch.dataAvailable() == false) {}
	tft.setCursor(120, 160);
	tft.fillRect(100,150, 20,100, ILI9341_BLACK);
	tft.print("*  HOLD!  *");
    while ((myTouch.dataAvailable() == true) && (cnt<iter))
    {
      myTouch.read();
      if (!((myTouch.TP_X==65535) || (myTouch.TP_Y==65535)))
      {
        tx += myTouch.TP_X;
        ty += myTouch.TP_Y;
        cnt++;
      }
    }
    if (cnt>=iter)
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

  drawCrossHair1(x,y);
  readCoordinates();

  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.setCursor(90, 100);
  tft.print("* RELEASE *");
 // myGLCD.setColor(80, 80, 80);
  
  drawCrossHair(x,y);
  rx[i]=cx;
  ry[i]=cy;
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
  for (int zz=9; zz>1; zz--)
  {
    if ((num & 0xF) > 9)
      buf[zz] = (num & 0xF) + 55;
    else
      buf[zz] = (num & 0xF) + 48;
    num=num>>4;
  }
}

void startup()
{

  tft.fillRect(0, 0, dispx-1, 13, ILI9341_YELLOW);

 tft.drawLine(0, 14, dispx-1, 14, ILI9341_GREEN);
 tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
 tft.setCursor(40, 80);
  tft.print("UTouch Calibration");
 tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
 tft.setCursor(10, 100);
 
 tft.print("Touch screen to continue");

  waitForTouch();
  tft.fillScreen(ILI9341_BLACK);
}

void done()
{
  //myGLCD.clrScr();
  //myGLCD.setColor(255, 0, 0);
  //myGLCD.fillRect(0, 0, dispx-1, 13);
  //myGLCD.setColor(255, 255, 255);
  //myGLCD.setBackColor(255, 0, 0);
  //myGLCD.drawLine(0, 14, dispx-1, 14);
  //myGLCD.print("UTouch Calibration", CENTER, 1);
  //myGLCD.setBackColor(0, 0, 0);
  /*
 
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
	*/
	tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
	tft.setCursor(10, 180);
    tft.print("CAL_X");
	tft.setCursor(10, 200);
    tft.print("CAL_Y");
	tft.setCursor(10, 220);
    tft.print("CAL_S");
	

	tft.setCursor(75, 180);
    toHex(calx);
    tft.print(buf);
	tft.setCursor(75, 200);
    toHex(caly);
    tft.print(buf); 
	tft.setCursor(75, 220);
    toHex(cals);
    tft.print(buf);

}

void loop()
{
	
  startup();
  
 // myGLCD.setColor(80, 80, 80);
  drawCrossHair(dispx-11, 10);
  drawCrossHair(dispx/2, 10);
  drawCrossHair(10, 10);
  drawCrossHair(dispx-11, dispy/2);
  drawCrossHair(10, dispy/2);
  drawCrossHair(dispx-11, dispy-11);
  drawCrossHair(dispx/2, dispy-11);
  drawCrossHair(10, dispy-11);
 /* myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("***********", CENTER, text_y_center-12);
  myGLCD.print("***********", CENTER, text_y_center+12);
*/
  calibrate(10, 10, 0);
  calibrate(10, dispy/2, 1);
  calibrate(10, dispy-11, 2);
  calibrate(dispx/2, 10, 3);
  calibrate(dispx/2, dispy-11, 4);
  calibrate(dispx-11, 10, 5);
  calibrate(dispx-11, dispy/2, 6);
  calibrate(dispx-11, dispy-11, 7);
  
  if (TOUCH_ORIENTATION == LANDSCAPE)
    cals=(long(dispx-1)<<12)+(dispy-1);
  else
    cals=(long(dispy-1)<<12)+(dispx-1);

  if (TOUCH_ORIENTATION == PORTRAIT)
    px = abs(((float(rx[2]+rx[4]+rx[7])/3)-(float(rx[0]+rx[3]+rx[5])/3))/(dispy-20));  // PORTRAIT
  else
    px = abs(((float(rx[5]+rx[6]+rx[7])/3)-(float(rx[0]+rx[1]+rx[2])/3))/(dispy-20));  // LANDSCAPE

  if (TOUCH_ORIENTATION == PORTRAIT)
  {
    clx = (((rx[0]+rx[3]+rx[5])/3));  // PORTRAIT
    crx = (((rx[2]+rx[4]+rx[7])/3));  // PORTRAIT
  }
  else
  {
    clx = (((rx[0]+rx[1]+rx[2])/3));  // LANDSCAPE
    crx = (((rx[5]+rx[6]+rx[7])/3));  // LANDSCAPE
  }
  if (clx<crx)
  {
    clx = clx - (px*10);
    crx = crx + (px*10);
  }
  else
  {
    clx = clx + (px*10);
    crx = crx - (px*10);
  }
  
  if (TOUCH_ORIENTATION == PORTRAIT)
    py = abs(((float(ry[5]+ry[6]+ry[7])/3)-(float(ry[0]+ry[1]+ry[2])/3))/(dispx-20));  // PORTRAIT
  else
    py = abs(((float(ry[0]+ry[3]+ry[5])/3)-(float(ry[2]+ry[4]+ry[7])/3))/(dispx-20));  // LANDSCAPE

  if (TOUCH_ORIENTATION == PORTRAIT)
  {
    cty = (((ry[5]+ry[6]+ry[7])/3));  // PORTRAIT
    cby = (((ry[0]+ry[1]+ry[2])/3));  // PORTRAIT
  }
  else
  {
    cty = (((ry[0]+ry[3]+ry[5])/3));  // LANDSCAPE
    cby = (((ry[2]+ry[4]+ry[7])/3));  // LANDSCAPE
  }
  if (cty<cby)
  {
    cty = cty - (py*10);
    cby = cby + (py*10);
  }
  else
  {
    cty = cty + (py*10);
    cby = cby - (py*10);
  }
  
  calx = (long(clx)<<14) + long(crx);
  caly = (long(cty)<<14) + long(cby);
  if (TOUCH_ORIENTATION == LANDSCAPE)
    cals = cals + (1L<<31);

  done();
  
  while(true) {}
}
