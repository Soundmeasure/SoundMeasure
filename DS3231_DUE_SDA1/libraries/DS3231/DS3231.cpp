/*
  DS3231.cpp - Arduino/chipKit library support for the DS3231 I2C Real-Time Clock
  Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
  
  This library has been made to easily interface and use the DS3231 RTC with
  an Arduino or chipKit.

  You can find the latest version of the library at 
  http://www.RinkyDinkElectronics.com/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.

  Commercial use of this library requires you to buy a license that
  will allow commercial use. This includes using the library,
  modified or not, as a tool to sell products.

  The license applies to all part of the library including the 
  examples and tools supplied with the library.
*/
#include "DS3231.h"

// Include hardware-specific functions for the correct MCU
#if defined(__AVR__)
	#include "hardware/avr/HW_AVR.h"
#elif defined(__PIC32MX__)
  #include "hardware/pic32/HW_PIC32.h"
#elif defined(__arm__)
	#include "hardware/arm/HW_ARM.h"
#endif

#define REG_SEC		0x00
#define REG_MIN		0x01
#define REG_HOUR	0x02
#define REG_DOW		0x03
#define REG_DATE	0x04
#define REG_MON		0x05
#define REG_YEAR	0x06
#define REG_CON		0x0e
#define REG_STATUS	0x0f
#define REG_AGING	0x10
#define REG_TEMPM	0x11
#define REG_TEMPL	0x12

#define SEC_1970_TO_2000 946684800

//static const uint8_t dim[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

const uint8_t daysArray[] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };
const uint8_t dowArray[] PROGMEM = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

/* Public */

RTCDateTime::RTCDateTime()
{
	this->year = 2014;
	this->month  = 1;
	this->day = 1;
	this->hour = 0;
	this->minute  = 0;
	this->second  = 0;
	this->dayOfWeek  = 3;
}

DS3231::DS3231(uint8_t data_pin, uint8_t sclk_pin)
{
	_sda_pin = data_pin;
	_scl_pin = sclk_pin;
}

RTCDateTime DS3231::getTime()
{
	RTCDateTime t;
	_burstRead();
	t.second	= _decode(_burstArray[0]);
	t.minute	= _decode(_burstArray[1]);
	t.hour	= _decodeH(_burstArray[2]);
	t.dayOfWeek	= _burstArray[3];
	t.day	= _decode(_burstArray[4]);
	t.month	= _decode(_burstArray[5]);
	t.year	= _decodeY(_burstArray[6])+2000;
	return t;
}

void DS3231::setTime(uint8_t hour, uint8_t minute, uint8_t second)
{
	if (((hour>=0) && (hour<24)) && ((minute>=0) && (minute<60)) && ((second>=0) && (second<60)))
	{
		_writeRegister(REG_HOUR, _encode(hour));
		_writeRegister(REG_MIN, _encode(minute));
		_writeRegister(REG_SEC, _encode(second));
	}
}

void DS3231::setDate(uint16_t year, uint8_t month, uint8_t day)
{
	if (((day>0) && (day<=31)) && ((month>0) && (month<=12)) && ((year>=2000) && (year<3000)))
	{
		year -= 2000;
		_writeRegister(REG_YEAR, _encode(year));
		_writeRegister(REG_MON, _encode(month));
		_writeRegister(REG_DATE, _encode(day));
	}
}

void DS3231::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{

	if (((day>0) && (day <= 31)) && ((month>0) && (month <= 12)) && ((year >= 2000) && (year<3000)))
	{
		year -= 2000;
		_writeRegister(REG_YEAR, _encode(year));
		_writeRegister(REG_MON, _encode(month));
		_writeRegister(REG_DATE, _encode(day));
	}
	if (((hour >= 0) && (hour<24)) && ((minute >= 0) && (minute<60)) && ((second >= 0) && (second<60)))
	{
		_writeRegister(REG_HOUR, _encode(hour));
		_writeRegister(REG_MIN, _encode(minute));
		_writeRegister(REG_SEC, _encode(second));
	}

}


void DS3231::setDOW()
{
	int dayOfWeek;
	byte mArr[12] = {6,2,2,5,0,3,5,1,4,6,2,4};
	RTCDateTime _t = getTime();
  
	dayOfWeek = (_t.year % 100);
	dayOfWeek = dayOfWeek*1.25;
	dayOfWeek += _t.day;
	dayOfWeek += mArr[_t.month-1];
	if (((_t.year % 4)==0) && (_t.month<3))
		dayOfWeek -= 1;
	while (dayOfWeek>7)
		dayOfWeek -= 7;
	_writeRegister(REG_DOW, dayOfWeek);
}

