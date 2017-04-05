/*
ArduinoNanoCabelTest.ino
Visual Studio 2010
VisualMicro

Программа тестирования исправности кабелей.
Версия:               - 1.0
Организация:          - ООО "Децима"
Автор:                - Мосейчук А.В.
Дата начала работ:    - 01.09.2016г.
Дата окончания работ: - 01.03.2017г.
 
*/

#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <MsTimer2.h>
#include <modbus.h>
#include <modbusDevice.h>
#include <modbusRegBank.h>
#include <modbusSlave.h>
#include "MCP23017.h"
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdlib.h> // div, div_t
#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <Arduino.h>
#include <SdFat.h>
#include <SdFatUtil.h>

#define led_Green 12                                     // Светодиод на передней панели зеленый
#define led_Red   13                                     // Светодиод на передней панели красный

#define Chanal_A   A8                                    // Выход канала А блока коммутаторов
#define Chanal_B   A9                                    // Выход канала B блока коммутаторов
#define Rele1       8                                    // Управление реле 1
#define Rele2       9                                    // Управление реле 2
#define Rele3      10                                    // Управление реле 3

MCP23017 mcp_Out1;                                       // Назначение портов расширения MCP23017  4 A - Out, B - Out
MCP23017 mcp_Out2;                                       // Назначение портов расширения MCP23017  6 A - Out, B - Out

//+++++++++++++++++++ MODBUS ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

modbusDevice regBank;
modbusSlave slave;

//+++++++++++++++++++++++ Настройка электронного резистора +++++++++++++++++++++++++++++++++++++
byte resistance          = 0x00;                        // Сопротивление 0x00..0xFF - 0Ом..100кОм

//+++++++++++++++++++++++++++++ Внешняя память +++++++++++++++++++++++++++++++++++++++
int deviceaddress        = 80;                          // Адрес микросхемы памяти
unsigned int eeaddress   =  0;                          // Адрес ячейки памяти
byte hi;                                                // Старший байт для преобразования числа
byte low;                                               // Младший байт для преобразования числа

//********************* Настройка монитора ***********************************
UTFT        myGLCD(ITDB32S,38,39,40,41);                // Дисплей 3.2"
UTouch        myTouch(6, 5, 4, 3, 2);                   // Standard Arduino Mega/Due shield            : 6,5,4,3,2
UTFT_Buttons  myButtons(&myGLCD, &myTouch);             // Finally we set up UTFT_Buttons :)

boolean default_colors = true;                          //
uint8_t menu_redraw_required = 0;
// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

boolean connect_gnd1 = false;
boolean connect_gnd2 = false;
//byte N_block = 1;
boolean view_tab_run = false;

//+++++++++++++++++++++++++++ Настройка часов +++++++++++++++++++++++++++++++
uint8_t second = 0;                                    //Initialization time
uint8_t minute = 10;
uint8_t hour   = 10;
uint8_t dow    = 2;
uint8_t day    = 15;
uint8_t month  = 3;
uint16_t year  = 16;
RTC_DS1307 RTC;                                       // define the Real Time Clock object

int clockCenterX               = 119;
int clockCenterY               = 119;
int oldsec                     = 0;
const char* str[]              = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
const char* str1[]             = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
const char* str_mon[]          = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
unsigned long wait_time        = 0;                               // Время простоя прибора
unsigned long wait_time_Old    = 0;                               // Время простоя прибора
int time_minute                = 5;                               // Время простоя прибора
int pin_cable                  = 0;                               // Количество выводов кабеля
//------------------------------------------------------------------------------

const unsigned int adr_control_command    PROGMEM       = 40001;  // Адрес передачи комманд на выполнение
const unsigned int adr_reg_count_err      PROGMEM       = 40002;  // Адрес счетчика всех ошибок
//-------------------------------------------------------------------------------------------------------

//++++++++++++++++++++++++++++ Переменные для цифровой клавиатуры +++++++++++++++++++++++++++++
int x, y, z;
char stCurrent[20]    = "";                                       // Переменная хранения введенной строки
int stCurrentLen      = 0;                                        // Переменная хранения длины введенной строки
int stCurrentLen1     = 0;                                        // Переменная временного хранения длины введенной строки
char stLast[20]       = "";                                       // Данные в введенной строке строке.
int ret               = 0;                                        // Признак прерывания операции
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
//Назначение переменных для хранения № опций меню (клавиш)
int but1, but2, but3, but4, but5, but6, but7, but8, but9, but10, butX, butY, butA, butB, butC, butD, but_m1, but_m2, but_m3, but_m4, but_m5, pressed_button;
//int kbut1, kbut2, kbut3, kbut4, kbut5, kbut6, kbut7, kbut8, kbut9, kbut0, kbut_save,kbut_clear, kbut_exit;
//int kbutA, kbutB, kbutC, kbutD, kbutE, kbutF;
int m2 = 1; // Переменная номера меню

//------------------------------------------------------------------------------------------------------------------
// Назначение переменных для хранения текстов

char  txt_menu1_1[]            = "Tec\xA4 ""\x9F""a\x96""e\xA0\xAF N1";                                    // Тест кабель N 1
char  txt_menu1_2[]            = "Tec\xA4 ""\x9F""a\x96""e\xA0\xAF N2";                                    // Тест кабель N 2
char  txt_menu1_3[]            = "Tec\xA4 ""\x9F""a\x96""e\xA0\xAF N3";                                    // Тест кабель N 3
char  txt_menu1_4[]            = "Tec\xA4 ""\x9F""a\x96""e\xA0\xAF N4";                                    // Тест кабель N 4
char  txt_menu2_1[]            = "Tec""\xA4"" ""\x80""BC";                                                 // Тест БВС                                           
char  txt_menu2_2[]            = "=========";                                                              // ================      
char  txt_menu2_3[]            = "=========";                                                              // ================      
char  txt_menu2_4[]            = "=========";                                                              // ================  
char  txt_menu3_1[]            = "Ko""\xA3\x9D\xAF"" ""\x9F""a""\x96""e""\xA0\xAC"" N1";                   // Копия кабель N 1
char  txt_menu3_2[]            = "Ko""\xA3\x9D\xAF"" ""\x9F""a""\x96""e""\xA0\xAC"" N2";                   // Копия кабель N 2
char  txt_menu3_3[]            = "Ko""\xA3\x9D\xAF"" ""\x9F""a""\x96""e""\xA0\xAC"" N3";                   // Копия кабель N 3 
char  txt_menu3_4[]            = "Ko""\xA3\x9D\xAF"" ""\x9F""a""\x96""e""\xA0\xAC"" N4";                   // Копия кабель N 4
char  txt_menu4_1[]            = "Ta""\x96\xA0\x9D\xA6""a coe""\x99"".";                                   // Таблица соед.
char  txt_menu4_2[]            = "Ko""\xA3\x9D\xAF"" ""\x9F""a""\x96""e""\xA0\xAF";                        // Копия кабеля
char  txt_menu4_3[]            = "\x85""a""\x98""py""\x9C"".\xA4""a""\x96\xA0\x9D\xA6";                    // Загруз. умолч.
char  txt_menu4_4[]            = "Tec""\xA4"" pa""\x9C\xAA""e""\xA1""o""\x97";                             // Тест разъемов 
char  txt_menu5_1[]            = "=========";                                                              // ================  
char  txt_menu5_2[]            = "=========";                                                              // ================  
char  txt_menu5_3[]            = "=========";                                                              // ================  
char  txt_menu5_4[]            = "Bpe""\xA1\xAF"" ""\xA3""poc""\xA4""o""\xAF";                             // Время простоя

const char  txt_head_instr[]        PROGMEM  = "=========";                                                              // ================  
const char  txt_head_disp[]         PROGMEM  = "=========";                                                              // ================  
const char  txt_info1[]             PROGMEM  = "Tec\xA4 ""\x9F""a\x96""e\xA0""e\x9E";                                    // Тест кабелей
const char  txt_info2[]             PROGMEM  = "=========";                                                              // ================  
const char  txt_info3[]             PROGMEM  = "Tec""\xA4"" o""\x96""y""\xA7"". ""\xA4""a""\x96\xA0\x9D\xA6""ax ";       // Тест по обуч. таблицах 
const char  txt_info4[]             PROGMEM  = "Hac\xA4po\x9E\x9F""a c\x9D""c\xA4""e\xA1\xAB";                           // Настройка системы 
const char  txt_info5[]             PROGMEM  = "=========";                                                              // ================  
const char  txt_MTT[]               PROGMEM  = "=========";                                                              // ================  
const char  txt_botton_otmena[]     PROGMEM  = "O""\xA4\xA1""e""\xA2""a";                                                // "Отмена"
const char  txt_botton_vvod[]       PROGMEM  = "B\x97o\x99 ";                                                            // Ввод
const char  txt_botton_ret[]        PROGMEM  = "B""\xAB""x";                                                             // "Вых"
const char  txt_system_clear3[]     PROGMEM  = " ";                                                                      //
const char  txt9[]                  PROGMEM  = "=========";                                                              // ================  
const char  txt10[]                 PROGMEM  = "=========";                                                              // ================  
const char  txt_time_wait[]         PROGMEM  = "\xA1\x9D\xA2"".""\x97""pe""\xA1\xAF"" ""\xA3""poc""\xA4""o""\xAF";       //  мин. время простоя
const char  txt_info29[]            PROGMEM  = "Stop->PUSH Disp";
const char  txt_info30[]            PROGMEM  = " ";
const char  txt_test_all[]          PROGMEM  = "Tec""\xA4"" ""\x97""cex pa""\x9C\xAA""e""\xA1""o""\x97";                 // Тест всех разъемов
const char  txt_test_all_exit1[]    PROGMEM  = "\x82\xA0\xAF"" ""\x97\xAB""xo""\x99""a";                                 // Для выхода
const char  txt_test_all_exit2[]    PROGMEM  = "\xA3""p""\x9D\x9F""oc""\xA2\x9D""c""\xAC"" ""\x9F"" ""\xAD\x9F""pa""\xA2""y";  // прикоснись к экрану
const char  txt_test_end[]          PROGMEM  = "\x85""a""\x97""ep""\xA8\x9D\xA4\xAC";                                    // Завершить
const char  txt_test_repeat[]       PROGMEM  = "\x89""o""\x97\xA4""op""\x9D\xA4\xAC";                                    // Повторить
const char  txt_error_connect1[]    PROGMEM  = "O""\x8E\x86\x80""KA";                                                    // Ошибка
const char  txt_error_connect2[]    PROGMEM  = "\xA3""o""\x99\x9F\xA0\xAE\xA7""e""\xA2\x9D\xAF"" ""\x9F""a""\x96""e""\xA0\xAF"; //подключения кабеля
const char  txt_error_connect3[]    PROGMEM  = "O""\xA8\x9D\x96""o""\x9F"" ""\xA2""e""\xA4";                             // Ошибок нет
const char  txt_error_connect4[]    PROGMEM  = "O""\xA8\x9D\x96""o""\x9F"" -         ";                                  // Ошибок  -
const char  txt__connect1[]         PROGMEM  = "O""\x96\xA2""apy""\x9B""e""\xA2"" ""\x9F""a""\x96""e""\xA0\xAC"" N1";    // Обнаружен кабель N1
const char  txt__connect2[]         PROGMEM  = "O""\x96\xA2""apy""\x9B""e""\xA2"" ""\x9F""a""\x96""e""\xA0\xAC"" N2";    // Обнаружен кабель N2
const char  txt__connect3[]         PROGMEM  = "O""\x96\xA2""apy""\x9B""e""\xA2"" ""\x9F""a""\x96""e""\xA0\xAC"" N3";    // Обнаружен кабель N3
const char  txt__connect4[]         PROGMEM  = "O""\x96\xA2""apy""\x9B""e""\xA2"" ""\x9F""a""\x96""e""\xA0\xAC"" N4";    // Обнаружен кабель N4
const char  txt__test_end[]         PROGMEM  = "TECT ""\x85""A""KOH""\x8D""EH";                                          // ТЕСТ ЗАКОНЧЕН
const char  txt__panel[]            PROGMEM  = "=========";                                                              // ================  
const char  txt__panel0[]           PROGMEM  = "                     ";                                                  //
const char  txt__disp[]             PROGMEM  = "=========";                                                              // ================  
const char  txt__instr[]            PROGMEM  = "=========";                                                              // ================  
const char  txt__MTT[]              PROGMEM  = "=========";                                                              // ================  
const char  txt__disp_connect[]     PROGMEM  = "=========";                                                              // ================  
const char  txt__disp_disconnect[]  PROGMEM  = "=========";                                                              // ================  
const char  txt__instr_connect[]    PROGMEM  = "=========";                                                              // ================  
const char  txt__instr_disconnect[] PROGMEM  = "=========";                                                              // ================  
const char  txt__mtt_disconnect[]   PROGMEM  = "=========";                                                              // ================  
const char  txt__cont1_connect[]    PROGMEM  = "Ko""\xA2\xA4"". N1 - O""\x9F";                                            // Конт. N1 - Ок
const char  txt__cont2_connect[]    PROGMEM  = "Ko""\xA2\xA4"". N2 - O""\x9F";                                            // Конт. N2 - Ок
const char  txt__cont3_connect[]    PROGMEM  = "Ko""\xA2\xA4"". N3 - O""\x9F";                                            // Конт. N3 - Ок
const char  txt__cont4_connect[]    PROGMEM  = "Ko""\xA2\xA4"". N4 - O""\x9F";                                            // Конт. N4 - Ок
const char  txt__cont5_connect[]    PROGMEM  = "Ko""\xA2\xA4"". N5 - O""\x9F";                                            // Конт. N5 - Ок
const char  txt__cont6_connect[]    PROGMEM  = "Ko""\xA2\xA4"". N6 - O""\x9F";                                            // Конт. N6 - Ок
const char  txt__cont7_connect[]    PROGMEM  = "Ko""\xA2\xA4"". N7 - O""\x9F";                                            // Конт. N7 - Ок
const char  txt__cont8_connect[]    PROGMEM  = "Ko""\xA2\xA4"". N8 - O""\x9F";                                            // Конт. N8 - Ок
const char  txt__cont9_connect[]    PROGMEM  = "Ko""\xA2\xA4"". N9 - O""\x9F";                                            // Конт. N9 - Ок
const char  txt__clear2[]           PROGMEM  = " ";                                                                       //
const char  txt__cont1_disconnect[] PROGMEM  = "Ko""\xA2\xA4"". N1 - He""\xA4""!";                                        // Конт. N1 - Нет!
const char  txt__cont2_disconnect[] PROGMEM  = "Ko""\xA2\xA4"". N2 - He""\xA4""!";                                        // Конт. N2 - Нет!
const char  txt__cont3_disconnect[] PROGMEM  = "Ko""\xA2\xA4"". N3 - He""\xA4""!";                                        // Конт. N3 - Нет!
const char  txt__cont4_disconnect[] PROGMEM  = "Ko""\xA2\xA4"". N4 - He""\xA4""!";                                        // Конт. N4 - Нет!
const char  txt__cont5_disconnect[] PROGMEM  = "Ko""\xA2\xA4"". N5 - He""\xA4""!";                                        // Конт. N5 - Нет!
const char  txt__cont6_disconnect[] PROGMEM  = "Ko""\xA2\xA4"". N6 - He""\xA4""!";                                        // Конт. N6 - Нет!
const char  txt__cont7_disconnect[] PROGMEM  = "Ko""\xA2\xA4"". N7 - He""\xA4""!";                                        // Конт. N7 - Нет!
const char  txt__cont8_disconnect[] PROGMEM  = "Ko""\xA2\xA4"". N8 - He""\xA4""!";                                        // Конт. N8 - Нет!
const char  txt__cont9_disconnect[] PROGMEM  = "Ko""\xA2\xA4"". N9 - He""\xA4""!";                                        // Конт. N9 - Нет!
const char  txt__test_cabel_soft[]  PROGMEM  = "Tec""\xA4"" ""\x9F""o""\xA3\x9D\xAF"" ""\x9F""a""\x96""e""\xA0\xAF""N";   // Тест копия кабеля N

char buffer[40];

const char* const table_message[] PROGMEM =
{
  txt_head_instr,          // 0 "=========";                                                                // ================  
  txt_head_disp,           // 1 "=========";                                                                // ================  
  txt_info1,               // 2 "Tec\xA4 ""\x9F""a\x96""e\xA0""e\x9E";                                      // Тест кабелей
  txt_info2,               // 3 "=========";                                                                // ================  
  txt_info3,               // 4 "Tec""\xA4"" ""\xA3""o o""\x96""y""\xA7"". ""\xA4""a""\x96\xA0\x9D\xA6""ax "; // Тест по обуч. таблицах 
  txt_info4,               // 5 "Hac\xA4po\x9E\x9F""a c\x9D""c\xA4""e\xA1\xAB";                             // Настройка системы   
  txt_info5,               // 6 "=========";                                                                // ================  
  txt_MTT,                 // 7 "=========";                                                                // ================  
  txt_botton_otmena,       // 8 " ";                                                                        //
  txt_botton_vvod,         // 9 " ";                                                                        //
  txt_botton_ret,          // 10 ""B""\xAB""x" ";                                                           //  Вых
  txt_system_clear3,       // 11 " ";                                                                       //
  txt9,                    // 12 "B\x97o\x99";                                                              // Ввод
  txt10,                   // 13 "O""\xA4\xA1""e""\xA2""a";                                                 // "Отмена"
  txt_time_wait,           // 14 "\xA1\x9D\xA2"".""\x97""pe""\xA1\xAF"" ""\xA3""poc""\xA4""o""\xAF";        //  мин. время простоя
  txt_info29,              // 15 "Stop->PUSH Disp";
  txt_info30,              // 16 " ";
  txt_test_all,            // 17 "Tec""\xA4"" ""\x97""cex pa""\x9C\xAA""e""\xA1""o""\x97";                  // Тест всех разъемов
  txt_test_all_exit1,      // 18 "\x82\xA0\xAF"" ""\x97\xAB""xo""\x99""a";                                  // Для выхода
  txt_test_all_exit2,      // 19 "\xA3""p""\x9D\x9F""oc""\xA2\x9D""c""\xAC"" ""\x9F"" ""\xAD\x9F""pa""\xA2""y";  // прикоснись к экрану
  txt_test_end,            // 20 "\x85""a""\x97""ep""\xA8\x9D\xA4\xAC";                                     // Завершить
  txt_test_repeat,         // 21 "\x89""o""\x97\xA4""op""\x9D\xA4\xAC";                                     // Повторить
  txt_error_connect1,      // 22 "O""\x8E\x86\x80""KA";                                                     // Ошибка
  txt_error_connect2,      // 23 "\xA3""o""\x99\x9F\xA0\xAE\xA7""e""\xA2\x9D\xAF"" ""\x9F""a""\x96""e""\xA0\xAF"; //подключения кабеля
  txt_error_connect3,      // 24 "O""\xA8\x9D\x96""o""\x9F"" ""\xA2""e""\xA4";                              // Ошибок нет
  txt_error_connect4,      // 25 "O""\xA8\x9D\x96""o""\x9F"" -         ";                                   // Ошибок  -
  txt__connect1,           // 26 "O""\x96\xA2""apy""\x9B""e""\xA2"" ""\x9F""a""\x96""e""\xA0\xAC"" N1";     // Обнаружен кабель N1
  txt__connect2,           // 27 "O""\x96\xA2""apy""\x9B""e""\xA2"" ""\x9F""a""\x96""e""\xA0\xAC"" N2";     // Обнаружен кабель N2
  txt__connect3,           // 28 "O""\x96\xA2""apy""\x9B""e""\xA2"" ""\x9F""a""\x96""e""\xA0\xAC"" N3";     // Обнаружен кабель N3
  txt__connect4,           // 29 "O""\x96\xA2""apy""\x9B""e""\xA2"" ""\x9F""a""\x96""e""\xA0\xAC"" N4";     // Обнаружен кабель N4
  txt__test_end,           // 30 "TECT ""\x85""A""KOH""\x8D""EH";                                           // ТЕСТ ЗАКОНЧЕН
  txt__panel,              // 31 "=========";                                                               // ================  
  txt__panel0,             // 32 "                          ";                                              //
  txt__disp,               // 33 "=========";                                                               // ================  
  txt__instr,              // 34 "=========";                                                               // ================  
  txt__MTT,                // 35 "=========";                                                               // ================  
  txt__disp_connect,       // 36 "=========";                                                               // ================  
  txt__disp_disconnect,    // 37 "=========";                                                               // ================  
  txt__instr_connect,      // 38 "=========";                                                               // ================  
  txt__instr_disconnect,   // 39 "=========";                                                               // ================  
  txt__mtt_disconnect,     // 40 " ";                                                                       //
  txt__cont1_connect,      // 41 "Ko""\xA2\xA4"". N1 - O""\x9F";                                            // Конт. N1 - Ок
  txt__cont2_connect,      // 42 "Ko""\xA2\xA4"". N2 - O""\x9F";                                            // Конт. N2 - Ок
  txt__cont3_connect,      // 43 "Ko""\xA2\xA4"". N3 - O""\x9F";                                            // Конт. N3 - Ок
  txt__cont4_connect,      // 44 "Ko""\xA2\xA4"". N4 - O""\x9F";                                            // Конт. N4 - Ок
  txt__cont5_connect,      // 45 "Ko""\xA2\xA4"". N5 - O""\x9F";                                            // Конт. N5 - Ок
  txt__cont6_connect,      // 46 "Ko""\xA2\xA4"". N6 - O""\x9F";                                            // Конт. N6 - Ок
  txt__cont7_connect,      // 47 "Ko""\xA2\xA4"". N7 - O""\x9F";                                            // Конт. N7 - Ок
  txt__cont8_connect,      // 48 "Ko""\xA2\xA4"". N8 - O""\x9F";                                            // Конт. N8 - Ок
  txt__cont9_connect,      // 49 "Ko""\xA2\xA4"". N9 - O""\x9F";                                            // Конт. N9 - Ок
  txt__clear2,             // 50 " ";                                                                       //
  txt__cont1_disconnect,   // 51 "Ko""\xA2\xA4"". N1 - He""\xA4""!";                                        // Конт. N1 - Нет!
  txt__cont2_disconnect,   // 52 "Ko""\xA2\xA4"". N2 - He""\xA4""!";                                        // Конт. N2 - Нет!
  txt__cont3_disconnect,   // 53 "Ko""\xA2\xA4"". N3 - He""\xA4""!";                                        // Конт. N3 - Нет!
  txt__cont4_disconnect,   // 54 "Ko""\xA2\xA4"". N4 - He""\xA4""!";                                        // Конт. N4 - Нет!
  txt__cont5_disconnect,   // 55 "Ko""\xA2\xA4"". N5 - He""\xA4""!";                                        // Конт. N5 - Нет!
  txt__cont6_disconnect,   // 56 "Ko""\xA2\xA4"". N6 - He""\xA4""!";                                        // Конт. N6 - Нет!
  txt__cont7_disconnect,   // 57 "Ko""\xA2\xA4"". N7 - He""\xA4""!";                                        // Конт. N7 - Нет!
  txt__cont8_disconnect,   // 58 "Ko""\xA2\xA4"". N8 - He""\xA4""!";                                        // Конт. N8 - Нет!
  txt__cont9_disconnect,   // 59 "Ko""\xA2\xA4"". N9 - He""\xA4""!";                                        // Конт. N9 - Нет!
  txt__test_cabel_soft     // 60 "Tec""\xA4"" ""\x9F""o""\xA3\x9D\xAF"" ""\x9F""a""\x96""e""\xA0\xAF"" N";  // Тест копия кабеля N
};

