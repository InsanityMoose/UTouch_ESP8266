// UTouch_Calibration 
// Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
// web: http://www.RinkyDinkElectronics.com/
//
// This program can be used to calibrate the touchscreen
// of the display modules.
// This program requires the UTFT library and a touch
// screen module that is compatible with UTFT.
//
// It is assumed that the display module is connected to an
// appropriate shield or that you know how to change the pin 
// numbers in the setup.
//
// Instructions will be given on the display.
//
#include <Adafruit_GFX.h>
#include <SPI.h>
//#include <Wire.h>
#include <ILI9341_ESP8266.h>
#include <UTouch_ESP8266.h>

#define TFT_CS D1
#define TFT_DC D2
#define TFT_RST -1
#define TOUCH_CS D4
#define TOUCH_IRQ D0

// Define the orientation of the touch screen. Further 
// information can be found in the instructions.
#define TOUCH_ORIENTATION  PORTRAIT

// Initialize display
// ------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : <display model>,19,18,17,16
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Due       : <display model>,25,26,27,28
// Teensy 3.x TFT Test Board                   : <display model>,23,22, 3, 4
// ElecHouse TFT LCD/SD Shield for Arduino Due : <display model>,22,23,31,33
//
// Remember to change the model parameter to suit your display module!
Adafruit_ILI9341 myGLCD(TFT_CS, TFT_DC, TFT_RST);

// Initialize touchscreen
// ----------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : 15,10,14, 9, 8
// Standard Arduino Mega/Due shield            :  6, 5, 4, 3, 2
// CTE TFT LCD/SD Shield for Arduino Due       :  6, 5, 4, 3, 2
// Teensy 3.x TFT Test Board                   : 26,31,27,28,29
// ElecHouse TFT LCD/SD Shield for Arduino Due : 25,26,27,29,30
//
UTouch  myTouch(TOUCH_CS, TOUCH_IRQ);

// ************************************
// DO NOT EDIT ANYTHING BELOW THIS LINE
// ************************************

uint32_t cx, cy;
uint32_t rx[8], ry[8];
uint32_t clx, crx, cty, cby;
float px, py;
int dispx, dispy, text_y_center;
uint32_t calx, caly, cals;
char buf[13];

void setup()
{
	Serial.begin(9600);
	pinMode(TFT_CS, OUTPUT);
	digitalWrite(TFT_CS, HIGH);
	pinMode(TOUCH_CS, OUTPUT);
	digitalWrite(TOUCH_CS, HIGH);
	myGLCD.begin();
	myGLCD.setRotation(0);
	myGLCD.fillScreen(ILI9341_BLACK);
	myTouch.InitTouch(TOUCH_ORIENTATION);
	dispx = 240;
	dispy = 320;
	text_y_center = (dispy / 2) - 6;
}

void drawCrossHair(int x, int y, int color)
{
	myGLCD.drawRect(x - 10, y - 10, 20, 20, color);
	myGLCD.drawLine(x - 5, y, x + 5, y, color);
	myGLCD.drawLine(x, y - 5, x, y + 5, color);
}