void DS3231::setDOW(uint8_t dayOfWeek)
{
	if ((dayOfWeek>0) && (dayOfWeek<8))
		_writeRegister(REG_DOW, dayOfWeek);
}

char *DS3231::getTimeStr(uint8_t format)
{
	static char output[] = "xxxxxxxx";
	RTCDateTime t;
	t=getTime();
	if (t.hour<10)
		output[0]=48;
	else
		output[0]=char((t.hour / 10)+48);
	output[1]=char((t.hour % 10)+48);
	output[2]=58;
	if (t.minute<10)
		output[3]=48;
	else
		output[3]=char((t.minute / 10)+48);
	output[4]=char((t.minute % 10)+48);
	output[5]=58;
	if (format==FORMAT_SHORT)
		output[5]=0;
	else
	{
	if (t.second<10)
		output[6]=48;
	else
		output[6]=char((t.second / 10)+48);
	output[7]=char((t.second % 10)+48);
	output[8]=0;
	}
	return (char*)&output;
}

char *DS3231::getDateStr(uint8_t slformat, uint8_t eformat, char divider)
{
	static char output[] = "xxxxxxxxxx";
	int yr, offset;
	RTCDateTime t;
	t=getTime();
	switch (eformat)
	{
		case FORMAT_LITTLEENDIAN:
			if (t.day<10)
				output[0]=48;
			else
				output[0]=char((t.day / 10)+48);
			output[1]=char((t.day % 10)+48);
			output[2]=divider;
			if (t.month<10)
				output[3]=48;
			else
				output[3]=char((t.month / 10)+48);
			output[4]=char((t.month % 10)+48);
			output[5]=divider;
			if (slformat==FORMAT_SHORT)
			{
				yr=t.year-2000;
				if (yr<10)
					output[6]=48;
				else
					output[6]=char((yr / 10)+48);
				output[7]=char((yr % 10)+48);
				output[8]=0;
			}
			else
			{
				yr=t.year;
				output[6]=char((yr / 1000)+48);
				output[7]=char(((yr % 1000) / 100)+48);
				output[8]=char(((yr % 100) / 10)+48);
				output[9]=char((yr % 10)+48);
				output[10]=0;
			}
			break;
		case FORMAT_BIGENDIAN:
			if (slformat==FORMAT_SHORT)
				offset=0;
			else
				offset=2;
			if (slformat==FORMAT_SHORT)
			{
				yr=t.year-2000;
				if (yr<10)
					output[0]=48;
				else
					output[0]=char((yr / 10)+48);
				output[1]=char((yr % 10)+48);
				output[2]=divider;
			}
			else
			{
				yr=t.year;
				output[0]=char((yr / 1000)+48);
				output[1]=char(((yr % 1000) / 100)+48);
				output[2]=char(((yr % 100) / 10)+48);
				output[3]=char((yr % 10)+48);
				output[4]=divider;
			}
			if (t.month<10)
				output[3+offset]=48;
			else
				output[3+offset]=char((t.month / 10)+48);
			output[4+offset]=char((t.month % 10)+48);
			output[5+offset]=divider;
			if (t.day<10)
				output[6+offset]=48;
			else
				output[6+offset]=char((t.day / 10)+48);
			output[7+offset]=char((t.day % 10)+48);
			output[8+offset]=0;
			break;
		case FORMAT_MIDDLEENDIAN:
			if (t.month<10)
				output[0]=48;
			else
				output[0]=char((t.month / 10)+48);
			output[1]=char((t.month % 10)+48);
			output[2]=divider;
			if (t.day<10)
				output[3]=48;
			else
				output[3]=char((t.day / 10)+48);
			output[4]=char((t.day % 10)+48);
			output[5]=divider;
			if (slformat==FORMAT_SHORT)
			{
				yr=t.year-2000;
				if (yr<10)
					output[6]=48;
				else
					output[6]=char((yr / 10)+48);
				output[7]=char((yr % 10)+48);
				output[8]=0;
			}
			else
			{
				yr=t.year;
				output[6]=char((yr / 1000)+48);
				output[7]=char(((yr % 1000) / 100)+48);
				output[8]=char(((yr % 100) / 10)+48);
				output[9]=char((yr % 10)+48);
				output[10]=0;
			}
			break;
	}
	return (char*)&output;
}