byte   temp_buffer[40] ;                                                                                    // Буфер хранения временной информации

const byte connektN1_default[]    PROGMEM  = { 20,
											   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,                                             // Разъем А
											   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
											   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                                        // Разъем B
											   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1                                                         // 1- соединение есть, 0- соединения нет
											 }; // 20 x 5 ячеек
const byte connektN2_default[]    PROGMEM  = { 26,
											   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,                    // Разъем А
											   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,                    // Разъем B
											   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											   1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1                                      // 1- соединение есть, 0- соединения нет
											 }; // 26 x 5 ячеек
const byte connektN3_default[]    PROGMEM  = { 39,
											    1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39, // Разъем А
											   20,22,24,26,28,30,32,34,36,39, 3, 5, 7, 9,11,13,15,17,19,21,23,25,27,29,31,33,35,37, 2, 4, 6, 8,10,12,14,16,18, 0,20, // Разъем B
											    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1    // 1- соединение есть, 0- соединения нет
											 }; // 39 x 5 ячеек
const byte connektN4_default[]    PROGMEM  = { 32,
											   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, // Разъем А
											   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, // Разъем B
											   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1                     // 1- соединение есть, 0- соединения нет
											 }; // 32 x 2 ячеек

const byte connekBVS_default[]    PROGMEM = { 24,
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, // Разъем А 
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, // Разъем B
0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
1, 1, 1, 1, 1, 1, 1, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1                    // 1- соединение есть, 0- соединения нет
}; // 24 x 2 ячеек

//++++++++++++++++++ Вариант № 1 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Определение адреса производится в программе  set_adr_EEPROM()
unsigned int adr_memN1_1 = 0;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №1А, №1В
unsigned int adr_memN1_2 = 0;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №2А, №2В
unsigned int adr_memN1_3 = 0;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №3А, №3В
unsigned int adr_memN1_4 = 0;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №4А, №4В

//++++++++++++++++++ Вариант № 2 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
unsigned int adr_memN1_5 = 0;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №1А, №1В
													
// ++++++++++++++++++++++++++++++ Таблица для обучаемых вариантов кабеля ++++++++++++++++++++++++++++++++++++++++++++++++++
unsigned int adr_memN2_1 = 0;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №1А, №2В
unsigned int adr_memN2_2 = 0;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №2А, №2В
unsigned int adr_memN2_3 = 0;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №3А, №3В
unsigned int adr_memN2_4 = 0;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №4А, №4В

//==========================================================================================================================

void serial_print_date()                           // Печать даты и времени
{
  DateTime now = RTC.now();
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print("  ");
  Serial.println(str1[now.dayOfWeek() - 1]);
}
void clock_read()
{
  DateTime now = RTC.now();
  second = now.second();
  minute = now.minute();
  hour   = now.hour();
  dow    = now.dayOfWeek();
  day    = now.day();
  month  = now.month();
  year   = now.year();
}

void set_time()
{
  RTC.adjust(DateTime(__DATE__, __TIME__));
  DateTime now = RTC.now();
  second = now.second();       //Initialization time
  minute = now.minute();
  hour   = now.hour();
  day    = now.day();
  day++;
  if (day > 31)day = 1;
  month  = now.month();
  year   = now.year();
  DateTime set_time = DateTime(year, month, day, hour, minute, second); // Занести данные о времени в строку "set_time"
  RTC.adjust(set_time);
}
void i2c_eeprom_write_byte( int deviceaddress, unsigned int eeaddress, byte data )
{
  int rdata = data;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(rdata);
  Wire.endTransmission();
  delay(10);
}
byte i2c_eeprom_read_byte( int deviceaddress, unsigned int eeaddress ) {
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, 1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}
void i2c_eeprom_read_buffer( int deviceaddress, unsigned int eeaddress, byte *buffer, int length )
{

  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, length);
  int c = 0;
  for ( c = 0; c < length; c++ )
	if (Wire.available()) buffer[c] = Wire.read();

}
void i2c_eeprom_write_page( int deviceaddress, unsigned int eeaddresspage, byte* data, byte length )
{

  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddresspage >> 8)); // MSB
  Wire.write((int)(eeaddresspage & 0xFF)); // LSB
  byte c;
  for ( c = 0; c < length; c++)
	Wire.write(data[c]);
  Wire.endTransmission();

}

void drawDisplay()
{
  // Clear screen
  myGLCD.clrScr();

  // Draw Clockface
  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(0, 0, 0);
  for (int i = 0; i < 5; i++)
  {
	myGLCD.drawCircle(clockCenterX, clockCenterY, 119 - i);
  }
  for (int i = 0; i < 5; i++)
  {
	myGLCD.drawCircle(clockCenterX, clockCenterY, i);
  }

  myGLCD.setColor(192, 192, 255);
  myGLCD.print("3", clockCenterX + 92, clockCenterY - 8);
  myGLCD.print("6", clockCenterX - 8, clockCenterY + 95);
  myGLCD.print("9", clockCenterX - 109, clockCenterY - 8);
  myGLCD.print("12", clockCenterX - 16, clockCenterY - 109);
  for (int i = 0; i < 12; i++)
  {
	if ((i % 3) != 0)
	  drawMark(i);
  }
  clock_read();
  drawMin(minute);
  drawHour(hour, minute);
  drawSec(second);
  oldsec = second;

  // Draw calendar
  myGLCD.setColor(255, 255, 255);
  myGLCD.fillRoundRect(240, 0, 319, 85);
  myGLCD.setColor(0, 0, 0);
  for (int i = 0; i < 7; i++)
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

  /* myGLCD.setColor(64, 64, 128);
	myGLCD.fillRoundRect(260, 140, 319, 180);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(260, 140, 319, 180);
	myGLCD.setBackColor(64, 64, 128);
	myGLCD.print("RET", 266, 150);
	myGLCD.setBackColor(0, 0, 0);*/

}
void drawMark(int h)
{
  float x1, y1, x2, y2;

  h = h * 30;
  h = h + 270;

  x1 = 110 * cos(h * 0.0175);
  y1 = 110 * sin(h * 0.0175);
  x2 = 100 * cos(h * 0.0175);
  y2 = 100 * sin(h * 0.0175);

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

  x1 = 95 * cos(ps * 0.0175);
  y1 = 95 * sin(ps * 0.0175);
  x2 = 80 * cos(ps * 0.0175);
  y2 = 80 * sin(ps * 0.0175);

  myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);

  myGLCD.setColor(255, 0, 0);
  s = s * 6;
  s = s + 270;

  x1 = 95 * cos(s * 0.0175);
  y1 = 95 * sin(s * 0.0175);
  x2 = 80 * cos(s * 0.0175);
  y2 = 80 * sin(s * 0.0175);

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

  x1 = 80 * cos(pm * 0.0175);
  y1 = 80 * sin(pm * 0.0175);
  x2 = 5 * cos(pm * 0.0175);
  y2 = 5 * sin(pm * 0.0175);
  x3 = 30 * cos((pm + 4) * 0.0175);
  y3 = 30 * sin((pm + 4) * 0.0175);
  x4 = 30 * cos((pm - 4) * 0.0175);
  y4 = 30 * sin((pm - 4) * 0.0175);

  myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY);
  myGLCD.drawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
  myGLCD.drawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY);
  myGLCD.drawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY);

  myGLCD.setColor(0, 255, 0);
  m = m * 6;
  m = m + 270;

  x1 = 80 * cos(m * 0.0175);
  y1 = 80 * sin(m * 0.0175);
  x2 = 5 * cos(m * 0.0175);
  y2 = 5 * sin(m * 0.0175);
  x3 = 30 * cos((m + 4) * 0.0175);
  y3 = 30 * sin((m + 4) * 0.0175);
  x4 = 30 * cos((m - 4) * 0.0175);
  y4 = 30 * sin((m - 4) * 0.0175);

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

  x1 = 60 * cos(ph * 0.0175);
  y1 = 60 * sin(ph * 0.0175);
  x2 = 5 * cos(ph * 0.0175);
  y2 = 5 * sin(ph * 0.0175);
  x3 = 20 * cos((ph + 5) * 0.0175);
  y3 = 20 * sin((ph + 5) * 0.0175);
  x4 = 20 * cos((ph - 5) * 0.0175);
  y4 = 20 * sin((ph - 5) * 0.0175);

  myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY);
  myGLCD.drawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
  myGLCD.drawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY);
  myGLCD.drawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY);

  myGLCD.setColor(255, 255, 0);
  h = (h * 30) + (m / 2);
  h = h + 270;

  x1 = 60 * cos(h * 0.0175);
  y1 = 60 * sin(h * 0.0175);
  x2 = 5 * cos(h * 0.0175);
  y2 = 5 * sin(h * 0.0175);
  x3 = 20 * cos((h + 5) * 0.0175);
  y3 = 20 * sin((h + 5) * 0.0175);
  x4 = 20 * cos((h - 5) * 0.0175);
  y4 = 20 * sin((h - 5) * 0.0175);

  myGLCD.drawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY);
  myGLCD.drawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
  myGLCD.drawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY);
  myGLCD.drawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY);
}
void printDate()
{
  clock_read();
  myGLCD.setFont(BigFont);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(255, 255, 255);

  myGLCD.print(str[dow - 1], 256, 8);
  if (day < 10)
	myGLCD.printNumI(day, 272, 28);
  else
	myGLCD.printNumI(day, 264, 28);

  myGLCD.print(str_mon[month - 1], 256, 48);
  myGLCD.printNumI(year, 248, 65);
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
  while (true)
  {
	if (oldsec != second)
	{
	  if ((second == 0) && (minute == 0) && (hour == 0))
	  {
		clearDate();
		printDate();
	  }
	  if (second == 0)
	  {
		drawMin(minute);
		drawHour(hour, minute);
	  }
	  drawSec(second);
	  oldsec = second;
	  wait_time_Old =  millis();
	}

	if (myTouch.dataAvailable())
	{
	  myTouch.read();
	  x = myTouch.getX();
	  y = myTouch.getY();
	  if (((y >= 200) && (y <= 239)) && ((x >= 260) && (x <= 319))) //установка часов
	  {
		myGLCD.setColor (255, 0, 0);
		myGLCD.drawRoundRect(260, 200, 319, 239);
		setClock();
	  }

	  if (((y >= 1) && (y <= 239)) && ((x >= 1) && (x <= 260))) //Возврат
	  {
		myGLCD.clrScr();
		myGLCD.setFont(BigFont);
		break;
	  }
	  if (((y >= 1) && (y <= 180)) && ((x >= 260) && (x <= 319))) //Возврат
	  {
		myGLCD.clrScr();
		myGLCD.setFont(BigFont);
		break;
	  }
	}
	delay(10);
	clock_read();
  }
}

void flash_time()                                              // Программа обработчик прерывания
{
  // PORTB = B00000000; // пин 12 переводим в состояние LOW
  slave.run();
  // PORTB = B01000000; // пин 12 переводим в состояние HIGH
}
void serialEvent3()
{
  control_command();
}

void reset_klav()
{
  myGLCD.clrScr();
  myButtons.deleteAllButtons();
  but1 = myButtons.addButton( 10,  20, 250,  35, txt_menu5_1);
  but2 = myButtons.addButton( 10,  65, 250,  35, txt_menu5_2);
  but3 = myButtons.addButton( 10, 110, 250,  35, txt_menu5_3);
  but4 = myButtons.addButton( 10, 155, 250,  35, txt_menu5_4);
  butX = myButtons.addButton(279, 199,  40,  40, "W", BUTTON_SYMBOL); // кнопка Часы
  but_m1 = myButtons.addButton(  10, 199, 45,  40, "1");
  but_m2 = myButtons.addButton(  61, 199, 45,  40, "2");
  but_m3 = myButtons.addButton(  112, 199, 45,  40, "3");
  but_m4 = myButtons.addButton(  163, 199, 45,  40, "4");
  but_m5 = myButtons.addButton(  214, 199, 45,  40, "5");

}

void klav123() // ввод данных с цифровой клавиатуры
{
  ret = 0;

  while (true)
  {
	if (myTouch.dataAvailable())
	{
	  myTouch.read();
	  x = myTouch.getX();
	  y = myTouch.getY();

	  if ((y >= 10) && (y <= 60)) // Upper row
	  {
		if ((x >= 10) && (x <= 60)) // Button: 1
		{
		  waitForIt(10, 10, 60, 60);
		  updateStr('1');
		}
		if ((x >= 70) && (x <= 120)) // Button: 2
		{
		  waitForIt(70, 10, 120, 60);
		  updateStr('2');
		}
		if ((x >= 130) && (x <= 180)) // Button: 3
		{
		  waitForIt(130, 10, 180, 60);
		  updateStr('3');
		}
		if ((x >= 190) && (x <= 240)) // Button: 4
		{
		  waitForIt(190, 10, 240, 60);
		  updateStr('4');
		}
		if ((x >= 250) && (x <= 300)) // Button: 5
		{
		  waitForIt(250, 10, 300, 60);
		  updateStr('5');
		}
	  }

	  if ((y >= 70) && (y <= 120)) // Center row
	  {
		if ((x >= 10) && (x <= 60)) // Button: 6
		{
		  waitForIt(10, 70, 60, 120);
		  updateStr('6');
		}
		if ((x >= 70) && (x <= 120)) // Button: 7
		{
		  waitForIt(70, 70, 120, 120);
		  updateStr('7');
		}
		if ((x >= 130) && (x <= 180)) // Button: 8
		{
		  waitForIt(130, 70, 180, 120);
		  updateStr('8');
		}
		if ((x >= 190) && (x <= 240)) // Button: 9
		{
		  waitForIt(190, 70, 240, 120);
		  updateStr('9');
		}
		if ((x >= 250) && (x <= 300)) // Button: 0
		{
		  waitForIt(250, 70, 300, 120);
		  updateStr('0');
		}
	  }
	  if ((y >= 130) && (y <= 180)) // Upper row
	  {
		if ((x >= 10) && (x <= 130)) // Button: Clear
		{
		  waitForIt(10, 130, 120, 180);
		  stCurrent[0] = '\0';
		  stCurrentLen = 0;
		  myGLCD.setColor(0, 0, 0);
		  myGLCD.fillRect(0, 224, 319, 239);
		}
		if ((x >= 250) && (x <= 300)) // Button: Exit
		{
		  waitForIt(250, 130, 300, 180);
		  myGLCD.clrScr();
		  myGLCD.setBackColor(VGA_BLACK);
		  ret = 1;
		  stCurrent[0] = '\0';
		  stCurrentLen = 0;
		  break;
		}
		if ((x >= 130) && (x <= 240)) // Button: Enter
		{
		  waitForIt(130, 130, 240, 180);
		  if (stCurrentLen > 0)
		  {
			for (x = 0; x < stCurrentLen + 1; x++)
			{
			  stLast[x] = stCurrent[x];
			}
			stCurrent[0] = '\0';
			stLast[stCurrentLen + 1] = '\0';
			//i2c_eeprom_write_byte(deviceaddress,adr_stCurrentLen1,stCurrentLen);
			stCurrentLen1 = stCurrentLen;
			stCurrentLen = 0;
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRect(0, 200, 319, 239);
			myGLCD.setColor(0, 255, 0);
			myGLCD.print(stLast, LEFT, 208);
			break;
		  }
		  else
		  {
			myGLCD.setColor(255, 0, 0);
			myGLCD.print("\x80\x8A\x8B\x8B""EP \x89\x8A""CTO\x87!", CENTER, 192);//"БУФФЕР ПУСТОЙ!"
			delay(500);
			myGLCD.print("                ", CENTER, 192);
			delay(500);
			myGLCD.print("\x80\x8A\x8B\x8B""EP \x89\x8A""CTO\x87!", CENTER, 192);//"БУФФЕР ПУСТОЙ!"
			delay(500);
			myGLCD.print("                ", CENTER, 192);
			myGLCD.setColor(0, 255, 0);
		  }
		}
	  }
	}
  }
}
void drawButtons1() // Отображение цифровой клавиатуры
{
  // Draw the upper row of buttons
  for (x = 0; x < 5; x++)
  {
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect (10 + (x * 60), 10, 60 + (x * 60), 60);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect (10 + (x * 60), 10, 60 + (x * 60), 60);
	myGLCD.printNumI(x + 1, 27 + (x * 60), 27);
  }
  // Draw the center row of buttons
  for (x = 0; x < 5; x++)
  {
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect (10 + (x * 60), 70, 60 + (x * 60), 120);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect (10 + (x * 60), 70, 60 + (x * 60), 120);
	if (x < 4)
	  myGLCD.printNumI(x + 6, 27 + (x * 60), 87);
  }

  myGLCD.print("0", 267, 87);
  // Draw the lower row of buttons
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (10, 130, 120, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 130, 120, 180);
  strcpy_P(buffer,(char*)pgm_read_word(&(table_message[8])));
  myGLCD.print(buffer, 20, 147);                                   // "Отмена"


  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (130, 130, 240, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (130, 130, 240, 180);
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[9])));
  myGLCD.print(buffer, 155, 147);                                  // "Ввод"


  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (250, 130, 300, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (250, 130, 300, 180);
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[10])));
  myGLCD.print(buffer, 252, 147);                                  // Вых
  myGLCD.setBackColor (0, 0, 0);
}
void updateStr(int val)
{
  if (stCurrentLen < 20)
  {
	stCurrent[stCurrentLen] = val;
	stCurrent[stCurrentLen + 1] = '\0';
	stCurrentLen++;
	myGLCD.setColor(0, 255, 0);
	myGLCD.print(stCurrent, LEFT, 224);
  }
  else
  { // Вывод строки "ПЕРЕПОЛНЕНИЕ!"
	myGLCD.setColor(255, 0, 0);
	myGLCD.print("\x89""EPE""\x89O\x88HEH\x86""E!", CENTER, 224);// ПЕРЕПОЛНЕНИЕ!
	delay(500);
	myGLCD.print("              ", CENTER, 224);
	delay(500);
	myGLCD.print("\x89""EPE""\x89O\x88HEH\x86""E!", CENTER, 224);// ПЕРЕПОЛНЕНИЕ!
	delay(500);
	myGLCD.print("              ", CENTER, 224);
	myGLCD.setColor(0, 255, 0);
  }
}
void waitForIt(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
  myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}

void control_command()
{
  /*
	Для вызова подпрограммы проверки необходимо записать номер проверки по адресу adr_control_command (40120)
	Код проверки
	0 -   Выполнение команды окончено
	1 -   Программа проверки кабеля №1
	2 -   Программа проверки кабеля №2
	3 -   Программа проверки кабеля №3
	4 -   Программа проверки кабеля №4
	5 -   Программа проверки панели гарнитур
	6 -   Записать таблицу проверки №1 по умолчанию
	7 -   Записать таблицу проверки №2 по умолчанию
	8 -   Записать таблицу проверки №3 по умолчанию
	9 -   Записать таблицу проверки №4 по умолчанию
	10 -  Установить уровень сигнала резистором №1
	11 -  Установить уровень сигнала резистором №2
	12 -  Чтение таблиц из EEPROM для передачи в ПК
	13 -  Получить таблицу из ПK и записать в EEPROM
	14 -
	15 -
	16 -
	17 -
	18 -
	19 -
	20 -
	21 -
	22 -
	23 -
	24 -
	25 -
	26 -
	27 -
	28 -
	29 -
	30 -

  */


  int test_n = regBank.get(adr_control_command);   //адрес  40000
  if (test_n != 0)
  {
	if (test_n != 0) Serial.println(test_n);
	switch (test_n)
	{
	  case 1:
		test_cabel_N1();             // Программа проверки кабеля №1
		break;
	  case 2:
		test_cabel_N2();             // Программа проверки кабеля №2
		break;
	  case 3:
		test_cabel_N3();             // Программа проверки кабеля №3
		break;
	  case 4:
		test_cabel_N4();             // Программа проверки кабеля №4
		break;
	  case 5:
		break;
	  case 6:
		save_default_pc();           // Записать таблицу проверки № по умолчанию
		break;
	  case 7:

		break;
	  case 8:

		break;
	  case 9:

		break;
	  case 10:
 
		break;
	  case 11:
 
		break;
	  case 12:
		mem_byte_trans_readPC();     // Чтение таблиц из EEPROM для передачи в ПК
		break;
	  case 13:
		mem_byte_trans_savePC();     // Получить таблицу из ПK и записать в EEPROM
		break;
	  case 14:
		//
		break;
	  case 15:
		//
		break;
	  case 16:
		//
		break;
	  case 17:
		//
		break;
	  case 18:
		//
		break;
	  case 19:
		//
		break;
	  case 20:                                         //
		//
		break;
	  case 21:                      		 		     //
		//
		break;
	  case 22:                                         //
		//
		break;
	  case 23:
		//
		break;
	  case 24:
		//
		break;
	  case 25:
		//
		break;
	  case 26:
		//
		break;
	  case 27:
		//
		break;
	  case 28:
		//
		break;
	  case 29:
		//
		break;
	  case 30:
		//
		break;

	  default:
		regBank.set(adr_control_command, 0);       // Установить резистором №1,№2  уровень сигнала
		break;
	}

  }
  else
  {
	regBank.set(adr_control_command, 0);
  }
}

