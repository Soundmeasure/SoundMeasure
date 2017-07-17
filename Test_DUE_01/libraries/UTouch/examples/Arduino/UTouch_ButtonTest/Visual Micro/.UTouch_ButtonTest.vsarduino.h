#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
//Board = Arduino Mega 2560 or Mega ADK
#define __AVR_ATmega2560__
#define 
#define ARDUINO 150
#define ARDUINO_MAIN
#define __AVR__
#define F_CPU 16000000L
#define __cplusplus
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__

#define __builtin_va_list
#define __builtin_va_start
#define __builtin_va_end
#define __DOXYGEN__
#define __attribute__(x)
#define NOINLINE __attribute__((noinline))
#define prog_void
#define PGM_VOID_P int
            
typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}

void drawButtons();
void updateStr(int val);
void waitForIt(int x1, int y1, int x2, int y2);
//
//

#include "c:\arduino\hardware\arduino\avr\variants\mega\pins_arduino.h" 
#include "c:\arduino\hardware\arduino\avr\cores\arduino\arduino.h"
#include "C:\arduinostudio\UTouch\examples\Arduino\UTouch_ButtonTest\UTouch_ButtonTest.pde"
#endif
