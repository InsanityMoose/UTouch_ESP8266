
/*
UTouch.cpp - Arduino/chipKit library support for Color TFT LCD Touch screens
Copyright (C)2010-2012 Henning Karlsen. All right reserved

* Modified by Richard Bushill to use hardware SPI instead of bit bang
* 06/04/2015

Basic functionality of this library are based on the demo-code provided by
ITead studio. You can find the latest version of the library at
http://www.henningkarlsen.com/electronics

If you make any modifications or improvements to the code, I would appreciate
that you share the code with me so that I might include it in the next release.
I can be contacted through http://www.henningkarlsen.com/electronics/contact.php

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <SPI.h>
#include "UTouch_ESP8266.h"
#include "UTouchCD_ESP8266.h"

UTouch::UTouch(byte tcs, byte irq)
{
	TOUCH_CS = tcs;
	T_IRQ = irq;
}

void UTouch::InitTouch(byte orientation)
{
	orient = orientation;
	_default_orientation = CAL_S >> 31;
	touch_x_left = (CAL_X >> 14) & 0x3FFF;
	touch_x_right = CAL_X & 0x3FFF;
	touch_y_top = (CAL_Y >> 14) & 0x3FFF;
	touch_y_bottom = CAL_Y & 0x3FFF;
	disp_x_size = (CAL_S >> 12) & 0x0FFF;
	disp_y_size = CAL_S & 0x0FFF;
	prec = 10;

	_csMask = digitalPinToBitMask(TOUCH_CS);
	pinMode(TOUCH_CS, OUTPUT);
	pinMode(T_IRQ, OUTPUT);
	digitalWrite(TOUCH_CS, HIGH);
	SPI.begin();
}


//combined the write data method into the read seeing as HW SPI does both at the same time - RB 06/04/15
word UTouch::touch_ReadData(byte command)
{
	//first send the command, this will not return anything useful
	//so disregard the return byte
	digitalWrite(TOUCH_CS, LOW);

	SPI.transfer(command);

	//next we need to send 12 bits of nothing to get the 12 bits we need back
	//we can only send in batches of 8, ie 1 byte so do this twice
	byte f1 = SPI.transfer(0x00);
	byte f2 = SPI.transfer(0x00);

	digitalWrite(TOUCH_CS, HIGH);

	//combine the 16 bits into a word
	word w = word(f1, f2);

	//the word is organised with MSB leftmost, shit right by 3 to make 12 bits
	w = w >> 3;

	//and return the result
	return w;
}

void UTouch::read()
{
	unsigned long tx = 0, temp_x = 0;
	unsigned long ty = 0, temp_y = 0;
	int datacount = 0;
	SPI.begin();
	digitalWrite(TOUCH_CS, LOW);

	for (int i = 0; i<prec; i++)
	{
		//added param command - RB 6/4/15
		temp_x = touch_ReadData(0x90);
		temp_y = touch_ReadData(0xD0);

		if (!((temp_x>max(touch_x_left, touch_x_right)) or (temp_x == 0) or (temp_y>max(touch_y_top, touch_y_bottom)) or (temp_y == 0)))
		{
			ty += temp_x;
			tx += temp_y;
			datacount++;
		}
	}

	digitalWrite(TOUCH_CS, HIGH);

	//SPI.end();
	if (datacount>0)
	{
		if (orient == _default_orientation)
		{
			TP_X = tx / datacount;
			TP_Y = ty / datacount;
		}
		else
		{
			TP_X = ty / datacount;
			TP_Y = tx / datacount;
		}
	}
	else
	{
		TP_X = -1;
		TP_Y = -1;
	}
}

bool UTouch::dataAvailable()
{
	bool avail;
	pinMode(T_IRQ, INPUT);
	avail = !digitalRead(T_IRQ);
	pinMode(T_IRQ, OUTPUT);
	return avail;
}

int UTouch::getX()
{
	long c;

	if (orient == _default_orientation)
	{
		c = long(long(TP_X - touch_x_left) * (disp_x_size)) / long(touch_x_right - touch_x_left);
		if (c<0)
			c = 0;
		if (c>disp_x_size)
			c = disp_x_size;
	}
	else
	{
		if (_default_orientation == PORTRAIT)
			c = long(long(TP_X - touch_y_top) * (-disp_y_size)) / long(touch_y_bottom - touch_y_top) + long(disp_y_size);
		else
			c = long(long(TP_X - touch_y_top) * (disp_y_size)) / long(touch_y_bottom - touch_y_top);
		if (c<0)
			c = 0;
		if (c>disp_y_size)
			c = disp_y_size;
	}

	return c;
}

int UTouch::getY()
{
	int c;

	if (orient == _default_orientation)
	{
		c = long(long(TP_Y - touch_y_top) * (disp_y_size)) / long(touch_y_bottom - touch_y_top);
		if (c<0)
			c = 0;
		if (c>disp_y_size)
			c = disp_y_size;
	}
	else
	{
		if (_default_orientation == PORTRAIT)
			c = long(long(TP_Y - touch_x_left) * (disp_x_size)) / long(touch_x_right - touch_x_left);
		else
			c = long(long(TP_Y - touch_x_left) * (-disp_x_size)) / long(touch_x_right - touch_x_left) + long(disp_x_size);
		if (c<0)
			c = 0;
		if (c>disp_x_size)
			c = disp_x_size;
	}
	return c;
}

void UTouch::setPrecision(byte precision)
{
	switch (precision)
	{
	case PREC_LOW:
		prec = 1;
		break;
	case PREC_MEDIUM:
		prec = 10;
		break;
	case PREC_HI:
		prec = 25;
		break;
	case PREC_EXTREME:
		prec = 100;
		break;
	default:
		prec = 10;
		break;
	}
}

void UTouch::calibrateRead()
{
	unsigned long tx = 0;
	unsigned long ty = 0;

	SPI.begin();
	digitalWrite(TOUCH_CS, LOW);

	tx = touch_ReadData(0x90);
	ty = touch_ReadData(0xD0);

	digitalWrite(TOUCH_CS, HIGH);
	//SPI.end();

	TP_X = ty;
	TP_Y = tx;
}