void draw_Glav_Menu()
{
  but1   = myButtons.addButton( 10,  20, 250,  35, txt_menu1_1);
  but2   = myButtons.addButton( 10,  65, 250,  35, txt_menu1_2);
  but3   = myButtons.addButton( 10, 110, 250,  35, txt_menu1_3);
  but4   = myButtons.addButton( 10, 155, 250,  35, txt_menu1_4);
  butX   = myButtons.addButton( 279, 199,  40,  40, "W", BUTTON_SYMBOL); // кнопка Часы
  but_m1 = myButtons.addButton(  10, 199, 45,  40, "1");
  but_m2 = myButtons.addButton(  61, 199, 45,  40, "2");
  but_m3 = myButtons.addButton(  112, 199, 45,  40, "3");
  but_m4 = myButtons.addButton(  163, 199, 45,  40, "4");
  but_m5 = myButtons.addButton(  214, 199, 45,  40, "5");
  myButtons.drawButtons(); // Восстановить кнопки
  myGLCD.setColor(VGA_BLACK);
  myGLCD.setBackColor(VGA_WHITE);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("                      ", CENTER, 0);

  switch (m2)
  {
	case 1:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[2])));
	  myGLCD.print(buffer, CENTER, 0);                               // txt_info1
	  break;
	case 2:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[3])));
	  myGLCD.print(buffer, CENTER, 0);                               // txt_info2
	  break;
	case 3:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[4])));
	  myGLCD.print(buffer, CENTER, 0);                               // txt_info3
	  break;
	case 4:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[5])));
	  myGLCD.print(buffer, CENTER, 0);                               // txt_info4
	  break;
	case 5:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[6])));
	  myGLCD.print(buffer, CENTER, 0);                               // txt_info5
	  break;
  }
}

void swichMenu()                                             // Тексты меню в строках "txt....."
{
  m2 = 1;                                                    // Устанивить первую странице меню
  while (1)
  {
	wait_time = millis();                                    // Программа вызова часов при простое
	if (wait_time - wait_time_Old > 60000 * time_minute)
	{
	  wait_time_Old =  millis();
	  AnalogClock();
	  myGLCD.clrScr();
	  myButtons.drawButtons();                               // Восстановить кнопки
	  print_up();                                            // Восстановить верхнюю строку
	}

	myButtons.setTextFont(BigFont);                          // Установить Большой шрифт кнопок

	if (myTouch.dataAvailable() == true)                     // Проверить нажатие кнопок
	{
	  pressed_button = myButtons.checkButtons();             // Если нажата - проверить что нажато
	  wait_time_Old =  millis();

	  if (pressed_button == butX)                            // Нажата - вызов часы
	  {
		AnalogClock();
		myGLCD.clrScr();
		myButtons.drawButtons();                             // Восстановить кнопки
		print_up();                                          // Восстановить верхнюю строку
	  }

	  if (pressed_button == but_m1)                          // Нажата 1 страница меню
	  {
		myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_BLUE); // Голубой фон меню
		myButtons.drawButtons();                             // Восстановить кнопки
		default_colors = true;
		m2 = 1;                                              // Устанивить первую странице меню
		myButtons.relabelButton(but1, txt_menu1_1, m2 == 1);
		myButtons.relabelButton(but2, txt_menu1_2, m2 == 1);
		myButtons.relabelButton(but3, txt_menu1_3, m2 == 1);
		myButtons.relabelButton(but4, txt_menu1_4, m2 == 1);
		myGLCD.setColor(0, 255, 0);
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print("                      ", CENTER, 0);
		strcpy_P(buffer, (char*)pgm_read_word(&(table_message[2])));
		myGLCD.print(buffer, CENTER, 0);                     // txt_info1 "Тест кабелей"

	  }
	  if (pressed_button == but_m2)
	  {
		myButtons.setButtonColors(VGA_WHITE, VGA_RED, VGA_YELLOW, VGA_BLUE, VGA_TEAL);
		myButtons.drawButtons();
		default_colors = false;
		m2 = 2;
		myButtons.relabelButton(but1, txt_menu2_1 , m2 == 2);
		myButtons.relabelButton(but2, txt_menu2_2 , m2 == 2);
		myButtons.relabelButton(but3, txt_menu2_3 , m2 == 2);
		myButtons.relabelButton(but4, txt_menu2_4 , m2 == 2);
		myGLCD.setColor(0, 255, 0);
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print("                      ", CENTER, 0);
		strcpy_P(buffer, (char*)pgm_read_word(&(table_message[3])));
		myGLCD.print(buffer, CENTER, 0);                     // txt_info2 Тест блока гарнитур
	  }

	  if (pressed_button == but_m3)
	  {
		myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_GREEN);
		myButtons.drawButtons();
		default_colors = false;
		m2 = 3;
		myButtons.relabelButton(but1, txt_menu3_1 , m2 == 3);
		myButtons.relabelButton(but2, txt_menu3_2 , m2 == 3);
		myButtons.relabelButton(but3, txt_menu3_3 , m2 == 3);
		myButtons.relabelButton(but4, txt_menu3_4 , m2 == 3);
		myGLCD.setColor(0, 255, 0);
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print("                      ", CENTER, 0);
		strcpy_P(buffer, (char*)pgm_read_word(&(table_message[4])));
		myGLCD.print(buffer, CENTER, 0);                     // txt_info3 Настройка системы
	  }
	  if (pressed_button == but_m4)
	  {
		myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_RED);
		myButtons.drawButtons();
		default_colors = false;
		m2 = 4;
		myButtons.relabelButton(but1, txt_menu4_1 , m2 == 4);
		myButtons.relabelButton(but2, txt_menu4_2 , m2 == 4);
		myButtons.relabelButton(but3, txt_menu4_3 , m2 == 4);
		myButtons.relabelButton(but4, txt_menu4_4 , m2 == 4);
		myGLCD.setColor(0, 255, 0);
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print("                      ", CENTER, 0);
		strcpy_P(buffer, (char*)pgm_read_word(&(table_message[5])));
		myGLCD.print(buffer, CENTER, 0);                     // txt_info4 
	  }

	  if (pressed_button == but_m5)
	  {
		myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_NAVY);
		myButtons.drawButtons();
		default_colors = false;
		m2 = 5;
		myButtons.relabelButton(but1, txt_menu5_1 , m2 == 5);
		myButtons.relabelButton(but2, txt_menu5_2 , m2 == 5);
		myButtons.relabelButton(but3, txt_menu5_3 , m2 == 5);
		myButtons.relabelButton(but4, txt_menu5_4 , m2 == 5);
		myGLCD.setColor(0, 255, 0);
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print("                      ", CENTER, 0);
		strcpy_P(buffer, (char*)pgm_read_word(&(table_message[6])));
		myGLCD.print(buffer, CENTER, 0);                     // txt_info5  Осциллограф
	  }

	  //*****************  Меню №1  **************

	  if (pressed_button == but1 && m2 == 1)
	  {
		// Тест кабеля №1
		myGLCD.clrScr();                                     // Очистить экран
		test_cabel_N1();
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }

	  if (pressed_button == but2 && m2 == 1)
	  {
		// Тест кабеля №2
		myGLCD.clrScr();                                     // Очистить экран
		test_cabel_N2();
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }

	  if (pressed_button == but3 && m2 == 1)
	  {
		// Тест кабеля №3
		myGLCD.clrScr();                                     // Очистить экран
		test_cabel_N3();
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }
	  if (pressed_button == but4 && m2 == 1)
	  {
		// Тест кабеля №4
		myGLCD.clrScr();                                     // Очистить экран
		test_cabel_N4();
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }

	  //*****************  Меню №2  **************


	  if (pressed_button == but1 && m2 == 2)
	  {
		myGLCD.clrScr();//Тест панели БВС
		test_BVS();
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }

	  if (pressed_button == but2 && m2 == 2)
	  {
		//Тест панели гарнитур 2
 
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }

	  if (pressed_button == but3 && m2 == 2)
	  {
 
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }
	  if (pressed_button == but4 && m2 == 2)
	  {

		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }

	  //*****************  Меню №3  **************
	  if (pressed_button == but1 && m2 == 3)                 // Первый пункт меню 3
	  {
		 test_cabel_soft(1);
		 myGLCD.clrScr();                                    // Очистить экран
		 myButtons.drawButtons();
		 print_up();
	  }

	  //--------------------------------------------------------------
	  if (pressed_button == but2 && m2 == 3)                 // Второй пункт меню 3
	  {
		test_cabel_soft(2);
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }

	  //------------------------------------------------------------------

	  if (pressed_button == but3 && m2 == 3)                 // Третий пункт меню 3
	  {
		test_cabel_soft(3);
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }

	  //------------------------------------------------------------------
	  if (pressed_button == but4 && m2 == 3)                 // Четвертый пункт меню 3
	  {
		test_cabel_soft(4);
		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }

	  //*****************  Меню №4  **************

	  if (pressed_button == but1 && m2 == 4) //
	  {
		  table_cont();
		  myGLCD.clrScr();                                    // Очистить экран
		  myButtons.drawButtons();
		  print_up();
	  }

	  if (pressed_button == but2 && m2 == 4)
	  {
		  myGLCD.clrScr();                                     // Очистить экран
		  copy_cable();
		  myGLCD.clrScr();                                     // Очистить экран
		  myButtons.drawButtons();
		  print_up();;
	  }

	  if (pressed_button == but3 && m2 == 4) //
	  {
		  myGLCD.clrScr();
		  save_tab_def();
		  myGLCD.clrScr();
		  myButtons.drawButtons();
		  print_up();
	  }
	  if (pressed_button == but4 && m2 == 4) //
	  {
		  // Тест входов всех разъемов
		  test_all_pin();
		  myGLCD.clrScr();
		  myButtons.drawButtons();
		  print_up();
	  }
	  //*****************  Меню №5  **************

	  if (pressed_button == but1 && m2 == 5)                 // Сброс данных
	  {
		myGLCD.clrScr();

		myGLCD.clrScr();
		myButtons.drawButtons();
		print_up();
	  }
	  if (pressed_button == but2 && m2 == 5)
	  {
		myGLCD.clrScr();

		myGLCD.clrScr();                                     // Очистить экран
		delay (500);
		myButtons.drawButtons();
		print_up();
	  }

	  if (pressed_button == but3 && m2 == 5)                 // Ввод 
	  {
		  myGLCD.clrScr();
		  myButtons.drawButtons();
		  print_up();
	  }

	  if (pressed_button == but4 && m2 == 5) //
	  {
		  myGLCD.clrScr();
		  myGLCD.setFont(BigFont);
		  myGLCD.setBackColor(0, 0, 255);
		  myGLCD.clrScr();
		  drawButtons1();                                      // Нарисовать цифровую клавиатуру
		  myGLCD.printNumI(time_minute, LEFT, 208);
		  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[14])));
		  myGLCD.print(buffer, 35, 208);                       // txt_time_wait
		  klav123();                                           // Считать информацию с клавиатуры
		  if (ret == 1)                                        // Если "Возврат" - закончить
		  {
			  goto bailout41;                                  // Перейти на окончание выполнения пункта меню
		  }
		  else                                                 // Иначе выполнить пункт меню
		  {
			  time_minute = atol(stLast);
		  }
	  bailout41:                                               // Восстановить пункты меню
		  myGLCD.clrScr();
		  myButtons.drawButtons();
		  print_up();
	  }

	  if (pressed_button == -1)
	  {
		//  myGLCD.print("HET", 220, 220);
	  }
	}
  }
}
void print_up() // Печать верхней строчки над меню
{
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("                      ", CENTER, 0);
  switch (m2)
  {
	case 1:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[2])));
	  myGLCD.print(buffer, CENTER, 0);                                 // txt_info1
	  break;
	case 2:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[3])));
	  myGLCD.print(buffer, CENTER, 0);                                 // txt_info2
	  break;
	case 3:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[4])));
	  myGLCD.print(buffer, CENTER, 0);                                 // txt_info3
	  break;
	case 4:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[5])));
	  myGLCD.print(buffer, CENTER, 0);                                 // txt_info4
	  break;
	case 5:
	  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[6])));
	  myGLCD.print(buffer, CENTER, 0);                                 // txt_info5
	  break;
  }
}