void readCoordinates()
{
	int iter = 5000;
	int failcount = 0;
	int cnt = 0;
	uint32_t tx = 0;
	uint32_t ty = 0;
	boolean OK = false;

	while (OK == false)
	{
		//myGLCD.setColor(255, 255, 255);
		myGLCD.setTextColor(ILI9341_BLACK);
		myGLCD.setCursor(100, 50);
		myGLCD.print("*  HOLD!  *");
		myGLCD.setTextColor(ILI9341_WHITE);
		myGLCD.setCursor(100, 50);
		myGLCD.print("*  PRESS  *");
		while (myTouch.dataAvailable() == false) { yield(); }
		myGLCD.setTextColor(ILI9341_BLACK);
		myGLCD.setCursor(100, 50);
		myGLCD.print("*  PRESS  *");
		myGLCD.setTextColor(ILI9341_WHITE);
		myGLCD.setCursor(100, 50);
		myGLCD.print("*  HOLD!  *");
		while ((myTouch.dataAvailable() == true) && (cnt<iter) && (failcount<10000))
		{
			yield();
			myTouch.calibrateRead();
			if (!((myTouch.TP_X == 65535) || (myTouch.TP_Y == 65535)))
			{
				tx += myTouch.TP_X;
				ty += myTouch.TP_Y;
				cnt++;
			}
			else
				failcount++;
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
		if (failcount >= 10000)
			fail();
	}
	cx = tx / iter;
	cy = ty / iter;
}

void calibrate(int x, int y, int i)
{
	drawCrossHair(x, y, myGLCD.color565(255, 255, 255));
	readCoordinates();
	myGLCD.setTextColor(ILI9341_BLACK);
	myGLCD.setCursor(100, 50);
	myGLCD.print("*  HOLD!  *");
	myGLCD.setTextColor(ILI9341_WHITE);
	myGLCD.setCursor(100, 50);
	myGLCD.println("* RELEASE *");
	drawCrossHair(x, y, myGLCD.color565(80, 80, 80));
	rx[i] = cx;
	ry[i] = cy;
	while (myTouch.dataAvailable() == true) { yield(); }
	myGLCD.setTextColor(ILI9341_BLACK);
	myGLCD.setCursor(100, 50);
	myGLCD.println("* RELEASE *");
}

void waitForTouch()
{
	while (myTouch.dataAvailable() == true) { yield(); }
	while (myTouch.dataAvailable() == false) { yield(); }
	while (myTouch.dataAvailable() == true) { yield(); }
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
	myGLCD.fillRect(0, 0, dispx - 1, 13, myGLCD.color565(255, 0, 0));
	myGLCD.drawLine(0, 14, dispx - 1, 14, myGLCD.color565(255, 255, 255));
	myGLCD.setCursor(5, 5);
	myGLCD.println("UTouch Calibration");

	if (dispx == 220)
	{
		myGLCD.println("");
		myGLCD.println("");
		myGLCD.print("Use a stylus or something");
		myGLCD.print("similar to touch as close");
		myGLCD.print("to the center of the");
		myGLCD.print("highlighted crosshair as");
		myGLCD.print("possible. Keep as still as");
		myGLCD.print("possible and keep holding");
		myGLCD.print("until the highlight is");
		myGLCD.print("removed. Repeat for all");
		myGLCD.print("crosshairs in sequence.");
		myGLCD.print("Touch screen to continue");
	}
	else
	{
		myGLCD.println("");
		myGLCD.println("");
		myGLCD.println("INSTRUCTIONS");
		myGLCD.println("Use a stylus or something similar to");
		myGLCD.println("touch as close to the center of the");
		myGLCD.println("highlighted crosshair as possible. Keep");
		myGLCD.println("as still as possible and keep holding");
		myGLCD.println("until the highlight is removed. Repeat");
		myGLCD.println("for all crosshairs in sequence.");
		myGLCD.println("");
		myGLCD.println("Further instructions will be displayed");
		myGLCD.println("when the calibration is complete.");
		myGLCD.println("");
		myGLCD.println("Do NOT use your finger as a calibration");
		myGLCD.println("stylus or the result WILL BE imprecise.");
		myGLCD.println("");
		myGLCD.println("Touch screen to continue");
	}

	waitForTouch();
	myGLCD.fillScreen(myGLCD.color565(0, 0, 0));
}

void done()
{
	myGLCD.fillScreen(myGLCD.color565(0, 0, 0));
	myGLCD.fillRect(0, 0, dispx - 1, 13, myGLCD.color565(255, 0, 0));
	myGLCD.drawLine(0, 14, dispx - 1, 14, myGLCD.color565(255, 255, 255));
	myGLCD.setCursor(5, 5);
	myGLCD.setTextColor(myGLCD.color565(255, 255, 255));
	myGLCD.print("UTouch Calibration");

	if (dispx == 220)
	{
		myGLCD.println("");
		myGLCD.println("");
		myGLCD.println("To use the new calibration");
		myGLCD.println("settings you must edit the");
		myGLCD.setTextColor(myGLCD.color565(160, 160, 255));
		myGLCD.println("UTouchCD.h");
		myGLCD.setTextColor(myGLCD.color565(255, 255, 255));
		myGLCD.println("file and change");
		myGLCD.println("the following values. The");
		myGLCD.println("values are located right");
		myGLCD.println("below the opening comment.");
		myGLCD.println("CAL_X");
		myGLCD.println("CAL_Y");
		myGLCD.println("CAL_S");
		toHex(calx);
		myGLCD.println(buf);
		toHex(caly);
		myGLCD.println(buf);
		toHex(cals);
		myGLCD.println(buf);
	}
	else
	{
		myGLCD.println("");
		myGLCD.println("");
		myGLCD.println("CALIBRATION COMPLETE");
		myGLCD.println("To use the new calibration");
		myGLCD.println("settings you must edit the");
		myGLCD.setTextColor(myGLCD.color565(160, 160, 255));
		myGLCD.println("UTouchCD.h");
		myGLCD.setTextColor(myGLCD.color565(255, 255, 255));
		myGLCD.println("file and change");
		myGLCD.println("the following values.");
		myGLCD.println("The values are located right");
		myGLCD.println("below the opening comment in");
		myGLCD.println("the file.");
		myGLCD.println("CAL_X");
		myGLCD.println("CAL_Y");
		myGLCD.println("CAL_S");
		Serial.println("CAL_X");
		Serial.println("CAL_Y");
		Serial.println("CAL_S");

		toHex(calx);
		myGLCD.println(buf);
		Serial.println(buf);
		toHex(caly);
		myGLCD.println(buf);
		Serial.println(buf);
		toHex(cals);
		myGLCD.println(buf);
		Serial.println(buf);
	}

}

void fail()
{
	myGLCD.fillScreen(myGLCD.color565(0, 0, 0));
	myGLCD.fillRect(0, 0, dispx - 1, 13, myGLCD.color565(255, 0, 0));
	myGLCD.drawLine(0, 14, dispx - 1, 14, myGLCD.color565(255, 0, 0));
	myGLCD.setCursor(5, 5);
	myGLCD.println("UTouch Calibration FAILED");
	myGLCD.println("");
	myGLCD.println("Unable to read the position");
	myGLCD.println("of the press. This is a");
	myGLCD.println("hardware issue and can");
	myGLCD.println("not be corrected in");
	myGLCD.println("software.");

	while (true) { yield(); };
}

void loop()
{
	startup();

	drawCrossHair(dispx - 11, 10, myGLCD.color565(80, 80, 80));
	drawCrossHair(dispx / 2, 10, myGLCD.color565(80, 80, 80));
	drawCrossHair(10, 10, myGLCD.color565(80, 80, 80));
	drawCrossHair(dispx - 11, dispy / 2, myGLCD.color565(80, 80, 80));
	drawCrossHair(10, dispy / 2, myGLCD.color565(80, 80, 80));
	drawCrossHair(dispx - 11, dispy - 11, myGLCD.color565(80, 80, 80));
	drawCrossHair(dispx / 2, dispy - 11, myGLCD.color565(80, 80, 80));
	drawCrossHair(10, dispy - 11, myGLCD.color565(80, 80, 80));
	myGLCD.setTextColor(myGLCD.color565(255, 255, 255));
	myGLCD.setCursor(100, 40);
	myGLCD.println("***********");
	myGLCD.setCursor(100, 60);
	myGLCD.println("***********");

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
	while (true) { yield(); }
}
