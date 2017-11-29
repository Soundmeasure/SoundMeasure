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
#ifndef DS3231_h
#define DS3231_h

#if defined(__AVR__)
	#include "Arduino.h"
	#include "hardware/avr/HW_AVR_defines.h"
#elif defined(__PIC32MX__)
	#include "WProgram.h"
	#include "hardware/pic32/HW_PIC32_defines.h"
#elif defined(__arm__)
	#include "Arduino.h"
	#include "hardware/arm/HW_ARM_defines.h"
#endif

#define DS3231_ADDR_R	0xD1
#define DS3231_ADDR_W	0xD0
#define DS3231_ADDR		0x68

#define FORMAT_SHORT	1
#define FORMAT_LONG		2

#define FORMAT_LITTLEENDIAN	1
#define FORMAT_BIGENDIAN	2
#define FORMAT_MIDDLEENDIAN	3

#define MONDAY		1
#define TUESDAY		2
#define WEDNESDAY	3
#define THURSDAY	4
#define FRIDAY		5
#define SATURDAY	6
#define SUNDAY		7

#define SQW_RATE_1		0
#define SQW_RATE_1K		1
#define SQW_RATE_4K		2
#define SQW_RATE_8K		3

#define OUTPUT_SQW		0
#define OUTPUT_INT		1

class Time
{
public:
	uint8_t		hour;
	uint8_t		min;
	uint8_t		sec;
	uint8_t		date;
	uint8_t		mon;
	uint16_t	year;
	uint8_t		dow;
	uint32_t    unixtime;

	Time();
};


#ifndef RTCDATETIME_STRUCT_H
#define RTCDATETIME_STRUCT_H

struct RTCAlarmTime
{
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};
#endif


typedef enum
{
	DS3231_EVERY_SECOND = 0b00001111,
	DS3231_MATCH_S = 0b00001110,
	DS3231_MATCH_M_S = 0b00001100,
	DS3231_MATCH_H_M_S = 0b00001000,
	DS3231_MATCH_DT_H_M_S = 0b00000000,
	DS3231_MATCH_DY_H_M_S = 0b00010000
} DS3231_alarm1_t;

typedef enum
{
	DS3231_EVERY_MINUTE = 0b00001110,
	DS3231_MATCH_M = 0b00001100,
	DS3231_MATCH_H_M = 0b00001000,
	DS3231_MATCH_DT_H_M = 0b00000000,
	DS3231_MATCH_DY_H_M = 0b00010000
} DS3231_alarm2_t;




class DS3231
{
	public:
		DS3231(uint8_t data_pin, uint8_t sclk_pin);
		void	begin();
		Time	getTime();
		void	setTime(uint8_t hour, uint8_t min, uint8_t sec);
		void	setDate(uint8_t date, uint8_t mon, uint16_t year);
		void	setDOW();
		void	setDOW(uint8_t dow);

		char	*getTimeStr(uint8_t format=FORMAT_LONG);
		char	*getDateStr(uint8_t slformat=FORMAT_LONG, uint8_t eformat=FORMAT_LITTLEENDIAN, char divider='.');
		char	*getDOWStr(uint8_t format=FORMAT_LONG);
		char	*getMonthStr(uint8_t format=FORMAT_LONG);
		long	getUnixTime(Time t);

		void	enable32KHz(bool enable);
		void	setOutput(byte enable);
		void	setSQWRate(int rate);
		float	getTemp();

		void setDateTime(const char* date, const char* time);

		char* dateFormat(const char* dateFormat, Time dt);
		char* dateFormat(const char* dateFormat, RTCAlarmTime dt);



		void setAlarm1(uint8_t dydw, uint8_t hour, uint8_t minute, uint8_t second, DS3231_alarm1_t mode, bool armed = true);
		RTCAlarmTime getAlarm1(void);
		//DS3231_alarm1_t getAlarmType1(void);
		bool isAlarm1(bool clear = true);
		void armAlarm1(bool armed);
		bool isArmed1(void);
		void clearAlarm1(void);

		//void setAlarm2(uint8_t dydw, uint8_t hour, uint8_t minute, DS3231_alarm2_t mode, bool armed = true);
		//RTCAlarmTime getAlarm2(void);
		//DS3231_alarm2_t getAlarmType2(void);
		bool isAlarm2(bool clear = true);
		void armAlarm2(bool armed);
		bool isArmed2(void);
		void clearAlarm2(void);





	private:
		uint8_t _scl_pin;
		uint8_t _sda_pin;
		uint8_t _burstArray[7];
		boolean	_use_hw;

		void	_sendStart(byte addr);
		void	_sendStop();
		void	_sendAck();
		void	_sendNack();
		void	_waitForAck();
		uint8_t	_readByte();
		void	_writeByte(uint8_t value);
		void	_burstRead();
		uint8_t	_readRegister(uint8_t reg);
		void 	_writeRegister(uint8_t reg, uint8_t value);
		uint8_t	_decode(uint8_t value);
		uint8_t	_decodeH(uint8_t value);
		uint8_t	_decodeY(uint8_t value);
		uint8_t	_encode(uint8_t vaule);

		Time t;

		char *strDayOfWeek(uint8_t dayOfWeek);
		char *strMonth(uint8_t month);
		char *strAmPm(uint8_t hour, bool uppercase);
		char *strDaySufix(uint8_t day);

		uint8_t hour12(uint8_t hour24);
		uint8_t bcd2dec(uint8_t bcd);
		uint8_t dec2bcd(uint8_t dec);

		long time2long(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
		uint16_t date2days(uint16_t year, uint8_t month, uint8_t day);
		uint8_t daysInMonth(uint16_t year, uint8_t month);
		uint16_t dayInYear(uint16_t year, uint8_t month, uint8_t day);
		bool isLeapYear(uint16_t year);
		uint8_t dow(uint16_t y, uint8_t m, uint8_t d);

		uint32_t unixtime(void);
		uint8_t conv2d(const char* p);

		void writeRegister8(uint8_t reg, uint8_t value);
		uint8_t readRegister8(uint8_t reg);










#if defined(__arm__)
		Twi		*twi;
#endif
};
#endif