void save_tab_def()                        // Запись в EEPROM  таблиц проверки по умолчанию
{
  for (int i = 1; i < 6; i++)
  {
	myGLCD.setColor(255, 255, 255);
	myGLCD.print("Save block N ", 25, 70);//
	myGLCD.printNumI(i, 230, 70);
	save_default(i);                 //Подпрограмма записи в EEPROM блока таблиц проверки по умолчанию

	for (int x = 10; x < 65; x++)
	{
	  myGLCD.setColor(0, 0, 255);
	  myGLCD.fillRoundRect (30, 100, 30 + (x * 4), 110);
	  myGLCD.setColor(255, 255, 255);
	  myGLCD.drawRoundRect (28, 98, 32 + (x * 4), 112);
	}
	myGLCD.clrScr();
  }
}
void save_default(byte adrN_eeprom)                                               //Подпрограмма записи в EEPROM блока таблиц проверки по умолчанию
{
  byte _u_konnekt     = 0;                                                      // Временное хранения содержимого регистра.
  byte _step_mem      = 0;                                                      // Длина блока с таблицы
  int adr_memN        = 0;
  int connekt_default = 0;                                                      // Адрес в постоянной памяти
  switch (adrN_eeprom)
  {
	case 1:
	  adr_memN   = adr_memN1_1;                                        // Адрес блока EEPROM № 1
	  _step_mem  = (pgm_read_byte_near(connektN1_default));            // Длина блока с таблицы
	  for (int i = 0; i < (_step_mem * 5) + 1; i++)                    // Записать 5 блоков таблицы
	  {
		_u_konnekt = pgm_read_byte_near(connektN1_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
	case 2:
	  adr_memN   = adr_memN1_2;                                        // Адрес блока EEPROM № 2
	  _step_mem  = (pgm_read_byte_near(connektN2_default));            // Длина блока с таблицы
	  for (int i = 0; i < (_step_mem * 5) + 1; i++)                    // Записать 5 блоков таблицы
	  {
		_u_konnekt = pgm_read_byte_near(connektN2_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
	case 3:
	  adr_memN   = adr_memN1_3;                                       // Адрес блока EEPROM № 3
	  _step_mem  = (pgm_read_byte_near(connektN3_default));           // Длина блока с таблицы
	  for (int i = 0; i < (_step_mem * 5) + 1; i++)                   // Записать 5 блоков таблицы
	  {
		_u_konnekt = pgm_read_byte_near(connektN3_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
	case 4:
	  adr_memN   = adr_memN1_4;                                       // Адрес блока EEPROM № 4
	  _step_mem  = (pgm_read_byte_near(connektN4_default));           // Длина блока с таблицы
	  for (int i = 0; i < (_step_mem * 5) + 1; i++)                   // Записать 5 блоков таблицы
	  {
		_u_konnekt = pgm_read_byte_near(connektN4_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
	case 5:
		adr_memN = adr_memN1_5;                                       // Адрес блока EEPROM № 4
		_step_mem = (pgm_read_byte_near(connekBVS_default));          // Длина блока с таблицы
		for (int i = 0; i < (_step_mem * 5) + 1; i++)                 // Записать 5 блоков таблицы
		{
			_u_konnekt = pgm_read_byte_near(connekBVS_default + i);
			i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
		}
		break;
	default:
	  adr_memN   = adr_memN1_1;                                       // Адрес блока EEPROM № 1
	  _step_mem  = (pgm_read_byte_near(connektN1_default));           // Длина блока с таблицы
	  for (int i = 0; i < (_step_mem * 5) + 1; i++)                   // Записать 5 блоков таблицы
	  {
		_u_konnekt = pgm_read_byte_near(connektN1_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
  }
}
void save_default_pc()                                                       // Запись заводских установок таблицы разъемов №1
{
  int _step_mem       = 0;                                                 // Длина блока с таблицы
  byte _u_konnekt     = 0;                                                 // Временное хранения содержимого регистра.
  int adr_memN        = 0;
  int adrN_eeprom     = regBank.get(40008);                                // Получить номер таблицы из регистра

  switch (adrN_eeprom)
  {
	case 1:
	  adr_memN = adr_memN1_1;                                     // Адрес блока EEPROM № 1
	  _step_mem = (pgm_read_byte_near(connektN1_default));        // Длина блока с таблицы
	  for (int i = 1; i < (_step_mem * 2) + 1; i++)
	  {
		_u_konnekt = pgm_read_byte_near(connektN1_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
	case 2:
	  adr_memN = adr_memN1_2;                                     // Адрес блока EEPROM № 2
	  _step_mem = (pgm_read_byte_near(connektN2_default));        // Длина блока с таблицы
	  for (int i = 1; i < (_step_mem * 2) + 1; i++)
	  {
		_u_konnekt = pgm_read_byte_near(connektN2_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
	case 3:
	  adr_memN = adr_memN1_3;                                     // Адрес блока EEPROM № 3
	  _step_mem = (pgm_read_byte_near(connektN3_default));        // Длина блока с таблицы
	  for (int i = 1; i < (_step_mem * 2) + 1; i++)
	  {
		_u_konnekt = pgm_read_byte_near(connektN3_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
	case 4:
	  adr_memN = adr_memN1_4;                                     // Адрес блока EEPROM № 4
	  _step_mem = (pgm_read_byte_near(connektN4_default));        // Длина блока с таблицы
	  for (int i = 1; i < (_step_mem * 2) + 1; i++)
	  {
		_u_konnekt = pgm_read_byte_near(connektN4_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
	default:
	  adr_memN = adr_memN1_1;                                     // Адрес блока EEPROM № 1
	  _step_mem = (pgm_read_byte_near(connektN1_default));        // Длина блока с таблицы
	  for (int i = 1; i < (_step_mem * 2) + 1; i++)
	  {
		_u_konnekt = pgm_read_byte_near(connektN1_default + i);
		i2c_eeprom_write_byte(deviceaddress, adr_memN + i, _u_konnekt);
	  }
	  break;
  }
  regBank.set(adr_control_command, 0);                                     // Завершить программу
}

void set_komm_mcp(char chanal_a_b, int chanal_n, char chanal_in_out ) // Программа включения аналового канала
{
  char _chanal_a_b     = chanal_a_b;                                  // Блок каналов коммутаторов  
																	  // А - блок U13,U17,U23 - работают на вход/выход. U14,U19,U21 - подключают канал к общему проводу.
																	  // В - блок U15,U18,U22 - работают на вход/выход. U16,U20,U24 - подключают канал к общему проводу.

  int _chanal_n        = chanal_n;                                    // Выбор № канала (1- 48).
  int _chanal_in_out   = chanal_in_out;                               // Вариант включения блока канала: 'O' - сигнальный, 'G' - подключить на общий(заземлить).

  if (_chanal_a_b == 'A')                                             // Установка каналов А
  {
	if (_chanal_in_out == 'O')                                        // Установка  аналового канала А  на вход/выход
	{
	  mcp_Out1.digitalWrite(8,  HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E1  U13
	  mcp_Out1.digitalWrite(9,  HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E2  U17
	  mcp_Out1.digitalWrite(10, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E3  U23
	  if (_chanal_n < 17)
	  {
		set_mcp_byte_1a(_chanal_n - 1);                               // Сформировать байт выбора канала (0 - 15)
		mcp_Out1.digitalWrite(8, LOW);                                // Выбрать EN микросхемы аналового коммутатора  1E1  U13
	  }
	  else if (_chanal_n > 16 && _chanal_n < 33)
	  {
		set_mcp_byte_1a(_chanal_n - 17);                              //  Сформировать байт выбора канала (15 - 31)
		mcp_Out1.digitalWrite(9, LOW);                                // Выбрать EN микросхемы аналового коммутатора  1E2  U17
	  }
	  else if (_chanal_n > 32 && _chanal_n < 49)
	  {
		set_mcp_byte_1a(_chanal_n - 33);                              // Сформировать байт выбора канала (32 - 48)
		mcp_Out1.digitalWrite(10, LOW);                               // Выбрать EN микросхемы аналового коммутатора  1E3  U23
	  }

	}
	if (_chanal_in_out == 'G')                                        // Заземлить канал А
	{
	  mcp_Out1.digitalWrite(11, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E4  U14
	  mcp_Out1.digitalWrite(12, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E5  U19
	  mcp_Out1.digitalWrite(13, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E6  U21
	  if (_chanal_n < 17)
	  {
		set_mcp_byte_1b(_chanal_n - 1);                               // Сформировать байт выбора канала (0 - 15)
		mcp_Out1.digitalWrite(11, LOW);                               // Выбрать  EN микросхемы аналового коммутатора  1E4  U14
	  }
	  else if (_chanal_n > 16 && _chanal_n < 33)
	  {
		set_mcp_byte_1b(_chanal_n - 17);                              // Сформировать байт выбора канала (16 - 31)
		mcp_Out1.digitalWrite(12, LOW);                               // Выбрать EN микросхемы аналового коммутатора  1E5  U19
	  }
	  else if (_chanal_n > 32 && _chanal_n < 49)
	  {
		set_mcp_byte_1b(_chanal_n - 33);                              // Сформировать байт выбора канала (32 - 48)
		mcp_Out1.digitalWrite(13, LOW);                               // Выбрать  EN микросхемы аналового коммутатора  1E6  U21
	  }

	}
  }
  else if (_chanal_a_b == 'B')                                        // Установка каналов В
  {
	if (_chanal_in_out == 'O')                                        // Установка  аналового канала А  на вход/выход
	{
	  mcp_Out2.digitalWrite(8,  HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E1  U15
	  mcp_Out2.digitalWrite(9,  HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E2  U18
	  mcp_Out2.digitalWrite(10, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E3  U22
	  if (_chanal_n < 17)
	  {
		set_mcp_byte_2a(_chanal_n - 1);                               // Сформировать байт выбора канала (0 - 15)
		mcp_Out2.digitalWrite(8, LOW);                                // Выбрать EN микросхемы аналового коммутатора  2E1  U15
	  }
	  else if (_chanal_n > 16 && _chanal_n < 33)
	  {
		set_mcp_byte_2a(_chanal_n - 17);                              // Сформировать байт выбора канала (16 - 31)
		mcp_Out2.digitalWrite(9, LOW);                                // Выбрать EN микросхемы аналового коммутатора  2E2  U18
	  }
	  else if (_chanal_n > 32 && _chanal_n < 49)
	  {
		set_mcp_byte_2a(_chanal_n - 33);                              // Сформировать байт выбора канала (32 - 48)
		mcp_Out2.digitalWrite(10, LOW);                               // Выбрать EN микросхемы аналового коммутатора  2E3  U22
	  }

	}
	if (_chanal_in_out == 'G')                                        // Заземлить канал B
	{
	  mcp_Out2.digitalWrite(11, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E4  U16
	  mcp_Out2.digitalWrite(12, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E5  U20
	  mcp_Out2.digitalWrite(13, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E6  U24
	  if (_chanal_n < 17)
	  {
		set_mcp_byte_2b(_chanal_n - 1);                               // Сформировать байт выбора канала (0 - 15)
		mcp_Out2.digitalWrite(11, LOW);                               // Выбрать EN микросхемы аналового коммутатора  2E4  U16
	  }
	  else if (_chanal_n > 16 && _chanal_n < 33)
	  {
		set_mcp_byte_2b(_chanal_n - 17);                              // Сформировать байт выбора канала (16 - 31)
		mcp_Out2.digitalWrite(12, LOW);                               // Выбрать EN микросхемы аналового коммутатора  2E5  U20
	  }
	  else if (_chanal_n > 32 && _chanal_n < 49)
	  {
		set_mcp_byte_2b(_chanal_n - 33);                              // Сформировать байт выбора канала (32 - 48)
		mcp_Out2.digitalWrite(13, LOW);                               // Выбрать EN микросхемы аналового коммутатора  2E6  U24
	  }
	}
  }
  //delay(10);
}
void set_mcp_byte_1a(int set_byte)
{

  int _chanal_n = set_byte;

  if (bitRead(_chanal_n, 0) == 1)     // Установить бит 0
  {
	mcp_Out1.digitalWrite(0, HIGH);
  }
  else
  {
	mcp_Out1.digitalWrite(0, LOW);
  }

  if (bitRead(_chanal_n, 1) == 1)     // Установить бит 1
  {
	mcp_Out1.digitalWrite(1, HIGH);
  }
  else
  {
	mcp_Out1.digitalWrite(1, LOW);
  }

  if (bitRead(_chanal_n, 2) == 1)     // Установить бит 2
  {
	mcp_Out1.digitalWrite(2, HIGH);
  }
  else
  {
	mcp_Out1.digitalWrite(2, LOW);
  }


  if (bitRead(_chanal_n, 3) == 1)     // Установить бит 3
  {
	mcp_Out1.digitalWrite(3, HIGH);
  }
  else
  {
	mcp_Out1.digitalWrite(3, LOW);
  }
}
void set_mcp_byte_1b(int set_byte)
{
  int _chanal_n = set_byte;

  if (bitRead(_chanal_n, 0) == 1)     // Установить бит 0
  {
	mcp_Out1.digitalWrite(4, HIGH);
  }
  else
  {
	mcp_Out1.digitalWrite(4, LOW);
  }

  if (bitRead(_chanal_n, 1) == 1)     // Установить бит 1
  {
	mcp_Out1.digitalWrite(5, HIGH);
  }
  else
  {
	mcp_Out1.digitalWrite(5, LOW);
  }

  if (bitRead(_chanal_n, 2) == 1)     // Установить бит 2
  {
	mcp_Out1.digitalWrite(6, HIGH);
  }
  else
  {
	mcp_Out1.digitalWrite(6, LOW);
  }


  if (bitRead(_chanal_n, 3) == 1)     // Установить бит 3
  {
	mcp_Out1.digitalWrite(7, HIGH);
  }
  else
  {
	mcp_Out1.digitalWrite(7, LOW);
  }
}
void set_mcp_byte_2a(int set_byte)
{
  int _chanal_n = set_byte;

  if (bitRead(_chanal_n, 0) == 1)     // Установить бит 0
  {
	mcp_Out2.digitalWrite(0, HIGH);
  }
  else
  {
	mcp_Out2.digitalWrite(0, LOW);
  }

  if (bitRead(_chanal_n, 1) == 1)     // Установить бит 1
  {
	mcp_Out2.digitalWrite(1, HIGH);
  }
  else
  {
	mcp_Out2.digitalWrite(1, LOW);
  }

  if (bitRead(_chanal_n, 2) == 1)     // Установить бит 2
  {
	mcp_Out2.digitalWrite(2, HIGH);
  }
  else
  {
	mcp_Out2.digitalWrite(2, LOW);
  }


  if (bitRead(_chanal_n, 3) == 1)     // Установить бит 3
  {
	mcp_Out2.digitalWrite(3, HIGH);
  }
  else
  {
	mcp_Out2.digitalWrite(3, LOW);
  }
}
void set_mcp_byte_2b(int set_byte)
{
  int _chanal_n = set_byte;

  if (bitRead(_chanal_n, 0) == 1)     // Установить бит 0
  {
	mcp_Out2.digitalWrite(4, HIGH);
  }
  else
  {
	mcp_Out2.digitalWrite(4, LOW);
  }

  if (bitRead(_chanal_n, 1) == 1)     // Установить бит 1
  {
	mcp_Out2.digitalWrite(5, HIGH);
  }
  else
  {
	mcp_Out2.digitalWrite(5, LOW);
  }

  if (bitRead(_chanal_n, 2) == 1)     // Установить бит 2
  {
	mcp_Out2.digitalWrite(6, HIGH);
  }
  else
  {
	mcp_Out2.digitalWrite(6, LOW);
  }


  if (bitRead(_chanal_n, 3) == 1)     // Установить бит 3
  {
	mcp_Out2.digitalWrite(7, HIGH);
  }
  else
  {
	mcp_Out2.digitalWrite(7, LOW);
  }
}
void mem_byte_trans_readPC()                                      //  Чтение таблиц из EEPROM для передачи в ПК
{
  unsigned int _adr_reg = regBank.get(40005) + 40000;           //  Адрес блока регистров для передачи в ПК таблиц.
  unsigned int _adr_mem = regBank.get(40006);                   //  Адрес блока памяти для передачи в ПК таблиц.
  unsigned int _size_block = regBank.get(40007);                //  Адрес длины блока таблиц

  for (unsigned int x_mem = 0; x_mem < _size_block; x_mem++)
  {
	regBank.set(_adr_reg + x_mem, i2c_eeprom_read_byte(deviceaddress, _adr_mem + x_mem));
  }
  regBank.set(adr_control_command, 0);                          // Завершить программу
  delay(200);
}
void mem_byte_trans_savePC()                                      //  Получить таблицу из ПK и записать в EEPROM
{
  unsigned int _adr_reg = regBank.get(40005);                   //  Адрес блока регистров для передачи в ПК таблиц.
  unsigned int _adr_mem = regBank.get(40006);                   //  Адрес блока памяти для передачи в ПК таблиц.
  unsigned int _size_block = regBank.get(40007);                //  Адрес длины блока таблиц

  for (unsigned int x_mem = 0; x_mem < _size_block; x_mem++)
  {
	i2c_eeprom_write_byte(deviceaddress, _adr_mem + x_mem, regBank.get(_adr_reg + x_mem));
  }
  regBank.set(adr_control_command, 0);                          // Завершить программу
  delay(200);
}
int search_cabel(int sc)
{
  pinMode(Chanal_A, OUTPUT);                                                        // Установить на выход выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
  digitalWrite(Chanal_A, LOW);                                                      // Установить контрольный уровень на коммутаторе
  pinMode(Chanal_B, INPUT);                                                         // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
  digitalWrite(Chanal_B, HIGH);                                                     // Установить высокий уровень на выводе Chanal_B
  int n_connect = 0;
  Serial.println(sc);
  switch (sc)
  {
	case 1:
	  set_komm_mcp('A', 1, 'O');
	  set_komm_mcp('B', 1, 'O');
	  if (digitalRead(Chanal_B) == LOW )
	  {
		n_connect = 2;
		strcpy_P(buffer, (char*)pgm_read_word(&(table_message[27])));
		myGLCD.print(buffer, CENTER, 20);                                  // txt__connect2
	  }
	  break;
	case 39:
	  set_komm_mcp('A', 39, 'O');
	  set_komm_mcp('B', 39, 'O');
	  Serial.println(sc);
	  Serial.println(digitalRead(Chanal_B));
	  if (digitalRead(Chanal_B) == LOW )
	  {
		n_connect = 3;
		strcpy_P(buffer, (char*)pgm_read_word(&(table_message[28])));
		myGLCD.print(buffer, CENTER, 20);                                 // txt__connect3
	  }
	  break;
	case 40:
	  set_komm_mcp('A', 40, 'O');
	  set_komm_mcp('B', 40, 'O');
	  if (digitalRead(Chanal_B) == LOW )
	  {
		n_connect = 1;
		strcpy_P(buffer, (char*)pgm_read_word(&(table_message[26])));
		myGLCD.print(buffer, CENTER, 20);                                 // txt__connect1
	  }
	  break;
	case 41:
	  set_komm_mcp('A', 41, 'O');
	  set_komm_mcp('B', 41, 'O');
	  if (digitalRead(Chanal_B) == LOW )
	  {
		n_connect = 4;
		strcpy_P(buffer, (char*)pgm_read_word(&(table_message[29])));
		myGLCD.print(buffer, CENTER, 20);                                 // txt__connect4
	  }
	  break;
  }
  if (n_connect == 0) Serial.println(F("Connector is not detected"));
  return n_connect;
}

void test_cabel_N1()
{
  myGLCD.clrScr();
  myGLCD.print(txt_menu1_1, CENTER, 1);                                      // "Тест кабель N 1"
  myGLCD.setColor(255, 255, 255);                                            // Белая окантовка
  myGLCD.drawRoundRect (5, 200, 155, 239);
  myGLCD.drawRoundRect (160, 200, 315, 239);
  myGLCD.drawLine( 10, 60, 310, 60);
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (6, 201, 154, 238);
  myGLCD.fillRoundRect (161, 201, 314, 238);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor( 0, 0, 255);
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[21])));
  myGLCD.print(buffer, 10, 210);                                             //txt_test_repeat  Повторить
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[20])));
  myGLCD.print(buffer, 168, 210);                                            //txt_test_end Завершить
  myGLCD.setBackColor( 0, 0, 0);                                             //
  mcp_Out2.digitalWrite(14, LOW);                                            // Отключить реле +12v
  if (search_cabel(40) == 1)                                                 // Найден кабель №1
  {
	test_cabel_N1_run();                                                   // Выполнить проверку
	while (true)                                                           // Ожидание очередных комманд
	{

	  if (myTouch.dataAvailable())
	  {
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();

		if (((y >= 200) && (y <= 239)) && ((x >= 5) && (x <= 155)))   //нажата кнопка "Повторить проверку"
		{
		  waitForIt(5, 200, 155, 239);
		  myGLCD.setFont(BigFont);
		  test_cabel_N1_run();                                  // Выполнить программу проверки
		}
		if (((y >= 200) && (y <= 239)) && ((x >= 160) && (x <= 315))) //нажата кнопка "Завершить  проверку"
		{
		  waitForIt(160, 200, 315, 239);
		  myGLCD.setFont(BigFont);
		  break;                                                // Выход из программы
		}
	  }

	}
  }
  else
  {
	myGLCD.setColor(VGA_RED);
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[22])));
	myGLCD.print(buffer, CENTER, 80);                                       // txt_error_connect1 "Ошибка"
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[23])));
	myGLCD.print(buffer, CENTER, 110);                                      // txt_error_connect2 "подключения кабеля"
	myGLCD.setColor(255, 255, 255);
	delay(1000);
  }
}
void test_cabel_N2()
{
  myGLCD.clrScr();
  myGLCD.print(txt_menu1_2, CENTER, 1);
  myGLCD.setColor(255, 255, 255);                                             // Белая окантовка
  myGLCD.drawRoundRect (5, 200, 155, 239);
  myGLCD.drawRoundRect (160, 200, 315, 239);
  myGLCD.drawLine( 10, 60, 310, 60);
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (6, 201, 154, 238);
  myGLCD.fillRoundRect (161, 201, 314, 238);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor( 0, 0, 255);
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[21])));
  myGLCD.print(buffer, 10, 210);                                             //txt_test_repeat  Повторить
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[20])));
  myGLCD.print(buffer, 168, 210);                                            //txt_test_end Завершить
  myGLCD.setBackColor( 0, 0, 0);
  mcp_Out2.digitalWrite(14, LOW);                                            // Отключить реле +12v

  if (search_cabel(1) == 2)
  {
	test_cabel_N2_run();

	while (true)
	{

	  if (myTouch.dataAvailable())
	  {
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();

		if (((y >= 200) && (y <= 239)) && ((x >= 5) && (x <= 155)))            //Повторить
		{
		  waitForIt(5, 200, 155, 239);
		  myGLCD.setFont(BigFont);
		  test_cabel_N2_run();
		}
		if (((y >= 200) && (y <= 239)) && ((x >= 160) && (x <= 315)))         //Завершить
		{
		  waitForIt(160, 200, 315, 239);
		  myGLCD.setFont(BigFont);
		  break;
		}
	  }
	}
  }
  else
  {
	myGLCD.setColor(VGA_RED);
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[22])));
	myGLCD.print(buffer, CENTER, 80);                                       // txt_error_connect1 "Ошибка"
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[23])));
	myGLCD.print(buffer, CENTER, 110);                                      // txt_error_connect2 "подключения кабеля"
	myGLCD.setColor(255, 255, 255);
	delay(1000);
  }
}
void test_cabel_N3()
{
  myGLCD.clrScr();
  myGLCD.print(txt_menu1_3, CENTER, 1);
  myGLCD.setColor(255, 255, 255);                                             // Белая окантовка
  myGLCD.drawRoundRect (5, 200, 155, 239);
  myGLCD.drawRoundRect (160, 200, 315, 239);
  myGLCD.drawLine( 10, 60, 310, 60);
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (6, 201, 154, 238);
  myGLCD.fillRoundRect (161, 201, 314, 238);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor( 0, 0, 255);
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[21])));
  myGLCD.print(buffer, 10, 210);                                             //txt_test_repeat  Повторить
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[20])));
  myGLCD.print(buffer, 168, 210);                                            //txt_test_end Завершить
  myGLCD.setBackColor( 0, 0, 0);
  mcp_Out2.digitalWrite(14, LOW);                                            // Отключить реле +12v

  //if (search_cabel(39) == 20)
  //{
	test_cabel_N3_run();

	while (true)
	{
	  if (myTouch.dataAvailable())
	  {
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();

		if (((y >= 200) && (y <= 239)) && ((x >= 5) && (x <= 155)))            //Повторить
		{
		  waitForIt(5, 200, 155, 239);
		  myGLCD.setFont(BigFont);
		  test_cabel_N3_run();
		}
		if (((y >= 200) && (y <= 239)) && ((x >= 160) && (x <= 315)))         //Завершить
		{
		  waitForIt(160, 200, 315, 239);
		  myGLCD.setFont(BigFont);
		  break;
		}
	  }
	}
 // }
 // else
 // {
	//myGLCD.setColor(VGA_RED);
	//strcpy_P(buffer, (char*)pgm_read_word(&(table_message[22])));
	//myGLCD.print(buffer, CENTER, 80);                                       // txt_error_connect1 "Ошибка"
	//strcpy_P(buffer, (char*)pgm_read_word(&(table_message[23])));
	//myGLCD.print(buffer, CENTER, 110);                                      // txt_error_connect2 "подключения кабеля"
	//myGLCD.setColor(255, 255, 255);
	//delay(1000);
 // }
}
void test_cabel_N4()
{
  myGLCD.clrScr();
  myGLCD.print(txt_menu1_4, CENTER, 1);
  myGLCD.setColor(255, 255, 255);                                             // Белая окантовка
  myGLCD.drawRoundRect (5, 200, 155, 239);
  myGLCD.drawRoundRect (160, 200, 315, 239);
  myGLCD.drawLine( 10, 60, 310, 60);
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (6, 201, 154, 238);
  myGLCD.fillRoundRect (161, 201, 314, 238);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor( 0, 0, 255);
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[21])));
  myGLCD.print(buffer, 10, 210);                                             //txt_test_repeat  Повторить
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[20])));
  myGLCD.print(buffer, 168, 210);                                            //txt_test_end Завершить
  myGLCD.setBackColor( 0, 0, 0);
  mcp_Out2.digitalWrite(14, LOW);                                            // Отключить реле +12v

  if (search_cabel(41) == 4)
  {
	test_cabel_N4_run();

	while (true)
	{

	  if (myTouch.dataAvailable())
	  {
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();

		if (((y >= 200) && (y <= 239)) && ((x >= 5) && (x <= 155)))            //Повторить
		{
		  waitForIt(5, 200, 155, 239);
		  myGLCD.setFont(BigFont);
		  test_cabel_N4_run();
		}
		if (((y >= 200) && (y <= 239)) && ((x >= 160) && (x <= 315)))         //Завершить
		{
		  waitForIt(160, 200, 315, 239);
		  myGLCD.setFont(BigFont);
		  break;
		}
	  }

	}
  }
  else
  {
	myGLCD.setColor(VGA_RED);
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[22])));
	myGLCD.print(buffer, CENTER, 80);                                       // txt_error_connect1 "Ошибка"
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[23])));
	myGLCD.print(buffer, CENTER, 110);                                      // txt_error_connect2 "подключения кабеля"
	myGLCD.setColor(255, 255, 255);
	delay(1000);
  }
}

void test_BVS()
{
	myGLCD.clrScr();
	test_GND();
	myGLCD.clrScr();
	myGLCD.print(txt_menu2_1, CENTER, 1);
	myGLCD.setColor(255, 255, 255);                                             // Белая окантовка
	myGLCD.drawRoundRect(5, 200, 155, 239);
	myGLCD.drawRoundRect(160, 200, 315, 239);
	myGLCD.drawLine(10, 60, 310, 60);
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect(6, 201, 154, 238);
	myGLCD.fillRoundRect(161, 201, 314, 238);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(0, 0, 255);
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[21])));
	myGLCD.print(buffer, 10, 210);                                             //txt_test_repeat  Повторить
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[20])));
	myGLCD.print(buffer, 168, 210);                                            //txt_test_end Завершить
	myGLCD.setBackColor(0, 0, 0);
	mcp_Out2.digitalWrite(14, LOW);                                            // Отключить реле +12v
  
	test_BVS_run();

		while (true)
		{

			if (myTouch.dataAvailable())
			{
				myTouch.read();
				x = myTouch.getX();
				y = myTouch.getY();

				if (((y >= 200) && (y <= 239)) && ((x >= 5) && (x <= 155)))            //Повторить
				{
					waitForIt(5, 200, 155, 239);
					myGLCD.setFont(BigFont);
					test_BVS_run();
				}
				if (((y >= 200) && (y <= 239)) && ((x >= 160) && (x <= 315)))         //Завершить
				{
					waitForIt(160, 200, 315, 239);
					myGLCD.setFont(BigFont);
					break;
				}
			}

		}
}