char *DS3231::getDOWStr(uint8_t format)
{
	char *output = "xxxxxxxxxx";
	char *daysLong[]  = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
	char *daysShort[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
	RTCDateTime t;
	t=getTime();
	if (format == FORMAT_SHORT)
		output = daysShort[t.dayOfWeek-1];
	else
		output = daysLong[t.dayOfWeek-1];
	return output;
}

char *DS3231::getMonthStr(uint8_t format)
{
	char *output= "xxxxxxxxx";
	char *monthLong[]  = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	char *monthShort[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	RTCDateTime t;
	t=getTime();
	if (format == FORMAT_SHORT)
		output = monthShort[t.month-1];
	else
		output = monthLong[t.month-1];
	return output;
}

long DS3231::getUnixTime(RTCDateTime t)
{
	uint16_t	dc;

	dc = t.day;
	for (uint8_t i = 0; i<(t.month-1); i++)
		dc += daysArray[i];
	if ((t.month > 2) && (((t.year-2000) % 4) == 0))
		++dc;
	dc = dc + (365 * (t.year-2000)) + (((t.year-2000) + 3) / 4) - 1;

	return ((((((dc * 24L) + t.hour) * 60) + t.minute) * 60) + t.second) + SEC_1970_TO_2000;

}

void DS3231::enable32KHz(bool enable)
{
  uint8_t _reg = _readRegister(REG_STATUS);
  _reg &= ~(1 << 3);
  _reg |= (enable << 3);
  _writeRegister(REG_STATUS, _reg);
}

void DS3231::setOutput(byte enable)
{
  uint8_t _reg = _readRegister(REG_CON);
  _reg &= ~(1 << 2);
  _reg |= (enable << 2);
  _writeRegister(REG_CON, _reg);
}

void DS3231::setSQWRate(int rate)
{
  uint8_t _reg = _readRegister(REG_CON);
  _reg &= ~(3 << 3);
  _reg |= (rate << 3);
  _writeRegister(REG_CON, _reg);
}

float DS3231::getTemp()
{
	uint8_t _msb = _readRegister(REG_TEMPM);
	uint8_t _lsb = _readRegister(REG_TEMPL);
	return (float)_msb + ((_lsb >> 6) * 0.25f);
}

/* Private */

void	DS3231::_sendStart(byte addr)
{
	pinMode(_sda_pin, OUTPUT);
	digitalWrite(_sda_pin, HIGH);
	digitalWrite(_scl_pin, HIGH);
	digitalWrite(_sda_pin, LOW);
	digitalWrite(_scl_pin, LOW);
	shiftOut(_sda_pin, _scl_pin, MSBFIRST, addr);
}

void	DS3231::_sendStop()
{
	pinMode(_sda_pin, OUTPUT);
	digitalWrite(_sda_pin, LOW);
	digitalWrite(_scl_pin, HIGH);
	digitalWrite(_sda_pin, HIGH);
	pinMode(_sda_pin, INPUT);
}

void	DS3231::_sendNack()
{
	pinMode(_sda_pin, OUTPUT);
	digitalWrite(_scl_pin, LOW);
	digitalWrite(_sda_pin, HIGH);
	digitalWrite(_scl_pin, HIGH);
	digitalWrite(_scl_pin, LOW);
	pinMode(_sda_pin, INPUT);
}

void	DS3231::_sendAck()
{
	pinMode(_sda_pin, OUTPUT);
	digitalWrite(_scl_pin, LOW);
	digitalWrite(_sda_pin, LOW);
	digitalWrite(_scl_pin, HIGH);
	digitalWrite(_scl_pin, LOW);
	pinMode(_sda_pin, INPUT);
}

void	DS3231::_waitForAck()
{
	pinMode(_sda_pin, INPUT);
	digitalWrite(_scl_pin, HIGH);
	while (digitalRead(_sda_pin)==HIGH) {}
	digitalWrite(_scl_pin, LOW);
}

uint8_t DS3231::_readByte()
{
	pinMode(_sda_pin, INPUT);

	uint8_t value = 0;
	uint8_t currentBit = 0;

	for (int i = 0; i < 8; ++i)
	{
		digitalWrite(_scl_pin, HIGH);
		currentBit = digitalRead(_sda_pin);
		value |= (currentBit << 7-i);
		delayMicroseconds(1);
		digitalWrite(_scl_pin, LOW);
	}
	return value;
}

void DS3231::_writeByte(uint8_t value)
{
	pinMode(_sda_pin, OUTPUT);
	shiftOut(_sda_pin, _scl_pin, MSBFIRST, value);
}

uint8_t	DS3231::_decode(uint8_t value)
{
	uint8_t decoded = value & 127;
	decoded = (decoded & 15) + 10 * ((decoded & (15 << 4)) >> 4);
	return decoded;
}

uint8_t DS3231::_decodeH(uint8_t value)
{
  if (value & 128)
    value = (value & 15) + (12 * ((value & 32) >> 5));
  else
    value = (value & 15) + (10 * ((value & 48) >> 4));
  return value;
}

uint8_t	DS3231::_decodeY(uint8_t value)
{
	uint8_t decoded = (value & 15) + 10 * ((value & (15 << 4)) >> 4);
	return decoded;
}

uint8_t DS3231::_encode(uint8_t value)
{
	uint8_t encoded = ((value / 10) << 4) + (value % 10);
	return encoded;
}

char* DS3231::dateFormat(const char* dateFormat, RTCDateTime dt)
{
	char buffer[255];

	buffer[0] = 0;

	char helper[11];

	while (*dateFormat != '\0')
	{
		switch (dateFormat[0])
		{
			// Day decoder
		case 'd':
			sprintf(helper, "%02d", dt.day);
			strcat(buffer, (const char *)helper);
			break;
		case 'j':
			sprintf(helper, "%d", dt.day);
			strcat(buffer, (const char *)helper);
			break;
		case 'l':
			strcat(buffer, (const char *)strDayOfWeek(dt.dayOfWeek));
			break;
		case 'D':
			strncat(buffer, strDayOfWeek(dt.dayOfWeek), 3);
			break;
		case 'N':
			sprintf(helper, "%d", dt.dayOfWeek);
			strcat(buffer, (const char *)helper);
			break;
		case 'w':
			sprintf(helper, "%d", (dt.dayOfWeek + 7) % 7);
			strcat(buffer, (const char *)helper);
			break;
		case 'z':
			sprintf(helper, "%d", dayInYear(dt.year, dt.month, dt.day));
			strcat(buffer, (const char *)helper);
			break;
		case 'S':
			strcat(buffer, (const char *)strDaySufix(dt.day));
			break;

			// Month decoder
		case 'm':
			sprintf(helper, "%02d", dt.month);
			strcat(buffer, (const char *)helper);
			break;
		case 'n':
			sprintf(helper, "%d", dt.month);
			strcat(buffer, (const char *)helper);
			break;
		case 'F':
			strcat(buffer, (const char *)strMonth(dt.month));
			break;
		case 'M':
			strncat(buffer, (const char *)strMonth(dt.month), 3);
			break;
		case 't':
			sprintf(helper, "%d", daysInMonth(dt.year, dt.month));
			strcat(buffer, (const char *)helper);
			break;

			// Year decoder
		case 'Y':
			sprintf(helper, "%d", dt.year);
			strcat(buffer, (const char *)helper);
			break;
		case 'y': sprintf(helper, "%02d", dt.year - 2000);
			strcat(buffer, (const char *)helper);
			break;
		case 'L':
			sprintf(helper, "%d", isLeapYear(dt.year));
			strcat(buffer, (const char *)helper);
			break;

			// Hour decoder
		case 'H':
			sprintf(helper, "%02d", dt.hour);
			strcat(buffer, (const char *)helper);
			break;
		case 'G':
			sprintf(helper, "%d", dt.hour);
			strcat(buffer, (const char *)helper);
			break;
		case 'h':
			sprintf(helper, "%02d", hour12(dt.hour));
			strcat(buffer, (const char *)helper);
			break;
		case 'g':
			sprintf(helper, "%d", hour12(dt.hour));
			strcat(buffer, (const char *)helper);
			break;
		case 'A':
			strcat(buffer, (const char *)strAmPm(dt.hour, true));
			break;
		case 'a':
			strcat(buffer, (const char *)strAmPm(dt.hour, false));
			break;

			// Minute decoder
		case 'i':
			sprintf(helper, "%02d", dt.minute);
			strcat(buffer, (const char *)helper);
			break;

			// Second decoder
		case 's':
			sprintf(helper, "%02d", dt.second);
			strcat(buffer, (const char *)helper);
			break;

			// Misc decoder
		case 'U':
			sprintf(helper, "%lu", dt.unixtime);
			strcat(buffer, (const char *)helper);
			break;

		default:
			strncat(buffer, dateFormat, 1);
			break;
		}
		dateFormat++;
	}

	return buffer;
}

//char* DS3231::dateFormat(const char* dateFormat, RTCAlarmTime dt)
//{
//	char buffer[255];
//
//	buffer[0] = 0;
//
//	char helper[11];
//
//	while (*dateFormat != '\0')
//	{
//		switch (dateFormat[0])
//		{
//			// Day decoder
//		case 'd':
//			sprintf(helper, "%02d", dt.day);
//			strcat(buffer, (const char *)helper);
//			break;
//		case 'j':
//			sprintf(helper, "%d", dt.day);
//			strcat(buffer, (const char *)helper);
//			break;
//		case 'l':
//			strcat(buffer, (const char *)strDayOfWeek(dt.day));
//			break;
//		case 'D':
//			strncat(buffer, strDayOfWeek(dt.day), 3);
//			break;
//		case 'N':
//			sprintf(helper, "%d", dt.day);
//			strcat(buffer, (const char *)helper);
//			break;
//		case 'w':
//			sprintf(helper, "%d", (dt.day + 7) % 7);
//			strcat(buffer, (const char *)helper);
//			break;
//		case 'S':
//			strcat(buffer, (const char *)strDaySufix(dt.day));
//			break;
//
//			// Hour decoder
//		case 'H':
//			sprintf(helper, "%02d", dt.hour);
//			strcat(buffer, (const char *)helper);
//			break;
//		case 'G':
//			sprintf(helper, "%d", dt.hour);
//			strcat(buffer, (const char *)helper);
//			break;
//		case 'h':
//			sprintf(helper, "%02d", hour12(dt.hour));
//			strcat(buffer, (const char *)helper);
//			break;
//		case 'g':
//			sprintf(helper, "%d", hour12(dt.hour));
//			strcat(buffer, (const char *)helper);
//			break;
//		case 'A':
//			strcat(buffer, (const char *)strAmPm(dt.hour, true));
//			break;
//		case 'a':
//			strcat(buffer, (const char *)strAmPm(dt.hour, false));
//			break;
//
//			// Minute decoder
//		case 'i':
//			sprintf(helper, "%02d", dt.minute);
//			strcat(buffer, (const char *)helper);
//			break;
//
//			// Second decoder
//		case 's':
//			sprintf(helper, "%02d", dt.second);
//			strcat(buffer, (const char *)helper);
//			break;
//
//		default:
//			strncat(buffer, dateFormat, 1);
//			break;
//		}
//		dateFormat++;
//	}
//
//	return buffer;
//}

char *DS3231::strDayOfWeek(uint8_t dayOfWeek)
{
	switch (dayOfWeek) {
	case 1:
		return "Monday";
		break;
	case 2:
		return "Tuesday";
		break;
	case 3:
		return "Wednesday";
		break;
	case 4:
		return "Thursday";
		break;
	case 5:
		return "Friday";
		break;
	case 6:
		return "Saturday";
		break;
	case 7:
		return "Sunday";
		break;
	default:
		return "Unknown";
	}
}

char *DS3231::strMonth(uint8_t month)
{
	switch (month) {
	case 1:
		return "January";
		break;
	case 2:
		return "February";
		break;
	case 3:
		return "March";
		break;
	case 4:
		return "April";
		break;
	case 5:
		return "May";
		break;
	case 6:
		return "June";
		break;
	case 7:
		return "July";
		break;
	case 8:
		return "August";
		break;
	case 9:
		return "September";
		break;
	case 10:
		return "October";
		break;
	case 11:
		return "November";
		break;
	case 12:
		return "December";
		break;
	default:
		return "Unknown";
	}
}

char *DS3231::strAmPm(uint8_t hour, bool uppercase)
{
	if (hour < 12)
	{
		if (uppercase)
		{
			return "AM";
		}
		else
		{
			return "am";
		}
	}
	else
	{
		if (uppercase)
		{
			return "PM";
		}
		else
		{
			return "pm";
		}
	}
}

char *DS3231::strDaySufix(uint8_t day)
{
	if (day % 10 == 1)
	{
		return "st";
	}
	else
		if (day % 10 == 2)
		{
			return "nd";
		}
	if (day % 10 == 3)
	{
		return "rd";
	}

	return "th";
}

uint8_t DS3231::hour12(uint8_t hour24)
{
	if (hour24 == 0)
	{
		return 12;
	}

	if (hour24 > 12)
	{
		return (hour24 - 12);
	}

	return hour24;
}

long DS3231::time2long(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	return ((days * 24L + hours) * 60 + minutes) * 60 + seconds;
}

uint16_t DS3231::dayInYear(uint16_t year, uint8_t month, uint8_t day)
{
	uint16_t fromDate;
	uint16_t toDate;

	fromDate = date2days(year, 1, 1);
	toDate = date2days(year, month, day);

	return (toDate - fromDate);
}

bool DS3231::isLeapYear(uint16_t year)
{
	return (year % 4 == 0);
}

uint8_t DS3231::daysInMonth(uint16_t year, uint8_t month)
{
	uint8_t days;

	days = pgm_read_byte(daysArray + month - 1);

	if ((month == 2) && isLeapYear(year))
	{
		++days;
	}

	return days;
}

uint16_t DS3231::date2days(uint16_t year, uint8_t month, uint8_t day)
{
	year = year - 2000;

	uint16_t days16 = day;

	for (uint8_t i = 1; i < month; ++i)
	{
		days16 += pgm_read_byte(daysArray + i - 1);
	}

	if ((month == 2) && isLeapYear(year))
	{
		++days16;
	}

	return days16 + 365 * year + (year + 3) / 4 - 1;
}

uint32_t DS3231::unixtime(void)
{
	uint32_t u;

	u = time2long(date2days(t.year, t.month, t.day), t.hour, t.minute, t.second);
	u += 946681200;

	return u;
}

uint8_t DS3231::conv2d(const char* p)
{
	uint8_t v = 0;

	if ('0' <= *p && *p <= '9')
	{
		v = *p - '0';
	}

	return 10 * v + *++p - '0';
}

uint8_t DS3231::dow(uint16_t y, uint8_t m, uint8_t d)
{
	uint8_t dow;

	y -= m < 3;
	dow = ((y + y / 4 - y / 100 + y / 400 + pgm_read_byte(dowArray + (m - 1)) + d) % 7);

	if (dow == 0)
	{
		return 7;
	}

	return dow;
}

//
//
//RTCAlarmTime DS3231::getAlarm1(void)
//{
//	uint8_t values[4];
//	RTCAlarmTime a;
//
//	Wire.beginTransmission(DS3231_ADDRESS);
//	Wire.write(DS3231_REG_ALARM_1);
//
//	Wire.endTransmission();
//
//	Wire.requestFrom(DS3231_ADDRESS, 4);
//
//	while (!Wire.available()) {};
//
//	for (int i = 3; i >= 0; i--)
//	{
//		values[i] = bcd2dec(Wire.read() & 0b01111111);
//	}
//
//	Wire.endTransmission();
//
//	a.day = values[0];
//	a.hour = values[1];
//	a.minute = values[2];
//	a.second = values[3];
//
//	return a;
//}
//
//DS3231_alarm1_t DS3231::getAlarmType1(void)
//{
//	uint8_t values[4];
//	uint8_t mode = 0;
//
//	Wire.beginTransmission(DS3231_ADDRESS);
//	Wire.write(DS3231_REG_ALARM_1);
//	Wire.endTransmission();
//
//	Wire.requestFrom(DS3231_ADDRESS, 4);
//
//	while (!Wire.available()) {};
//
//	for (int i = 3; i >= 0; i--)
//	{
//		values[i] = bcd2dec(Wire.read());
//	}
//
//	Wire.endTransmission();
//
//	mode |= ((values[3] & 0b01000000) >> 6);
//	mode |= ((values[2] & 0b01000000) >> 5);
//	mode |= ((values[1] & 0b01000000) >> 4);
//	mode |= ((values[0] & 0b01000000) >> 3);
//	mode |= ((values[0] & 0b00100000) >> 1);
//
//	return (DS3231_alarm1_t)mode;
//}
//
//void DS3231::setAlarm1(uint8_t dydw, uint8_t hour, uint8_t minute, uint8_t second, DS3231_alarm1_t mode, bool armed)
//{
//	second = dec2bcd(second);
//	minute = dec2bcd(minute);
//	hour = dec2bcd(hour);
//	dydw = dec2bcd(dydw);
//
//	switch (mode)
//	{
//	case DS3231_EVERY_SECOND:
//		second |= 0b10000000;
//		minute |= 0b10000000;
//		hour |= 0b10000000;
//		dydw |= 0b10000000;
//		break;
//
//	case DS3231_MATCH_S:
//		second &= 0b01111111;
//		minute |= 0b10000000;
//		hour |= 0b10000000;
//		dydw |= 0b10000000;
//		break;
//
//	case DS3231_MATCH_M_S:
//		second &= 0b01111111;
//		minute &= 0b01111111;
//		hour |= 0b10000000;
//		dydw |= 0b10000000;
//		break;
//
//	case DS3231_MATCH_H_M_S:
//		second &= 0b01111111;
//		minute &= 0b01111111;
//		hour &= 0b01111111;
//		dydw |= 0b10000000;
//		break;
//
//	case DS3231_MATCH_DT_H_M_S:
//		second &= 0b01111111;
//		minute &= 0b01111111;
//		hour &= 0b01111111;
//		dydw &= 0b01111111;
//		break;
//
//	case DS3231_MATCH_DY_H_M_S:
//		second &= 0b01111111;
//		minute &= 0b01111111;
//		hour &= 0b01111111;
//		dydw &= 0b01111111;
//		dydw |= 0b01000000;
//		break;
//	}
//
//
//
//
//
//
//	//Wire.beginTransmission(DS3231_ADDRESS);
//
//	//Wire.write(DS3231_REG_ALARM_1);
//	//Wire.write(second);
//	//Wire.write(minute);
//	//Wire.write(hour);
//	//Wire.write(dydw);
//
//	//Wire.endTransmission();
//
//	armAlarm1(armed);
//
//	clearAlarm1();
//}
//
//bool DS3231::isAlarm1(bool clear)
//{
//	uint8_t alarm;
//
//	alarm = _readRegister8(DS3231_REG_STATUS);
//	alarm &= 0b00000001;
//
//	if (alarm && clear)
//	{
//		clearAlarm1();
//	}
//
//	return alarm;
//}
//
//void DS3231::armAlarm1(bool armed)
//{
//	uint8_t value;
//	value = _readRegister8(DS3231_REG_CONTROL);
//
//	if (armed)
//	{
//		value |= 0b00000001;
//	}
//	else
//	{
//		value &= 0b11111110;
//	}
//
//	_writeRegister8(DS3231_REG_CONTROL, value);
//}
//
//bool DS3231::isArmed1(void)
//{
//	uint8_t value;
//	value = _readRegister8(DS3231_REG_CONTROL);
//	value &= 0b00000001;
//	return value;
//}
//
//void DS3231::clearAlarm1(void)
//{
//	uint8_t value;
//
//	value = _readRegister(DS3231_REG_STATUS);
//	//value = readRegister8(DS3231_REG_STATUS);
//	value &= 0b11111110;
//
//	//writeRegister8(DS3231_REG_STATUS, value);
//
//	_writeRegister(DS3231_REG_STATUS, value));
//}

/*
RTCAlarmTime DS3231::getAlarm2(void)
{
	uint8_t values[3];
	RTCAlarmTime a;

	Wire.beginTransmission(DS3231_ADDRESS);
#if ARDUINO >= 100
	Wire.write(DS3231_REG_ALARM_2);
#else
	Wire.send(DS3231_REG_ALARM_2);
#endif
	Wire.endTransmission();

	Wire.requestFrom(DS3231_ADDRESS, 3);

	while (!Wire.available()) {};

	for (int i = 2; i >= 0; i--)
	{
#if ARDUINO >= 100
		values[i] = bcd2dec(Wire.read() & 0b01111111);
#else
		values[i] = bcd2dec(Wire.receive() & 0b01111111);
#endif
	}

	Wire.endTransmission();

	a.day = values[0];
	a.hour = values[1];
	a.minute = values[2];
	a.second = 0;

	return a;
}

DS3231_alarm2_t DS3231::getAlarmType2(void)
{
	uint8_t values[3];
	uint8_t mode = 0;

	Wire.beginTransmission(DS3231_ADDRESS);
#if ARDUINO >= 100
	Wire.write(DS3231_REG_ALARM_2);
#else
	Wire.send(DS3231_REG_ALARM_2);
#endif
	Wire.endTransmission();

	Wire.requestFrom(DS3231_ADDRESS, 3);

	while (!Wire.available()) {};

	for (int i = 2; i >= 0; i--)
	{
#if ARDUINO >= 100
		values[i] = bcd2dec(Wire.read());
#else
		values[i] = bcd2dec(Wire.receive());
#endif
	}

	Wire.endTransmission();

	mode |= ((values[2] & 0b01000000) >> 5);
	mode |= ((values[1] & 0b01000000) >> 4);
	mode |= ((values[0] & 0b01000000) >> 3);
	mode |= ((values[0] & 0b00100000) >> 1);

	return (DS3231_alarm2_t)mode;
}

void DS3231::setAlarm2(uint8_t dydw, uint8_t hour, uint8_t minute, DS3231_alarm2_t mode, bool armed)
{
	minute = dec2bcd(minute);
	hour = dec2bcd(hour);
	dydw = dec2bcd(dydw);

	switch (mode)
	{
	case DS3231_EVERY_MINUTE:
		minute |= 0b10000000;
		hour |= 0b10000000;
		dydw |= 0b10000000;
		break;

	case DS3231_MATCH_M:
		minute &= 0b01111111;
		hour |= 0b10000000;
		dydw |= 0b10000000;
		break;

	case DS3231_MATCH_H_M:
		minute &= 0b01111111;
		hour &= 0b01111111;
		dydw |= 0b10000000;
		break;

	case DS3231_MATCH_DT_H_M:
		minute &= 0b01111111;
		hour &= 0b01111111;
		dydw &= 0b01111111;
		break;

	case DS3231_MATCH_DY_H_M:
		minute &= 0b01111111;
		hour &= 0b01111111;
		dydw &= 0b01111111;
		dydw |= 0b01000000;
		break;
	}

	Wire.beginTransmission(DS3231_ADDRESS);
#if ARDUINO >= 100
	Wire.write(DS3231_REG_ALARM_2);
	Wire.write(minute);
	Wire.write(hour);
	Wire.write(dydw);
#else
	Wire.send(DS3231_REG_ALARM_2);
	Wire.send(minute);
	Wire.send(hour);
	Wire.send(dydw);
#endif

	Wire.endTransmission();

	armAlarm2(armed);

	clearAlarm2();
}

void DS3231::armAlarm2(bool armed)
{
	uint8_t value;
	value = readRegister8(DS3231_REG_CONTROL);

	if (armed)
	{
		value |= 0b00000010;
	}
	else
	{
		value &= 0b11111101;
	}

	writeRegister8(DS3231_REG_CONTROL, value);
}

bool DS3231::isArmed2(void)
{
	uint8_t value;
	value = readRegister8(DS3231_REG_CONTROL);
	value &= 0b00000010;
	value >>= 1;
	return value;
}


void DS3231::clearAlarm2(void)
{
	uint8_t value;

	value = readRegister8(DS3231_REG_STATUS);
	value &= 0b11111101;

	writeRegister8(DS3231_REG_STATUS, value);
}


bool DS3231::isAlarm2(bool clear)
{
	uint8_t alarm;

	alarm = readRegister8(DS3231_REG_STATUS);
	alarm &= 0b00000010;

	if (alarm && clear)
	{
		clearAlarm2();
	}

	return alarm;
}
*/

//
//DS3231_alarm1_t DS3231::getAlarmType1(void)
//{
//	uint8_t values[4];
//	uint8_t mode = 0;
//
//	Wire.beginTransmission(DS3231_ADDRESS);
//
//	Wire.write(DS3231_REG_ALARM_1);
//
//	Wire.endTransmission();
//
//	Wire.requestFrom(DS3231_ADDRESS, 4);
//
//	while (!Wire.available()) {};
//
//	for (int i = 3; i >= 0; i--)
//	{
//		values[i] = bcd2dec(Wire.read());
//	}
//
//	Wire.endTransmission();
//
//	mode |= ((values[3] & 0b01000000) >> 6);
//	mode |= ((values[2] & 0b01000000) >> 5);
//	mode |= ((values[1] & 0b01000000) >> 4);
//	mode |= ((values[0] & 0b01000000) >> 3);
//	mode |= ((values[0] & 0b00100000) >> 1);
//
//	return (DS3231_alarm1_t)mode;
//}
