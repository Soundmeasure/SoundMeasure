#ifndef _GLOBALS_H
#define _GLOBALS_H

#define  ledPin13  13                              // Назначение светодиодов на плате
#define  ledPin12  12                              // Назначение светодиодов на плате

//+++++++++++++++++++++++ Настройка электронного резистора +++++++++++++++++++++++++++++++++++++
#define address_AD5252   0x2F                      // Адрес микросхемы AD5252  
#define control_word1    0x07                      // Байт инструкции резистор №1
#define control_word2    0x87                      // Байт инструкции резистор №2

//+++++++++++++++++++++++++++ Порты управления платой Arduino Nano +++++++++++++++++++++++++++++++

#define  kn1Nano   A0                                            // Назначение кнопок управления Nano  A0  pulse    импульс
#define  kn2Nano   A1                                            // Назначение кнопок управления Nano  A1  triangle треугольник
#define  kn3Nano   A2                                            // Назначение кнопок управления Nano  A2  saw      пила
#define  kn4Nano   A3                                            // Назначение кнопок управления Nano  A3  sine     синус

#define  kn5Nano   A4                                            // Назначение кнопок управления Nano  A4
#define  kn6Nano   A5                                            // Назначение кнопок управления Nano  A5


#endif