void test_cabel_N1_run()
{

  byte  _size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN1_1);        // Получить количество выводов проверяемого разъема
  pinMode(Chanal_A, OUTPUT);                                                   // Установить на выход выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
  pinMode(Chanal_B, INPUT);                                                    // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
  digitalWrite(Chanal_B, HIGH);                                                // Установить высокий уровень на выводе Chanal_B
  myGLCD.print("                    ", 1, 40);                                 // Очистить строчку результатов проверки
  byte canal_N     = 0;                                                        // Переменная хранения № канала в памяти
  unsigned int x_A = 1;                                                        // Переменная установления канала А
  unsigned int x_B = 1;                                                        // Переменная установления канала В
  int x_p          = 1;                                                        // Определить начало вывода ошибок по Х
  int y_p          = 82;                                                       // Определить начало вывода ошибок по У
  int count_error  = 0;                                                        // Счетчик количества ошибок
  int ware_on      = 0;                                                        // Проверка должно ли быть сединение
 
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRoundRect(1, 82, 319, 199);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(10, 60, 310, 60);
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[24])));
  myGLCD.print(buffer, 50, 65);                                                // txt_error_connect3 "Ошибок нет"
  if (search_cabel(40) == 1)                                                   // Проверить корректность подключения кабеля №1
  {
	digitalWrite(Chanal_A, LOW);                                                     // Установить контрольный уровень на коммутаторах U13,U17,U23
	delay(10);                                                                 // Время на переключение вывода Chanal_A
	for (x_A = 1; x_A < _size_block + 1; x_A++)                                // Последовательное чтение контактов разьемов.
	{
	  canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_1 + x_A);        // Получить № канала из EEPROM
	  ware_on = i2c_eeprom_read_byte(deviceaddress, adr_memN1_1 + x_A + (_size_block * 4)); // Получить из таблицы признак соединения.
	  if (canal_N == 1)                                                        // 40 канал для проверки номера проверяемого разъема
	  {
		set_komm_mcp('A', 40, 'O');                                            // Установить вход коммутатора на контрольный 40 выход
	  }
	  else
	  {
		set_komm_mcp('A', canal_N, 'O');                                 // Установить текущий вход коммутатора
	  }
	  // Последовательно проверить все вывода разьема "В"
	  // Проверяем все выхода разьема "В"
	  for (x_B = 1; x_B < _size_block + 1; x_B++)                          // Последовательное чтение контактов разьемов "В" .
	  {
		canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_1 + x_B + _size_block); // Получить из таблицы номер входа коммутатора.

		if (canal_N == 1)                                                // 40 канал для проверки номера проверяемого разъема
		{
		  set_komm_mcp('B', 40, 'O');                                     // Установить контрольный вход коммутатора
		}
		else
		{
		  set_komm_mcp('B', canal_N, 'O');                                // Установить текущий вход коммутатора
		}
		// ++++++++++++++++++++++++ Проверка на соединение А - В +++++++++++++++++++++++++++++++++++
		if (x_A == x_B)
		{
		  myGLCD.printNumI(x_A, 30, 40);
		  if (ware_on == 1)myGLCD.print("<->", 66, 40);
		  else myGLCD.print("<X>", 66, 40);
		  myGLCD.print("  ",130, 40);
		  myGLCD.printNumI(canal_N, 130, 40);
		  if (digitalRead(Chanal_B) == LOW && ware_on == 1)
		  {
			myGLCD.print(" - Pass", 170, 40);
		  }
		  else
		  {
			if (digitalRead(Chanal_B) != LOW && ware_on == 0)                  // Должен быть соединен
			{
			  myGLCD.print(" - Pass", 170, 40);
			}
			else
			{
			  count_error++;
			  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
			  myGLCD.print(buffer, 50, 65);                            // txt_error_connect4
			  myGLCD.printNumI(count_error, 190, 65);

			  if ( ware_on == 1)
			  {
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("-", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("-", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
			  }
			  else
			  {
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
			  }
			  y_p += 19;
			  if ( y_p > 190)                                          // Вывод на экран таблицы ошибок
			  {
				myGLCD.drawLine( x_p + 75, 85, x_p + 75, 190);
				x_p += 80;
				y_p = 82;
			  }
			}
		  }
		}

		//------------------------ Конец проверки на соединение ---------------------------------------

		//++++++++++++++++++++++++ Проверка остальных проводов на замыкание ---------------------------
		if (x_A != x_B)                                                      //Проверяемые провода не не должны быть соеденены
		{
		  if (digitalRead(Chanal_B) == LOW)                                      // Все таки замкнуты
		  {
			// Проверим дополнительную 3 таблицу, возможно должны иметь соединение
			int canal_N_err = i2c_eeprom_read_byte(deviceaddress, adr_memN1_1 + x_A + (_size_block * 2)); // Получить из таблицы номер входа коммутатора.
			if (x_B != canal_N_err)                                      // Проверяемое соединение не записано в таблицу
			{
			  // Проверим дополнительную 4 таблицу
			  int canal_N_err = i2c_eeprom_read_byte(deviceaddress, adr_memN1_1 + x_A + (_size_block * 3)); // Получить из таблицы номер входа коммутатора.
			  if (x_B != canal_N_err)                                  // Проверяемое соединение не записано в таблицу
			  {
				count_error++;
				strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
				myGLCD.print(buffer, 50, 65);                        // txt_error_connect4
				myGLCD.printNumI(count_error, 190, 65);
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
				y_p += 19;
				if ( y_p > 190)                                      // Вывод на экран таблицы ошибок
				{
				  myGLCD.drawLine( x_p + 75, 85, x_p + 75, 190);
				  x_p += 80;
				  y_p = 82;
				}
			  }
			}
		  }
		} 	//----------------------- Конец проверки на замыкание -----------------------------------------
	  }
	}
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[30])));
	if (count_error == 0) myGLCD.print(buffer, CENTER, 120);               // txt__test_end
  }
  else
  {
	myGLCD.setColor(VGA_RED);
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[22])));
	myGLCD.print(buffer, CENTER, 82 + 19);                                // txt_error_connect1 Сообщить что кабель не обнаружен
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[23])));
	myGLCD.print(buffer, CENTER, 82 + 38);                                // txt_error_connect2
	myGLCD.setColor(255, 255, 255);                                       // Восстановить белый шрифт
	delay(2000);
  }
}
void test_cabel_N2_run()
{
  byte  _size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN1_2);        // Получить количество выводов проверяемого разъема
  pinMode(Chanal_A, OUTPUT);                                                         // Установить на выход выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
  pinMode(Chanal_B, INPUT);                                                          // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
  digitalWrite(Chanal_B, HIGH);                                                      // Установить высокий уровень на выводе Chanal_B
  myGLCD.print("                    ", 1, 40);                                 // Очистить строчку результатов проверки
  byte canal_N     = 0;                                                        // Переменная хранения № канала в памяти
  unsigned int x_A = 1;                                                        // Переменная установления канала А
  unsigned int x_B = 1;                                                        // Переменная установления канала В
  int x_p          = 1;                                                        // Определить начало вывода ошибок по Х
  int y_p          = 82;                                                       // Определить начало вывода ошибок по У
  int count_error  = 0;                                                        // Счетчик количества ошибок
  int ware_on      = 0;                                                        // Проверка должно ли быть сединение

  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRoundRect(1, 82, 319, 199);
  myGLCD.setColor(255, 255, 255);

  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[24])));
  myGLCD.print(buffer, 50, 65);                                                // txt_error_connect3 "Ошибок нет"
  if (search_cabel(1) == 2)                                                    // Проверить корректность подключения кабеля №1
  {
	digitalWrite(Chanal_A, LOW);                                                   // Установить контрольный уровень на коммутаторах U13,U17,U23
	delay(10);                                                               // Время на переключение вывода Chanal_A
	// Начало проверки
	for (x_A = 1; x_A < _size_block + 1; x_A++)                              // Последовательное чтение контактов разьемов.
	{
	  canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_2 + x_A);    // Получить № канала из EEPROM
	  ware_on = i2c_eeprom_read_byte(deviceaddress, adr_memN1_2 + x_A + (_size_block * 4)); // Получить из таблицы признак соединения.
	  if (canal_N == 1)                                                    // 40 канал для проверки номера проверяемого разъема
	  {
		set_komm_mcp('A', 1, 'O');                                       // Установить вход коммутатора на контрольный 40 выход
	  }
	  else
	  {
		set_komm_mcp('A', canal_N, 'O');                                 // Установить текущий вход коммутатора
	  }
	  // Последовательно проверить все вывода разьема "В"
	  // Проверяем все выхода разьема "В"
	  for (x_B = 1; x_B < _size_block + 1; x_B++)                          // Последовательное чтение контактов разьемов "В" .
	  {
		canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_2 + x_B + _size_block); // Получить из таблицы номер входа коммутатора.
		if (canal_N == 1)                                                // 40 канал для проверки номера проверяемого разъема
		{
		  set_komm_mcp('B', 1, 'O');                                   // Установить контрольный вход коммутатора
		}
		else
		{
		  set_komm_mcp('B', canal_N, 'O');                             // Установить текущий вход коммутатора
		}
		// ++++++++++++++++++++++++ Проверка на соединение А - В +++++++++++++++++++++++++++++++++++
		if (x_A == x_B)
		{
		  myGLCD.printNumI(x_A, 30, 40);
		  if (ware_on == 1)myGLCD.print("<->", 66, 40);
		  else myGLCD.print("<X>", 66, 40);
		  myGLCD.print("  ",130, 40);
		  myGLCD.printNumI(canal_N, 130, 40);
		  if (digitalRead(Chanal_B) == LOW && ware_on == 1)
		  {
			myGLCD.print(" - Pass", 170, 40);
		  }
		  else
		  {
			if (digitalRead(Chanal_B) != LOW && ware_on == 0)                  // Должен быть соединен
			{
			  myGLCD.print(" - Pass", 170, 40);
			}
			else
			{
			  count_error++;
			  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
			  myGLCD.print(buffer, 50, 65);                            // txt_error_connect4							myGLCD.printNumI(count_error, 190, 65);
			  myGLCD.printNumI(count_error, 190, 65);
			  if ( ware_on == 1)
			  {
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("-", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("-", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
			  }
			  else
			  {
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
			  }
			  y_p += 19;
			  if ( y_p > 190)                                          // Вывод на экран таблицы ошибок
			  {
				myGLCD.drawLine( x_p + 75, 85, x_p + 75, 190);
				x_p += 80;
				y_p = 82;
			  }
			}
		  }
		}

		//------------------------ Конец проверки на соединение ---------------------------------------


		//++++++++++++++++++++++++ Проверка остальных проводов на замыкание ---------------------------
		if (x_A != x_B)                                                      //Проверяемые провода не не должны быть соеденены
		{
		  if (digitalRead(Chanal_B) == LOW)                                      // Все таки замкнуты
		  {
			// Проверим дополнительную 3 таблицу, возможно должны иметь соединение
			int canal_N_err = i2c_eeprom_read_byte(deviceaddress, adr_memN1_2 + x_A + (_size_block * 2)); // Получить из таблицы номер входа коммутатора.
			if (x_B != canal_N_err)                                      // Проверяемое соединение не записано в таблицу
			{
			  // Проверим дополнительную 4 таблицу
			  int canal_N_err = i2c_eeprom_read_byte(deviceaddress, adr_memN1_2 + x_A + (_size_block * 3)); // Получить из таблицы номер входа коммутатора.
			  if (x_B != canal_N_err)                                  // Проверяемое соединение не записано в таблицу
			  {
				count_error++;
				strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
				myGLCD.print(buffer, 50, 65);                        // txt_error_connect4
				myGLCD.printNumI(count_error, 190, 65);
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
				y_p += 19;
				if ( y_p > 190)                                      // Вывод на экран таблицы ошибок
				{
				  myGLCD.drawLine( x_p + 75, 85, x_p + 75, 190);
				  x_p += 80;
				  y_p = 82;
				}
			  }
			}
		  }
		}
		//----------------------- Конец проверки на замыкание -----------------------------------------
	  }
	}
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[30])));
	if (count_error == 0) myGLCD.print(buffer, CENTER, 120);                  // txt__test_end
  }
  else
  {
	myGLCD.setColor(VGA_RED);
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[22])));
	myGLCD.print(buffer, CENTER, 82 + 19);                                // txt_error_connect1 Сообщить что кабель не обнаружен
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[23])));
	myGLCD.print(buffer, CENTER, 82 + 38);                                // txt_error_connect2
	myGLCD.setColor(255, 255, 255);                                       // Восстановить белый шрифт
	delay(2000);
  }
}
void test_cabel_N3_run()
{
  byte  _size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN1_3);        // Получить количество выводов проверяемого разъема
  pinMode(Chanal_A, OUTPUT);                                                   // Установить на выход выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
  pinMode(Chanal_B, INPUT);                                                    // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
  digitalWrite(Chanal_B, HIGH);                                                // Установить высокий уровень на выводе Chanal_B
  myGLCD.print("                    ", 1, 40);                                 // Очистить строчку результатов проверки
  byte canal_N     = 0;                                                        // Переменная хранения № канала в памяти
  unsigned int x_A = 1;                                                        // Переменная установления канала А
  unsigned int x_B = 1;                                                        // Переменная установления канала В
  int x_p          = 1;                                                        // Определить начало вывода ошибок по Х
  int y_p          = 82;                                                       // Определить начало вывода ошибок по У
  int count_error  = 0;                                                        // Счетчик количества ошибок
  int ware_on      = 0;                                                        // Проверка должно ли быть сединение

  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRoundRect(1, 82, 319, 199);
  myGLCD.setColor(255, 255, 255);

  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[24])));
  myGLCD.print(buffer, 50, 65);                                                // txt_error_connect3 "Ошибок нет"
  //if (search_cabel(39) == 3)                                                   // Проверить корректность подключения кабеля №1
  //{
	digitalWrite(Chanal_A, LOW);                                               // Установить контрольный уровень на коммутаторах U13,U17,U23
	delay(10);                                                                 // Время на переключение вывода Chanal_A
	for (x_A = 1; x_A < _size_block + 1; x_A++)                                // Последовательное чтение контактов разьемов.
	{
	  canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_3 + x_A);        // Получить № канала из EEPROM
	  ware_on = i2c_eeprom_read_byte(deviceaddress, adr_memN1_3 + x_A + (_size_block * 4)); // Получить из таблицы признак соединения.

	  if (canal_N == 1)                                                        // 40 канал для проверки номера проверяемого разъема
	  {
		set_komm_mcp('A', 39, 'O');                                             // Установить вход коммутатора на контрольный 39 выход
	  }
	  else
	  {
		set_komm_mcp('A', canal_N, 'O');                                 // Установить текущий вход коммутатора
	  }
	  // Последовательно проверить все вывода разьема "В"
	  // Проверяем все выхода разьема "В"
	  for (x_B = 1; x_B < _size_block + 1; x_B++)                          // Последовательное чтение контактов разьемов "В" .
	  {
		canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_3 + x_B + _size_block); // Получить из таблицы номер входа коммутатора.

		if (canal_N == 1)                                                // 39 канал для проверки номера проверяемого разъема
		{
		  set_komm_mcp('B', 39, 'O');                                  // Установить контрольный вход коммутатора
		}
		else
		{
		  set_komm_mcp('B', canal_N, 'O');                             // Установить текущий вход коммутатора
		}
		// ++++++++++++++++++++++++ Проверка на соединение А - В +++++++++++++++++++++++++++++++++++
		if (x_A == x_B)
		{
		  myGLCD.printNumI(x_A, 30, 40);
		  if (ware_on == 1)myGLCD.print("<->", 66, 40);
		  else myGLCD.print("<X>", 66, 40);
		  myGLCD.print("  ",130, 40);
		  myGLCD.printNumI(canal_N, 130, 40);
		  if (digitalRead(Chanal_B) == LOW && ware_on == 1)
		  {
			myGLCD.print(" - Pass", 170, 40);
		  }
		  else
		  {
			if (digitalRead(Chanal_B) != LOW && ware_on == 0)                  // Должен быть соединен
			{
			  myGLCD.print(" - Pass", 170, 40);
			}
			else
			{
			  count_error++;
			  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
			  myGLCD.print(buffer, 50, 65);                            // txt_error_connect4							myGLCD.printNumI(count_error, 190, 65);
			  myGLCD.printNumI(count_error, 190, 65);
			  if ( ware_on == 1)
			  {
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("-", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("-", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
			  }
			  else
			  {
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
			  }
			  y_p += 19;
			  if ( y_p > 190)                                          // Вывод на экран таблицы ошибок
			  {
				myGLCD.drawLine( x_p + 75, 85, x_p + 75, 190);
				x_p += 80;
				y_p = 82;
			  }
			}
		  }
		}

		//------------------------ Конец проверки на соединение ---------------------------------------

		//++++++++++++++++++++++++ Проверка остальных проводов на замыкание ---------------------------
		if (x_A != x_B)                                                      //Проверяемые провода не не должны быть соеденены
		{
		  if (digitalRead(Chanal_B) == LOW)                                      // Все таки замкнуты
		  {
			// Проверим дополнительную 3 таблицу, возможно должны иметь соединение
			int canal_N_err = i2c_eeprom_read_byte(deviceaddress, adr_memN1_3 + x_A + (_size_block * 2)); // Получить из таблицы номер входа коммутатора.
			if (x_B != canal_N_err)                                      // Проверяемое соединение не записано в таблицу
			{
			  // Проверим дополнительную 4 таблицу
			  int canal_N_err = i2c_eeprom_read_byte(deviceaddress, adr_memN1_3 + x_A + (_size_block * 3)); // Получить из таблицы номер входа коммутатора.
			  if (x_B != canal_N_err)                                  // Проверяемое соединение не записано в таблицу
			  {
				count_error++;
				strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
				myGLCD.print(buffer, 50, 65);                        // txt_error_connect4
				myGLCD.printNumI(count_error, 190, 65);
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
				y_p += 19;
				if ( y_p > 190)                                      // Вывод на экран таблицы ошибок
				{
				  myGLCD.drawLine( x_p + 75, 85, x_p + 75, 190);
				  x_p += 80;
				  y_p = 82;
				}
			  }
			}
		  }
		}	//----------------------- Конец проверки на замыкание -----------------------------------------
	  }
	}
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[30])));
	if (count_error == 0) myGLCD.print(buffer, CENTER, 120);                  // txt__test_end
 // }
 // else
 // {
	//myGLCD.setColor(VGA_RED);
	//strcpy_P(buffer, (char*)pgm_read_word(&(table_message[22])));
	//myGLCD.print(buffer, CENTER, 82 + 19);                                // txt_error_connect1 Сообщить что кабель не обнаружен
	//strcpy_P(buffer, (char*)pgm_read_word(&(table_message[23])));
	//myGLCD.print(buffer, CENTER, 82 + 38);                                // txt_error_connect2
	//myGLCD.setColor(255, 255, 255);                                       // Восстановить белый шрифт
	//delay(2000);
 // }
}
void test_cabel_N4_run()
{
  byte  _size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN1_4);        // Получить количество выводов проверяемого разъема
  pinMode(Chanal_A, OUTPUT);                                                         // Установить на выход выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
  pinMode(Chanal_B, INPUT);                                                          // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
  digitalWrite(Chanal_B, HIGH);                                                      // Установить высокий уровень на выводе Chanal_B
  myGLCD.print("                    ", 1, 40);                                 // Очистить строчку результатов проверки
  byte canal_N     = 0;                                                        // Переменная хранения № канала в памяти
  unsigned int x_A = 1;                                                        // Переменная установления канала А
  unsigned int x_B = 1;                                                        // Переменная установления канала В
  int x_p          = 1;                                                        // Определить начало вывода ошибок по Х
  int y_p          = 82;                                                       // Определить начало вывода ошибок по У
  int count_error  = 0;                                                        // Счетчик количества ошибок
  int ware_on      = 0;                                                        // Проверка должно ли быть сединение

  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRoundRect(1, 82, 319, 199);
  myGLCD.setColor(255, 255, 255);

  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[24])));
  myGLCD.print(buffer, 50, 65);                                                // txt_error_connect3 "Ошибок нет"
  if (search_cabel(41) == 4)                                                   // Проверить корректность подключения кабеля №1
  {
	digitalWrite(Chanal_A, LOW);                                                   // Установить контрольный уровень на коммутаторах U13,U17,U23
	delay(10);                                                               // Время на переключение вывода Chanal_A
	for (x_A = 1; x_A < _size_block + 1; x_A++)                              // Последовательное чтение контактов разьемов.
	{
	  canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_4 + x_A);    // Получить № канала из EEPROM
	  ware_on = i2c_eeprom_read_byte(deviceaddress, adr_memN1_4 + x_A + (_size_block * 4)); // Получить из таблицы признак соединения.
	  if (canal_N == 1)                                                    // 40 канал для проверки номера проверяемого разъема
	  {
		set_komm_mcp('A', 41, 'O');                                      // Установить вход коммутатора на контрольный 40 выход
	  }
	  else
	  {
		set_komm_mcp('A', canal_N, 'O');                                 // Установить текущий вход коммутатора
	  }
	  // Последовательно проверить все вывода разьема "В"
	  // Проверяем все выхода разьема "В"
	  for (x_B = 1; x_B < _size_block + 1; x_B++)                          // Последовательное чтение контактов разьемов "В" .
	  {
		canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_4 + x_B + _size_block); // Получить из таблицы номер входа коммутатора.

		if (canal_N == 1)                                                   // 40 канал для проверки номера проверяемого разъема
		{
		  set_komm_mcp('B', 41, 'O');                                       // Установить контрольный вход коммутатора
		}
		else
		{
		  set_komm_mcp('B', canal_N, 'O');                                   // Установить текущий вход коммутатора
		}
		// ++++++++++++++++++++++++ Проверка на соединение А - В +++++++++++++++++++++++++++++++++++
		if (x_A == x_B)
		{
		  myGLCD.printNumI(x_A, 30, 40);
		  if (ware_on == 1)myGLCD.print("<->", 66, 40);
		  else myGLCD.print("<X>", 66, 40);
		  myGLCD.print("  ",130, 40);
		  myGLCD.printNumI(canal_N, 130, 40);
		  if (digitalRead(Chanal_B) == LOW && ware_on == 1)
		  {
			myGLCD.print(" - Pass", 170, 40);
		  }
		  else
		  {
			if (digitalRead(Chanal_B) != LOW && ware_on == 0)                  // Должен быть соединен
			{
			  myGLCD.print(" - Pass", 170, 40);
			}
			else
			{
			  count_error++;
			  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
			  myGLCD.print(buffer, 50, 65);                            // txt_error_connect4
			  myGLCD.printNumI(count_error, 190, 65);

			  if ( ware_on == 1)
			  {
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("-", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("-", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
			  }
			  else
			  {
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
			  }
			  y_p += 19;
			  if ( y_p > 190)                                          // Вывод на экран таблицы ошибок
			  {
				myGLCD.drawLine( x_p + 75, 85, x_p + 75, 190);
				x_p += 80;
				y_p = 82;
			  }
			}
		  }
		}

		//------------------------ Конец проверки на соединение ---------------------------------------

		//++++++++++++++++++++++++ Проверка остальных проводов на замыкание ---------------------------
		if (x_A != x_B)                                                      //Проверяемые провода не не должны быть соеденены
		{
		  if (digitalRead(Chanal_B) == LOW)                                      // Все таки замкнуты
		  {
			// Проверим дополнительную 3 таблицу, возможно должны иметь соединение
			int canal_N_err = i2c_eeprom_read_byte(deviceaddress, adr_memN1_4 + x_A + (_size_block * 2)); // Получить из таблицы номер входа коммутатора.
			if (x_B != canal_N_err)                                      // Проверяемое соединение не записано в таблицу
			{
			  // Проверим дополнительную 4 таблицу
			  int canal_N_err = i2c_eeprom_read_byte(deviceaddress, adr_memN1_4 + x_A + (_size_block * 3)); // Получить из таблицы номер входа коммутатора.
			  if (x_B != canal_N_err)                                  // Проверяемое соединение не записано в таблицу
			  {
				count_error++;
				strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
				myGLCD.print(buffer, 50, 65);                        // txt_error_connect4
				myGLCD.printNumI(count_error, 190, 65);
				if (x_A < 10)
				{
				  myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				else
				{
				  myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
				  myGLCD.print("+", x_p + 29, y_p);
				}
				if (canal_N < 10)
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
				}
				else
				{
				  myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
				}
				y_p += 19;
				if ( y_p > 190)                                      // Вывод на экран таблицы ошибок
				{
				  myGLCD.drawLine( x_p + 75, 85, x_p + 75, 190);
				  x_p += 80;
				  y_p = 82;
				}
			  }
			}
		  }
		}  //----------------------- Конец проверки на замыкание -----------------------------------------
	  }
	}
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[30])));
	if (count_error == 0) myGLCD.print(buffer, CENTER, 120);                  // txt__test_end
  }
  else
  {
	myGLCD.setColor(VGA_RED);
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[22])));
	myGLCD.print(buffer, CENTER, 82 + 19);                                // txt_error_connect1 Сообщить что кабель не обнаружен
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[23])));
	myGLCD.print(buffer, CENTER, 82 + 38);                                // txt_error_connect2
	myGLCD.setColor(255, 255, 255);                                       // Восстановить белый шрифт
	delay(2000);
  }
}

void test_BVS_run()
{
	byte  _size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN1_5);        // Получить количество выводов проверяемого разъема
	pinMode(Chanal_A, INPUT);                                                   // Установить на вход выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
	pinMode(Chanal_B, INPUT);                                                    // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
	digitalWrite(Chanal_A, HIGH);                                                // Подключить подтягивающий резистор на выводе Chanal_B
	digitalWrite(Chanal_B, HIGH);                                                // Подключить подтягивающий резистор на выводе Chanal_B
	myGLCD.print("                    ", 1, 40);                                 // Очистить строчку результатов проверки
	byte canal_N = 0;                                                            // Переменная хранения № канала в памяти
	unsigned int x_A = 1;                                                        // Переменная установления канала А
	unsigned int x_B = 1;                                                        // Переменная установления канала В
	int x_p = 1;                                                                 // Определить начало вывода ошибок по Х
	int y_p = 82;                                                                // Определить начало вывода ошибок по У
	int count_error = 0;                                                         // Счетчик количества ошибок
	int ware_on = 0;                                                             // Проверка должно ли быть сединение

	myGLCD.setColor(0, 0, 0);                                                    // Установить черный экран 
	myGLCD.fillRoundRect(1, 40, 319, 199);                                       // Очистить экран
	myGLCD.setColor(255, 255, 255);                                              // Установить белый цвет
	y_p = 82;                                                                    // Восстановить начало вывода ошибок по У

	myGLCD.drawLine(10, 60, 310, 60);

	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[24])));                // "Ошибок нет"
	myGLCD.print(buffer, 50, 65);                                                // txt_error_connect3 "Ошибок нет"
	pinMode(Chanal_A, OUTPUT);                                                   // Установить на вход выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
	digitalWrite(Chanal_A, LOW);                                                 // Установить контрольный уровень на коммутаторах U13,U17,U23
	delay(10);                                                                   // Время на переключение вывода Chanal_A
	for (x_A = 1; x_A < _size_block + 1; x_A++)                                  // Последовательное чтение контактов разьемов.
	{
		canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_5 + x_A);        // Получить № канала из EEPROM
		ware_on = i2c_eeprom_read_byte(deviceaddress, adr_memN1_5 + x_A + (_size_block * 4)); // Получить из таблицы признак соединения.
		
		set_komm_mcp('A', canal_N, 'O');                                         // Установить текущий вход коммутатора
			
		// Последовательно проверить все вывода разьема "В"
		// Проверяем все выхода разьема "В"
		for (x_B = 1; x_B < _size_block + 1; x_B++)                              // Последовательное чтение контактов разьемов "В" .
		{
			canal_N = i2c_eeprom_read_byte(deviceaddress, adr_memN1_5 + x_B + _size_block); // Получить из таблицы номер входа коммутатора.

				set_komm_mcp('B', canal_N, 'O');                                 // Установить текущий вход коммутатора
			
			// ++++++++++++++++++++++++ Проверка на соединение А - В +++++++++++++++++++++++++++++++++++
			if (x_A == x_B)
			{
				myGLCD.printNumI(x_A, 30, 40);
				if (ware_on == 1)myGLCD.print("<->", 66, 40);
				else myGLCD.print("<x>", 66, 40);
				myGLCD.print("  ", 130, 40);
				myGLCD.printNumI(canal_N, 130, 40);
				if (digitalRead(Chanal_B) == LOW && ware_on == 1)
				{
					myGLCD.print(" - Pass", 170, 40);
				}
				else
				{
					if (digitalRead(Chanal_B) != LOW && ware_on == 0)                  // Должен быть соединен
					{
						myGLCD.print(" - Pass", 170, 40);
					}
					else
					{
						count_error++;
						strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
						myGLCD.print(buffer, 50, 65);                                 // txt_error_connect4
						myGLCD.printNumI(count_error, 190, 65);

						if (ware_on == 1)
						{
							if (x_A < 10)
							{
								myGLCD.printNumI(x_A, x_p + 13, y_p);                // Перечисление ошибочных контактов
								myGLCD.print("x", x_p + 29, y_p);
							}
							else
							{
								myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
								myGLCD.print("x", x_p + 29, y_p);
							}
							if (canal_N < 10)
							{
								myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
							}
							else
							{
								myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
							}
						}
						else
						{
							if (x_A < 10)
							{
								myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
								myGLCD.print("+", x_p + 29, y_p);
							}
							else
							{
								myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
								myGLCD.print("+", x_p + 29, y_p);
							}
							if (canal_N < 10)
							{
								myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);   // Перечисление ошибочных контактов
							}
							else
							{
								myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);   // Перечисление ошибочных контактов
							}
						}
						y_p += 19;
						if (y_p > 190)                                          // Вывод на экран таблицы ошибок
						{
							myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
							x_p += 80;
							y_p = 82;
						}
					}
				}
			}

			//------------------------ Конец проверки на соединение ---------------------------------------

			//++++++++++++++++++++++++ Проверка остальных проводов на замыкание ---------------------------
			if (x_A != x_B)                                                            //Проверяемые провода не не должны быть соеденены
			{
				if (digitalRead(Chanal_B) == LOW)                                      // Все таки замкнуты
				{
					count_error++;
					strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
					myGLCD.print(buffer, 50, 65);                             // txt_error_connect4
					myGLCD.printNumI(count_error, 190, 65);
					if (x_A < 10)
					{
						myGLCD.printNumI(x_A, x_p + 13, y_p);                 // Перечисление ошибочных контактов 
						myGLCD.print("+", x_p + 29, y_p);
					}
					else
					{
						myGLCD.printNumI(x_A, x_p, y_p);                      // Перечисление ошибочных контактов
						myGLCD.print("+", x_p + 29, y_p);
					}
					if (canal_N < 10)
					{
						myGLCD.printNumI(canal_N, x_p + 32 + 26, y_p);       // Перечисление ошибочных контактов
					}
					else
					{
						myGLCD.printNumI(canal_N, x_p + 32 + 10, y_p);       // Перечисление ошибочных контактов
					}
					y_p += 19;
					if (y_p > 190)                                           // Вывод на экран таблицы ошибок
					{
						myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
						x_p += 80;
						y_p = 82;
					}
				}
			}  //----------------------- Конец проверки на замыкание -----------------------------------------
		}
	}
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[30])));
	if (count_error == 0) myGLCD.print(buffer, CENTER, 120);                  // txt__test_end
}

void test_GND()
{
	myGLCD.clrScr();
	myGLCD.setColor(255, 255, 255);
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[18])));
	myGLCD.print(buffer, CENTER, 200);                                           // txt_test_all_exit1 
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[19])));
	myGLCD.print(buffer, CENTER, 220);                                           // txt_test_all_exit2
	byte canal_N = 0;
	pinMode(Chanal_B, INPUT);                                                    // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
	pinMode(Chanal_A, INPUT);                                                    // Установить на вход  выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
	digitalWrite(Chanal_B, HIGH);                                                // Установить высокий уровень на выводе Chanal_B
	digitalWrite(Chanal_A, HIGH);                                                // Установить высокий уровень на выводе Chanal_A
	int i_step = 1;

	byte  _size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN1_5);        // Получить количество выводов проверяемого разъема

	myGLCD.print("                    ", 1, 40);                                 // Очистить строчку результатов проверки
	unsigned int x_A = 1;                                                        // Переменная установления канала А
	unsigned int x_B = 1;                                                        // Переменная установления канала В
	int x_p = 1;                                                                 // Определить начало вывода ошибок по Х
	int y_p = 82;                                                                // Определить начало вывода ошибок по У
	int count_error = 0;                                                         // Счетчик количества ошибок
	int ware_on = 0;                                                             // Проверка должно ли быть сединение
	for (int p = 0; p < 6; p++)                                                  // Очистить поле ошибок на дисплее
	{
		myGLCD.print("                    ", x_p, y_p);                          // Очистить 6 строк
		y_p += 19;
	}
	y_p = 82;                                                                    // Восстановить начало вывода ошибок по У


	while (true)
	{
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			x = myTouch.getX();
			y = myTouch.getY();

			if (((y >= 1) && (y <= 150)) && ((x >= 10) && (x <= 319)))           //Возврат
			{
				myGLCD.setFont(BigFont);
				break;
			}
		}

		set_komm_mcp('A', 25, 'O');                                             // Переключить коммутатор разъемов серии "А" на вход
		set_komm_mcp('B', 25, 'O');                                             // Переключить коммутатор разъемов серии "В" на вход
		delay(10);

		if (digitalRead(Chanal_B) == LOW)
		{
			myGLCD.print("                 ", CENTER, 80);
			myGLCD.print("\x85""a""\x9C""e""\xA1\xA0""e""\xA2""o", CENTER, 80);    // 
			connect_gnd1 = true;
		}
		else
		{
			myGLCD.print("\x89""o""\x99\x9F\xA0\xAE\xA7\x9D\xA4""e ""\x9C""e""\xA1\xA0\xAE", CENTER, 80);    //  Подключите землю
			connect_gnd1 = false;

		}

		if (digitalRead(Chanal_A) == LOW)
		{
			myGLCD.print("                 ", CENTER, 105);
			myGLCD.print("\x85""a""\x9C""e""\xA1\xA0""e""\xA2""o", CENTER, 105);    // 
			connect_gnd2 = true;
		}
		else
		{
			myGLCD.print("\x89""o""\x99\x9F\xA0\xAE\xA7\x9D\xA4""e ""\x9C""e""\xA1\xA0\xAE", CENTER, 105);   // Подключите землю
			connect_gnd2 = false;

		}

		if (connect_gnd1 && connect_gnd2)
		{
			break;
		}
	}

	myGLCD.clrScr();

	myGLCD.print("\x89""po""\x97""ep""\x9F""a ""\x9C""a""\xA1\xAB\x9F""a""\xA2\x9D\xAF"" ", CENTER, 3);
	myGLCD.print("\xA2""a ""\x9C""e""\xA1\xA0\xAE"", o""\x96\xA9\x9D\x9E", CENTER,20);                       // на землю, общий
	myGLCD.drawLine(10, 60, 310, 60);

	//++++++++++++++++++++++++++++++++++++++ Проверка замыкания на землю +++++++++++++++++++++++++++++++++++++++++++++++++++
	for (x_A = 1; x_A < 49; x_A++)                                               // Последовательное чтение контактов разьемов.
	{
		set_komm_mcp('A', x_A, 'O');                                             // Установить текущий вход коммутатора
		set_komm_mcp('B', x_A, 'O');                                             // Установить текущий вход коммутатора
		if (x_A < 10)
		{
			myGLCD.printNumI(x_A, 30 + 12, 40);
		}
		else
		{
			myGLCD.printNumI(x_A, 30, 40);
		}
		myGLCD.print("<->", 66, 40);
		myGLCD.print("  ", 130, 40);
		//myGLCD.printNumI(canal_N, 130, 40);
		if (digitalRead(Chanal_A) == HIGH)                                       // Последовательно проверить все вывода разьема "A"
		{
			myGLCD.print(" - Pass ", 170, 40);
		}
		else
		{
			myGLCD.print(" - Error", 170, 40);
			count_error++;
			strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
			myGLCD.print(buffer, 50, 65);                                       // txt_error_connect4
			myGLCD.printNumI(count_error, 190, 65);

			if (x_A < 10)
			{
				myGLCD.printNumI(x_A, x_p + 13, y_p);                          // Перечисление ошибочных контактов
				myGLCD.print(" - A", x_p + 29, y_p);
			}
			else
			{
				myGLCD.printNumI(x_A, x_p, y_p);                               // Перечисление ошибочных контактов
				myGLCD.print(" - A", x_p + 29, y_p);
			}
			y_p += 19;
			if (y_p > 190)                                                     // Вывод на экран таблицы ошибок
			{
				myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
				x_p += 80;
				y_p = 82;
			}
		}

		if (digitalRead(Chanal_B) == HIGH)                                     // Последовательно проверить все вывода разьема "В"
		{
			myGLCD.print(" - Pass ", 170, 40);
		}
		else
		{
			myGLCD.print(" - Error", 170, 40);
			count_error++;
			strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
			myGLCD.print(buffer, 50, 65);                                     // txt_error_connect4
			myGLCD.printNumI(count_error, 190, 65);
			if (x_A < 10)
			{
				myGLCD.printNumI(x_A, x_p + 13, y_p);                         // Перечисление ошибочных контактов
				myGLCD.print(" - B", x_p + 29, y_p);
			}
			else
			{
				myGLCD.printNumI(x_A, x_p, y_p);                              // Перечисление ошибочных контактов
				myGLCD.print(" - B", x_p + 29, y_p);
			}
			y_p += 19;
			if (y_p > 190)                                                    // Вывод на экран таблицы ошибок
			{
				myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
				x_p += 80;
				y_p = 82;
			}
		}
	}

	while(!myTouch.dataAvailable())
	{
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();

		if (((y >= 1) && (y <= 150)) && ((x >= 10) && (x <= 319))) //Возврат
		{
			myGLCD.setFont(BigFont);
			break;
		}
	}
}
 
void test_all_pin()
{
  myGLCD.clrScr();
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[17])));
  myGLCD.print(buffer, CENTER, 20);                                           // txt_test_all
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[18])));
  myGLCD.print(buffer, CENTER, 180);                                          // txt_test_all_exit1
  strcpy_P(buffer, (char*)pgm_read_word(&(table_message[19])));
  myGLCD.print(buffer, CENTER, 200);                                          // txt_test_all_exit2
  byte canal_N = 0;
  pinMode(Chanal_B, INPUT);                                                         // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
  pinMode(Chanal_A, INPUT);                                                         // Установить на вход  выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
  digitalWrite(Chanal_B, HIGH);                                                     // Установить высокий уровень на выводе Chanal_B
  digitalWrite(Chanal_A, HIGH);                                                     // Установить высокий уровень на выводе Chanal_A
  int i_step = 1;

  while (true)
  {
	if (myTouch.dataAvailable())
	{
	  myTouch.read();
	  x = myTouch.getX();
	  y = myTouch.getY();

	  if (((y >= 1) && (y <= 150)) && ((x >= 10) && (x <= 319))) //Возврат
	  {
		myGLCD.setFont(BigFont);
		break;
	  }
	}
	set_komm_mcp('A', i_step, 'O');                                         // Переключить коммутатор разъемов серии "А" на вход
	set_komm_mcp('B', i_step, 'O');                                         // Переключить коммутатор разъемов серии "В" на вход
	delay(10);
	if (digitalRead(Chanal_B) == LOW)
	{
	  myGLCD.print("B", CENTER, 80);
	  myGLCD.print("  ", CENTER, 105);
	  if (i_step == 39 || i_step == 40 ||  i_step == 41)
	  {
		myGLCD.print("1", CENTER, 105);
	  }
	  else
	  {
		myGLCD.printNumI(i_step, CENTER, 105);
	  }
	}
	else if (digitalRead(Chanal_A) == LOW)
	{
	  myGLCD.print("A", CENTER, 80);
	  myGLCD.print("  ", CENTER, 105);
	  if (i_step == 39 || i_step == 40 ||  i_step == 41)
	  {
		myGLCD.print("1", CENTER, 105);
	  }
	  else
	  {
		myGLCD.printNumI(i_step, CENTER, 105);
	  }
	}
	i_step++;
	if (i_step == 42) i_step = 1;
  }
}
void kommut_off()
{
  mcp_Out1.digitalWrite(8,  HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E1  U13
  mcp_Out1.digitalWrite(9,  HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E2  U17
  mcp_Out1.digitalWrite(10, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E3  U23
  mcp_Out1.digitalWrite(11, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E4  U14
  mcp_Out1.digitalWrite(12, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E5  U19
  mcp_Out1.digitalWrite(13, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  1E6  U21

  mcp_Out2.digitalWrite(8,  HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E1  U15
  mcp_Out2.digitalWrite(9,  HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E2  U18
  mcp_Out2.digitalWrite(10, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E3  U22
  mcp_Out2.digitalWrite(11, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E4  U16
  mcp_Out2.digitalWrite(12, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E5  U20
  mcp_Out2.digitalWrite(13, HIGH);                                // Сброс выбора EN микросхемы аналового коммутатора  2E6  U24
}

void view_tab()
{
	myGLCD.clrScr();
	myGLCD.print("Ta""\x96\xA0\x9D\xA6""a coe""\x99\x9D\xA2""e""\xA2\x9D\x9E", CENTER, 1);
	myGLCD.setColor(255, 255, 255);                                    // Установить белый цвет текста
	myGLCD.drawRoundRect(160, 200, 315, 239);                          // Белая окантовка кнопки Завершить
	myGLCD.setColor(0, 0, 255);                                        // Установить синий цвет текста
	myGLCD.fillRoundRect(6, 201, 154, 238);                            // Закрасить фон синим
	myGLCD.fillRoundRect(161, 201, 314, 238);                          // Закрасить фон синим
	myGLCD.setColor(255, 255, 255);                                    // Установить белый цвет текста
	myGLCD.drawRoundRect(5, 200, 155, 239);                            // Белая окантовка кнопки Повторить
	myGLCD.setBackColor(0, 0, 255);                                    // Установить синий цвет фона
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[21])));
	myGLCD.print(buffer, 10, 210);                                     //txt_test_repeat  Повторить
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[20])));
	myGLCD.print(buffer, 168, 210);                                    //txt_test_end Завершить
	myGLCD.setBackColor(0, 0, 0);                                      // Черный фон

	myGLCD.print("N1", 27, 32);
	myGLCD.print("N2", 105, 32);
	myGLCD.print("N3", 183, 32);
	myGLCD.print("N4", 261, 32);
	myGLCD.print("\x80""BC", 20, 72);

	myGLCD.setColor(255, 255, 255);                                   // Установить белый цвет текста
	myGLCD.drawRoundRect(5, 25, 78, 55);                              // Белая окантовка
	myGLCD.drawRoundRect(83, 25, 156, 55);                            // Белая окантовка
	myGLCD.drawRoundRect(161, 25, 234, 55);                           // Белая окантовка
	myGLCD.drawRoundRect(239, 25, 312, 55);                           // Белая окантовка
	myGLCD.drawRoundRect(5, 65, 78, 95);                              // Белая окантовка

	myGLCD.setColor(255, 0, 0);
	myGLCD.print("Ta""\x96\xA0\x9D\xA6\xAB"" o""\x96""y""\xA7""e""\xA2\x9D\xAF", CENTER, 100);   // 

	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(5, 125, 78, 155);
	myGLCD.drawRoundRect(83, 125, 156, 155);
	myGLCD.drawRoundRect(161, 125, 234, 155);
	myGLCD.drawRoundRect(239, 125, 312, 155);


	myGLCD.print("N1", 27, 132);
	myGLCD.print("N2", 105, 132);
	myGLCD.print("N3", 183, 132);
	myGLCD.print("N4", 261, 132);
}

void table_cont()                                                             // Вывод таблицы соединений контактов
{
	int tab_n     = 0;
	int x_p       = 1;                                                        // Определить начало вывода по Х
	int y_p       = 70;                                                       // Определить начало вывода по У
	int canal_N   = 1;
	bool info_run = false;                                                    // Предотвратить повторный вызов программы                                
	view_tab();                                                               // Нарисовать меню выбора 
	while (true)                                                              // Ожидание очередных комманд
	{
	  if (myTouch.dataAvailable())
	  {
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();
		
		/*if (!info_run)
		{*/
			// ***************************  Вызов фиксированной таблицы **************************************
			if (((x >= 5) && (x <= 78)) && ((y >= 25) && (y <= 55)))                 //нажата кнопка 1
			{
				waitForIt(5, 25, 78, 55);
				info_run = true;
				tab_n = adr_memN1_1;
				info_table(tab_n);
			}
			else if (((x >= 83) && (x <= 156)) && ((y >= 25) && (y <= 55)))         //нажата кнопка 2
			{
				waitForIt(83, 25, 156, 55);
				info_run = true;
				tab_n = adr_memN1_2;
				info_table(tab_n);
			}

			else if (((x >= 161) && (x <= 234)) && ((y >= 25) && (y <= 55)))         //нажата кнопка 3
			{
				waitForIt(161, 25, 234, 55);
				info_run = true;
				tab_n = adr_memN1_3;
				info_table(tab_n);
			}
			else if (((x >= 239) && (x <= 312)) && ((y >= 25) && (y <= 55)))         //нажата кнопка 4
			{
				waitForIt(239, 25, 312, 55);
				info_run = true;
				tab_n = adr_memN1_4;
				info_table(tab_n);
			}
			// ***************************  Вызов настраиваемой таблицы (таблица обучения) **************************************
			else if (((x >= 5) && (x <= 78)) && ((y >= 125) && (y <= 155)))           //нажата кнопка 1
			{
				waitForIt(5, 125, 78, 155);
				info_run = true;
				tab_n = adr_memN2_1;
				info_table(tab_n);
			}
			else if (((x >= 83) && (x <= 156)) && ((y >= 125) && (y <= 155)))         //нажата кнопка 2
			{
				waitForIt(83, 125, 156, 155);
				info_run = true;
				tab_n = adr_memN2_2;
				info_table(tab_n);
			}

			else if (((x >= 161) && (x <= 234)) && ((y >= 125) && (y <= 155)))         //нажата кнопка 3
			{
				waitForIt(161, 125, 234, 155);
				info_run = true;
				tab_n = adr_memN2_3;
				info_table(tab_n);
			}
			else if (((x >= 239) && (x <= 312)) && ((y >= 125) && (y <= 155)))         //нажата кнопка 4
			{
				waitForIt(239, 125, 312, 155);
				info_run = true;
				tab_n = adr_memN2_4;
				info_table(tab_n);
			}
			else if (((x >= 5) && (x <= 78)) && ((y >= 65) && (y <= 95)))             //нажата кнопка БВС
			{
				waitForIt(5, 65, 78, 95);
				info_run = true;
				tab_n = adr_memN1_5;
				info_table(tab_n);
			}
		//}

		else if ((y >= 200) && (y <= 239))
		{
			if ((x >= 5) && (x <= 155))                                     //нажата кнопка "Повторить проверку"  
			{
				waitForIt(5, 200, 155, 239);
				if (view_tab_run == false)                                  // Проверить окончание вывода таблицы
				{
					info_table(tab_n);
				}

			}
			if ((x >= 160) && (x <= 315))                                   //нажата кнопка "Завершить  проверку"
			{
				waitForIt(160, 200, 315, 239);
				break;                                                      // Выход из программы
			}
		}
	  }
	}
}
void info_table(int adr_mem)
{
	
	int _adr_mem = adr_mem;
	byte  _size_block = i2c_eeprom_read_byte(deviceaddress, _adr_mem);              // Получить количество выводов проверяемого разъема
	byte canal_N = 0;                                                               // Переменная хранения № канала в памяти
	int x_p = 0;                                                                    // Определить начало вывода ошибок по Х
	int y_p = 20;                                                                   // Определить начало вывода ошибок по У
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRoundRect(0, 20, 319, 199);

	myGLCD.setColor(0, 0, 255);                                                     // Установить синий цвет текста
	myGLCD.fillRoundRect(6, 201, 154, 238);                                         // Закрасить фон синим
	myGLCD.setColor(255, 255, 255);
	//Serial.println(_size_block);
	view_tab_run = true;
	
	for (int i = 1; i < _size_block + 1; i++)                                       // Последовательное чтение контактов разьемов.
	{
		myGLCD.drawLine(x_p + 75, 20, x_p + 75, 194);
		myGLCD.drawLine(x_p + 75+42, 20, x_p + 75+42, 194);
		
		//canal_N = i2c_eeprom_read_byte(deviceaddress, _adr_mem + i);                // Получить № канала из EEPROM
		if (i < 10)
		{
			myGLCD.printNumI(i, x_p + 13, y_p);                                     // Вывод на экран номара контактов меньше 10
			myGLCD.print("-", x_p + 29, y_p);
		}
		else
		{
			myGLCD.printNumI(i, x_p, y_p);                                          // Вывод на экран номера контактов больше 9
			myGLCD.print("-", x_p + 29, y_p);
		}
		Serial.print(_adr_mem + i); Serial.print("\t"); Serial.print(canal_N); Serial.print("\t");
		canal_N = i2c_eeprom_read_byte(deviceaddress, _adr_mem + i );               // Получить из таблицы номер входа коммутатора.
		Serial.println(canal_N);
		if (canal_N < 10)
		{
			if (canal_N == 0)
			{
				myGLCD.print("X", x_p + 58, y_p);                                   // Вывод на экран "Х" (не подключен)
			}
			else
			{
				myGLCD.printNumI(canal_N, x_p + 58, y_p);                           // Вывод на экран № контактов
			}
		}
		else
		{
			if (canal_N == 0)
			{
				myGLCD.print("X", x_p + 58, y_p);                                   // Вывод на экран  "Х" (не подключен)
			}
			else
			{
				myGLCD.printNumI(canal_N, x_p + 42, y_p);                             // Вывод на экран № контактов
			}
		}

		canal_N = i2c_eeprom_read_byte(deviceaddress, _adr_mem + i + _size_block);    // Получить из таблицы (второй блок данных)номер входа коммутатора.

		if (canal_N < 10)
		{
			if ((canal_N == 0) || (canal_N == 255))
			{
				myGLCD.print("X", x_p + 58+40, y_p);                                  // Вывод на экран  "Х" (не подключен)
			}
			else
			{
				myGLCD.printNumI(canal_N, x_p + 58+40, y_p);                         // Вывод на экран № контактов
			}
		}
		else
		{
			if ((canal_N == 0) || (canal_N == 255))
			{
				myGLCD.print("X", x_p + 58+40, y_p);                                //  Вывод на экран  "Х" (не подключен)
			}
			else
			{
				myGLCD.printNumI(canal_N, x_p + 42+40, y_p);                        // Вывод на экран № контактов
			}
		}

		canal_N = i2c_eeprom_read_byte(deviceaddress, _adr_mem + i + _size_block * 2);  // Получить из таблицы (третий блок данных) номер входа коммутатора.

		if (canal_N < 10)
		{
			if ((canal_N == 0) || (canal_N == 255))
			{
				myGLCD.print("X", x_p + 58 + 40+40, y_p);                              //  Вывод на экран  "Х" (не подключен)
			}
			else
			{
				myGLCD.printNumI(canal_N, x_p + 58 + 40+40, y_p);                         // Вывод на экран № контактов
			}
		}
		else
		{
			if ((canal_N == 0) || (canal_N == 255))
			{ 
				myGLCD.print("X", x_p + 58 + 40+40, y_p);                                //  Вывод на экран  "Х" (не подключен)
			}
			else
			{
				myGLCD.printNumI(canal_N, x_p + 42 + 40+40, y_p);                        // Вывод на экран № контактов
			}
		}

		y_p += 18;                                                                      // Перейти на следующую строку
	
		if ((y_p > 190))                                                                //  Если страница заполнена - перейти в начало
		{
			myGLCD.drawLine(161, 20, 161, 199);
			myGLCD.drawLine(159, 20, 159, 199);
			x_p += 162;                                                                 //  Вторая колонка
			y_p = 20;                                                                   //  Если страница заполнена - перейти в начало
			if (x_p > 240)                                                              // Если таблица сформирована полностью.
			{
				if (i >= _size_block)        break;                                   // Определить последнюю страницу вывода таблицы  
				while (!myTouch.dataAvailable()){}                                      // Ожидание очередных комманд
				if (myTouch.dataAvailable())                                            
				{
					myTouch.read();
					x = myTouch.getX();
					y = myTouch.getY();
					if (((x >= 1) && (x <= 319)) && ((y >= 1) && (y <= 199))||((x >= 1) && (x <= 155)) && ((y >= 200) && (y <= 239)))          //нажат экран (продолжить вывод)
					{
						waitForIt(1, 1, 319, 199);
						if (i < _size_block)                                           // Определить последнюю страницу вывода таблицы  
						{
							x_p = 0;                                                   // Перейти на следующий экран    
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect(0, 20, 319, 199);                     // Очистить экран
							myGLCD.setColor(255, 255, 255);
							view_tab_run = true;
						}
					}
					if (((x >= 160) && (x <= 315)) && ((y >= 200) && (y <= 239)))      //нажата кнопка "Завершить  проверку"
					{
						waitForIt(160, 200, 315, 239);
						view_tab_run = false;
						break;                                                         // Выход из программы просмотра
					}
				}
			}
		}
	}

	if(view_tab_run) while (!myTouch.dataAvailable()) {}                              // Ожидание прекращения нажатия на экран (защита от дребезга)
	myGLCD.setColor(255, 255, 255);                                                   // Установить белый цвет текста
	myGLCD.setBackColor(0, 0, 255);                                                   // Установить синий цвет фона
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[21])));
	myGLCD.print(buffer, 10, 210);                                                    //txt_test_repeat  Повторить
	myGLCD.setBackColor(0, 0, 0);
	view_tab();
	view_tab_run = false;
}

void view_menu_search()
{
	myGLCD.clrScr();

	myGLCD.setColor(255, 255, 255);                                             // Белая окантовка
	myGLCD.drawRoundRect(5, 200, 155, 239);
	myGLCD.drawRoundRect(160, 200, 315, 239);

	myGLCD.drawRoundRect(5, 20, 285, 55);
	myGLCD.drawRoundRect(5, 65, 285, 100);
	myGLCD.drawRoundRect(5, 110, 285, 145);
	myGLCD.drawRoundRect(5, 155, 285, 190);

	//myGLCD.drawLine(10, 60, 310, 60);
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect(6, 201, 154, 238);
	myGLCD.fillRoundRect(161, 201, 314, 238);

	myGLCD.fillRoundRect(6, 21, 284, 54);
	myGLCD.fillRoundRect(6, 66, 284, 99);
	myGLCD.fillRoundRect(6, 111, 284, 144);
	myGLCD.fillRoundRect(6, 156, 284, 189);



	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(0, 0, 255);

	myGLCD.print("Bap""\x9D""a""\xA2\xA4"" ""\x9F""a""\x96""e""\xA0\xAF"" N1", 10, 30);    // Вариант кабеля N1
	myGLCD.print("Bap""\x9D""a""\xA2\xA4"" ""\x9F""a""\x96""e""\xA0\xAF"" N2", 10, 75);    // Вариант кабеля N2
	myGLCD.print("Bap""\x9D""a""\xA2\xA4"" ""\x9F""a""\x96""e""\xA0\xAF"" N3", 10, 120);   // Вариант кабеля N3
	myGLCD.print("Bap""\x9D""a""\xA2\xA4"" ""\x9F""a""\x96""e""\xA0\xAF"" N4", 10, 165);   // Вариант кабеля N4

	myGLCD.print("B""\xAB\xA3""o""\xA0\xA2\x9D\xA4\xAC", 10, 210);                         // Выполнить
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[20])));
	myGLCD.print(buffer, 168, 210);                                                        // Завершить
	myGLCD.setBackColor(0, 0, 0);

}
void copy_cable()
{
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	myGLCD.clrScr();
	view_menu_search();
	byte N_block;

	while (true)
	{
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			x = myTouch.getX();
			y = myTouch.getY();


			if (((y >= 20) && (y <= 55)) && ((x >= 5) && (x <= 285)))          //Выполнить
			{
				waitForIt(5, 20, 285, 55);
				myGLCD.setColor(255, 0, 0);                                    // Красная окантовка
				myGLCD.drawRoundRect(5, 20, 285, 55);
				myGLCD.setColor(255, 255, 255);                                // Белая окантовка
				myGLCD.drawRoundRect(5, 65, 285, 100);
				myGLCD.drawRoundRect(5, 110, 285, 145);
				myGLCD.drawRoundRect(5, 155, 285, 190);
				N_block = 1;

			}
			if (((y >= 65) && (y <= 100)) && ((x >= 5) && (x <= 285)))          //Выполнить
			{
				waitForIt(5, 65, 285, 100);
				myGLCD.setColor(255, 0, 0);                                     // Красная окантовка
				myGLCD.drawRoundRect(5, 65, 285, 100);
				myGLCD.setColor(255, 255, 255);                                 // Белая окантовка
				myGLCD.drawRoundRect(5, 20, 285, 55);
				myGLCD.drawRoundRect(5, 110, 285, 145);
				myGLCD.drawRoundRect(5, 155, 285, 190);
				N_block = 2;

			}

			if (((y >= 110) && (y <= 145)) && ((x >= 5) && (x <= 285)))         //Выполнить
			{
				waitForIt(5, 110, 285, 145);
				myGLCD.setColor(255, 0, 0);                                     // Красная окантовка
				myGLCD.drawRoundRect(5, 110, 285, 145);
				myGLCD.setColor(255, 255, 255);                                 // Белая окантовка
				myGLCD.drawRoundRect(5, 20, 285, 55);
				myGLCD.drawRoundRect(5, 65, 285, 100);
				myGLCD.drawRoundRect(5, 155, 285, 190);
				N_block = 3;
			}

			if (((y >= 155) && (y <= 190)) && ((x >= 5) && (x <= 285)))         //Выполнить
			{
				waitForIt(5, 155, 285, 190);
				myGLCD.setColor(255, 0, 0);                                     // Красная окантовка
				myGLCD.drawRoundRect(5, 155, 285, 190);
				myGLCD.setColor(255, 255, 255);                                 // Белая окантовка
				myGLCD.drawRoundRect(5, 20, 285, 55);
				myGLCD.drawRoundRect(5, 65, 285, 100);
				myGLCD.drawRoundRect(5, 110, 285, 145);
				N_block = 4;
			}

			if (((y >= 200) && (y <= 239)) && ((x >= 5) && (x <= 155)))       //Выполнить
			{
				waitForIt(5, 200, 155, 239);
				myGLCD.setFont(BigFont);
				myGLCD.clrScr();
				search_contacts(N_block);
				view_menu_search();
			}
			if (((y >= 200) && (y <= 239)) && ((x >= 160) && (x <= 315)))     //Завершить
			{
				waitForIt(160, 200, 315, 239);
				myGLCD.setFont(BigFont);
				break;
			}
		}
	}
}

void search_contacts(byte N_block)
{
	byte  _size_block = 48;                                                      // Получить количество выводов проверяемого разъема
	pinMode(Chanal_A, OUTPUT);                                                   // Установить на выход выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
	pinMode(Chanal_B, INPUT);                                                    // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
	digitalWrite(Chanal_B, HIGH);                                                // Установить высокий уровень на выводе Chanal_B
	myGLCD.print("                    ", 1, 40);                                 // Очистить строчку результатов проверки
	byte canal_N = 0;                                                            // Переменная хранения № канала в памяти
	unsigned int x_A = 1;                                                        // Переменная установления канала А
	unsigned int x_B = 1;                                                        // Переменная установления канала В
	int x_p = 1;                                                                 // Определить начало вывода ошибок по Х
	int y_p = 1;                                                                 // Определить начало вывода ошибок по У
	int count_error = 0;                                                         // Счетчик количества ошибок
	int ware_on = 0;                                                             // Проверка должно ли быть сединение
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRoundRect(1, 1 , 319, 209);
	myGLCD.setColor(255, 255, 255);

	y_p = 40;                                                                    // Восстановить начало вывода ошибок по У
	x_p = 1;                                                                     // Восстановить начало вывода ошибок по Х

	int adr_block = adr_memN2_1;                                                 // Переменная для хранения адреса блока

	switch (N_block)
	{
	case 1:
		adr_block = adr_memN2_1;
		break;
	case 2:
		adr_block = adr_memN2_2;
		break;
	case 3:
		adr_block = adr_memN2_3;
		break;
	case 4:
		adr_block = adr_memN2_4;
		break;
	default:
		adr_block = adr_memN2_1;
		break;
	}
	myGLCD.print("O""\xA7\x9D""c""\xA4\x9F""a ""\xA4""a""\x96\xA0\x9D\xA6\xAB", CENTER, 10);                    // Очистка таблицы
	for (int i=0;i<240;i++)
	{
		i2c_eeprom_write_byte(deviceaddress, adr_block+i, 0);                                                   // Очистить таблицу соединений
	}
	myGLCD.print("\x8B""op""\xA1\x9D""po""\x97""a""\xA2\x9D""e ""\xA4""a""\x96\xA0\x9D\xA6\xAB", CENTER, 10);   // Формирование таблицы

	i2c_eeprom_write_byte(deviceaddress, adr_block, _size_block);                                               // Сохранить в "0" ячейку количество выводов кабеля 

	digitalWrite(Chanal_A, LOW);                                                                                // Установить контрольный уровень на коммутаторах U13,U17,U23
	delay(10);                                                                                                  // Время на переключение вывода Chanal_A
	
	myGLCD.drawLine(80, 40, 80, 194);
	myGLCD.drawLine(122, 40, 122, 194);
	myGLCD.drawLine(164, 40, 164, 199);
	myGLCD.drawLine(208, 40, 208, 199);


	for (x_A = 1; x_A < _size_block + 1; x_A++)                                                                 // Последовательное чтение контактов разьемов.
	{
		//i2c_eeprom_write_byte(deviceaddress, adr_block + x_A, x_A);                                           // Записать номер проверяемого контакта в таблицу с учетом варианта кабеля
		Serial.print(adr_block + x_A); Serial.print("\t"); Serial.print(x_A); Serial.print("\t");
	
		set_komm_mcp('A', x_A, 'O');                                                                            // Установить текущий вход коммутатора
		
		if (x_A < 10)
		{
			myGLCD.printNumI(x_A, x_p + 13, y_p);                                                                // Перечисление  контактов
			myGLCD.print("-", x_p + 30, y_p);
		}
		else
		{
			myGLCD.printNumI(x_A, x_p, y_p);                                                                     // Перечисление   контактов
			myGLCD.print("-", x_p + 30, y_p);
		}
																												// Проверить на парралельное включение контактов
		int i_block = 1;																						// Последовательно проверить все вывода разьема "В"
																												// Проверяем все выхода разьема "В"
		for (x_B = 1; x_B < _size_block + 1; x_B++)                                                             // Последовательное чтение контактов разьемов "В" .
		{
			set_komm_mcp('B', x_B, 'O');                                                                        // Установить текущий вход коммутатора
		
			// ++++++++++++++++++++++++ Проверка на соединение А - В +++++++++++++++++++++++++++++++++++
		
			if (digitalRead(Chanal_B) == LOW)                                                                   // вывод на экран при соединение проводов 
			{                
				i2c_eeprom_write_byte(deviceaddress, adr_block + x_A + (_size_block*(i_block-1)), x_B);             // Записать номер контакта в таблицу с учетом варианта кабеля
				Serial.println(adr_block + x_A); Serial.print("\t"); Serial.print( x_A ); Serial.print("\t"); Serial.println(x_B);
			

				if (x_B < 10)
				{
					myGLCD.printNumI(x_B, (x_p + 13 + (42 * i_block)), y_p);                                     // Перечисление   контактов
				}
				else
				{
					myGLCD.printNumI(x_B, (x_p + 1 + (42 * i_block)), y_p);                                        // Перечисление  контактов
				}

				i_block++;
				if (i_block > 4) i_block = 4;
			}
			else
			{
				myGLCD.print("x", (x_p + 13 + (42 * i_block)), y_p);                                     // Перечисление   контактов

			}


		}
		y_p += 19;
	
		if (y_p > 185)                                            // Вывод на экран таблицы ошибок
		{
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(1, 40, 319, 209);
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawLine(80, 40, 80, 194);
			myGLCD.drawLine(122, 40, 122, 194);
			myGLCD.drawLine(164, 40, 164, 199);
			myGLCD.drawLine(208, 40, 208, 199);
			y_p = 40;
			delay(300);
		}
	}
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRoundRect(1, 40, 319, 209);
	myGLCD.setColor(0, 255, 0);
	myGLCD.print("\x85""ABEP""\x8E""EHO", CENTER, 110);                                               // ЗАВЕРШЕНО
	myGLCD.print("Ha""\x9B\xA1\x9D\xA4""e ""\xA2""a ""\x99\x9D""c""\xA3\xA0""e""\x9E", CENTER, 208);  // Нажмите на дисплей
	myGLCD.print("\x99\xA0\xAF"" ""\x9C""a""\x97""ep""\xA8""e""\xA2\x9D\xAF", CENTER, 222);           // для завершения
	myGLCD.setColor(255, 255, 255);
	delay(2000);

	//do {
	//	if (myTouch.dataAvailable())
	//	{
	//		myTouch.read();
	//		x = myTouch.getX();
	//		y = myTouch.getY();
	//		if (((y >= 1) && (y <= 239)) && ((x >= 1) && (x <= 319)))         //нажат экран
	//		{
	//			waitForIt(1, 1, 319, 239);
	//			break;
	//		}
	//	}
	//} while (true);
}

void test_cabel_soft(int cable_soft)
{
	int _cable_soft = cable_soft;
	myGLCD.clrScr();
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[60])));
	myGLCD.print(buffer,5, 1);                                                 // Тест копия кабеля N
	myGLCD.printNumI(_cable_soft, RIGHT, 1);                                   // Номер кабеля
	myGLCD.setColor(255, 255, 255);                                            // Белая окантовка
	myGLCD.drawRoundRect(5, 200, 155, 239);
	myGLCD.drawRoundRect(160, 200, 315, 239);
	myGLCD.drawLine(10, 60, 310, 60);
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect(6, 201, 154, 238);
	myGLCD.fillRoundRect(161, 201, 314, 238);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(0, 0, 255);
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[21])));
	myGLCD.print(buffer, 10, 210);                                             //txt_test_repeat  Повторить
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[20])));
	myGLCD.print(buffer, 168, 210);                                            //txt_test_end Завершить
	myGLCD.setBackColor(0, 0, 0);                                              //
	mcp_Out2.digitalWrite(14, LOW);                                            // Отключить реле +12v

	test_cabel_soft_run(_cable_soft);                                          // Выполнить проверку
	while (true)                                                               // Ожидание очередных комманд
	{

		if (myTouch.dataAvailable())
		{
			myTouch.read();
			x = myTouch.getX();
			y = myTouch.getY();

			if (((y >= 200) && (y <= 239)) && ((x >= 5) && (x <= 155)))        //нажата кнопка "Повторить проверку"
			{
				waitForIt(5, 200, 155, 239);
				myGLCD.setFont(BigFont);
				test_cabel_soft_run(_cable_soft);                              // Выполнить проверку
			}
			if (((y >= 200) && (y <= 239)) && ((x >= 160) && (x <= 315))) //нажата кнопка "Завершить  проверку"
			{
				waitForIt(160, 200, 315, 239);
				myGLCD.setFont(BigFont);
				break;                                                    // Выход из программы
			}
		}

	}
}

void test_cabel_soft_run(int cable_soft_run)
{
	int _cable_soft_run = cable_soft_run;
	int _adr_mem_soft = 0;
	byte  _size_block = 0;
	switch (_cable_soft_run) {
	case 1:
		 _size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN2_1);        // Получить количество выводов проверяемого разъема
		 _adr_mem_soft = adr_memN2_1;
		break;
	case 2:
		_size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN2_2);        // Получить количество выводов проверяемого разъема
		_adr_mem_soft = adr_memN2_2;
		break;
	case 3:
		_size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN2_3);        // Получить количество выводов проверяемого разъема
		_adr_mem_soft = adr_memN2_3;
		break;
	case 4:
		_size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN2_4);        // Получить количество выводов проверяемого разъема
		_adr_mem_soft = adr_memN2_4;
		break;
	default:
		_size_block = i2c_eeprom_read_byte(deviceaddress, adr_memN2_1);        // Получить количество выводов проверяемого разъема
		_adr_mem_soft = adr_memN2_1;
		break;
	}

	
	pinMode(Chanal_A, INPUT);                                                    // Установить на вход  выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
	pinMode(Chanal_B, INPUT);                                                    // Установить на вход  выход коммутаторов U15,U18,U22 (разъемы серии А на передней панели)
	digitalWrite(Chanal_A, HIGH);                                                // Установить высокий уровень на выводе Chanal_A (подключить резисторы PULL_UP)
	digitalWrite(Chanal_B, HIGH);                                                // Установить высокий уровень на выводе Chanal_B (подключить резисторы PULL_UP)
	myGLCD.print("                    ", 1, 40);                                 // Очистить строчку результатов проверки
	byte chanal_1 = 0;                                                           // Переменная хранения №1 канала в памяти
	byte chanal_2 = 0;                                                           // Переменная хранения №2 канала в памяти
	byte chanal_3 = 0;                                                           // Переменная хранения №3 канала в памяти

	unsigned int x_A = 1;                                                        // Переменная установления канала А
	unsigned int x_B = 1;                                                        // Переменная установления канала В
	int x_p = 1;                                                                 // Определить начало вывода ошибок по Х
	int y_p = 82;                                                                // Определить начало вывода ошибок по У
	int count_error = 0;                                                         // Счетчик количества ошибок
	int ware_on = 0;                                                             // Проверка должно ли быть сединение

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRoundRect(1, 82, 319, 199);
	myGLCD.setColor(255, 255, 255);

	myGLCD.print("Tec""\xA4"" ""\x9C""a""\xA1\xAB\x9F"". ""\xA2""a ""\x9C""e""\xA1\xA0\xAE", CENTER, 20);    //Тест замык. на землю
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[24])));                // txt_error_connect3 "Ошибок нет"
	myGLCD.print(buffer, 50, 65);                                                // txt_error_connect3 "Ошибок нет"
	                                                                             // Проверить на отсутствие замыканий на землю  
	digitalWrite(Chanal_A, HIGH);                                                // Установить высокий уровень на коммутаторах U13,U17,U23
	delay(10);                                                                   // Время на переключение вывода Chanal_A
	
	//++++++++++++++++++++++++++++++++++++++ Проверка замыкания на землю +++++++++++++++++++++++++++++++++++++++++++++++++++
	for (x_A = 1; x_A < 49; x_A++)                                               // Последовательное чтение контактов разьемов.
	{
		set_komm_mcp('A', x_A, 'O');                                             // Установить текущий вход коммутатора
		set_komm_mcp('B', x_A, 'O');                                             // Установить текущий вход коммутатора
		if (x_A < 10)
		{
			myGLCD.printNumI(x_A, 30+12, 40);
		}
		else
		{
			myGLCD.printNumI(x_A, 30, 40);
		}
		myGLCD.print("<->", 66, 40);
		myGLCD.print("  ", 130, 40);
		//myGLCD.printNumI(canal_N, 130, 40);
		if (digitalRead(Chanal_A) == HIGH)                                       // Последовательно проверить все вывода разьема "A"
		{
			myGLCD.print(" - Pass ", 170, 40);
		}
		else
		{
			myGLCD.print(" - Error", 170, 40);
			count_error++;
			strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
			myGLCD.print(buffer, 50, 65);                                       // txt_error_connect4
			myGLCD.printNumI(count_error, 190, 65);

			if (x_A < 10)
			{
				myGLCD.printNumI(x_A, x_p + 13, y_p);                          // Перечисление ошибочных контактов
				myGLCD.print(" - A", x_p + 29, y_p);
			}
			else
			{
				myGLCD.printNumI(x_A, x_p, y_p);                               // Перечисление ошибочных контактов
				myGLCD.print(" - A", x_p + 29, y_p);
			}
			y_p += 19;
			if (y_p > 190)                                                     // Вывод на экран таблицы ошибок
			{
				myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
				x_p += 80;
				y_p = 82;
			}
		}

		if (digitalRead(Chanal_B) == HIGH)                                     // Последовательно проверить все вывода разьема "В"
		{
			myGLCD.print(" - Pass ", 170, 40);
		}
		else
		{
			myGLCD.print(" - Error", 170, 40);
			count_error++;
			strcpy_P(buffer, (char*)pgm_read_word(&(table_message[25])));
			myGLCD.print(buffer, 50, 65);                                     // txt_error_connect4
			myGLCD.printNumI(count_error, 190, 65);
			if (x_A < 10)
			{
				myGLCD.printNumI(x_A, x_p + 13, y_p);                         // Перечисление ошибочных контактов
				myGLCD.print(" - B", x_p + 29, y_p);
			}
			else
			{
				myGLCD.printNumI(x_A, x_p, y_p);                              // Перечисление ошибочных контактов
				myGLCD.print(" - B", x_p + 29, y_p);
			}
			y_p += 19;
			if (y_p > 190)                                                    // Вывод на экран таблицы ошибок
			{
				myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
				x_p += 80;
				y_p = 82;
			}
		}																        
  	}

	//------------------------------------------------------------------------------------------------------

//	while (!myTouch.dataAvailable()) {}                                         // Ожидание очередных комманд

	delay(1000);                                                                 // Ожидание просмотра ошибок
	myGLCD.print("                                ", CENTER, 20);                                                // 
	myGLCD.print("                   ", 1, 40);
	pinMode(Chanal_A, OUTPUT);                                                   // Установить на выход выход коммутаторов U13,U17,U23 (разъемы серии В на задней панели)
	digitalWrite(Chanal_A, LOW);                                                 // Установить контрольный уровень на коммутаторах U13,U17,U23
	delay(10);                                                                   // Время на переключение вывода Chanal_A
	
	
	for (x_A = 1; x_A < _size_block + 1; x_A++)                                  // Последовательное чтение контактов разьемов.
	{
		chanal_1 = i2c_eeprom_read_byte(deviceaddress, _adr_mem_soft + x_A);     // Получить из таблицы (блок 1) признак соединения.
		chanal_2 = i2c_eeprom_read_byte(deviceaddress, _adr_mem_soft + x_A + _size_block);     // Получить из таблицы (блок 2) признак соединения.
		chanal_3 = i2c_eeprom_read_byte(deviceaddress, _adr_mem_soft + x_A + (_size_block*2)); // Получить из таблицы (блок 3) признак соединения.

		set_komm_mcp('A', x_A, 'O');                                             // Установить текущий вход коммутатора

		if (x_A < 10)
		{
			myGLCD.printNumI(x_A, 30 + 12, 40);
		}
		else
		{
			myGLCD.printNumI(x_A, 30, 40);
		}
		myGLCD.print("<->", 66, 40);

		delay(100);

		for (x_B = 1; x_B < _size_block + 1; x_B++)                      // Последовательное чтение контактов разьемов "В" .
		{
			set_komm_mcp('B', x_B, 'O');                                 // Установить текущий вход коммутатора
			if (chanal_1 < 10)
			{
				myGLCD.print(" ", 117, 40);
				myGLCD.printNumI(chanal_1, 133, 40);
			}
			else
			{
  				myGLCD.printNumI(chanal_1, 117, 40);
			}
			if (digitalRead(Chanal_B) == LOW)                            // Должен быть соединен
			{
				if (x_B == chanal_1)
				{
					myGLCD.print(" - Pass ", 170, 40);
					//myGLCD.printNumI(chanal_1, 130, 40);
					/*
					if (x_A < 10)
					{
						myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление проверяемых контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
					else
					{
						myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление  проверяемых контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
						myGLCD.print(" - Pass ", 170, 40);

					if (chanal_1 < 10)
					{
						myGLCD.printNumI(chanal_1, x_p + 32 + 26, y_p);   // Перечисление обнаруженных  контактов
					}
					else
					{
						myGLCD.printNumI(chanal_1, x_p + 32 + 10, y_p);   // Перечисление обнаруженных  контактов
					}
					*/
				}
				else if (x_B == chanal_2)
				{
					myGLCD.print(" - Pass ", 170, 40);
				}
				else if (x_B == chanal_3)
				{
					myGLCD.print(" - Pass ", 170, 40);
				}
				else
				{
					if (x_A < 10)
					{
						myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
					else
					{
						myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
					if (chanal_1 < 10)
					{
						myGLCD.printNumI(chanal_1, x_p + 32 + 26, y_p);   // Перечисление обнаруженных  контактов
					}
					else
					{
						myGLCD.printNumI(chanal_1, x_p + 32 + 10, y_p);   // Перечисление обнаруженных  контактов
					}
					myGLCD.print(" - Error", 170, 40);
					count_error++;
					myGLCD.print("   ", 164, 65);
					myGLCD.printNumI(count_error, 164, 65);
					y_p += 19;
					if (y_p > 190)                                          // Вывод на экран таблицы ошибок
					{
						myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
						x_p += 80;
						y_p = 82;
						if (x_p > 320)                                                              // Если таблица сформирована полностью.
						{
							if (x_A >= _size_block)        break;                                   // Определить последнюю страницу вывода таблицы  
							while (!myTouch.dataAvailable()) {}                                      // Ожидание очередных комманд
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect(1, 82, 319, 199);
							myGLCD.setColor(255, 255, 255);
							x_p = 1;                                                                 // Определить начало вывода ошибок по Х
							y_p = 82;                                                                // Определить начало вывода ошибок по У
						}
					}
				}
			}
			else
			{
				if (x_B == chanal_1 && chanal_1 != 0 )
				{
					if (x_A < 10)
					{
						myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
					else
					{
						myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
					if (chanal_1 < 10)
					{
						myGLCD.printNumI(chanal_1, x_p + 32 + 26, y_p);   // Перечисление обнаруженных  контактов
					}
					else
					{
						myGLCD.printNumI(chanal_1, x_p + 32 + 10, y_p);   // Перечисление обнаруженных  контактов
					}

					myGLCD.print(" - Error", 170, 40);
					count_error++;
					myGLCD.print("   ", 164, 65);
					myGLCD.printNumI(count_error, 164, 65);

					y_p += 19;
					if (y_p > 190)                                          // Вывод на экран таблицы ошибок
					{
						myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
						x_p += 80;
						y_p = 82;
						if (x_p > 320)                                                              // Если таблица сформирована полностью.
						{
							if (x_A >= _size_block)        break;                                   // Определить последнюю страницу вывода таблицы  
							while (!myTouch.dataAvailable()) {}                                      // Ожидание очередных комманд
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect(1, 82, 319, 199);
							myGLCD.setColor(255, 255, 255);
							x_p = 1;                                                                 // Определить начало вывода ошибок по Х
							y_p = 82;                                                                // Определить начало вывода ошибок по У
						}
					}
				}
				if (x_B == chanal_2 && chanal_2 != 0)
				{
					if (x_A < 10)
					{
						myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
					else
					{
						myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
					if (chanal_1 < 10)
					{
						myGLCD.printNumI(chanal_2, x_p + 32 + 26, y_p);   // Перечисление обнаруженных  контактов
					}
					else
					{
						myGLCD.printNumI(chanal_2, x_p + 32 + 10, y_p);   // Перечисление обнаруженных  контактов
					}

					myGLCD.print(" - Error", 170, 40);
					count_error++;
					myGLCD.print("   ", 164, 65);
					myGLCD.printNumI(count_error, 164, 65);

					y_p += 19;
					if (y_p > 190)                                          // Вывод на экран таблицы ошибок
					{
						myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
						x_p += 80;
						y_p = 82;
						if (x_p > 320)                                                              // Если таблица сформирована полностью.
						{
							if (x_A >= _size_block)        break;                                   // Определить последнюю страницу вывода таблицы  
							while (!myTouch.dataAvailable()) {}                                      // Ожидание очередных комманд
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect(1, 82, 319, 199);
							myGLCD.setColor(255, 255, 255);
							x_p = 1;                                                                 // Определить начало вывода ошибок по Х
							y_p = 82;                                                                // Определить начало вывода ошибок по У
						}
					}
				}
				if (x_B == chanal_3 && chanal_3 != 0)
				{
					if (x_A < 10)
					{
						myGLCD.printNumI(x_A, x_p + 13, y_p);            // Перечисление ошибочных контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
					else
					{
						myGLCD.printNumI(x_A, x_p, y_p);                 // Перечисление ошибочных контактов
						myGLCD.print("-", x_p + 29, y_p);
					}
					if (chanal_1 < 10)
					{
						myGLCD.printNumI(chanal_3, x_p + 32 + 26, y_p);   // Перечисление обнаруженных  контактов
					}
					else
					{
						myGLCD.printNumI(chanal_3, x_p + 32 + 10, y_p);   // Перечисление обнаруженных  контактов
					}

					myGLCD.print(" - Error", 170, 40);
					count_error++;
					myGLCD.print("   ", 164, 65);
					myGLCD.printNumI(count_error, 164, 65);

					y_p += 19;
					if (y_p > 190)                                          // Вывод на экран таблицы ошибок
					{
						myGLCD.drawLine(x_p + 75, 85, x_p + 75, 190);
						x_p += 80;
						y_p = 82;
						if (x_p > 320)                                                              // Если таблица сформирована полностью.
						{
							if (x_A >= _size_block)        break;                                   // Определить последнюю страницу вывода таблицы  
							while (!myTouch.dataAvailable()) {}                                      // Ожидание очередных комманд
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect(1, 82, 319, 199);
							myGLCD.setColor(255, 255, 255);
							x_p = 1;                                                                 // Определить начало вывода ошибок по Х
							y_p = 82;                                                                // Определить начало вывода ошибок по У
						}
					}
				}
			}
		}
	}
	
	strcpy_P(buffer, (char*)pgm_read_word(&(table_message[30])));
	myGLCD.print(buffer, CENTER, 20);                  // txt__test_end
}

void set_adr_EEPROM()
{
	adr_memN1_1 = 100;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №1А, №1В
	adr_memN1_2 = adr_memN1_1 + sizeof(connektN1_default) + 1;                   // Начальный адрес памяти таблицы соответствия контактов разъемов №2А, №2В
	adr_memN1_3 = adr_memN1_2 + sizeof(connektN2_default) + 1;                   // Начальный адрес памяти таблицы соответствия контактов разъемов №3А, №3В
	adr_memN1_4 = adr_memN1_3 + sizeof(connektN3_default) + 1;                   // Начальный адрес памяти таблицы соответствия контактов разъемов №4А, №4В
	//++++++++++++++++++Меню № 2 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	adr_memN1_5 = adr_memN1_4 + sizeof(connektN4_default) + 1;                   // Начальный адрес памяти таблицы соответствия контактов разъемов №1А, №1В

	adr_memN2_1 = adr_memN1_5+sizeof(connekBVS_default)+1;                       // Начальный адрес памяти таблицы соответствия контактов разъемов №2А, №2В
	adr_memN2_2 = adr_memN2_1+240+1;                                             // Начальный адрес памяти таблицы соответствия контактов разъемов №2А, №2В
	adr_memN2_3 = adr_memN2_2+240+1;                                             // Начальный адрес памяти таблицы соответствия контактов разъемов №3А, №3В
	adr_memN2_4 = adr_memN2_3+240+1;                                             // Начальный адрес памяти таблицы соответствия контактов разъемов №4А, №4В
}
void setup_pin()
{
	pinMode(led_Red, OUTPUT);                             //
	pinMode(led_Green, OUTPUT);                           //
	digitalWrite(led_Red, HIGH);                          //
	digitalWrite(led_Green, LOW);                         //
	pinMode(Chanal_A, INPUT);                                   // Выход коммутаторов блока А
	pinMode(Chanal_B, INPUT);                                   // Выход коммутаторов блока В
	pinMode(Rele1 , OUTPUT);                             //
	pinMode(Rele2 , OUTPUT);                             //
	pinMode(Rele2 , OUTPUT);                             //
	digitalWrite(Rele1 , LOW);                         //
	digitalWrite(Rele2 , LOW);                         //
	digitalWrite(Rele3 , LOW);                         //
}
void setup_mcp()
{
  // Настройка расширителя портов

  mcp_Out1.begin(1);                               //  Адрес (1) U6 первого  расширителя портов
  mcp_Out1.pinMode(0, OUTPUT);                     //  1A1
  mcp_Out1.pinMode(1, OUTPUT);                     //  1B1
  mcp_Out1.pinMode(2, OUTPUT);                     //  1C1
  mcp_Out1.pinMode(3, OUTPUT);                     //  1D1
  mcp_Out1.pinMode(4, OUTPUT);                     //  1A2
  mcp_Out1.pinMode(5, OUTPUT);                     //  1B2
  mcp_Out1.pinMode(6, OUTPUT);                     //  1C2
  mcp_Out1.pinMode(7, OUTPUT);                     //  1D2

  mcp_Out1.pinMode(8, OUTPUT);                     //  1E1   U13  порты А in/out
  mcp_Out1.pinMode(9, OUTPUT);                     //  1E2   U17  порты А in/out
  mcp_Out1.pinMode(10, OUTPUT);                    //  1E3   U23  порты А in/out
  mcp_Out1.pinMode(11, OUTPUT);                    //  1E4   U14  порты А GND
  mcp_Out1.pinMode(12, OUTPUT);                    //  1E5   U19  порты А GND
  mcp_Out1.pinMode(13, OUTPUT);                    //  1E6   U21  порты А GND
  mcp_Out1.pinMode(14, OUTPUT);                    //  1E7   Свободен
  mcp_Out1.pinMode(15, OUTPUT);                    //  1E8   Свободен

  mcp_Out2.begin(2);                               //  Адрес (2) U9 второго  расширителя портов
  mcp_Out2.pinMode(0, OUTPUT);                     //  2A1
  mcp_Out2.pinMode(1, OUTPUT);                     //  2B1
  mcp_Out2.pinMode(2, OUTPUT);                     //  2C1
  mcp_Out2.pinMode(3, OUTPUT);                     //  2D1
  mcp_Out2.pinMode(4, OUTPUT);                     //  2A2
  mcp_Out2.pinMode(5, OUTPUT);                     //  2B2
  mcp_Out2.pinMode(6, OUTPUT);                     //  2C2
  mcp_Out2.pinMode(7, OUTPUT);                     //  2D2

  mcp_Out2.pinMode(8, OUTPUT);                     //  2E1   U15  порты B in/out
  mcp_Out2.pinMode(9, OUTPUT);                     //  2E2   U18  порты B in/out
  mcp_Out2.pinMode(10, OUTPUT);                    //  2E3   U22  порты B in/out
  mcp_Out2.pinMode(11, OUTPUT);                    //  2E4   U16  порты B GND
  mcp_Out2.pinMode(12, OUTPUT);                    //  2E5   U20  порты B GND
  mcp_Out2.pinMode(13, OUTPUT);                    //  2E6   U24  порты B GND
  mcp_Out2.pinMode(14, OUTPUT);                    //  2E7   Реле №1, №2
  mcp_Out2.pinMode(15, OUTPUT);                    //  2E8   Свободен
  for (int i = 0; i < 16; i++)
  {
	mcp_Out1.digitalWrite(i, HIGH);
	mcp_Out2.digitalWrite(i, HIGH);
  }
  //mcp_Out2.digitalWrite(14, LOW);                 // Отключить реле
}

void setup_regModbus()
{
  regBank.setId(1);    // Slave ID 1

  regBank.add(1);      //
  regBank.add(2);      //
  regBank.add(3);      //
  regBank.add(4);      //
  regBank.add(5);      //
  regBank.add(6);      //
  regBank.add(7);      //
  regBank.add(8);      //

  regBank.add(10001);  //
  regBank.add(10002);  //
  regBank.add(10003);  //
  regBank.add(10004);  //
  regBank.add(10005);  //
  regBank.add(10006);  //
  regBank.add(10007);  //
  regBank.add(10008);  //

  regBank.add(30001);  //
  regBank.add(30002);  //
  regBank.add(30003);  //
  regBank.add(30004);  //
  regBank.add(30005);  //
  regBank.add(30006);  //
  regBank.add(30007);  //
  regBank.add(30008);  //

  regBank.add(40001);  //  Адрес передачи комманд на выполнение
  regBank.add(40002);  //  Адрес счетчика всех ошибок
  regBank.add(40003);  //  Адрес хранения величины сигнала резистором № 1
  regBank.add(40004);  //  Адрес хранения величины сигнала резистором № 2
  regBank.add(40005);  //  Адрес блока регистров для передачи в ПК таблиц.
  regBank.add(40006);  //  Адрес блока памяти для передачи в ПК таблиц.
  regBank.add(40007);  //  Адрес длины блока таблиц
  regBank.add(40008);  //  Номер блока таблиц по умолчанию
  regBank.add(40009);  //

  regBank.add(40010);  //  Регистры временного хранения для передачи таблицы
  regBank.add(40011);
  regBank.add(40012);
  regBank.add(40013);
  regBank.add(40014);
  regBank.add(40015);
  regBank.add(40016);
  regBank.add(40017);
  regBank.add(40018);
  regBank.add(40019);

  regBank.add(40020);
  regBank.add(40021);
  regBank.add(40022);
  regBank.add(40023);
  regBank.add(40024);
  regBank.add(40025);
  regBank.add(40026);
  regBank.add(40027);
  regBank.add(40028);
  regBank.add(40029);

  regBank.add(40030);
  regBank.add(40031);
  regBank.add(40032);
  regBank.add(40033);
  regBank.add(40034);
  regBank.add(40035);
  regBank.add(40036);
  regBank.add(40037);
  regBank.add(40038);
  regBank.add(40039);

  regBank.add(40040);
  regBank.add(40041);
  regBank.add(40042);
  regBank.add(40043);
  regBank.add(40044);
  regBank.add(40045);
  regBank.add(40046);
  regBank.add(40047);
  regBank.add(40048);
  regBank.add(40049);
  // Текущее время
  regBank.add(40050);  // адрес день модуля часов контроллера
  regBank.add(40051);  // адрес месяц модуля часов контроллера
  regBank.add(40052);  // адрес год модуля часов контроллера
  regBank.add(40053);  // адрес час модуля часов контроллера
  regBank.add(40054);  // адрес минута модуля часов контроллера
  regBank.add(40055);  // адрес секунда модуля часов контроллера
  // Установка времени в контроллере
  regBank.add(40056);  // адрес день
  regBank.add(40057);  // адрес месяц
  regBank.add(40058);  // адрес год
  regBank.add(40059);  // адрес час
  regBank.add(40060);  // адрес минута
  regBank.add(40061);  //
  regBank.add(40062);  //
  regBank.add(40063);  //
  slave._device = &regBank;
}

void setup()
{
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setFont(BigFont);
  myTouch.InitTouch();
  delay(1000);
  //myTouch.setPrecision(PREC_MEDIUM);
  myTouch.setPrecision(PREC_HI);
  //myTouch.setPrecision(PREC_EXTREME);
  myButtons.setTextFont(BigFont);
  myButtons.setSymbolFont(Dingbats1_XL);
  Serial.begin(9600);                                    // Подключение к USB ПК
  Serial1.begin(115200);                                 // Подключение к
  slave.setSerial(3, 57600);                             // Подключение к протоколу MODBUS компьютера Serial3
  Serial2.begin(115200);                                 // Подключение к
  setup_pin();
  Wire.begin();
  if (!RTC.begin())                                      // Настройка часов
  {
	Serial.println(F("RTC failed"));
	while (1);
  };
  //DateTime set_time = DateTime(16, 3, 15, 10, 19, 0);  // Занести данные о времени в строку "set_time" год, месяц, число, время...
  //RTC.adjust(set_time);                                // Записать дату
  Serial.println(" ");
  Serial.println(F(" ***** Start system  *****"));
  Serial.println(" ");
  //set_time();
  serial_print_date();
  setup_mcp();                                          // Настроить порты расширения
  //mcp_Out2.digitalWrite(15, LOW);                     // Выключить реле подключения "земли" от блока проверки//
  //mcp_Out2.digitalWrite(14, LOW);                     // Отключить реле питания +12в. от блока проверки
  MsTimer2::set(300, flash_time);                       // 300ms период таймера прерывани
  setup_regModbus();

  Serial.print(F("FreeRam: "));
  Serial.println(FreeRam());
  wait_time_Old =  millis();
  digitalWrite(led_Green, HIGH);                          
  digitalWrite(led_Red, LOW);                           
  set_adr_EEPROM();
  Serial.println(" ");                                   
  Serial.println(F("System initialization OK!."));        // Информация о завершении настройки

}

void loop()
{
  draw_Glav_Menu();
  swichMenu();
}
