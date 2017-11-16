/*

Начало работ 21.08.2017г.


Восстановление 11.11.2017г.

Изменение 14.11.2017г.
  
 */

#define __SAM3X8E__


#include <SPI.h>
#include "AnalogBinLogger.h"
#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <DueTimer.h>
#include "Wire.h"
#include <DS3231.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"


extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

// Настройка монитора

UTFT myGLCD(TFT01_28, 38, 39, 40, 41);     // Настройка монитора
//UTFT myGLCD(ITDB28, 38, 39, 40, 41);     // Настройка монитора
UTouch        myTouch(6,5,4,3,2);          // Настройка клавиатуры

UTFT_Buttons  myButtons(&myGLCD, &myTouch);
boolean default_colors = true;
uint8_t menu_redraw_required = 0;


#define intensityLCD 9             // Порт управления яркостью экрана
#define synhro_pin 66              // Порт синхронизации модулей
#define alarm_pin 7                // Порт прерывания по таймеру

//----------------------Конец  Настройки дисплея --------------------------------

//+++++++++++++++++++++++ Настройка электронного резистора +++++++++++++++++++++++++++++++++++++

#define address_AD5252   0x2C                       // Адрес микросхемы AD5252  
#define control_word1    0x07                       // Байт инструкции резистор №1
#define control_word2    0x87                       // Байт инструкции резистор №2
byte resistance = 0x00;                             // Сопротивление 0x00..0xFF - 0Ом..100кОм
//byte level_resist      = 0;                       // Байт считанных данных величины резистора
//-----------------------------------------------------------------------------------------------

#define kn_red           43                         // AD4 Кнопка красная +
#define kn_blue          42                         // AD6 Кнопка синяя -
#define vibro            11                         // Вибромотор
#define sounder          53                         // Зуммер

#define LED_PIN13        13                         // Индикация
bool Interrupt_enable = false;                      // Разрешить выполнение прерывания

volatile int kn = 0;
byte counter_kn = 0;
byte Chanal_volume = 0;
volatile int adr_count1_kn = 2;                    // Адрес хранения уровня громкости 1 канала усилителя Sound Test Base 
volatile int adr_count2_kn = 4;                    // Адрес хранения уровня громкости 2 канала усилителя Sound Test Base
volatile int adr_count3_kn = 6;                    // Адрес хранения уровня громкости 1 канала трансмиттера 
volatile int adr_count4_kn = 8;                    // Адрес хранения уровня громкости 2 канала трансмиттера 
int adr_timePeriod = 10;
int adr_set_timePeriod = 20;                       // Время коррекции задержки на выполнение программы (опция синхронизация по таймеру)
int adr_set_timeSynhro = 30;                       // Время между посылками синхро импульсов
int adr_set_corr_time = 40;
int adr_set_timePeriod_RF = 60;                    // Время коррекции задержки на выполнение программы (опция радиосинхронизация)

volatile byte volume_variant = 0;                  // Управление переключением настройки эл. резисторов. 

volatile byte mem_variant = 0;

volatile int count_ms = 0;

int scale_strob = 1;
int xpos;
int ypos1;
int ypos2;

int ypos_osc1_0;
int ypos_osc2_0;
int ypos_trig;
int ypos_trig_old;



float koeff_volume[] = {0.0, 1.0, 1.4, 2.0, 2.8, 4.0, 5.6, 8.0, 11.2, 15.87};
const char* proc_volume[] = {"0%","6%","8%","12%","17%","25%","35%","50%","70%","99%"};

unsigned long timePeriod        = 3000000;              // Период следования синхроимпульсов   (1)  Уточнить
unsigned long set_timePeriod    = 0;                    // Переменная для вычитания времени выполнения программы  по таймеру
unsigned long set_timePeriod_RF = 0;                    // Переменная для вычитания времени выполнения программы  по радио
unsigned long set_timeSynhro    = 3000000;              // Период следования синхроимпульсов   (2)
unsigned int corr_time          = 0;                    // Подстройка синхроимпульсов
unsigned long timeStartRadio    = 0;                    // Время старта радио синхро импульса
unsigned long timeStopRadio     = 0;                    // Время окончания радио синхро импульса
unsigned long timeStartTrig     = 0;                    // Время окончания радио синхро импульса
unsigned long timeStopTrig      = 0;                    // Время окончания радио синхро импульса


//+++++++++++++++++++++++++++++ Внешняя память +++++++++++++++++++++++++++++++++++++++
int deviceaddress = 80;                      // Адрес микросхемы памяти
byte hi;                                     // Старший байт для преобразования числа
byte low;                                    // Младший байт для преобразования числа
int mem_start = 23;                          // признак первого включения прибора после сборки. Очистить память  

RF24 radio(48, 49);                                                        // DUE

unsigned long timeoutPeriod = 3000;     // Set a user-defined timeout period. With auto-retransmit set to (15,15) retransmission will take up to 60ms and as little as 7.5ms with it set to (1,15).
										// With a timeout period of 1000, the radio will retry each payload for up to 1 second before giving up on the transmission and starting over

										/***************************************************************/

const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };              // Radio pipe addresses for the 2 nodes to communicate.

byte data_in[24];                                                          // Буфер хранения принятых данных
byte data_out[24];                                                         // Буфер хранения данных для отправки

int time_sound = 50;
int freq_sound = 1800;

int filterUp = 2550;
int filterDown = 2150;
int i_trig_syn = 0;

byte volume1 = 100;
byte volume2 = 100;
unsigned long tim1 = 0;
bool trig_sin = false;                            // Есть срабатывание триггера
//bool trig_sinF = false;                           // Есть срабатывание триггера фильтра
//bool Filter_enable = false;                       // Разрешение применения фильтра
volatile bool start_enable = false;

volatile unsigned long counter;
unsigned long rxTimer, startTime, stopTime, payloads = 0;
bool TX = 1, RX = 0, role = 0, transferInProgress = 0;

typedef enum { role_ping_out = 1, role_pong_back } role_e;                  // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back" };  // The debug-friendly names of those roles
role_e role_test = role_pong_back;                                          // The role of the current running sketch
byte counter_test = 0;

const char str[] = "My very long string";
extern "C" char *sbrk(int i);
uint32_t message;                          // Эта переменная для сбора обратного сообщения от приемника;


//**************************  Initialization time ***************************************

const int clockCenterX=119;
const int clockCenterY=119;
int oldsec=0;
char* str_mon[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

DS3231 DS3231_clock;
RTCDateTime dt;
char* daynames[]={"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

//-----------------------------------------------------------------------------------------------
uint8_t sec = 0;       //Initialization time
uint8_t min = 0;
uint8_t hour = 0;
uint8_t dow1 = 1;
uint8_t date = 1;
uint8_t mon1 = 1;
uint16_t year = 14;
unsigned long timeF;
int flag_time = 0;

boolean isAlarm = false;
boolean alarmState = false;
int alarm_synhro = 0;
unsigned long alarm_count = 0;

//******************Назначение переменных для хранения № опций меню (клавиш)****************************

 int but1, but2, but3, but4, but5, but6, but7, but8, but9, but10, butX, butY, but_m1, but_m2, but_m3, but_m4, but_m5, pressed_button;
 int m2 = 1; // Переменная номера меню
 
//=====================================================================================

	#define new_info        0                         // Текущая информация
	#define old_info        1                         // Сохраненная информация

	int x_kn = 30;                                    // Смещение кнопок по Х
	int dgvh;
	const int hpos = 95;                              //set 0v on horizontal  grid
    #define page_max  12
	int port = 0;
	int x_osc,y_osc;
	int Input = 0;
	int Old_Input = 0;
	int Sample_osc[240][page_max];
	int OldSample_osc[240][page_max];
	int Synhro_osc[240][page_max][2];
	int Page_count = 0;
	int x_pos_count = 0;
	int YSample_osc[240][1];
	float VSample_osc[240][1];
	unsigned long LongFile = 0;
	unsigned long StartSample = 0;
	unsigned long StartSynhro = 0;
	unsigned long EndSample = 0;                 // 
	float koeff_h = 7.759*4;
	int MaxAnalog = 0;
	int MaxAnalog0 = 0;
	unsigned long SrednAnalog = 0;
	unsigned long SrednAnalog0 = 0;
	unsigned long SrednCount = 0;
	bool Set_x = false;
	bool osc_line_off0 = false;
	bool repeat = false;
	int16_t count_repeat = 0;
	bool save_files = false;
	bool sled = false;
	int Set_ADC = 10;
	int MinAnalog = 500;
	int MinAnalog0 = 500;
	int mode = 3;              //Время развертки  

	int mode1 = 0;             //Переключение чувствительности
	int dTime = 2;
	int x_dTime = 276;
	int tmode = 5;
	int t_in_mode = 0;
	int Trigger = 0;
	int SampleSize = 0;
	float SampleTime = 0;
	float v_const = 0.0008057;
	int x_measure = 0 ;              // Переменная для изменения частоты измерения источника питания
	bool strob_start = true;
	int page = 0;
	int page_trig = 0;


 //***************** Назначение переменных для хранения текстов*****************************************************

char  txt_Start_Menu[]       = "HA""CTPO""\x87""K""\x86";                                                   // "НАСТРОЙКИ"

char  txt_menu1_1[]          = "\x86\x9C\xA1""ep.""\x9C""a""\x99""ep""\x9B\x9F\x9D";                        // "Измер.задержки"
char  txt_menu1_2[]          = "HA""CTPO""\x87""K""\x86";                                                   // "НАСТРОЙКИ"
char  txt_menu1_3[]          = "\x8D""AC""\x91"" C""\x86""HXPO.";                                           // "ЧАСЫ СИНХРО."
char  txt_menu1_4[]          = "PA\x80OTA c SD";                                                            // "РАБОТА с SD"
char  txt_menu1_5[]          = "B\x91XO\x82";                                                               // "ВЫХОД"         

char  txt_ADC_menu1[]        = " ========= ";                               //
//char  txt_ADC_menu1[] = "\x85""a\xA3\x9D""c\xAC \x99""a\xA2\xA2\xABx";                               //
char  txt_ADC_menu2[]        = "\x89poc\xA1o\xA4p \xA5""a\x9E\xA0""a";                                      //
char  txt_ADC_menu3[]        = "\x89""epe\x99""a\xA7""a \x97 KOM";                                          //
char  txt_ADC_menu4[]        = "B\x91XO\x82";                                                               // Выход                                                              //

char  txt_tuning_menu1[]     = "Oc\xA6\x9D\xA0\xA0o\x98pa\xA5";                                             // "Осциллограф"
char  txt_tuning_menu2[]     = "C""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\xAF"" ";                        // "Синхронизация"
char  txt_tuning_menu3[]     = "PA""\x82\x86""O";                                                           // Радио
char  txt_tuning_menu4[]     = "B\x91XO\x82";                                                               // Выход     

char  txt_synhro_menu1[]     = "C""\x9D\xA2""xpo pa""\x99\x9D""o";                                          // "Синхро радио"
char  txt_synhro_menu2[]     = "C""\x9D\xA2""xpo ""\xA3""po""\x97""o""\x99";                                // "Синхро провод"
char  txt_synhro_menu3[]     = "C""\xA4""o""\xA3"" c""\x9D\xA2""xpo.";                                      // "Стоп синхро."
char  txt_synhro_menu4[]     = "B\x91XO\x82";                                                               // Выход          

char  txt_delay_measure1[]   = "C""\x9D\xA2""xpo ""\xA3""o ""\xA4""a""\x9E\xA1""epy";                       // Синхро по таймеру
char  txt_delay_measure2[]   = "C""\x9D\xA2""xpo ""\xA3""o pa""\x99\x9D""o";                                // Синхро по радио 
char  txt_delay_measure3[]   = "";                                          //
char  txt_delay_measure4[]   = "B\x91XO\x82";                                                               // Выход          

char  txt_SD_menu1[]         = "\x89poc\xA1o\xA4p \xA5""a\x9E\xA0""a";                                      //
char  txt_SD_menu2[]         = "\x86\xA2\xA5o SD";                                                          //
char  txt_SD_menu3[]         = "\x8Bop\xA1""a\xA4 SD";                                                      //
char  txt_SD_menu4[]         = "B\x91XO\x82";                                                              // Выход         

char  txt_radio_menu1[]      = "Test ping";                                                                //
char  txt_radio_menu2[]      = "Test transfer";                                                                        //
char  txt_radio_menu3[]      = "";                                                                        //  
char  txt_radio_menu4[]      = "B\x91XO\x82";                                                              // Выход


char  txt_info6[]             = "Info: ";                                                                   //Info: 
char  txt_info7[]             = "Writing:"; 
char  txt_info8[]             = "%"; 
char  txt_info9[]             = "Done: "; 
char  txt_info10[]            = "Seconds"; 
char  txt_info11[]            = "ESC->PUSH Display"; 
char  txt_info12[]            = "CTAPT"; 
char  txt_info13[]            = "Deleting tmp file"; 
char  txt_info14[]            = "Erasing all data"; 
//char  txt_info15[]            = "Stop->PUSH Display"; 
char  txt_info16[]            = "File: "; 
char  txt_info17[]            = "Max block :"; 
char  txt_info18[]            = "Rec time ms: "; 
char  txt_info19[]            = "Sam count:"; 
char  txt_info20[]            = "Samples/sec: "; 
char  txt_info21[]            = "O\xA8\x9D\x96\x9F\x9D:"; 
char  txt_info22[]            = "Sample pins:"; 
char  txt_info23[]            = "ADC bits:"; 
char  txt_info24[]            = "ADC clock kHz:"; 
char  txt_info25[]            = "Sample Rate:"; 
char  txt_info26[]            = "Sample interval:"; 
char  txt_info27[]            = "Creating new file"; 
char  txt_info28[]            = "\x85""a\xA3\x9D""c\xAC \x99""a\xA2\xA2\xABx"; 
char  txt_info29[]            = "Stop->PUSH Disp"; 
char  txt_info30[]            = "\x89o\x97\xA4op."; 




// ADC speed one channel 480,000 samples/sec (no enable per read)
//           one channel 288,000 samples/sec (enable per read)

#define ADC_MR * (volatile unsigned int *) (0x400C0004)              /*adc mode word*/
#define ADC_CR * (volatile unsigned int *) (0x400C0000)              /*write a 2 to start convertion*/
#define ADC_ISR * (volatile unsigned int *) (0x400C0030)             /*status reg -- bit 24 is data ready*/
#define ADC_ISR_DRDY 0x01000000
//
#define ADC_START 2
#define ADC_LCDR * (volatile unsigned int *) (0x400C0020)            /*last converted low 12 bits*/
#define ADC_DATA 0x00000FFF 
#define ADC_STARTUP_FAST 12
//
#define ADC_CHER * (volatile unsigned int *) (0x400C0010)           /*ADC Channel Enable Register  Только запись*/
#define ADC_CHSR * (volatile unsigned int *) (0x400C0018)           /*ADC Channel Status Register  Только чтение */
#define ADC_CDR0 * (volatile unsigned int *) (0x400C0050)           /*ADC Channel Только чтение */
//#define ADC_ISR_EOC0 0x00000001


#define Analog_pinA0 ADC_CHER_CH7    // Вход A0
#define Analog_pinA1 ADC_CHER_CH6    // Вход A1
#define Analog_pinA2 ADC_CHER_CH5    // Вход A2
#define Analog_pinA3 ADC_CHER_CH4    // Вход A3
#define Analog_pinA4 ADC_CHER_CH3    // Вход A4
#define Analog_pinA5 ADC_CHER_CH2    // Вход A5
#define Analog_pinA6 ADC_CHER_CH1    // Вход A6
#define Analog_pinA7 ADC_CHER_CH0    // Вход A7

/*
Примерный порядок работы с АЦП (минимальный набор действий)
Прежде всего необходимо выполнить настройку АЦП с использованием регистра ADC_MR, указав тем самым разрядность преобразования (8 или 10 бит), 
режим работы (нормальный/спящий), частоту тактирования АЦП, время выборки и другие параметры.
Также необходимо включить каналы, по которым будет выполняться преобразование с помощью регистра ADC_CHER.
Если предполагается работать с АЦП в режиме «по прерываниям», необходимо настроить контроллер прерываний на обработку сигналов прерываний от АЦП, 
указать процедуру обработки прерывания, а также включить выдачу сигналов прерываний на стороне АЦП по определённым событиям (например по окончании преобразования).
Далее производится запуск АЦП с использованием регистра ADC_CR.
Считывание результата преобразования (целого числа) производится либо из регистра результата преобразования по каналу (например, ADC_CDR4 для канала 4),
либо из регистра последнего результата преобразования (ADC_LCDR). Считывание результата нельзя производить во время преобразования! 
Поэтому при работе с АЦП в режиме «по готовности» перед считыванием результата необходимо подождать,
пока не установится соответствующий флаг готовности в регистре ADC_SR. Если микроконтроллер настроен на генерацию прерываний по окончании преобразования,
результат считывается в обработчике прерывания.
Результат преобразования, полученный в виде целого числа не отражает значения напряжения на входе АЦП.
Для получения данной величины необходимо выполнить процедуру «масштабирования». Для этого результат преобразования необходимо умножить на величину кванта преобразования. 
Величина кванта определяется как
q = Uref / 2n,
где Uref – опорное напряжение АЦП, n – разрядность преобразования (определяется полем LOW_RES регистра ADC_MR).






*/



#define strob_pin A4    // Вход для запуска измерения
uint32_t ulChannel;

//--------------------------------------------------------------------------

int Channel_x = 0;
int Channel_trig = 0;
bool Channel0 = true;
int count_pin = 0;
int set_strob = 10;                                           // Время развертки сканирования осциллографа

const float SAMPLE_RATE = 5000;  // Must be 0.25 or greater.

// The interval between samples in seconds, SAMPLE_INTERVAL, may be set to a
// constant instead of being calculated from SAMPLE_RATE.  SAMPLE_RATE is not
// used in the code below.  For example, setting SAMPLE_INTERVAL = 2.0e-4
// will result in a 200 microsecond sample interval.

// Интервал между выборками в секундах, SAMPLE_INTERVAL, может быть установлен на
// константа вместо вычисления SAMPLE_RATE. SAMPLE_RATE не
// используется в приведенном ниже коде. Например, установка SAMPLE_INTERVAL = 2.0e-4
// приведет к интервалу выборки 200 микросекунд.



const float SAMPLE_INTERVAL = 1.0/SAMPLE_RATE;

// Setting ROUND_SAMPLE_INTERVAL non-zero will cause the sample interval to
// be rounded to a a multiple of the ADC clock period and will reduce sample
// time jitter.
#define ROUND_SAMPLE_INTERVAL 1
//------------------------------------------------------------------------------
// ADC clock rate.
// The ADC clock rate is normally calculated from the pin count and sample
// interval.  The calculation attempts to use the lowest possible ADC clock
// rate.
//
// You can select an ADC clock rate by defining the symbol ADC_PRESCALER to
// one of these values.  You must choose an appropriate ADC clock rate for
// your sample interval. 
// #define ADC_PRESCALER 7 // F_CPU/128 125 kHz on an Uno
// #define ADC_PRESCALER 6 // F_CPU/64  250 kHz on an Uno
// #define ADC_PRESCALER 5 // F_CPU/32  500 kHz on an Uno
// #define ADC_PRESCALER 4 // F_CPU/16 1000 kHz on an Uno
// #define ADC_PRESCALER 3 // F_CPU/8  2000 kHz on an Uno (8-bit mode only)
//------------------------------------------------------------------------------
// Reference voltage.  See the processor data-sheet for reference details.
// uint8_t const ADC_REF = 0; // External Reference AREF pin.
//!!uint8_t const ADC_REF = (1 << REFS0);  // Vcc Reference.
// uint8_t const ADC_REF = (1 << REFS1);  // Internal 1.1 (only 644 1284P Mega)
// uint8_t const ADC_REF = (1 << REFS1) | (1 << REFS0);  // Internal 1.1 or 2.56
//------------------------------------------------------------------------------

const uint32_t FILE_BLOCK_COUNT = 256000;

// log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "ANALOG"

#define FILE_BASE_NAME_TIME "TIMESa"
// Set RECORD_EIGHT_BITS non-zero to record only the high 8-bits of the ADC.
//Установите RECORD_EIGHT_BITS ненулевое значение для записи только высоких 8 бит бита АЦП??
#define RECORD_EIGHT_BITS 0
//------------------------------------------------------------------------------
// Pin definitions.
//
// Digital pin to indicate an error, set to -1 if not used.
// The led blinks for fatal errors. The led goes on solid for SD write
// overrun errors and logging continues.
const int8_t ERROR_LED_PIN = 13;

// SD chip select pin.
//const uint8_t SD_CS_PIN = 53;
const uint8_t SD_CS_PIN = 10;
uint32_t const ERASE_SIZE = 262144L;
//------------------------------------------------------------------------------

const uint8_t BUFFER_BLOCK_COUNT = 12;//12;
// Dimension for queues of 512 byte SD blocks.
// Размер очередей по 512 байт памяти SD блоков.
const uint8_t QUEUE_DIM = 16;//16;  // Must be a power of two! Должно быть степенью двойки!

//==============================================================================
// End of configuration constants.
//==============================================================================
// Temporary log file.  Will be deleted if a reset or power failure occurs.
#define TMP_FILE_NAME "TMP_LOG.BIN"

// Size of file base name.  Must not be larger than six.
//Размер базовой части имени файла. Должно быть не больше, чем шесть.
const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;

//==============================================================================
// Number of analog pins to log.
// const uint8_t PIN_COUNT = sizeof(PIN_LIST)/sizeof(PIN_LIST[0]);

// Minimum ADC clock cycles per sample interval Минимальные циклы синхронизации АЦП за интервал дискретизации
const uint16_t MIN_ADC_CYCLES = 15;

// Extra cpu cycles to setup ADC with more than one pin per sample.
// Дополнительные циклов процессора настройки АЦП с более чем одним pin на образец.
const uint16_t ISR_SETUP_ADC = 100;

// Maximum cycles for timer0 system interrupt, millis, micros.
// Максимальные циклы таймер 0 системы прерывания, Millis, Micros.
const uint16_t ISR_TIMER0 = 160;
//==============================================================================

// serial output steam

size_t SAMPLES_PER_BLOCK ;                         //= DATA_DIM16/PIN_COUNT; // 254 разделить на количество входов
typedef block16_t block_t;

block_t* emptyQueue[QUEUE_DIM];
uint8_t emptyHead;
uint8_t emptyTail;

block_t* fullQueue[QUEUE_DIM];
volatile uint8_t fullHead;  // volatile insures non-interrupt code sees changes. обеспечивает код без прерывания видит изменения
uint8_t fullTail;

// queueNext assumes QUEUE_DIM is a power of two
inline uint8_t queueNext(uint8_t ht) {return (ht + 1) & (QUEUE_DIM -1);}

//==============================================================================
// Процедуры обслуживания прерываний
block_t* isrBuf;                                // Указатель на текущий буфер.  Настройки в block16_t
bool isrBufNeeded = true;                       // Необходим новый  буфер, если true
uint16_t isrOver  = 0;                          // счетчик переполнения
// Подстраховать, никакие события таймера не пропущены.
volatile bool timerError = false;
volatile bool timerFlag  = false;
//------------------------------------------------------------------------------
bool ledOn = false;


void firstHandler()                                         // Timer3  запись результатов измерения  в файл 
{
	//ADC_CHER = Channel_x;    // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3    
	ADC_CR = ADC_START ; 	                                // Запустить преобразование
	if (isrBufNeeded && emptyHead == emptyTail)             //  Необходим новый  буфер, если true 
	{
		if (isrOver < 0XFFFF) isrOver++;                    // no buffers - count overrun нет буферов - рассчитывайте перерасход
		timerFlag = false;		                            // Avoid missed timer error. Избежать пропущенных ошибку таймера.
		return;
	}

	if (isrBufNeeded)                                       // Необходимо проверить состояние буфера
		{   
                                                   		    //  Удалить буфер из пустого очереди.
		isrBuf = emptyQueue[emptyTail];
		emptyTail = queueNext(emptyTail);
		isrBuf->count = 0;                                  // Счетчик в 0
		isrBuf->overrun = isrOver;                          // 
		isrBufNeeded = false;    
		}
	// Store ADC data.                                      // Сохранить данные АЦП.   
	while (!(ADC_ISR & ADC_ISR_DRDY));                             
	isrBuf->data[isrBuf->count++] = ADC->ADC_CDR[7];        // Записать результат измерения
	if (isrBuf->count >= count_pin*SAMPLES_PER_BLOCK)       //  SAMPLES_PER_BLOCK -количество памяти выделенное на  один вход 
 		{
															// Put buffer isrIn full queue.  Положите буфер isrIn полной очереди.
		uint8_t tmp = fullHead;                             // Avoid extra fetch of volatile fullHead.  Избегайте дополнительной выборки volatile fullHead.
		fullQueue[tmp] = (block_t*)isrBuf;
		fullHead = queueNext(tmp);
		isrBufNeeded = true;                                // Установите необходимый буфер и очистите переполнения.
		isrOver = 0;
		}
 }
void secondHandler()                                        // Timer4 -  запись временных меток в файл
{
	count_ms +=2;
	//Serial.println(count_ms);
	isrBuf->data[isrBuf->count++] = 4095;                   // Записать временные метки
	isrBuf->data[isrBuf->count++] = count_ms;               // Записать временные метки
}
void send_synhro()                                          // Timer5 -   формирование временных меток синхронизации измерения
{
	StartSynhro = micros();                                 // Записать время
	myGLCD.setColor(VGA_LIME);
	myGLCD.fillCircle(200, 10, 10);
	count_ms = 0;
	delayMicroseconds(100000);
	myGLCD.setColor(VGA_BLACK);
	myGLCD.fillCircle(200, 10, 10);
	myGLCD.setColor(VGA_WHITE);
	start_enable = true;
}
void synhroHandler()                                        // Timer6 -   формирование импульсов синхронизации измерения. Настройка
{
	digitalWrite(LED_PIN13, HIGH);
	delayMicroseconds(500);
	digitalWrite(LED_PIN13, LOW);
}
void sevenHandler()                                         // Timer7 -  запись временных меток  в массив для вывода на экран
{
	count_ms += scale_strob;
	Synhro_osc[xpos][page][new_info] = count_ms;                      // Записать временные метки в массив для вывода на экран
	if(xpos>0) Synhro_osc[xpos-1][page][new_info] = 4095;             // Записать временные метки в массив для вывода на экран
}


//==============================================================================
// Error messages stored in flash.
#define error(msg) error_P(PSTR(msg))
//------------------------------------------------------------------------------
void error_P(const char* msg) 
{
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("Error: ", CENTER, 80);
  myGLCD.print(msg, CENTER, 120);
  delay(2000);

//  sd.errorPrint_P(msg);
  fatalBlink();
}
//------------------------------------------------------------------------------
//
void fatalBlink() 
{
  while (true) 
  {
	if (ERROR_LED_PIN >= 0) {
	  digitalWrite(ERROR_LED_PIN, HIGH);
	  delay(200);
	  digitalWrite(ERROR_LED_PIN, LOW);
	  delay(200);
	}
  }
}
//==============================================================================
void adcInit(metadata_t* meta) 
{

  uint8_t adps;  // prescaler bits for ADCSRA 
  uint32_t ticks = F_CPU*SAMPLE_INTERVAL + 0.5;  // Sample interval cpu cycles.

#ifdef ADC_PRESCALER
  if (ADC_PRESCALER > 7 || ADC_PRESCALER < 2) {
	error("Invalid ADC prescaler");
  }
  adps = ADC_PRESCALER;
#else  // ADC_PRESCALER
  // Allow extra cpu cycles to change ADC settings if more than one pin.
  int32_t adcCycles = (ticks - ISR_TIMER0)/count_pin;
					  - (count_pin > 1 ? ISR_SETUP_ADC : 0);
					  
  for (adps = 7; adps > 0; adps--) {
	 if (adcCycles >= (MIN_ADC_CYCLES << adps)) break;
  }
#endif  // ADC_PRESCALER
   meta->adcFrequency = F_CPU >> adps;
  if (meta->adcFrequency > (RECORD_EIGHT_BITS ? 2000000 : 1000000)) 
  {
	error("Sample Rate Too High");
  }

  #if ROUND_SAMPLE_INTERVAL
  // Round so interval is multiple of ADC clock.
  ticks += 1 << (adps - 1);
  ticks >>= adps;
  ticks <<= adps;
#endif  // ROUND_SAMPLE_INTERVAL



	  meta->pinCount = count_pin;
	  meta->recordEightBits = RECORD_EIGHT_BITS;
	//  isrOver = 0;
  
		 int i = 0;
		 meta->pinNumber[i] = 0;


// разделить на предделителе
		 uint8_t tshift = 10;
 // divide by prescaler
  ticks >>= tshift;
  // set TOP for timer reset
 // ICR1 = ticks - 1;
  // compare for ADC start
 // OCR1B = 0;
  
  // multiply by prescaler
  ticks <<= tshift;


	  // Sample interval in CPU clock ticks.
	  meta->sampleInterval = ticks;

	  meta->cpuFrequency = F_CPU;
  
	  float sampleRate = (float)meta->cpuFrequency/meta->sampleInterval;
	 // Serial.print(F("Sample pins:"));
	 // for (int i = 0; i < meta->pinCount; i++) 
	 // {
		//Serial.print(' ');
		//Serial.print(meta->pinNumber[i], DEC);
	 // }
 
	 // Serial.println(); 
	 // Serial.println(F("ADC bits: 12 "));
	 // Serial.print(F("ADC interval usec: "));
	 // Serial.println(set_strob);
	 // Serial.print(F("Sample Rate: "));
	 // Serial.println(sampleRate);  
	 // Serial.print(F("Sample interval usec: "));
	 // Serial.println(1000000.0/sampleRate, 4); 
}

void adcStart()
{
  // initialize ISR
  isrBufNeeded = true;
  isrOver = 0;
//  adcindex = 1;

  //// Clear any pending interrupt.
  //ADCSRA |= 1 << ADIF;
  //
  //// Setup for first pin.
  //ADMUX = adcmux[0];
  //ADCSRB = adcsrb[0];
  //ADCSRA = adcsra[0];

  // Enable timer1 interrupts.
  timerError = false;
  timerFlag = false;
 /* TCNT1 = 0;
  TIFR1 = 1 << OCF1B;
  TIMSK1 = 1 << OCIE1B;*/
}
//
//void binaryToCsv() 
//{
//	uint8_t lastPct = 0;
//	block_t buf;
//	metadata_t* pm;
//	uint32_t t0 = millis();
//	char csvName[13];
// 
//	if (!binFile.isOpen()) 
//		{
//			//Serial.println(F("No current binary file"));
//			myGLCD.clrScr();
//			myGLCD.setBackColor(0, 0, 0);
//			myGLCD.print("Error: ", CENTER, 80);
//			myGLCD.print("No binary file", CENTER, 120);
//			delay(2000);
//			Draw_menu_ADC_RF();
//			return;
//		}
//	binFile.rewind();
//	if (!binFile.read(&buf , 512) == 512) error("Read metadata failed");
//	// Create a new CSV file.
//	strcpy(csvName, binName);
//	strcpy_P(&csvName[BASE_NAME_SIZE + 3], PSTR("CSV"));
//
//	if (!csvStream.fopen(csvName, "w")) 
//		{
//			error("open csvStream failed");  
//		}
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print(txt_info7,LEFT, 145);  //
//	myGLCD.setColor(VGA_YELLOW);
//	myGLCD.print(csvName,RIGHT, 145);   // 
//	myGLCD.setColor(255, 255, 255);
//	pm = (metadata_t*)&buf;
//	csvStream.print(F("File - "));
//	csvStream.println(F(csvName));
//	csvStream.print(F("Interval - "));
//	float intervalMicros = set_strob;
//	csvStream.print(intervalMicros, 4);
//	csvStream.println(F(",usec"));
//	csvStream.print(F("Step = "));
//	csvStream.print(v_const, 8);
//	csvStream.println(F(" volt"));
//	csvStream.print(F("Data : "));
//
//	dt   = DS3231_clock.getDateTime();
//	dow1 = dt.dayOfWeek;
//	sec  = dt.second;       //Initialization time
//	min  = dt.minute;
//	hour = dt.hour;
//	date = dt.day;
//	mon1 = dt.month;
//	year = dt.year;
//
//	//DS3231_clock.get_time(&hh,&mm,&ss);
//	//DS3231_clock.get_date(&dow,&dd,&mon,&yyyy);
//	//dow1=dow;
//	//sec = ss;       //Initialization time
//	//min = mm;
//	//hour = hh;
//	//date = dd;
//	//mon1 = mon;
//	//year = yyyy;
//
//	csvStream.print(date);
//	csvStream.print(F("/"));
//	csvStream.print(mon1);
//	csvStream.print(F("/"));
//	csvStream.print(year);
//	csvStream.print(F("   "));
//
//	csvStream.print(hour);
//	csvStream.print(F(":"));
//	csvStream.print(min);
//	csvStream.print(F(":"));
//	csvStream.print(sec);
//	csvStream.println(); 
//	csvStream.print(F("@"));                  // Признак начала определения количества входов  
//	for (uint8_t i = 0; i < pm->pinCount; i++) 
//		{
//			if (i) csvStream.putc(',');
//			csvStream.print(F("pin "));
//			csvStream.print(pm->pinNumber[i]);
//		}
//	csvStream.println(); 
//	csvStream.println('#');                  // Признак начала данных
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print("Converting:",2, 165);      //
// 
//	uint32_t tPct = millis();
//
//  while (!Serial.available() && binFile.read(&buf, 512) == 512) 
//  {
//	uint16_t i;
//	if (buf.count == 0) break;
//	//if (buf.overrun) 
//	//{
//	//  csvStream.print(F("OVERRUN,"));
//	//  csvStream.println(buf.overrun);     
//	//}
//	for (uint16_t j = 0; j < buf.count; j += count_pin) 
//	{
//	  for (uint16_t i = 0; i < count_pin; i++) 
//	  {
//		if (i) csvStream.putc(',');
//		csvStream.print(buf.data[i + j]);     
//	  }
//	  csvStream.println();
//	}
//	if ((millis() - tPct) > 1000) 
//	{
//	  uint8_t pct = binFile.curPosition()/(binFile.fileSize()/100);
//	  if (pct != lastPct) 
//	  {
//		tPct = millis();
//		lastPct = pct;
//		myGLCD.setColor(VGA_YELLOW);
//		myGLCD.printNumI(pct, 180, 165);       // 
//		myGLCD.print(txt_info8,215, 165);     //
//		myGLCD.setColor(255, 255, 255);
//	  }
//	}
//	if (myTouch.dataAvailable()) break;
//  }
//
//	csvStream.println();                       // Признак  5555 окончания данных в файле
//	csvStream.print('5');                      // Признак  5555 окончания данных в файле
//	csvStream.print('5');                      // Признак  5555 окончания данных в файле
//	csvStream.print('5');                      // Признак  5555 окончания данных в файле
//	csvStream.print('5');                      // Признак  5555 окончания данных в файле
//	csvStream.println(); 
//	csvStream.print("Time measure = ");
//
//	dt = DS3231_clock.getDateTime();
//	dow1 = dt.dayOfWeek;
//	sec = dt.second;       //Initialization time
//	min = dt.minute;
//	hour = dt.hour;
//	date = dt.day;
//	mon1 = dt.month;
//	year = dt.year;
//
//
//
//	//DS3231_clock.get_time(&hh,&mm,&ss);
//	//DS3231_clock.get_date(&dow,&dd,&mon,&yyyy);
//	//dow1=dow;
//	//sec = ss;       //Initialization time
//	//min = mm;
//	//hour = hh;
//	//date = dd;
//	//mon1 = mon;
//	//year = yyyy;
//
//	csvStream.print(date);
//	csvStream.print(F("/"));
//	csvStream.print(mon1);
//	csvStream.print(F("/"));
//	csvStream.print(year);
//	csvStream.print(F("   "));
//
//	csvStream.print(hour);
//	csvStream.print(F(":"));
//	csvStream.print(min);
//	csvStream.print(F(":"));
//	csvStream.print(sec);
//	csvStream.println(); 
//
//	csvStream.fclose();  
//	//	binFile.remove();
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print(txt_info9,2, 185);   
//	myGLCD.setColor(VGA_YELLOW);   //
//	myGLCD.printNumF((0.001*(millis() - t0)),2, 90, 185);// 
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print(txt_info10, 210, 185);//
//}
//void checkOverrun() 
//{
//  bool headerPrinted = false;
//  block_t buf;
//  uint32_t bgnBlock, endBlock;
//  uint32_t bn = 0;
//  
//  if (!binFile.isOpen()) 
//  {
//	//Serial.println(F("No current binary file"));
//	myGLCD.clrScr();
//	myGLCD.setBackColor(0, 0, 0);
////	myGLCD.setFont( SmallFont);
//	myGLCD.setColor (255, 255, 255);
//	myGLCD.print("No binary file", CENTER, 120);
//	delay(2000);
//	return;
//  }
//  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
//	error("contiguousRange failed");
//  }
//  binFile.rewind();
// // Serial.println();
////  Serial.println(F("Checking overrun errors - type any character to stop"));
//  if (!binFile.read(&buf , 512) == 512) {
//	error("Read metadata failed");
//  }
//  bn++;
//  while (binFile.read(&buf, 512) == 512) {
//	if (buf.count == 0) break;
//	if (buf.overrun) 
//	{
//	  if (!headerPrinted) 
//	  {
//		//Serial.println();
//		//Serial.println(F("Overruns:"));
//		//Serial.println(F("fileBlockNumber,sdBlockNumber,overrunCount"));
//		headerPrinted = true;
//	  }
//	/*  Serial.print(bn);
//	  Serial.print(',');
//	  Serial.print(bgnBlock + bn);
//	  Serial.print(',');
//	  Serial.println(buf.overrun);*/
//	}
//	bn++;
//  }
//  if (!headerPrinted) {
//	//Serial.println(F("No errors found"));
//  } else {
//	//Serial.println(F("Done"));
//  }
//}
//void dumpData() 
//{
//	block_t buf;
//	myGLCD.clrScr();
//	myGLCD.setBackColor(0, 0, 0);
//	myGLCD.print(txt_info12,CENTER, 40);
//
//  if (!binFile.isOpen()) 
//  {
//	Serial.println(F("No current binary file"));
//	myGLCD.clrScr();
//	myGLCD.setBackColor(0, 0, 0);
//	myGLCD.print("Error: ", CENTER, 80);
//	myGLCD.print("No binary file", CENTER, 120);
//	delay(2000);
//	Draw_menu_ADC_RF();
//	return;
//  }
//  binFile.rewind();
//  if (binFile.read(&buf , 512) != 512) 
//  {
//	error("Read metadata failed");
//  }
//  Serial.println();
//  Serial.println(F("Type any character to stop"));
//	myGLCD.setColor(VGA_LIME);
//	myGLCD.print(txt_info11,CENTER, 200);
//	myGLCD.setColor(255, 255, 255);
//  delay(1000);
//  while (!myTouch.dataAvailable() && binFile.read(&buf , 512) == 512) 
//  {
//	if (buf.count == 0) break;
//	if (buf.overrun) 
//	{
//	  Serial.print(F("OVERRUN,"));
//	  Serial.println(buf.overrun);
//	}
//	for (uint16_t i = 0; i < buf.count; i++) 
//	{
//	  Serial.print(buf.data[i], DEC);
//	  if ((i+1)%count_pin) 
//	  {
//		Serial.print(',');
//	  } else {
//		Serial.println();
//	  }
//	}
//  }
//  Serial.println(F("Done"));
//	myGLCD.setColor(VGA_YELLOW);
//	myGLCD.print(txt_info9,CENTER, 80);
//	myGLCD.setColor(255, 255, 255);
//	delay(500);
//	while (myTouch.dataAvailable()){}
//	Draw_menu_ADC_RF();
//
//
//}
//void dumpData_Osc()
//{
//	myGLCD.clrScr();
//	myGLCD.setBackColor(0, 0, 0);
//	myGLCD.print(txt_info12,CENTER, 40);
//	block_t buf;
//	uint32_t count = 0;
//	uint32_t count1 = 0;
//	koeff_h = 7.759*4;
//	int xpos = 0;
//	int ypos1;
//	int ypos2;
//	int kl[buf.count];         //Текущий блок
//
//	if (!binFile.isOpen()) 
//		{
//			Serial.println(F("No current binary file"));
//			return;
//		}
//
//	binFile.rewind();
//	if (binFile.read(&buf , 512) != 512) 
//	{
//		error("Read metadata failed");
//	}
//	myGLCD.setColor(VGA_LIME);
//	myGLCD.print(txt_info11,CENTER, 200);
//	myGLCD.setColor(255, 255, 255);
//	delay(1000);
//	myGLCD.clrScr();
//	LongFile = 0;
//	DrawGrid1();
//	myGLCD.setColor(0, 0, 255);
//	myGLCD.setBackColor( 0, 0, 255);
//	myGLCD.fillRoundRect (250, 90, 310, 130);
//	myGLCD.setColor( 255, 255, 255);
//	myGLCD.drawRoundRect (250, 90, 310, 130);
//	myGLCD.setBackColor( 0, 0, 255);
//	myGLCD.setColor( 255, 255, 255);
//	myGLCD.setFont( SmallFont);
//	myGLCD.print("V/del.", 260, 95);
//	myGLCD.print("      ", 260, 110);
//	if (mode1 == 0)myGLCD.print("1", 275, 110);
//	if (mode1 == 1)myGLCD.print("0.5", 268, 110);
//	if (mode1 == 2)myGLCD.print("0.2", 268, 110);
//	if (mode1 == 3)myGLCD.print("0.1", 268, 110);
//
//	while (binFile.read(&buf , 512) == 512) 
//	{
//		if (buf.count == 0) break;
//		if (buf.overrun) 
//			{
//				Serial.print(F("OVERRUN,"));
//				Serial.println(buf.overrun);
//			}
//
//		DrawGrid1();
//
//		 if (myTouch.dataAvailable())
//			{
//				delay(10);
//				myTouch.read();
//				x_osc=myTouch.getX();
//				y_osc=myTouch.getY();
//
//				if ((x_osc>=2) && (x_osc<=240))  //  Delay Button
//					{
//						if ((y_osc>=1) && (y_osc<=160))  // Delay row
//						{
//							break;
//						} 
//					}
//			if ((x_osc>=250) && (x_osc<=310))  //  Delay Button
//			  {
//
//				 if ((y_osc>=90) && (y_osc<=130))  // Port select   row
//					 {
//						waitForIt(250, 90, 310, 130);
//						mode1 ++ ;
//						myGLCD.clrScr();
//						myGLCD.setColor(0, 0, 255);
//						myGLCD.fillRoundRect (250, 90, 310, 130);
//						myGLCD.setColor( 255, 255, 255);
//						myGLCD.drawRoundRect (250, 90, 310, 130);
//				//		buttons();
//						if (mode1 > 3) mode1 = 0;   
//						if (mode1 == 0) koeff_h = 7.759*4;
//						if (mode1 == 1) koeff_h = 3.879*4;
//						if (mode1 == 2) koeff_h = 1.939*4;
//						if (mode1 == 3) koeff_h = 0.969*4;
//					//	print_set();
//						myGLCD.setBackColor( 0, 0, 255);
//						myGLCD.setColor( 255, 255, 255);
//						myGLCD.setFont( SmallFont);
//						myGLCD.print("V/del.", 260, 95);
//						myGLCD.print("      ", 260, 110);
//						if (mode1 == 0)myGLCD.print("1", 275, 110);
//						if (mode1 == 1)myGLCD.print("0.5", 268, 110);
//						if (mode1 == 2)myGLCD.print("0.2", 268, 110);
//						if (mode1 == 3)myGLCD.print("0.1", 268, 110);
//					 }
//			  }
//		   }
//
//		for (uint16_t i = 0; i < buf.count; i++) 
//		{
//
//			Sample_osc[xpos][0] = buf.data[i];//.adc[0]; 
//				xpos++;
//			if(xpos == 240)
//				{
//				DrawGrid1();
//				for( int xpos = 0; xpos < 239;	xpos ++)
//					{
//										// Erase previous display Стереть предыдущий экран
//						myGLCD.setColor( 0, 0, 0);
//						ypos1 = 255-(OldSample_osc[ xpos + 1][0]/koeff_h) - hpos;
//						ypos2 = 255-(OldSample_osc[ xpos + 2][0]/koeff_h) - hpos;
//
//						if(ypos1<0) ypos1 = 0;
//						if(ypos2<0) ypos2 = 0;
//						if(ypos1>200) ypos1 = 200;
//						if(ypos2>200) ypos2 = 200;
//						myGLCD.drawLine (xpos + 1, ypos1, xpos + 2, ypos2);
//						if (xpos == 0) myGLCD.drawLine (xpos + 1, 1, xpos + 1, 220);
//						//Draw the new data
//						myGLCD.setColor( 255, 255, 255);
//						ypos1 = 255-(Sample_osc[ xpos][0]/koeff_h) - hpos;
//						ypos2 = 255-(Sample_osc[ xpos + 1][0]/koeff_h)- hpos;
//
//						if(ypos1<0) ypos1 = 0;
//						if(ypos2<0) ypos2 = 0;
//						if(ypos1>220) ypos1 = 200;
//						if(ypos2>220) ypos2 = 200;
//						myGLCD.drawLine (xpos, ypos1, xpos + 1, ypos2);
//						OldSample_osc[xpos][0] = Sample_osc[ xpos][0];
//					}
//					xpos = 0;
//					myGLCD.setFont( BigFont);
//					myGLCD.setBackColor( 0, 0, 0);
//					count1++;
//					myGLCD.printNumI(count, RIGHT, 220);// 
//					myGLCD.setColor(VGA_LIME);
//					myGLCD.printNumI(count1*240, LEFT, 220);// 
//				}
//		}
//	}
//	koeff_h = 7.759*4;
//	mode1 = 0;
//	Trigger = 0;
//	myGLCD.setFont( BigFont);
//	myGLCD.setColor(VGA_YELLOW);
//	myGLCD.print(txt_info9,CENTER, 80);
//	myGLCD.setColor(255, 255, 255);
//	delay(500);
//	while (myTouch.dataAvailable()){}
//
//}
//------------------------------------------------------------------------------
/*

void logData() 
{
	uint32_t bgnBlock, endBlock;
	// Allocate extra buffer space.
	block_t block[BUFFER_BLOCK_COUNT];
	bool ind_start = false;
	uint32_t logTime = 0;
	int mode_strob = 0;
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	buttons_channel();        // Отобразить кнопки переключения входов
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect (250, 1, 318, 40);
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255, 255);
	myGLCD.print("Delay", 264, 5);
	myGLCD.print("-      +", 254, 20);
	myGLCD.printNumI(set_strob, 270, 20);
	myGLCD.setFont( SmallFont);
	DrawGrid();               // Отобразить сетку и нарисовать окантовку кнопок справа и внизу

	myGLCD.setColor (255, 255, 255);
	myGLCD.drawLine(250, 45, 318, 85);
	myGLCD.drawLine(250, 85, 318, 45);

	myGLCD.drawLine(250, 90, 318, 130);
	myGLCD.drawLine(250, 130, 318, 90);

	myGLCD.drawLine(250, 135, 318, 175);
	myGLCD.drawLine(250, 175, 318,135);

	myGLCD.drawLine(250, 200, 318, 239);
	myGLCD.drawLine(250, 239, 318, 200);


	myGLCD.setColor(VGA_LIME);
	myGLCD.fillRoundRect (1, 1, 60, 35);
	myGLCD.setColor (255, 0, 0);
	myGLCD.setBackColor(VGA_LIME);
	myGLCD.setFont(BigFont);
	myGLCD.print("ESC", 6, 9);
	myGLCD.setColor (255, 255, 255);
	myGLCD.drawRoundRect (1, 1, 60, 35);
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.fillRoundRect (40, 40, 200, 120);
	myGLCD.setBackColor(VGA_YELLOW);
	myGLCD.setColor (255, 0, 0);
	myGLCD.setFont(BigFont);
	myGLCD.print("CTAPT", 80, 72);
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setColor (255, 255,255);
	myGLCD.drawRoundRect (40, 40, 200, 120);

	StartSample = millis();

	while(1) 
	 {
				logTime = millis();


			 if(logTime - StartSample > 1000)  // Прерывистая индикация надписи "START"
				{
					StartSample = millis();
					ind_start = !ind_start;
					 if (ind_start)
						{
							myGLCD.setColor(VGA_YELLOW);
							myGLCD.fillRoundRect (40, 40, 200, 120);
							myGLCD.setBackColor(VGA_YELLOW);
							myGLCD.setColor (255, 0, 0);
							myGLCD.setFont(BigFont);
							myGLCD.print("CTAPT", 80, 72);
							myGLCD.setBackColor( 0, 0, 255);
							myGLCD.setColor (255, 255,255);
							myGLCD.drawRoundRect (40, 40, 200, 120);
						}
					else
						{
							myGLCD.setColor(VGA_YELLOW);
							myGLCD.fillRoundRect (40, 40, 200, 120);
							myGLCD.setBackColor(VGA_YELLOW);
							myGLCD.setFont( BigFont);
							myGLCD.print("     ", 80, 72);
							myGLCD.setBackColor( 0, 0, 255);
							myGLCD.setColor (255, 255,255);
							myGLCD.drawRoundRect (40, 40, 200, 120);
						}
				}
		 strob_start = digitalRead(strob_pin);
		 if (!strob_start) break;

		 if (myTouch.dataAvailable())
			{
				delay(10);
				myTouch.read();
				x_osc=myTouch.getX();
				y_osc=myTouch.getY();
				myGLCD.setBackColor( 0, 0, 255);
				myGLCD.setColor (255, 255,255);
				myGLCD.setFont( SmallFont);

				if ((x_osc>=1) && (x_osc<=60))          //  Выход из программы
					{
						if ((y_osc>=1) && (y_osc<=35))  //
						{
							waitForIt(1, 1, 60, 35);
							Draw_menu_ADC1();
							return;
						} 
					}

				if ((x_osc>=40) && (x_osc<=200))        //  Выход из ожидания, Старт
					{
						if ((y_osc>=40) && (y_osc<=120))  //
						{
							waitForIt(40, 40, 200, 120);
							break;
						} 
					}


			if ((x_osc>=250) && (x_osc<=284))  // Боковые кнопки
				  {
				 
					  if ((y_osc>=1) && (y_osc<=40))  // Первая  период -
					  {
						waitForIt(250, 1, 318, 40);
						mode_strob -- ;
						if (mode_strob < 0) mode_strob = 0;  
						if (mode_strob == 0) { set_strob = 50; }
						if (mode_strob == 1) {set_strob = 100;}
						if (mode_strob == 2) {set_strob = 250;}
						if (mode_strob == 3) {set_strob = 500;}
						if (mode_strob == 4) {set_strob = 1000;}
						if (mode_strob == 5) {set_strob = 5000;}
						myGLCD.print("-      +", 254, 20);
						myGLCD.printNumI(set_strob, 270, 20);

					  }

					}
			if ((x_osc>=284) && (x_osc<=318))  // Боковые кнопки
				  {
					  if ((y_osc>=1) && (y_osc<=40))  // Первая  период  +
					  {
						waitForIt(250, 1, 318, 40);
						mode_strob ++ ;
						if (mode_strob> 5) mode_strob = 5;   
						if (mode_strob == 0) { set_strob = 50; }
						if (mode_strob == 1) {set_strob = 100;}
						if (mode_strob == 2) {set_strob = 250;}
						if (mode_strob == 3) {set_strob = 500;}
						if (mode_strob == 4) {set_strob = 1000;}
						if (mode_strob == 5) {set_strob = 5000;}
						myGLCD.print("-      +", 254, 20);
						myGLCD.printNumI(set_strob, 270, 20);
					  }
				 }

			if ((y_osc>=205) && (y_osc<=239))  // Нижние кнопки переключения входов
					{
						touch_osc();
					}
			}
	}

	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setFont(BigFont);
	adcInit((metadata_t*) &block[0]);   
	preob_num_str();
  // Find unused file name.
  if (BASE_NAME_SIZE > 6) 
	  {
		error("FILE_BASE_NAME too long");
	  }
  while (sd.exists(binName)) 
	  {
		if (binName[BASE_NAME_SIZE + 1] != '9') 
			{
			  binName[BASE_NAME_SIZE + 1]++;
			}
		else 
			{
			  binName[BASE_NAME_SIZE + 1] = '0';
			  if (binName[BASE_NAME_SIZE] == '9') 
			  {
				error("Can't create file name");
			  }
			  binName[BASE_NAME_SIZE]++;
			}
	  }
  // Delete old tmp file.
  if (sd.exists(TMP_FILE_NAME)) 
	  {
	//	Serial.println(F("Deleting tmp file"));
		myGLCD.print(txt_info13,LEFT, 135);              //
		if (!sd.remove(TMP_FILE_NAME)) 
			{
			  error("Can't remove tmp file");
			}
	  }
  // Create new file.
 // Serial.println(F("Creating new file"));
  myGLCD.print(txt_info27,LEFT, 155);//
  binFile.close();
  if (!binFile.createContiguous(sd.vwd(),
	TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT)) 
	  {
		error("createContiguous failed");
	  }
  // Get the address of the file on the SD.
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) 
	  {
		error("contiguousRange failed");
	  }
  // Use SdFat's internal buffer.
  uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
  if (cache == 0) error("cacheClear failed"); 
 
  // Flash erase all data in the file.
  myGLCD.print(txt_info14,LEFT, 175);  //Erasing all data
  uint32_t bgnErase = bgnBlock;
  uint32_t endErase;
  while (bgnErase < endBlock) 
	  {
		endErase = bgnErase + ERASE_SIZE;
		if (endErase > endBlock) endErase = endBlock;
		if (!sd.card()->erase(bgnErase, endErase)) 
			{
			  error("erase failed");
			}
		bgnErase = endErase + 1;
	  }

  // Start a multiple block write.
  if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT)) 
	  {
		error("writeBegin failed");
	  }
  // Write metadata. Написать метаданные.

  if (!sd.card()->writeData((uint8_t*)&block[0])) 
	  {
		error("Write metadata failed");
	  } 
  // Initialize queues. Инициализация очереди
  emptyHead = emptyTail = 0;  // Начало и окончание равно 0
  fullHead = fullTail = 0;    // 
  
  // Use SdFat buffer for one block.
  emptyQueue[emptyHead] = (block_t*)cache;
  emptyHead = queueNext(emptyHead);
  
  // Put rest of buffers in the empty queue. Поместите остальные буферов в пустую очередь.
  for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) 
	  {
		emptyQueue[emptyHead] = &block[i];
		emptyHead = queueNext(emptyHead);
	  }
  // Give SD time to prepare for big write.
  delay(1000);
  myGLCD.setColor(VGA_LIME);
  myGLCD.print(txt_info11, CENTER, 200);
  // Wait for Serial Idle.
  Serial.flush();
  delay(10);
  uint32_t bn = 1;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overruns = 0;
  uint32_t count = 0;
  uint32_t maxLatency = 0;
  int proc_step = 0;
  myGLCD.setColor(255,150,50);
  myGLCD.print("                    ", CENTER, 100);
  myGLCD.print(txt_info28, CENTER, 100);

  // Start logging interrupts.
  adcStart();

  Timer3.start(set_strob);

  while (1) 
	  {
		if (fullHead != fullTail) 
			{
			  // Get address of block to write.  Получить адрес блока, чтобы написать
			  block_t* pBlock = fullQueue[fullTail];
	  
			  // Write block to SD.  Написать блок SD
			  uint32_t usec = micros();
			  if (!sd.card()->writeData((uint8_t*)pBlock)) 
				  {
					error("write data failed");
	//					myGLCD.clrScr();
					myGLCD.setBackColor(0, 0, 0);
					delay(2000);
					Draw_menu_ADC1();
					return;
				  }
			  usec = micros() - usec;
			  t1 = millis();
			  if (usec > maxLatency) maxLatency = usec; // Максимальное время записи блока в SD
			  count += pBlock->count;
	  
			  // Add overruns and possibly light LED. 
			  if (pBlock->overrun) 
				  {
					overruns += pBlock->overrun;
					if (ERROR_LED_PIN >= 0) 
						{
						  digitalWrite(ERROR_LED_PIN, HIGH);
						}
				  }
			  // Move block to empty queue.
			  emptyQueue[emptyHead] = pBlock;
			  emptyHead = queueNext(emptyHead);
			  fullTail = queueNext(fullTail);
			  bn++;
			  myGLCD.print("R", CENTER, 55, proc_step);
			  proc_step++;
			  if(proc_step > 359) proc_step = 0;
	
			  if (bn == FILE_BLOCK_COUNT) 
				  {
					// File full so stop ISR calls.

				   Timer3.stop();
					break;
				  }
			}
		 if (timerError) 
			{
			  error("Missed timer event - rate too high");
			}
		if (!strob_start)
				{
					strob_start = digitalRead(strob_pin);                      // Проверка входа внешнего запуска 
						if (strob_start)
						{
							repeat = false;
								if (!strob_start) 
									{
										myGLCD.setColor(VGA_RED);
										myGLCD.fillCircle(227,12,10);
									}
								else
									{
										myGLCD.setColor(255,255,255);
										myGLCD.drawCircle(227,12,10);
									}
								myGLCD.setColor(255,255,255);
							break;
						}
				}

		if (myTouch.dataAvailable()) 
			{
				myGLCD.setColor(VGA_YELLOW);
				myGLCD.print("                    ", CENTER, 100);
				myGLCD.print("C\xA4o\xA3 \x9C""a\xA3\x9D""c\xAC", CENTER, 100);
				myGLCD.setColor(255, 255, 255);
			  // Stop ISR calls.
				 Timer3.stop();

			  if (isrBuf != 0 && isrBuf->count >= count_pin) 
				  {
					// Truncate to last complete sample.
					isrBuf->count = count_pin*(isrBuf->count/count_pin);
					// Put buffer in full queue.
					fullQueue[fullHead] = isrBuf;
					fullHead = queueNext(fullHead);
					isrBuf = 0;
				  }

			  if (fullHead == fullTail) break;
				while (myTouch.dataAvailable()){}
				delay(1000);
				break;
			}
	  }
  if (!sd.card()->writeStop()) 
	  {
		error("writeStop failed");
	  }
  // Truncate file if recording stopped early.
  if (bn != FILE_BLOCK_COUNT) 
	  {    
		//Serial.println(F("Truncating file"));
		if (!binFile.truncate(512L * bn)) 
		{
		  error("Can't truncate file");
		}
	  }
  if (!binFile.rename(sd.vwd(), binName)) 
	   {
		 error("Can't rename file");
	   }
 
	delay(100);
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info6,CENTER, 5);//
	myGLCD.print(txt_info16,LEFT, 25);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(binName,RIGHT , 25);
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info17,LEFT, 45);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumI(maxLatency, RIGHT, 45);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info18,LEFT, 65);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumF(0.001*(t1 - t0),2, RIGHT, 65);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info19,LEFT, 85);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumI(count/count_pin, RIGHT, 85);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info20,LEFT, 105);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumF((1000.0/count_pin)*count/(t1-t0),1, RIGHT, 105);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info21,LEFT, 125);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.printNumI(overruns, RIGHT, 125);// 
	binaryToCsv();
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info11,CENTER, 210);
	myGLCD.setColor(255, 255, 255);
	while (!myTouch.dataAvailable()){}
	while (myTouch.dataAvailable()){}
	Draw_menu_ADC1();
}

*/

void Draw_menu_ADC_RF()
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x = 0; x<4; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(30, 20 + (50 * x), 290, 60 + (50 * x));
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(30, 20 + (50 * x), 290, 60 + (50 * x));
	}
	myGLCD.print(txt_ADC_menu1, CENTER, 30);       // "Record data"
	myGLCD.print(txt_ADC_menu2, CENTER, 80);       // "List fales"
	myGLCD.print(txt_ADC_menu3, CENTER, 130);      // "Data to Serial"
	myGLCD.print(txt_ADC_menu4, CENTER, 180);      // "EXIT"
}
void menu_ADC_RF()
{
	//	while (Serial.read() >= 0) {} // Удалить все символы из буфера
//	char c;

	while (true)
	{
		delay(10);
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			int	x = myTouch.getX();
			int	y = myTouch.getY();

			if ((x >= 30) && (x <= 290))                   // Upper row
			{
				if ((y >= 20) && (y <= 60))                // Button: 1
				{
					waitForIt(30, 20, 290, 60);            // "Запись данных"
				//	oscilloscope_file();
					//logDataRF();
					//logData();
					Draw_menu_ADC_RF();
				}
				if ((y >= 70) && (y <= 110))               // Button: 2
				{
					waitForIt(30, 70, 290, 110);
					//root = sd.open("/");                   // "Просмотр файла"
					//printDirectory_RF(root, 0);
					Draw_menu_ADC_RF();
				}
				if ((y >= 120) && (y <= 160))             // Button: 3
				{
					waitForIt(30, 120, 290, 160);
					//root = sd.open("/");                  // "Передача в КОМ"
					//print_serial(root, 0);
					Draw_menu_ADC_RF();
				}
				if ((y >= 170) && (y <= 220))             // Button: 4
				{
					waitForIt(30, 170, 290, 210);
					break;                                // "ВЫХОД"
				}
			}
		}
	}
}
//void logDataRF()
//{
//	uint32_t bgnBlock, endBlock;
//	block_t block[BUFFER_BLOCK_COUNT];          // Выделить дополнительное пространство буфера. BUFFER_BLOCK_COUNT = 12
//	bool ind_start = false;                     //
//	uint32_t logTime = 0;                       //
//	int mode_strob = 0;                         // Переключатель длительности сканирования          
//	myGLCD.clrScr();
//	myGLCD.setBackColor(0, 0, 0);               //
//	set_Channel();                              //  Установить номер аналогового входа
//	myGLCD.setColor(0, 0, 255);                 //
//	myGLCD.fillRoundRect(250, 1, 318, 40);      //  Очистить фрагмент экрана  
//	myGLCD.setBackColor(0, 0, 255);
//	myGLCD.setFont(SmallFont);
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print("MS*10", 261, 5);
//	myGLCD.print("-      +", 254, 20);
//	myGLCD.printNumI(set_strob, 270, 20);
////	myGLCD.setFont(SmallFont);
//	DrawGrid();                                 // Отобразить сетку и нарисовать окантовку кнопок справа
//
//	myGLCD.setColor(255, 255, 255);
//	
//	myGLCD.drawLine(250, 45, 318, 85);
//	myGLCD.drawLine(250, 85, 318, 45);
//
//	myGLCD.drawLine(250, 90, 318, 130);
//	myGLCD.drawLine(250, 130, 318, 90);
//
//	myGLCD.drawLine(250, 135, 318, 175);
//	myGLCD.drawLine(250, 175, 318, 135);
//
//
//	myGLCD.setColor(VGA_LIME);
//	myGLCD.fillRoundRect(1, 1, 60, 35);
//	myGLCD.setColor(255, 0, 0);
//	myGLCD.setBackColor(VGA_LIME);
//	myGLCD.setFont(BigFont);
//	myGLCD.print("ESC", 6, 9);
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.drawRoundRect(1, 1, 60, 35);
//	myGLCD.setColor(VGA_YELLOW);
//	myGLCD.fillRoundRect(40, 40, 200, 120);    // Заполнение надписи "СТАРТ"
//	myGLCD.setBackColor(VGA_YELLOW);
//	myGLCD.setColor(255, 0, 0);
//	myGLCD.setFont(BigFont);
//	myGLCD.print("CTAPT", 80, 72);
//	myGLCD.setBackColor(0, 0, 255);
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.drawRoundRect(40, 40, 200, 120);    // Белая окантовка надписи "СТАРТ"
//
//	StartSample = millis();                    // Подготовка мигания надписи "СТАРТ"
//
//	while (1)
//	{
//		logTime = millis();
//
//		if (logTime - StartSample > 500)       // Прерывистая индикация надписи "START"
//		{
//			StartSample = millis();
//			ind_start = !ind_start;
//			if (ind_start)
//			{
//				myGLCD.setColor(VGA_YELLOW);
//				myGLCD.fillRoundRect(40, 40, 200, 120);
//				myGLCD.setBackColor(VGA_YELLOW);
//				myGLCD.setColor(255, 0, 0);
//				myGLCD.setFont(BigFont);
//				myGLCD.print("CTAPT", 80, 72);
//				myGLCD.setBackColor(0, 0, 255);
//				myGLCD.setColor(255, 255, 255);
//				myGLCD.drawRoundRect(40, 40, 200, 120);
//			}
//			else
//			{
//				myGLCD.setColor(VGA_YELLOW);
//				myGLCD.fillRoundRect(40, 40, 200, 120);
//				myGLCD.setBackColor(VGA_YELLOW);
//				myGLCD.setFont(BigFont);
//				myGLCD.print("     ", 80, 72);
//				myGLCD.setBackColor(0, 0, 255);
//				myGLCD.setColor(255, 255, 255);
//				myGLCD.drawRoundRect(40, 40, 200, 120);
//			}
//		}
//
//
//		if (myTouch.dataAvailable())
//		{
//			delay(10);
//			myTouch.read();
//			x_osc = myTouch.getX();
//			y_osc = myTouch.getY();
//			myGLCD.setBackColor(0, 0, 255);
//			myGLCD.setColor(255, 255, 255);
//			myGLCD.setFont(SmallFont);
//
//			if ((x_osc >= 1) && (x_osc <= 60))          //  Выход из программы без измерения, Завершить выполнение программы
//			{
//				if ((y_osc >= 1) && (y_osc <= 35))      //
//				{
//					waitForIt(1, 1, 60, 35);
//					Draw_menu_ADC_RF();
//					return;
//				}
//			}
//
//			if ((x_osc >= 40) && (x_osc <= 200))        //  Выход из ожидания, продолжить выполнение измерения
//			{
//				if ((y_osc >= 40) && (y_osc <= 120))    //
//				{
//					waitForIt(40, 40, 200, 120);
//					break;
//				}
//			}
//
//
//			if ((x_osc >= 250) && (x_osc <= 284))                // Боковые кнопки. Установки времени развертки
//			{
//
//				if ((y_osc >= 1) && (y_osc <= 40))               // Первая,  период уменьшить
//				{
//					waitForIt(250, 1, 318, 40);
//					mode_strob--;
//					if (mode_strob < 0) mode_strob = 0;
//					if (mode_strob == 0) { set_strob = 10; }
//					if (mode_strob == 1) { set_strob = 25; }
//					if (mode_strob == 2) { set_strob = 50; }
//					if (mode_strob == 3) { set_strob = 100; }
//					if (mode_strob == 4) { set_strob = 250; }
//					if (mode_strob == 5) { set_strob = 500; }
//					myGLCD.print("-      +", 254, 20);
//					myGLCD.printNumI(set_strob, 270, 20);
//				}
//			}
//			if ((x_osc >= 284) && (x_osc <= 318))              // Боковые кнопки
//			{
//				if ((y_osc >= 1) && (y_osc <= 40))             // Первая  период  увеличить
//				{
//					waitForIt(250, 1, 318, 40);
//					mode_strob++;
//					if (mode_strob> 5) mode_strob = 5;
//					if (mode_strob == 0) { set_strob = 10; }
//					if (mode_strob == 1) { set_strob = 25; }
//					if (mode_strob == 2) { set_strob = 50; }
//					if (mode_strob == 3) { set_strob = 100; }
//					if (mode_strob == 4) { set_strob = 250; }
//					if (mode_strob == 5) { set_strob = 500; }
//					myGLCD.print("-      +", 254, 20);
//					myGLCD.printNumI(set_strob, 270, 20);
//				}
//			}
//
//			if ((y_osc >= 205) && (y_osc <= 239))                   // Нижние кнопки переключения входов. Не используется!!
//			{
//				//touch_osc();      
//			}
//		}
//	}
//
//	// Выбор закончен, переходим к измерению
//	// Сначала формируем имя файла и открываем новый файл
//
//	myGLCD.clrScr();
//	myGLCD.setBackColor(0, 0, 0);
//	myGLCD.setFont(BigFont);
//	adcInit((metadata_t*)&block[0]);                                // 
//	preob_num_str();                                                // Сформировать имя файла
//	// Find unused file name.
//	if (BASE_NAME_SIZE > 6)
//	{
//		error("FILE_BASE_NAME too long");
//	}
//	while (sd.exists(binName))
//	{
//		if (binName[BASE_NAME_SIZE + 1] != '9')
//		{
//			binName[BASE_NAME_SIZE + 1]++;
//		}
//		else
//		{
//			binName[BASE_NAME_SIZE + 1] = '0';
//			if (binName[BASE_NAME_SIZE] == '9')
//			{
//				error("Can't create file name");
//			}
//			binName[BASE_NAME_SIZE]++;
//		}
//	}
//	// Delete old tmp file.
//	if (sd.exists(TMP_FILE_NAME))
//	{
//		//	Serial.println(F("Deleting tmp file"));
//		myGLCD.print(txt_info13, LEFT, 135);              //
//		if (!sd.remove(TMP_FILE_NAME))
//		{
//			error("Can't remove tmp file");
//		}
//	}
//	// Create new file.
//	myGLCD.print(txt_info27, LEFT, 155);                                              // "Creating new file";
//	binFile.close();                                                                  //
//	if (!binFile.createContiguous(sd.vwd(), TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT))   //
//	{
//		error("createContiguous failed");
//	}
//	if (!binFile.contiguousRange(&bgnBlock, &endBlock))                               // Получить адрес файла на SD.
//	{
//		error("contiguousRange failed");
//	}
//	// Use SdFat's internal buffer.                                                   // Использовать внутренний буфер SdFat.
//	uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
//	if (cache == 0) error("cacheClear failed");
//
//	// Flash erase all data in the file.                                              // Flash удаляет все данные в файле.
//	myGLCD.print(txt_info14, LEFT, 175);                                              //Erasing all data
//	uint32_t bgnErase = bgnBlock;
//	uint32_t endErase;
//	while (bgnErase < endBlock)
//	{
//		endErase = bgnErase + ERASE_SIZE;
//		if (endErase > endBlock) endErase = endBlock;
//		if (!sd.card()->erase(bgnErase, endErase))
//		{
//			error("erase failed");
//		}
//		bgnErase = endErase + 1;
//	}
//
//	// Start a multiple block write. // Запустить запись нескольких блоков.
//	if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT))        // Запись заголовка файла
//	{
//		error("writeBegin failed");
//	}
//	// Write metadata. Написать метаданные.
//	if (!sd.card()->writeData((uint8_t*)&block[0]))
//	{
//		error("Write metadata failed");
//	}
//	// Initialize queues. Инициализация очереди
//	emptyHead = emptyTail = 0;                                       // Начало и окончание равно 0
//	fullHead = fullTail = 0;                                         // 
//									                                     // Use SdFat buffer for one block.
//	emptyQueue[emptyHead] = (block_t*)cache;
//	emptyHead = queueNext(emptyHead);
//
//	// Put rest of buffers in the empty queue. Поместите остальные буферов в пустую очередь.
//	for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++)
//	{
//		emptyQueue[emptyHead] = &block[i];
//		emptyHead = queueNext(emptyHead);
//	}
//	// Give SD time to prepare for big write.                        // Дайте SD время для подготовки к большой записи.
//	delay(1000);
//	myGLCD.setColor(VGA_LIME);
//	myGLCD.print(txt_info11, CENTER, 200);                          // "ESC->PUSH Display";
//
//	uint32_t bn = 1;
//	uint32_t t0 = 0;
//	uint32_t t1 = t0;
//	uint32_t overruns = 0;
//	uint32_t count = 0;
//	uint32_t maxLatency = 0;
//	int proc_step = 0;
//	myGLCD.setColor(255, 150, 50);
//	myGLCD.print("                    ", CENTER, 100);
//	myGLCD.print(txt_info28, CENTER, 100);
//	setup_radio();                                         // Настроить радиоканал
//	// Start logging interrupts.
//	data_out[8] = 254;                               // Установить максимальную громкость сигнала
//	data_out[9] = 254;                               // Установить максимальную громкость сигнала
//	radio_transfer();                                //  Отправить радио синхро сигнал
//	delay(200);
//	adcStart();                                      // Разрешение на измерение
//
//	radio_transfer();                                // Отправить радио синхро сигнал
//	count_ms = 0;
//	t0 = millis();
//	t1 = t0;
//	Timer3.start(set_strob);                         // Запуск стробирования
//	Timer7.start(scale_strob * 1000);                                        // Включить формирование  временных меток на экране
//	// Переходим к измерению аналогового сигнала
//
//	while (1)
//	{
//		if (fullHead != fullTail)
//		{
//			// Get address of block to write.  Получить адрес блока, чтобы написать
//			block_t* pBlock = fullQueue[fullTail];
//
//			// Write block to SD.  Написать блок SD
//			uint32_t usec = micros();
//			if (!sd.card()->writeData((uint8_t*)pBlock))
//			{
//				error("write data failed");
//				//					myGLCD.clrScr();
//				myGLCD.setBackColor(0, 0, 0);
//				delay(1000);
//				Draw_menu_ADC_RF();
//				return;
//			}
//
//			usec = micros() - usec;
//		//	t1 = millis();
//			if (usec > maxLatency) maxLatency = usec;               // Максимальное время записи блока в SD
//			count += pBlock->count;
//
//			// Add overruns and possibly light LED. 
//			if (pBlock->overrun)
//			{
//				overruns += pBlock->overrun;
//				if (ERROR_LED_PIN >= 0)
//				{
//					digitalWrite(ERROR_LED_PIN, HIGH);
//				}
//			}
//			// Move block to empty queue.
//			emptyQueue[emptyHead] = pBlock;
//			emptyHead = queueNext(emptyHead);
//			fullTail = queueNext(fullTail);
//			bn++;
//			myGLCD.drawLine(0, 70, proc_step*2, 70);
//			proc_step++;
//			if (proc_step > 60)                         // Время записи
//			{
//				proc_step = 0;
//				t1 = millis();
//				Timer3.stop();
//				Timer7.stop();
//
//				break;
//			}
//
//			if (bn == FILE_BLOCK_COUNT)  
//			{
//				t1 = millis();
//				Timer3.stop();
//				Timer7.stop();
//			//	Serial.println(t1 - t0);
//				break;
//			}
//		}
//		if (timerError)
//		{
//			error("Missed timer event - rate too high");
//		}
//
//		if (myTouch.dataAvailable())
//		{
//			myGLCD.setColor(VGA_YELLOW);
//			myGLCD.print("                    ", CENTER, 100);
//			myGLCD.print("C\xA4o\xA3 \x9C""a\xA3\x9D""c\xAC", CENTER, 100);
//			myGLCD.setColor(255, 255, 255);
//			// Stop ISR calls.
//			Timer3.stop();
//			Timer7.stop();
//			if (isrBuf != 0 && isrBuf->count >= count_pin)
//			{
//				// Truncate to last complete sample.
//				isrBuf->count = count_pin*(isrBuf->count / count_pin);
//				// Put buffer in full queue.  // Поместите буфер в полную очередь.
//				fullQueue[fullHead] = isrBuf;
//				fullHead = queueNext(fullHead);
//				isrBuf = 0;
//			}
//
//			if (fullHead == fullTail) break;
//			while (myTouch.dataAvailable()) {}
////			delay(1000);
//			break;
//		}
//	}
//	if (!sd.card()->writeStop())
//	{
//		error("writeStop failed");
//	}
//	// Truncate file if recording stopped early.
//	if (bn != FILE_BLOCK_COUNT)
//	{
//		if (!binFile.truncate(512L * bn))
//		{
//			error("Can't truncate file");
//		}
//	}
//	if (!binFile.rename(sd.vwd(), binName))
//	{
//		error("Can't rename file");
//	}
//
//	delay(100);
//	myGLCD.setFont(BigFont);
//	myGLCD.clrScr();
//	myGLCD.setBackColor(0, 0, 0);
//	myGLCD.print(txt_info6, CENTER, 5);                 // "Info: "
//	myGLCD.print(txt_info16, LEFT, 25);                 // "File: "
//	myGLCD.setColor(VGA_YELLOW);
//	myGLCD.print(binName, RIGHT, 25);
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print(txt_info17, LEFT, 45);                 // "Max block :"
//	myGLCD.setColor(VGA_YELLOW);
//	myGLCD.printNumI(maxLatency, RIGHT, 45);            // 
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print(txt_info18, LEFT, 65);                 // "Record time: "
//	myGLCD.setColor(VGA_YELLOW);
////	myGLCD.printNumF(0.001*(t1 - t0), 2, RIGHT, 65);// 
//	myGLCD.printNumF((t1 - t0), 2, RIGHT, 65);// 
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print(txt_info19, LEFT, 85);//
//	myGLCD.setColor(VGA_YELLOW);
//	myGLCD.printNumI(count / count_pin, RIGHT, 85);// 
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print(txt_info20, LEFT, 105);//
//	myGLCD.setColor(VGA_YELLOW);
//	myGLCD.printNumF((1000.0 / count_pin)*count / (t1 - t0), 1, RIGHT, 105);// 
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.print(txt_info21, LEFT, 125);//
//	myGLCD.setColor(VGA_YELLOW);
//	myGLCD.printNumI(overruns, RIGHT, 125);// 
//	binaryToCsv();
//	myGLCD.setColor(VGA_LIME);
//	myGLCD.print(txt_info11, CENTER, 210);
//	myGLCD.setColor(255, 255, 255);
//	while (!myTouch.dataAvailable()) {}
//	while (myTouch.dataAvailable()) {}
//	Draw_menu_ADC_RF();
//}



//************************** Аналоговые часы ************************************
int bcd2bin(int temp)//BCD  to decimal
{
	int a,b,c;
	a=temp;
	b=0;
	if(a>=16)
	{
		while(a>=16)
		{
			a=a-16;
			b=b+10;
			c=a+b;
			temp=c;
		}
	}
	return temp;
}

void clock_print_serial()
{
	/*
	  Serial.print(date, DEC);
	  Serial.print('/');MLML
	  Serial.print(mon, DEC);
	  Serial.print('/');
	  Serial.print(year, DEC);//Serial display time
	  Serial.print(' ');
	  Serial.print(hour, DEC);
	  Serial.print(':');
	  Serial.print(min, DEC);
	  Serial.print(':');
	  Serial.print(sec, DEC);
	  Serial.println();
	  Serial.print(" week: ");
	  Serial.print(dow, DEC);
	  Serial.println();
	  */
}
void drawDisplay()
{
  myGLCD.clrScr();  // Clear screen
  
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


  dt = DS3231_clock.getDateTime();
 
   drawMin(dt.minute);
   drawHour(dt.hour, dt.minute);
   drawSec(dt.second);
   oldsec= dt.second;

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
  
  myGLCD.setColor(64, 64, 128);
  myGLCD.fillRoundRect(260, 140, 319, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(260, 140, 319, 180);
  myGLCD.setBackColor(64, 64, 128);
  myGLCD.print("RET", 266, 150);
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
	myGLCD.setFont(BigFont);
	myGLCD.setColor(0, 0, 0);
	myGLCD.setBackColor(255, 255, 255);
	myGLCD.print(daynames[dt.dayOfWeek -1], 256, 8);
  if (dt.day<10)
	myGLCD.printNumI(dt.day, 272, 28);
  else
	myGLCD.printNumI(dt.day, 264, 28);

  myGLCD.print(str_mon[dt.month -1], 256, 48);
  myGLCD.printNumI(dt.year, 248, 65);
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
	 // dt = DS3231_clock.getDateTime();
	  if (isAlarm)
	  {
		  if (oldsec != dt.second)
		  {
			  if ((dt.second == 0) && (dt.minute == 0) && (dt.hour == 0))
			  {
				  if (isAlarm)
				  {
					  clearDate();
					  printDate();
				  }
			  }
			  if (dt.second == 0)
			  {
				  if (isAlarm)
				  {
					  drawMin(dt.minute);
					  drawHour(dt.hour, dt.minute);
				  }
			  }
			  if (isAlarm)
			  {
				  drawSec(dt.second);
				  oldsec = dt.second;
			  }
		  }

		  if (myTouch.dataAvailable())
		  {
			  myTouch.read();
			  x = myTouch.getX();
			  y = myTouch.getY();
			  sound1();
			  if (((y >= 200) && (y <= 239)) && ((x >= 260) && (x <= 319))) //установка часов
			  {
				  myGLCD.setColor(255, 0, 0);
				  myGLCD.drawRoundRect(260, 200, 319, 239);
				  setClockRTC();
			  }

			  if (((y >= 140) && (y <= 180)) && ((x >= 260) && (x <= 319))) //Возврат
			  {
				  myGLCD.setColor(255, 0, 0);
				  myGLCD.drawRoundRect(260, 140, 319, 180);
				  myGLCD.clrScr();
				  myGLCD.setFont(BigFont);
				  break;
			  }
		  }

		  delay(10);
	  }
	}
}

void digitprint(int value, int lenght) 
{
	for (int i = 0; i < (lenght - numdigits(value)); i++) 
	{
		Serial.print("0");
	}
	Serial.print(value);
}

int numdigits(int i) 
{
	int digits;
	if (i < 10)
		digits = 1;
	else
		digits = (int)(log10((double)i)) + 1;
	return digits;
}

void count_time()
{

	Serial.print("Long number format:          ");
	Serial.println(DS3231_clock.dateFormat("d-m-Y H:i:s", dt));

	Serial.print("Long format with month name: ");
	Serial.println(DS3231_clock.dateFormat("d F Y H:i:s", dt));

	Serial.print("Short format witch 12h mode: ");
	Serial.println(DS3231_clock.dateFormat("jS M y, h:ia", dt));

	Serial.print("Today is:                    ");
	Serial.print(DS3231_clock.dateFormat("l, z", dt));
	Serial.println(" days of the year.");

	Serial.print("Actual month has:            ");
	Serial.print(DS3231_clock.dateFormat("t", dt));
	Serial.println(" days.");

	Serial.print("Unixtime:                    ");
	Serial.println(DS3231_clock.dateFormat("U", dt));

	Serial.println();





	//Serial.print("Unixtime: ");
	//Serial.println(DS3231_clock.dateFormat("U", dt));
	//Serial.println("And in plain for everyone");
	//Serial.print("Time: ");
	//digitprint(DS3231_clock.get_hours(), 2);
	//Serial.print(":");
	//digitprint(DS3231_clock.get_minutes(), 2);
	//Serial.print(":");
	//digitprint(DS3231_clock.get_seconds(), 2);
	//Serial.println("");
	//Serial.print("Date: ");
	//Serial.print(daynames[DS3231_clock.get_day_of_week() - 1]);
	//Serial.print(" ");
	//digitprint(DS3231_clock.get_days(), 2);
	//Serial.print(".");
	//digitprint(DS3231_clock.get_months(), 2);
	//Serial.print(".");
	//Serial.println(DS3231_clock.get_years());
	//Serial.println("");


}

// ******************* Главное меню ********************************
void draw_Start_Menu()
{
	myGLCD.clrScr();
	but4 = myButtons.addButton(10, 200, 250, 35, txt_Start_Menu);
	butX = myButtons.addButton(279, 199, 40, 40, "W", BUTTON_SYMBOL); // кнопка Часы 
	myGLCD.setColor(VGA_BLACK);
	myGLCD.setBackColor(VGA_WHITE);
	myGLCD.setColor(0, 255, 0);
	myGLCD.setBackColor(0, 0, 0);
	myButtons.drawButtons();
}

void Start_Menu() // Тексты меню в строках "txt....."
{
	while (1)
	{
		myButtons.setTextFont(BigFont);                      // Установить Большой шрифт кнопок  
		measure_power();
		if (myTouch.dataAvailable() == true)              // Проверить нажатие кнопок
		{
			//sound1();
			pressed_button = myButtons.checkButtons();    // Если нажата - проверить что нажато
			sound1();
			if (pressed_button == butX)                // Нажата вызов часы
			{
				// sound1();
				myGLCD.setFont(BigFont);
				AnalogClock();
				myGLCD.clrScr();
				myButtons.drawButtons();         // Восстановить кнопки
			}

			//*****************  Меню №1  **************

			//if (pressed_button == but1)              // Измерение задержки
			//{
			//	myGLCD.clrScr();
			//	menu_delay_measure();
			//	myGLCD.clrScr();
			//	myButtons.drawButtons();
			//}

			//if (pressed_button == but2)              //Меню Настройка
			//{
			//	Draw_menu_tuning();
			//	menu_tuning();
			//	myGLCD.clrScr();
			//	myButtons.drawButtons();
			//}

			//if (pressed_button == but3)             // 
			//{
			//	if (SD_enable)
			//	{
			//		Draw_menu_ADC_RF();          // Нарисовать меню регистратора
			//		menu_ADC_RF();               // Перейти в меню регистратора 
			//	}
			//	else
			//	{
			//		myGLCD.setFont(BigFont);
			//		myGLCD.clrScr();
			//		myGLCD.print("\x89""po""\x96\xA0""e""\xA1""a c SD", CENTER, 40);    //  Проблема с SD
			//		myGLCD.print("\x9F""ap""\xA4""o""\x9E""?", CENTER, 70);             //  картой?
			//		delay(2000);
			//	}
			//	myGLCD.clrScr();
			//	myButtons.drawButtons();
			//}
			if (pressed_button == but4)             // Работа с SD
			{
			//	draw_Glav_Menu();
				Swich_Glav_Menu();
				myGLCD.clrScr();
				myButtons.drawButtons();
			}
		}
	}
}

void Draw_Glav_Menu()
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x = 0; x<5; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(10, 15 + (45 * x), 250, 50 + (45 * x));
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(10, 15 + (45 * x), 250, 50 + (45 * x));
	}

	myGLCD.print(txt_menu1_1, 20, 25);        // 
	myGLCD.print(txt_menu1_2, 55, 70);
	myGLCD.print(txt_menu1_3, 45, 115);
	myGLCD.print(txt_menu1_4, 45, 160);
	myGLCD.print(txt_menu1_5, 90, 205);
	myGLCD.setColor(255, 255, 255);

}
void Swich_Glav_Menu()
{
	Draw_Glav_Menu();
	while (true)
	{
		delay(10);
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			int	x = myTouch.getX();
			int	y = myTouch.getY();

			if ((x >= 10) && (x <= 250))       // 
			{
				if ((y >= 15) && (y <= 50))    // Button: 1   
				{
					waitForIt(10, 15, 250, 50);
					myGLCD.clrScr();
					menu_delay_measure();
					Draw_Glav_Menu();
				}
				if ((y >= 60) && (y <= 100))   // Button: 2  
				{
					waitForIt(10, 60, 250, 100);
					Draw_menu_tuning();
					menu_tuning();
					Draw_Glav_Menu();
				}
				if ((y >= 105) && (y <= 145))  // Button: 3  
				{
					waitForIt(10, 105, 250, 145);
					synhro_DS3231_clock();
					Draw_Glav_Menu();
				}
				if ((y >= 150) && (y <= 190))  // Button: 4  
				{
					waitForIt(10, 150, 250, 190);

					Draw_Glav_Menu();
				}
				if ((y >= 195) && (y <= 235))  // Button: 5 "EXIT" Выход
				{
					waitForIt(10, 195, 250, 235);
					break;
				}
			}
		}
	}
}

void waitForIt(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  sound1();
  while (myTouch.dataAvailable())
  myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}
//++++++++++++++++++++++++++ Конец меню прибора ++++++++++++++++++++++++
void Draw_menu_tuning()
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x=0; x<4; x++)
		{
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect (30, 20+(50*x), 290,60+(50*x));
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+(50*x), 290,60+(50*x));
		}
	myGLCD.print( txt_tuning_menu1, CENTER, 30);     // 
	myGLCD.print( txt_tuning_menu2, CENTER, 80);
	myGLCD.print( txt_tuning_menu3, CENTER, 130);
	myGLCD.print( txt_tuning_menu4, CENTER, 180);
}
void menu_tuning()   // Меню "Настройка", вызывается из главного меню 
{
	while (true)
		{
		delay(10);
		if (myTouch.dataAvailable())
			{
				myTouch.read();
				int	x=myTouch.getX();
				int	y=myTouch.getY();

				if ((x>=30) && (x<=290))       // 
					{
					if ((y>=20) && (y<=60))    // Button: 1  "Oscilloscope"
						{
							waitForIt(30, 20, 290, 60);
							myGLCD.clrScr();
							oscilloscope();
							Draw_menu_tuning();
						}
					if ((y>=70) && (y<=110))   // Button: 2 "Синхронизация мод."
						{
							waitForIt(30, 70, 290, 110);
							myGLCD.clrScr();
							Draw_menu_synhro();
							menu_synhro();
							Draw_menu_tuning();
						}
					if ((y>=120) && (y<=160))  // Button: 3  
						{
							waitForIt(30, 120, 290, 160);
							myGLCD.clrScr();
							menu_radio();
							Draw_menu_tuning();
						}
					if ((y>=170) && (y<=220))  // Button: 4 "EXIT" Выход
						{
							waitForIt(30, 170, 290, 210);
							break;
						}
				}
			}
	   }

}

void Draw_menu_synhro()
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x = 0; x<4; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(20, 20 + (50 * x), 300, 60 + (50 * x));
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(20, 20 + (50 * x), 300, 60 + (50 * x));
	}
	myGLCD.print(txt_synhro_menu1, CENTER, 30);     // 
	myGLCD.print(txt_synhro_menu2, CENTER, 80);
	myGLCD.print(txt_synhro_menu3, CENTER, 130);
	myGLCD.print(txt_synhro_menu4, CENTER, 180);
}
void menu_synhro()                                                        // Меню "Настройка", вызывается из главного меню 
{
	while (true)
	{
		delay(10);
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			int	x = myTouch.getX();
			int	y = myTouch.getY();

			if ((x >= 20) && (x <= 300))       // 
			{
				if ((y >= 20) && (y <= 60))                                 // Button: 1  "Синхронизация по радио"
				{
					waitForIt(20, 20, 300, 60);
					myGLCD.clrScr();
					data_out[2] = 2;                                        // Отправить команду синхронизации модулей(включить таймер прерываний)
					tuning_mod();                                           // Синхронизация по радио
					Draw_menu_synhro();
				}
				if ((y >= 70) && (y <= 110))                               // Button: 2 "Синхронизация проводная"
				{
					waitForIt(20, 70, 300, 110);
					myGLCD.clrScr();
					data_out[2] = 3;                                        // Отправить команду синхронизации модулей(включить таймер прерываний)
					radio_send_command();
					synhro_ware();
					Draw_menu_synhro();
				}
				if ((y >= 120) && (y <= 160))  // Button: 3  
				{
					waitForIt(20, 120, 300, 160);
					myGLCD.clrScr();
					data_out[2] = 4;                                        // Отправить команду синхронизации модулей(включить таймер прерываний)
					setup_radio();                                     // Настроить радиоканал
					delayMicroseconds(500);
					radio_send_command();                                   //  Отправить по радио команду стоп
					Timer4.stop();
					Timer5.stop();
					Timer6.stop();
					Draw_menu_synhro();
				}
				if ((y >= 170) && (y <= 220))  // Button: 4 "EXIT" Выход
				{
					waitForIt(20, 170, 300, 210);
					break;
				}
			}
		}
	}
}
void trigger()
{
	myGLCD.setFont(SmallFont);
	ADC_CHER = 0;    // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3    
	for(int tr = 0; tr < 1000; tr++)   // Определение нижней точки амплитуды сигнала
	{
		ADC_CR = ADC_START ; 	// Запустить преобразование
		while (!(ADC_ISR_DRDY));
					Input = ADC->ADC_CDR[7];
				//	Input = ADC->ADC_CDR[Analog_pinA0];   // A0
		// if (Input<Trigger) break;
		 if (Input< 50) break;
	}

	for(int tr = 0; tr < 1000; tr++)      // Определение  точки срабатывания триггера
	{
		 ADC_CR = ADC_START ; 	          // Запустить преобразование
		 while (!(ADC_ISR_DRDY));
		 Input = ADC->ADC_CDR[7];
		//Input = ADC->ADC_CDR[Analog_pinA0];   // A0
		if (Input > Trigger)
		{
			myGLCD.setColor(VGA_RED);
			myGLCD.fillCircle(227,10,10);
			//myGLCD.setColor(255, 255, 255);
			//myGLCD.print("     ", 250, 212);
			//myGLCD.printNumI(tr, 250, 212);
			//myGLCD.print("     ", 250, 224);
			//myGLCD.printNumI(Trigger, 250, 224);
			timeStartTrig = micros();                   // Записать время обнаружения первого фронта импульса
			trig_sin = true;                            // Есть срабатывание триггера
			break;
		}
		if (tr > 997)
		{
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillCircle(227,10, 10);
	/*		myGLCD.setColor(255, 255, 255);
			myGLCD.print("     ", 250, 212);
			myGLCD.printNumI(tr, 250, 212);
			myGLCD.print("     ", 250, 224);*/
			myGLCD.printNumI(Trigger, 250, 224);
			trig_sin = false;                            // Нет срабатывания триггера
		}
	}
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawCircle(227, 10, 10);
}
void trigger_synhro(int trig)
{
	ADC_CHER = 0;                                                  // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3    
	for (i_trig_syn = 0; i_trig_syn < 1000; i_trig_syn++)          // Определение нижней точки амплитуды сигнала
	{
		ADC_CR = ADC_START; 	                                   // Запустить преобразование
		while (!(ADC_ISR_DRDY));
		Input = ADC->ADC_CDR[7];    		                       //	Input = ADC->ADC_CDR[Analog_pinA0];   // A0
		if (Input< 50) break;
	}

	for (int i_trig_syn = 0; i_trig_syn < 1000; i_trig_syn++)      // Определение  точки срабатывания триггера
	{
		ADC_CR = ADC_START; 	                                   // Запустить преобразование
		while (!(ADC_ISR_DRDY));
		Input = ADC->ADC_CDR[7];		                           //Input = ADC->ADC_CDR[Analog_pinA0];   // A0
		if (Input > trig)
		{
			timeStartTrig = micros();                              // Записать время обнаружения первого фронта импульса
			trig_sin = true;                                       // Есть срабатывание триггера
			break;
		}
		if (i_trig_syn > 997)
		{
			trig_sin = false;                                      // Нет срабатывания триггера
		}
	}
}

void Draw_menu_delay_measure()                                    // Отображение меню синхронизации
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x = 0; x<4; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(20, 20 + (50 * x), 300, 60 + (50 * x));
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(20, 20 + (50 * x), 300, 60 + (50 * x));
	}
	myGLCD.print(txt_delay_measure1, CENTER, 30);     // 
	myGLCD.print(txt_delay_measure2, CENTER, 80);
	myGLCD.print(txt_delay_measure3, CENTER, 130);
	myGLCD.print(txt_delay_measure4, CENTER, 180);

}
void menu_delay_measure()                                                // Меню вариантов синхронизации
{
	Draw_menu_delay_measure();
	while (true)
	{
		delay(10);
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			int	x = myTouch.getX();
			int	y = myTouch.getY();

			if ((x >= 20) && (x <= 3000))                                  // 
			{
				if ((y >= 20) && (y <= 60))                                // Button: 1  Синхронизация по таймеру
				{
					waitForIt(20, 20, 300, 60);
					myGLCD.clrScr();
					synhro_by_timer();
					Draw_menu_delay_measure();
				}
				if ((y >= 70) && (y <= 110))                               // Button: 2 Синхронизация по радио
				{
					waitForIt(20, 70, 300, 110);
					myGLCD.clrScr();
					synhro_by_radio();                                    //  "Синхронизация по радио"
					Draw_menu_delay_measure();
				}
				if ((y >= 120) && (y <= 160))                             // Button: 3  
				{
					waitForIt(20, 120, 300, 160);
					myGLCD.clrScr();
					Draw_menu_ADC_RF();                                   // Нарисовать меню регистратора
					menu_ADC_RF();                                        // Перейти в меню регистратора с синхро RF
					Draw_menu_delay_measure();
				}
				if ((y >= 170) && (y <= 220))                             // Button: 4 "EXIT" Выход
				{
					waitForIt(20, 170, 300, 210);
					break;
				}
			}
		}
	}
}

void synhro_by_timer()                                    // просмотр в реальном времени. Синхронизация по таймеру
{
	myGLCD.clrScr();
	myGLCD.setBackColor( 0, 0, 0);
	buttons_right();                                      // Нарисовать кнопки справа экрана;
	buttons_channelNew();                                 // Нарисовать кнопки внизу экрана;
	myGLCD.setBackColor( 0, 0, 0);
	myGLCD.setFont( BigFont);
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info29,5, 190);                      // "Stop->PUSH Disp"
	mode = 3;                                             // Время развертки  
	count_ms = 0;                                         // Шкалу отсчета мс в начальное состояние
	long time_temp = 0;
	bool start_info = false;
	unsigned long StartTime = 0;                          // Ограничение времени измерения сигнала (2 секунды )
	set_volume(5, 1);                                     // Отобразить уровень громкости  каналов усилителя на Базе

	myGLCD.setColor(255, 255, 255);
	myGLCD.drawCircle(200, 10, 10);

	while (1)                                                                        //                
	{
		// Начало поиска синхроимпульса
		bool synhro_Off = false;                                                     // Признак синхронизации в исходное состояние. Предназначен для поиска синхроимпульсов
		trig_sin = false;                                                            // Признак триггера в исходное состояние   
		StartSample = micros();
		while (!start_enable)                                                        // Ожидание времени начала синхроимпульса.  Сигнал формируется таймером Timer5
		{
			if (micros() - StartSample > 4000000)                                    // Ожидаем синхроимпульс в течении 3 секунд
			{
				synhro_Off = true;
				break;                                                               // Завершить ожидание синхроимпульса
			}

			// Проверка кнопок меню
			if (myTouch.dataAvailable())
			{
				delay(10);
				myTouch.read();
				x_osc = myTouch.getX();
				y_osc = myTouch.getY();

				if ((x_osc >= 2) && (x_osc <= 240))               // Выход из программы. Нажать на область экрана
				{
					if ((y_osc >= 1) && (y_osc <= 160))           // 
					{
						Timer7.stop();                            //  Остановить вывода на экран временных меток
						return;
					}                                             //
				}

				myGLCD.setBackColor(0, 0, 255);                   // Синий фон кнопок
				myGLCD.setColor(255, 255, 255);
				myGLCD.setFont(SmallFont);

				if ((x_osc >= 245) && (x_osc <= 280))               // Боковые кнопки
				{
					if ((y_osc >= 1) && (y_osc <= 40))              // Первая  период
					{
						waitForIt(245, 1, 318, 40);
						//chench_mode(0);                             //
					}

					if ((y_osc >= 45) && (y_osc <= 85))             // Вторая - триггер
					{
						waitForIt(245, 45, 318, 85);
						chench_tmode(0);
					}
					if ((y_osc >= 90) && (y_osc <= 130))           // Третья - делитель
					{
						waitForIt(245, 90, 318, 130);
						chench_mode1(0);
					}
				}

				if ((x_osc >= 282) && (x_osc <= 318))              // Боковые кнопки
				{
					if ((y_osc >= 1) && (y_osc <= 40))             // Первая  период
					{
						waitForIt(245, 1, 318, 40);
						//chench_mode(1);                            //  
					}

					if ((y_osc >= 45) && (y_osc <= 85))            // Вторая - триггер
					{
						waitForIt(245, 45, 318, 85);
						chench_tmode(1);
					}
					if ((y_osc >= 90) && (y_osc <= 130))          // Третья - делитель
					{
						waitForIt(245, 90, 318, 130);
						chench_mode1(1);
					}
				}

				if ((x_osc >= 245) && (x_osc <= 318))                  // Боковые кнопки
				{
					if ((y_osc >= 135) && (y_osc <= 175))              // Четвертая разрешение
					{
						waitForIt(245, 135, 318, 175);
						i2c_eeprom_ulong_write(adr_set_timePeriod, EndSample - StartSynhro);  // Записать контрольное время 
					}
				}

				if ((y_osc >= 205) && (y_osc <= 239))                 // Нижние кнопки переключения 
				{
					if ((x_osc >= 10) && (x_osc <= 60))               //  Кнопка №1
					{
						waitForIt(10, 210, 60, 239);
						set_volume(1, 1);                             // Регулировка "+" усилителя №1

					}
					if ((x_osc >= 70) && (x_osc <= 120))              //  Кнопка №2
					{
						waitForIt(70, 210, 120, 239);
						set_volume(1, 2);                            // Регулировка "-" усилителя №1

					}
					if ((x_osc >= 130) && (x_osc <= 180))            //  Кнопка №3
					{
						waitForIt(130, 210, 180, 239);
						set_volume(2, 1);                            // Регулировка "+" усилителя №2

					}
					if ((x_osc >= 190) && (x_osc <= 240))            //  Кнопка №4
					{
						waitForIt(190, 210, 240, 239);
						set_volume(2, 2);                            // Регулировка "-" усилителя №1

					}
				}
			}
			// Закончили проверку кнопок  меню

		}

		if (synhro_Off)                                                              // Если нет синхроимпульсов - завершить
		{
			myGLCD.setFont(BigFont);
			myGLCD.setColor(VGA_YELLOW);
			myGLCD.print("He""\xA4", 100, 30);                                       // Нет
			myGLCD.print("c""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\x9D", 20, 50); // синхронизации
			delay(2000);
			break;                                                                   // Завершить измерение. Выход  в меню
		}

       if(start_enable == true)		                                                 // Синхроимпульс найден. Начинаем измерение звукового сигнала
	   {                                                                             // Все нормально. Измеряем !!
			start_enable = false;                                                    // Время начала синхроимпульса пришло. Выставляем флаг старта в исходное для ожидания следующего синхро импульса
			Timer7.start(scale_strob * 1000);                                        // Включить формирование  временных меток на экране
			page = 0;
			StartTime = micros();                                                                     //  Программа чтения данных АЦП. Записать аналоговый сигнал в блок памяти
			ADC_CHER = 0;                                                            // Включить аналоговый вход "0"          (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3
		
			while (!trig_sin)                                                        // Ожидание времени начала синхроимпульса.  Сигнал формируется таймером Timer5
			{
				if (micros() - StartTime > 2000000)                                    // Ожидаем синхроимпульс в течении 1 секунд
				{
					EndSample = micros();                                        // Записать время срабатывания триггера порога
					trig_sin = true;                                             // установить флаг срабатывания триггера порога
					break;                                                               // Завершить ожидание синхроимпульса
				}
				for (xpos = 0; xpos < 240; xpos++)                                   // Блоки по 240 байтов
				{
					ADC_CR = ADC_START; 	                                         // Запустить преобразование
					while (!(ADC_ISR_DRDY));                                         // Ожидание завершения преобразования
					Sample_osc[xpos][0] = ADC->ADC_CDR[7];                        // Получить данные А0 и записать в массив
					Synhro_osc[xpos][0][new_info] = 0;                                      // Стереть старые данные временных меток
					if ((Sample_osc[xpos][page] > Trigger) && (trig_sin == false)&&(xpos > 2))   // Поиск превышения уровня порога
					{
						EndSample = micros();                                        // Записать время срабатывания триггера порога
						trig_sin = true;                                             // установить флаг срабатывания триггера порога
						page_trig = page;                                            // Записываем номер блока в котором сработал триггер
					}
					delayMicroseconds(dTime);                                        // dTime временной интервал (задержка) развертки 
				}
				if (trig_sin == true) break;                                       // Завершить сканирование на блоке в котором сработал порог
			}
			Timer7.stop();                                                           // Завершить формирование временных меток
			//if (!strob_start)                                         // Нарисовать круг - индикатор синхронизации
			//{
			//	myGLCD.setColor(VGA_RED);
			//	myGLCD.fillCircle(227, 12, 10);
			//}
			//else
			//{
			//	myGLCD.setColor(255, 255, 255);
			//	myGLCD.drawCircle(227, 12, 10);
			//}
			count_ms = 0;                                                            // Шкалу отсчета мс в начальное состояние
			myGLCD.printNumI(page, 255, 177);                                        // Вывести на экран общее количество страниц
			myGLCD.printNumI(page_trig, 300, 177);                                   // Вывести на экран страницу срабатывания триггера порога
			// Выводит информацию об измерении
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(0, 0, 240, 176);                                    // Очистить область экрана для вывода временных меток. 
			DrawGrid1();
			myGLCD.setBackColor(0, 0, 255);                                          // Фон экрана синий
			myGLCD.setColor(255, 255, 255);                                          // Цвет текста белый
			myGLCD.print("        ", 255, 160);                                      // Очистить область вывода информации 
			myGLCD.printNumI(set_timePeriod, 255, 160);             
			myGLCD.print("        ", 255, 150);
			time_temp = EndSample - StartSynhro;
			myGLCD.printNumI(time_temp / 1000, 255, 150);                            // Время получения ответа
			myGLCD.setBackColor(0, 0, 0);                                            // Фон экрана черный
			myGLCD.setFont(BigFont);
			myGLCD.print("\x85""a""\x99""ep""\x9B\x9F""a", LEFT + 2, 8);             // "Задержка"
			myGLCD.print("c""\x9D\x98\xA2""a""\xA0""a", LEFT + 2, 25);               // "сигнала"
			myGLCD.print("      ", LEFT, 44);

			myGLCD.setFont(SmallFont);
			myGLCD.setBackColor(0, 0, 255);
			if (trig_sin)
			{
				myGLCD.setColor(255, 0, 0);
				myGLCD.drawRoundRect(245, 135, 318, 175);
				myGLCD.setBackColor(0, 0, 255);
				myGLCD.setColor(255, 255, 255);
				myGLCD.print("        ", 255, 140);

				time_temp = EndSample - StartSynhro - set_timePeriod;
				myGLCD.printNumF(time_temp / 1000.00, 2, 255, 140);  // вывод контрольной информации
				myGLCD.setFont(BigFont);
				myGLCD.setBackColor(0, 0, 0);
				myGLCD.printNumF(time_temp / 1000.00, 2, 5, 44); // вывод основной информации
				myGLCD.print(" ms", 88, 44);
				myGLCD.setFont(SmallFont);
				myGLCD.setBackColor(0, 0, 255);
				myGLCD.setColor(VGA_RED);                                                       // Вывести индикатор срабатывания триггера порога
				myGLCD.fillCircle(227, 10, 10);                                                 // Вывести индикатор срабатывания триггера порога
			}
			else
			{
				myGLCD.setColor(255, 255, 255);
				myGLCD.drawRoundRect(245, 135, 318, 175);
				myGLCD.setBackColor(0, 0, 255);
				myGLCD.setColor(255, 255, 255);
				myGLCD.print("        ", 255, 140);
				myGLCD.printNumI(0, 255, 140);
				myGLCD.setFont(BigFont);
				myGLCD.setBackColor(0, 0, 0);
				myGLCD.printNumI(0, 5, 44);
				myGLCD.print(" ms", 88, 44);
				myGLCD.setFont(SmallFont);
				myGLCD.setBackColor(0, 0, 255);
				myGLCD.setColor(0, 0, 0);
				myGLCD.fillCircle(227, 10, 10);
			}
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.setColor(255, 255, 255);
			myGLCD.print("     ", 250, 212);
			myGLCD.printNumI(i_trig_syn, 250, 212);
			myGLCD.print("     ", 250, 224);
			myGLCD.printNumI(Trigger, 250, 224);
			myGLCD.drawCircle(227, 10, 10);

			myGLCD.setBackColor(0, 0, 0);
			set_timePeriod = i2c_eeprom_ulong_read(adr_set_timePeriod);
			timePeriod = i2c_eeprom_ulong_read(adr_timePeriod);
			myGLCD.printNumF(set_timeSynhro / 1000000.00, 2, 250, 200, ',');
			myGLCD.print("sec", 285, 200);
			int b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);                // Отобразить  уровень громкости 1 канала усилителя Трансмиттера
			myGLCD.print("     ", 250, 188);                                           // Отобразить  уровень громкости 1 канала усилителя Трансмиттера
			myGLCD.print(proc_volume[b], 250, 188);                                    // Отобразить  уровень громкости 1 канала усилителя Трансмиттера

			ypos_trig = 255 - (Trigger / koeff_h) - hpos;                              // Получить уровень порога
			if (ypos_trig != ypos_trig_old)                                            // Стереть старый уровень порога при изменении
			{
				myGLCD.setColor(0, 0, 0);
				myGLCD.drawLine(1, ypos_trig_old, 240, ypos_trig_old);
				ypos_trig_old = ypos_trig;
			}
			myGLCD.setColor(255, 0, 0);
			myGLCD.drawLine(1, ypos_trig, 240, ypos_trig);                             // Нарисовать новую линию уровня порога
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(0, 162, 242, 176);                                    // Очистить область экрана вывода меток
			for (int xpos = 0; xpos < 239; xpos++)                                     // вывод на экран
			{
				// Нарисовать линию  осциллограммы
				myGLCD.setColor(255, 255, 255);
				ypos_osc1_0 = 255 - (Sample_osc[xpos][page_trig] / koeff_h) - hpos;
				ypos_osc2_0 = 255 - (Sample_osc[xpos + 1][page_trig] / koeff_h) - hpos;
				if (ypos_osc1_0 < 0) ypos_osc1_0 = 0;
				if (ypos_osc2_0 < 0) ypos_osc2_0 = 0;
				if (ypos_osc1_0 > 220) ypos_osc1_0 = 220;
				if (ypos_osc2_0 > 220) ypos_osc2_0 = 220;
				myGLCD.drawLine(xpos, ypos_osc1_0, xpos + 1, ypos_osc2_0);

				if (Synhro_osc[xpos][page_trig][new_info] > 0)
				{
					myGLCD.setColor(VGA_YELLOW);
					if (Synhro_osc[xpos][page_trig][new_info] == 4095)
					{
						myGLCD.drawLine(xpos, 60, xpos, 160);
					}
					else
					{
						myGLCD.setColor(255, 255, 255);
						if (xpos > 230)
						{
							myGLCD.printNumI(Synhro_osc[xpos][page_trig][new_info], xpos - 12, 165);
						}
						else
						{
							myGLCD.printNumI(Synhro_osc[xpos][page_trig][new_info], xpos, 165);

						}
					}
					myGLCD.setColor(255, 255, 255);
					Synhro_osc[xpos][page_trig][new_info] = 0;
				}
				myGLCD.setColor(0, 0, 0);
				myGLCD.fillCircle(200, 10, 10);
				myGLCD.setColor(255, 255, 255);
				myGLCD.drawCircle(200, 10, 10);
			}

			myGLCD.setColor(0,0,0);
			myGLCD.fillCircle(200, 10, 10);
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawCircle(200, 10, 10);
		}

		// Проверка кнопок меню
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_osc = myTouch.getX();
			y_osc = myTouch.getY();

			if ((x_osc >= 2) && (x_osc <= 240))               // Выход из программы. Нажать на область экрана
			{
				if ((y_osc >= 1) && (y_osc <= 160))           // 
				{
					Timer7.stop();                            //  Остановить вывода на экран временных меток
					break;                                    //
				}                                             //
			}

			myGLCD.setBackColor(0, 0, 255);                   // Синий фон кнопок
			myGLCD.setColor(255, 255, 255);
			myGLCD.setFont(SmallFont);

			if ((x_osc >= 245) && (x_osc <= 280))               // Боковые кнопки
			{
				if ((y_osc >= 1) && (y_osc <= 40))              // Первая  период
				{
					waitForIt(245, 1, 318, 40);
					//chench_mode(0);                             //
				}

				if ((y_osc >= 45) && (y_osc <= 85))             // Вторая - триггер
				{
					waitForIt(245, 45, 318, 85);
					chench_tmode(0);
				}
				if ((y_osc >= 90) && (y_osc <= 130))           // Третья - делитель
				{
					waitForIt(245, 90, 318, 130);
					chench_mode1(0);
				}
			}

			if ((x_osc >= 282) && (x_osc <= 318))              // Боковые кнопки
			{
				if ((y_osc >= 1) && (y_osc <= 40))             // Первая  период
				{
					waitForIt(245, 1, 318, 40);
					//chench_mode(1);                            //  
				}

				if ((y_osc >= 45) && (y_osc <= 85))            // Вторая - триггер
				{
					waitForIt(245, 45, 318, 85);
					chench_tmode(1);
				}
				if ((y_osc >= 90) && (y_osc <= 130))          // Третья - делитель
				{
					waitForIt(245, 90, 318, 130);
					chench_mode1(1);
				}
			}

			if ((x_osc >= 245) && (x_osc <= 318))                  // Боковые кнопки
			{
				if ((y_osc >= 135) && (y_osc <= 175))              // Четвертая разрешение
				{
					waitForIt(245, 135, 318, 175);
					i2c_eeprom_ulong_write(adr_set_timePeriod, EndSample - StartSynhro);  // Записать контрольное время 
				}
			}

			if ((y_osc >= 205) && (y_osc <= 239))                 // Нижние кнопки переключения 
			{
				if ((x_osc >= 10) && (x_osc <= 60))               //  Кнопка №1
				{
					waitForIt(10, 210, 60, 239);
					set_volume(1, 1);                             // Регулировка "+" усилителя №1

				}
				if ((x_osc >= 70) && (x_osc <= 120))              //  Кнопка №2
				{
					waitForIt(70, 210, 120, 239);
					set_volume(1, 2);                            // Регулировка "-" усилителя №1

				}
				if ((x_osc >= 130) && (x_osc <= 180))            //  Кнопка №3
				{
					waitForIt(130, 210, 180, 239);
					set_volume(2, 1);                            // Регулировка "+" усилителя №2

				}
				if ((x_osc >= 190) && (x_osc <= 240))            //  Кнопка №4
				{
					waitForIt(190, 210, 240, 239);
					set_volume(2, 2);                            // Регулировка "-" усилителя №1

				}
			}
		}
		// Закончили проверку кнопок  меню

		attachInterrupt(kn_red, volume_up, FALLING);
		attachInterrupt(kn_blue, volume_down, FALLING);
		data_out[2] = 6;                                    //
		if (kn != 0) radio_send_command();                                             //  Отправить радио сигнал изменения уровня громкости;
	}
koeff_h = 7.759*4;
mode1 = 0;             // в/дел
mode = 3;              // Время развертки  
tmode = 5;             // Уровень триггера порога
myGLCD.setFont( BigFont);
while (myTouch.dataAvailable()){}
}
void synhro_by_radio()                                     // просмотр в реальном времени. Синхронизация по радиоимпульсу
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	buttons_right();                                      // Нарисовать кнопки справа экрана;
	buttons_channelNew();                                 // Нарисовать кнопки внизу экрана;
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setFont(BigFont);
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info29, 5, 190);                    // "Stop->PUSH Disp"
	mode = 3;                                            // Время развертки  
	count_ms = 0;                                        // Шкалу отсчета мс в начальное состояние
	unsigned long StartTime = 0;                          // Ограничение времени измерения сигнала (2 секунды )
	long time_temp = 0;                                  // Для правильного отображения результатов измерения
	set_volume(5, 1);                                    // Отобразить уровень громкости  каналов усилителя на Базе
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawCircle(200, 10, 10);

	Timer5.start(set_timeSynhro);                        //  

	while (1)                                            //                
	{
		if (start_enable)                                           // Время начала синхроимпульса пришло. 
		{
			start_enable = false;                                   // Выставляем флаг старта в исходное для ожидания следующего синхро импульса
			trig_sin = false;                                       // Признак триггера в исходное состояние   
			StartSample = micros();                                 // Записать время
			radio_transfer();                                       // Отправить радио синхро сигнал
			// Все нормально. Измеряем !!
			// Проверка кнопок меню
			//if (myTouch.dataAvailable())
			//{
			//	delay(10);
			//	myTouch.read();
			//	x_osc = myTouch.getX();
			//	y_osc = myTouch.getY();

			//	if ((x_osc >= 2) && (x_osc <= 240))               // Выход из программы. Нажать на область экрана
			//	{
			//		if ((y_osc >= 1) && (y_osc <= 160))           // 
			//		{
			//			Timer7.stop();                            //  Остановить вывода на экран временных меток
			//			return;
			//		}                                             //
			//	}

			//	myGLCD.setBackColor(0, 0, 255);                   // Синий фон кнопок
			//	myGLCD.setColor(255, 255, 255);
			//	myGLCD.setFont(SmallFont);

			//	if ((x_osc >= 245) && (x_osc <= 280))               // Боковые кнопки
			//	{
			//		if ((y_osc >= 1) && (y_osc <= 40))              // Первая  период
			//		{
			//			waitForIt(245, 1, 318, 40);
			//			//chench_mode(0);                             //
			//		}

			//		if ((y_osc >= 45) && (y_osc <= 85))             // Вторая - триггер
			//		{
			//			waitForIt(245, 45, 318, 85);
			//			chench_tmode(0);
			//		}
			//		if ((y_osc >= 90) && (y_osc <= 130))           // Третья - делитель
			//		{
			//			waitForIt(245, 90, 318, 130);
			//			chench_mode1(0);
			//		}
			//	}

			//	if ((x_osc >= 282) && (x_osc <= 318))              // Боковые кнопки
			//	{
			//		if ((y_osc >= 1) && (y_osc <= 40))             // Первая  период
			//		{
			//			waitForIt(245, 1, 318, 40);
			//			//chench_mode(1);                            //  
			//		}

			//		if ((y_osc >= 45) && (y_osc <= 85))            // Вторая - триггер
			//		{
			//			waitForIt(245, 45, 318, 85);
			//			chench_tmode(1);
			//		}
			//		if ((y_osc >= 90) && (y_osc <= 130))          // Третья - делитель
			//		{
			//			waitForIt(245, 90, 318, 130);
			//			chench_mode1(1);
			//		}
			//	}

			//	if ((x_osc >= 245) && (x_osc <= 318))                  // Боковые кнопки
			//	{
			//		if ((y_osc >= 135) && (y_osc <= 175))              // Четвертая разрешение
			//		{
			//			waitForIt(245, 135, 318, 175);
			//			i2c_eeprom_ulong_write(adr_set_timePeriod_RF, EndSample - StartSample);  // Записать контрольное время 
			//		}
			//	}

			//	if ((y_osc >= 205) && (y_osc <= 239))                 // Нижние кнопки переключения 
			//	{
			//		if ((x_osc >= 10) && (x_osc <= 60))               //  Кнопка №1
			//		{
			//			waitForIt(10, 210, 60, 239);
			//			set_volume(1, 1);                             // Регулировка "+" усилителя №1

			//		}
			//		if ((x_osc >= 70) && (x_osc <= 120))              //  Кнопка №2
			//		{
			//			waitForIt(70, 210, 120, 239);
			//			set_volume(1, 2);                            // Регулировка "-" усилителя №1

			//		}
			//		if ((x_osc >= 130) && (x_osc <= 180))            //  Кнопка №3
			//		{
			//			waitForIt(130, 210, 180, 239);
			//			set_volume(2, 1);                            // Регулировка "+" усилителя №2

			//		}
			//		if ((x_osc >= 190) && (x_osc <= 240))            //  Кнопка №4
			//		{
			//			waitForIt(190, 210, 240, 239);
			//			set_volume(2, 2);                            // Регулировка "-" усилителя №1

			//		}
			//	}
			//}
			// Закончили проверку кнопок  меню
		//}

			Timer7.start(scale_strob * 1000);                                        // Включить формирование  временных меток на экране
			page = 0;
			StartTime = micros();
			//  Программа чтения данных АЦП. Записать аналоговый сигнал в блок памяти
			ADC_CHER = 0;                                                            // Включить аналоговый вход "0"          (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3
			while (!trig_sin)                                                        // Ожидание времени начала синхроимпульса.  Сигнал формируется таймером Timer5
			{
				if (micros() - StartTime > 2000000)                                    // Ожидаем синхроимпульс в течении 1 секунд
				{
					break;                                                               // Завершить ожидание синхроимпульса
				}
				for (xpos = 0; xpos < 240; xpos++)                                   // Блоки по 240 байтов
				{
					ADC_CR = ADC_START; 	                                         // Запустить преобразование
					while (!(ADC_ISR_DRDY));                                         // Ожидание завершения преобразования
					Sample_osc[xpos][0] = ADC->ADC_CDR[7];                        // Получить данные А0 и записать в массив
					Synhro_osc[xpos][0][new_info] = 0;                                      // Стереть старые данные временных меток
					if ((Sample_osc[xpos][page] > Trigger) && (trig_sin == false) && (xpos > 2))   // Поиск превышения уровня порога
					{
						EndSample = micros();                                        // Записать время срабатывания триггера порога
						trig_sin = true;                                             // установить флаг срабатывания триггера порога
						page_trig = page;                                            // Записываем номер блока в котором сработал триггер
					}
					delayMicroseconds(dTime);                                        // dTime временной интервал (задержка) развертки 
				}
				if (trig_sin == true) break;                                       // Завершить сканирование на блоке в котором сработал порог
			}
			Timer7.stop();                                                           // Завершить формирование временных меток

			count_ms = 0;                                                            // Шкалу отсчета мс в начальное состояние
			myGLCD.printNumI(page, 255, 177);                                        // Вывести на экран общее количество страниц
			myGLCD.printNumI(page_trig, 300, 177);                                   // Вывести на экран страницу срабатывания триггера порога
																					 // Выводит информацию об измерении
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(0, 0, 240, 176);                                    // Очистить область экрана для вывода временных меток. 
			DrawGrid1();
			myGLCD.setBackColor(0, 0, 255);                                          // Фон экрана синий
			myGLCD.setColor(255, 255, 255);                                          // Цвет текста белый
			myGLCD.print("        ", 255, 160);                                      // Очистить область вывода информации 
			myGLCD.printNumI(set_timePeriod_RF, 255, 160);
			myGLCD.print("        ", 255, 150);
			time_temp = EndSample - StartSample;
			myGLCD.printNumI(time_temp / 1000, 255, 150);                            // Время получения ответа
			myGLCD.setBackColor(0, 0, 0);                                            // Фон экрана черный
			myGLCD.setFont(BigFont);
			myGLCD.print("\x85""a""\x99""ep""\x9B\x9F""a", LEFT + 2, 8);             // "Задержка"
			myGLCD.print("c""\x9D\x98\xA2""a""\xA0""a", LEFT + 2, 25);               // "сигнала"
			myGLCD.print("      ", LEFT, 44);

			myGLCD.setFont(SmallFont);
			myGLCD.setBackColor(0, 0, 255);
			if (trig_sin)
			{
				myGLCD.setColor(255, 0, 0);
				myGLCD.drawRoundRect(245, 135, 318, 175);
				myGLCD.setBackColor(0, 0, 255);
				myGLCD.setColor(255, 255, 255);
				myGLCD.print("        ", 255, 140);

				time_temp = EndSample - StartSample - set_timePeriod_RF;
				myGLCD.printNumF(time_temp / 1000.00, 2, 255, 140);  // вывод контрольной информации
				myGLCD.setFont(BigFont);
				myGLCD.setBackColor(0, 0, 0);
				myGLCD.printNumF(time_temp / 1000.00, 2, 5, 44); // вывод основной информации
				myGLCD.print(" ms", 88, 44);
				myGLCD.setFont(SmallFont);
				myGLCD.setBackColor(0, 0, 255);
				myGLCD.setColor(VGA_RED);                                                       // Вывести индикатор срабатывания триггера порога
				myGLCD.fillCircle(227, 10, 10);                                                 // Вывести индикатор срабатывания триггера порога
			}
			else
			{
				myGLCD.setColor(255, 255, 255);
				myGLCD.drawRoundRect(245, 135, 318, 175);
				myGLCD.setBackColor(0, 0, 255);
				myGLCD.setColor(255, 255, 255);
				myGLCD.print("        ", 255, 140);
				myGLCD.printNumI(0, 255, 140);
				myGLCD.setFont(BigFont);
				myGLCD.setBackColor(0, 0, 0);
				myGLCD.printNumI(0, 5, 44);
				myGLCD.print(" ms", 88, 44);
				myGLCD.setFont(SmallFont);
				myGLCD.setBackColor(0, 0, 255);
				myGLCD.setColor(0, 0, 0);
				myGLCD.fillCircle(227, 10, 10);
			}
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.setColor(255, 255, 255);
			myGLCD.print("     ", 250, 212);
			myGLCD.printNumI(i_trig_syn, 250, 212);
			myGLCD.print("     ", 250, 224);
			myGLCD.printNumI(Trigger, 250, 224);
			myGLCD.drawCircle(227, 10, 10);

			myGLCD.setBackColor(0, 0, 0);
			set_timePeriod_RF = i2c_eeprom_ulong_read(adr_set_timePeriod_RF);
			timePeriod = i2c_eeprom_ulong_read(adr_timePeriod);
			myGLCD.printNumF(set_timeSynhro / 1000000.00, 2, 250, 200, ',');
			myGLCD.print("sec", 285, 200);
			int b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);                // Отобразить  уровень громкости 1 канала усилителя Трансмиттера
			myGLCD.print("     ", 250, 188);                                           // Отобразить  уровень громкости 1 канала усилителя Трансмиттера
			myGLCD.print(proc_volume[b], 250, 188);                                    // Отобразить  уровень громкости 1 канала усилителя Трансмиттера

			ypos_trig = 255 - (Trigger / koeff_h) - hpos;                              // Получить уровень порога
			if (ypos_trig != ypos_trig_old)                                            // Стереть старый уровень порога при изменении
			{
				myGLCD.setColor(0, 0, 0);
				myGLCD.drawLine(1, ypos_trig_old, 240, ypos_trig_old);
				ypos_trig_old = ypos_trig;
			}
			myGLCD.setColor(255, 0, 0);
			myGLCD.drawLine(1, ypos_trig, 240, ypos_trig);                             // Нарисовать новую линию уровня порога
			myGLCD.setColor(0, 0, 0);
			myGLCD.fillRoundRect(0, 162, 242, 176);                                    // Очистить область экрана вывода меток
			for (int xpos = 0; xpos < 239; xpos++)                                     // вывод на экран
			{
				// Нарисовать линию  осциллограммы
				myGLCD.setColor(255, 255, 255);
				ypos_osc1_0 = 255 - (Sample_osc[xpos][page_trig] / koeff_h) - hpos;
				ypos_osc2_0 = 255 - (Sample_osc[xpos + 1][page_trig] / koeff_h) - hpos;
				if (ypos_osc1_0 < 0) ypos_osc1_0 = 0;
				if (ypos_osc2_0 < 0) ypos_osc2_0 = 0;
				if (ypos_osc1_0 > 220) ypos_osc1_0 = 220;
				if (ypos_osc2_0 > 220) ypos_osc2_0 = 220;
				myGLCD.drawLine(xpos, ypos_osc1_0, xpos + 1, ypos_osc2_0);

				if (Synhro_osc[xpos][page_trig][new_info] > 0)
				{
					myGLCD.setColor(VGA_YELLOW);
					if (Synhro_osc[xpos][page_trig][new_info] == 4095)
					{
						myGLCD.drawLine(xpos, 60, xpos, 160);
					}
					else
					{
						myGLCD.setColor(255, 255, 255);
						if (xpos > 230)
						{
							myGLCD.printNumI(Synhro_osc[xpos][page_trig][new_info], xpos - 12, 165);
						}
						else
						{
							myGLCD.printNumI(Synhro_osc[xpos][page_trig][new_info], xpos, 165);

						}
					}
					myGLCD.setColor(255, 255, 255);
					Synhro_osc[xpos][page_trig][new_info] = 0;
				}
				myGLCD.setColor(0, 0, 0);
				myGLCD.fillCircle(200, 10, 10);
				myGLCD.setColor(255, 255, 255);
				myGLCD.drawCircle(200, 10, 10);
			}

			myGLCD.setColor(0, 0, 0);
			myGLCD.fillCircle(200, 10, 10);
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawCircle(200, 10, 10);
		}

		// Проверка кнопок меню
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_osc = myTouch.getX();
			y_osc = myTouch.getY();

			if ((x_osc >= 2) && (x_osc <= 240))               // Выход из программы. Нажать на область экрана
			{
				if ((y_osc >= 1) && (y_osc <= 160))           // 
				{
					Timer7.stop();                            //  Остановить вывода на экран временных меток
					break;                                    //
				}                                             //
			}

			myGLCD.setBackColor(0, 0, 255);                   // Синий фон кнопок
			myGLCD.setColor(255, 255, 255);
			myGLCD.setFont(SmallFont);

			if ((x_osc >= 245) && (x_osc <= 280))               // Боковые кнопки
			{
				if ((y_osc >= 1) && (y_osc <= 40))              // Первая  период
				{
					waitForIt(245, 1, 318, 40);
					//chench_mode(0);                             //
				}

				if ((y_osc >= 45) && (y_osc <= 85))             // Вторая - триггер
				{
					waitForIt(245, 45, 318, 85);
					chench_tmode(0);
				}
				if ((y_osc >= 90) && (y_osc <= 130))           // Третья - делитель
				{
					waitForIt(245, 90, 318, 130);
					chench_mode1(0);
				}
			}

			if ((x_osc >= 282) && (x_osc <= 318))              // Боковые кнопки
			{
				if ((y_osc >= 1) && (y_osc <= 40))             // Первая  период
				{
					waitForIt(245, 1, 318, 40);
					//chench_mode(1);                            //  
				}

				if ((y_osc >= 45) && (y_osc <= 85))            // Вторая - триггер
				{
					waitForIt(245, 45, 318, 85);
					chench_tmode(1);
				}
				if ((y_osc >= 90) && (y_osc <= 130))          // Третья - делитель
				{
					waitForIt(245, 90, 318, 130);
					chench_mode1(1);
				}
			}

			if ((x_osc >= 245) && (x_osc <= 318))                  // Боковые кнопки
			{
				if ((y_osc >= 135) && (y_osc <= 175))              // Четвертая разрешение
				{
					waitForIt(245, 135, 318, 175);
					i2c_eeprom_ulong_write(adr_set_timePeriod_RF, EndSample - StartSample);  // Записать контрольное время 
				}
			}

			if ((y_osc >= 205) && (y_osc <= 239))                 // Нижние кнопки переключения 
			{
				if ((x_osc >= 10) && (x_osc <= 60))               //  Кнопка №1
				{
					waitForIt(10, 210, 60, 239);
					set_volume(1, 1);                             // Регулировка "+" усилителя №1

				}
				if ((x_osc >= 70) && (x_osc <= 120))              //  Кнопка №2
				{
					waitForIt(70, 210, 120, 239);
					set_volume(1, 2);                            // Регулировка "-" усилителя №1
				}
				if ((x_osc >= 130) && (x_osc <= 180))            //  Кнопка №3
				{
					waitForIt(130, 210, 180, 239);
					set_volume(2, 1);                            // Регулировка "+" усилителя №2
				}
				if ((x_osc >= 190) && (x_osc <= 240))            //  Кнопка №4
				{
					waitForIt(190, 210, 240, 239);
					set_volume(2, 2);                            // Регулировка "-" усилителя №1
				}
			}

		}
		// Закончили проверку кнопок меню

		attachInterrupt(kn_red, volume_up, FALLING);
		attachInterrupt(kn_blue, volume_down, FALLING);
		data_out[2] = 6;                                               //
		if (kn != 0) radio_send_command();                             //  Отправить радио сигнал изменения уровня громкости;
		
	}
	koeff_h = 7.759 * 4;
	mode1 = 0;                                                         // в/дел
	mode = 3;                                                          // Время развертки  
	tmode = 5;                                                         // Уровень триггера порога
	myGLCD.setFont(BigFont);
	while (myTouch.dataAvailable()) {}
}

void oscilloscope()                                     // просмотр в реальном времени. Синхронизация по радиоимпульсу
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	buttons_right();                                      // Нарисовать кнопки справа экрана;
	buttons_channelNew();                                 // Нарисовать кнопки внизу экрана;
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setFont(BigFont);
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info29, 5, 190);                    // "Stop->PUSH Disp"
	mode = 3;                                            // Время развертки  
	count_ms = 0;                                        // Шкалу отсчета мс в начальное состояние
	set_volume(5, 1);                                    // Отобразить уровень громкости  каналов усилителя на Базе
	page = 0;                                            // Только 1 страница
	for (xpos = 0; xpos < 240; xpos++)                   // Стереть старые данные отображения и временных меток
	{
		OldSample_osc[xpos][page] = 0;                   // Стереть старые данные отображения
		Synhro_osc[xpos][page][new_info] = 0;            // Стереть новые данные временных меток
		Synhro_osc[xpos][page][old_info] = 0;            // Стереть старые данные временных меток
	}
	while (1)                                            //                
	{
			Timer7.start(scale_strob * 1000);                       // Включить формирование  временных меток на экране

																	// Программа чтения данных АЦП. Записать аналоговый сигнал в блок памяти
			ADC_CHER = 0;                                           // Включить аналоговый вход "0"          (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3
				for (xpos = 0; xpos < 240; xpos++)                                  // Блоки по 240 байтов
				{
					ADC_CR = ADC_START; 	                                        // Запустить преобразование
					while (!(ADC_ISR_DRDY));                                        // Ожидание завершения преобразования
					Sample_osc[xpos][page] = ADC->ADC_CDR[7];                       // Получить данные А0 и записать в массив
					Synhro_osc[xpos][page][new_info] = 0;                                     // Стереть старые данные временных меток
					delayMicroseconds(dTime);                                       // dTime временной интервал (задержка) развертки 
				}
			Timer7.stop();                                                          // Завершить формирование временных меток
			count_ms = 0;                                                           // Шкалу отсчета мс в начальное состояние


			ypos_trig = 255 - (Trigger / koeff_h) - hpos;                           // Получить уровень порога
			if (ypos_trig != ypos_trig_old)                                         // Стереть старый уровень порога при изменении
			{
				myGLCD.setColor(0, 0, 0);
				myGLCD.drawLine(1, ypos_trig_old, 240, ypos_trig_old);
				ypos_trig_old = ypos_trig;
			}
			myGLCD.setColor(255, 0, 0);
			myGLCD.drawLine(1, ypos_trig, 240, ypos_trig);                           // Нарисовать новую линию уровня порога


			for (int xpos = 0; xpos < 239; xpos++)                                   // вывод на экран
			{
				// Стереть предыдущий экран

				myGLCD.setColor(0, 0, 0);
				myGLCD.setBackColor(0, 0, 0);
				ypos_osc1_0 = 255 - (OldSample_osc[xpos][page] / koeff_h) - hpos;
				ypos_osc2_0 = 255 - (OldSample_osc[xpos + 1][page] / koeff_h) - hpos;
				if (ypos_osc1_0 < 0) ypos_osc1_0 = 0;
				if (ypos_osc2_0 < 0) ypos_osc2_0 = 0;
				if (ypos_osc1_0 > 220) ypos_osc1_0 = 220;
				if (ypos_osc2_0 > 220) ypos_osc2_0 = 220;
				myGLCD.drawLine(xpos, ypos_osc1_0, xpos + 1, ypos_osc2_0);
				myGLCD.drawLine(xpos+1, ypos_osc1_0, xpos + 2, ypos_osc2_0);

				if (Synhro_osc[xpos][page][old_info] > 0)
				{
					if (Synhro_osc[xpos][page][old_info] != Synhro_osc[xpos][page][new_info])
					{
						if (Synhro_osc[xpos][page][old_info] == 4095)
						{
							myGLCD.drawLine(xpos, 60, xpos, 160);
						}
						else
						{
							if (xpos > 230)
							{
								myGLCD.print("  ", xpos, 165);                    // Стереть временную метку
							}
							else
							{
								myGLCD.print("  ", xpos, 165);                    // Стереть временную метку

							}
						}
					}
				}

				// Нарисовать линию  осциллограммы
				myGLCD.setColor(255, 255, 255);
				ypos_osc1_0 = 255 - (Sample_osc[xpos][page] / koeff_h) - hpos;
				ypos_osc2_0 = 255 - (Sample_osc[xpos + 1][page] / koeff_h) - hpos;
				if (ypos_osc1_0 < 0) ypos_osc1_0 = 0;
				if (ypos_osc2_0 < 0) ypos_osc2_0 = 0;
				if (ypos_osc1_0 > 220) ypos_osc1_0 = 220;
				if (ypos_osc2_0 > 220) ypos_osc2_0 = 220;
				myGLCD.drawLine(xpos, ypos_osc1_0, xpos + 1, ypos_osc2_0);

				if (Synhro_osc[xpos][page][new_info] > 0)
				{
					myGLCD.setColor(VGA_YELLOW);
					if (Synhro_osc[xpos][page][new_info] == 4095)
					{
						myGLCD.drawLine(xpos, 60, xpos, 160);
					}
					else
					{
						if (Synhro_osc[xpos][page][old_info] != Synhro_osc[xpos][page][new_info])
						{
							myGLCD.setColor(255, 255, 255);
							if (xpos > 230)
							{
								myGLCD.printNumI(Synhro_osc[xpos][page][new_info], xpos - 12, 165);
							}
							else
							{
								myGLCD.printNumI(Synhro_osc[xpos][page][new_info], xpos, 165);

							}
						}
			 		}
					myGLCD.setColor(255, 255, 255);
				}
				OldSample_osc[xpos][page] = Sample_osc[xpos][page];
				Synhro_osc[xpos][page][old_info] = Synhro_osc[xpos][page][new_info];
			}
			DrawGrid1();
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_osc = myTouch.getX();
			y_osc = myTouch.getY();

			if ((x_osc >= 2) && (x_osc <= 240))               // Выход из программы. Нажать на область экрана
			{
				if ((y_osc >= 1) && (y_osc <= 160))           // 
				{
					Timer7.stop();                            //  Остановить вывода на экран временных меток
					break;                                    //
				}                                             //
			}                                                 //
			myGLCD.setBackColor(0, 0, 255);                   // Синий фон кнопок
			myGLCD.setColor(255, 255, 255);
			myGLCD.setFont(SmallFont);

			if ((x_osc >= 245) && (x_osc <= 280))               // Боковые кнопки
			{
				if ((y_osc >= 1) && (y_osc <= 40))              // Первая  период
				{
					waitForIt(245, 1, 318, 40);
					chench_mode(0);                             //
				}

				if ((y_osc >= 45) && (y_osc <= 85))             // Вторая - триггер
				{
					waitForIt(245, 45, 318, 85);
					chench_tmode(0);
				}
				if ((y_osc >= 90) && (y_osc <= 130))           // Третья - делитель
				{
					waitForIt(245, 90, 318, 130);
					chench_mode1(0);
				}
			}

			if ((x_osc >= 282) && (x_osc <= 318))              // Боковые кнопки
			{
				if ((y_osc >= 1) && (y_osc <= 40))             // Первая  период
				{
					waitForIt(245, 1, 318, 40);
					chench_mode(1);                            //  
				}

				if ((y_osc >= 45) && (y_osc <= 85))            // Вторая - триггер
				{
					waitForIt(245, 45, 318, 85);
					chench_tmode(1);
				}
				if ((y_osc >= 90) && (y_osc <= 130))          // Третья - делитель
				{
					waitForIt(245, 90, 318, 130);
					chench_mode1(1);
				}
			}

			if ((x_osc >= 245) && (x_osc <= 318))                  // Боковые кнопки
			{
				if ((y_osc >= 135) && (y_osc <= 175))              // Четвертая разрешение
				{
					waitForIt(245, 135, 318, 175);
				}
			}

			if ((y_osc >= 205) && (y_osc <= 239))                 // Нижние кнопки переключения 
			{
				if ((x_osc >= 10) && (x_osc <= 60))               //  Кнопка №1
				{
					waitForIt(10, 210, 60, 239);
					set_volume(1, 1);                             // Регулировка "+" усилителя №1

				}
				if ((x_osc >= 70) && (x_osc <= 120))              //  Кнопка №2
				{
					waitForIt(70, 210, 120, 239);
					set_volume(1, 2);                            // Регулировка "-" усилителя №1

				}
				if ((x_osc >= 130) && (x_osc <= 180))            //  Кнопка №3
				{
					waitForIt(130, 210, 180, 239);
					set_volume(2, 1);                            // Регулировка "+" усилителя №2

				}
				if ((x_osc >= 190) && (x_osc <= 240))            //  Кнопка №4
				{
					waitForIt(190, 210, 240, 239);
					set_volume(2, 2);                            // Регулировка "-" усилителя №1

				}
			}
		}
	}

	koeff_h = 7.759 * 4;
	mode1 = 0;                                                   // в/дел
	mode = 3;                                                    // Время развертки  
	tmode = 5;                                                   // Уровень триггера порога
	myGLCD.setFont(BigFont);
	while (myTouch.dataAvailable()) {}
}

void chench_mode(bool mod)   // Переключение развертки
{
	if (mod)
	{
		mode++;
	}
	else
	{
		mode--;
	}
	if (mode < 0) mode = 0;
	if (mode > 9) mode = 9;
	// Select delay times you can change values to suite your needs
	if (mode == 0) { dTime = 1;    x_dTime = 278; scale_strob = 1;}
	if (mode == 1) { dTime = 10;   x_dTime = 274; scale_strob = 1;}   
	if (mode == 2) { dTime = 25;   x_dTime = 274; scale_strob = 2;}
	if (mode == 3) { dTime = 50;   x_dTime = 274; scale_strob = 5;}
	if (mode == 4) { dTime = 100;  x_dTime = 272; scale_strob = 10;}
	if (mode == 5) { dTime = 200;  x_dTime = 272; scale_strob = 20;}
	if (mode == 6) { dTime = 300;  x_dTime = 272; scale_strob = 30;}
	if (mode == 7) { dTime = 500;  x_dTime = 272; scale_strob = 40;}
	if (mode == 8) { dTime = 1000; x_dTime = 267; scale_strob = 50;}
	if (mode == 9) { dTime = 5000; x_dTime = 267; scale_strob = 100;}
	myGLCD.print("    ", 262, 22);
	myGLCD.printNumI(dTime, x_dTime, 22);
}
void chench_tmode(bool mod)  // Уровень порога
{
	if (mod)
	{
		tmode++;
	}
	else
	{
		tmode--;
	}
	if (tmode < 0)tmode = 0;
	if (tmode > 9)tmode = 9;

	if (tmode == 1) { Trigger = 512; myGLCD.print(" 10% ", 264, 65);}
	if (tmode == 2) { Trigger = 768;  myGLCD.print(" 18% ", 264, 65); }
	if (tmode == 3) { Trigger = 1024;  myGLCD.print(" 25% ", 264, 65); }
	if (tmode == 4) { Trigger = 1536;  myGLCD.print(" 38% ", 264, 65); }
	if (tmode == 5) { Trigger = 2047;  myGLCD.print(" 50% ", 264, 65); }
	if (tmode == 6) { Trigger = 2448;  myGLCD.print(" 60% ", 264, 65); }
	if (tmode == 7) { Trigger = 2856;  myGLCD.print(" 70% ", 264, 65); }
	if (tmode == 8) { Trigger = 3264;  myGLCD.print(" 80% ", 264, 65); }
	if (tmode == 9) { Trigger = 4080; myGLCD.print("100%", 265, 65); }
	if (tmode == 0) { Trigger = 0; myGLCD.print(" Off ", 265, 65); }
}
void chench_mode1(bool mod)  // Разрешение экрана
{
	if (mod)
	{
		mode1++;
	}
	else
	{
		mode1--;
	}
	if (mode1 < 0) mode1 = 0;
	if (mode1 > 3) mode1 = 3;
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRoundRect(0, 0, 240, 160);
	DrawGrid();
	myGLCD.setColor(255, 255, 255);
	if (mode1 == 0) { koeff_h = 7.759 * 4; myGLCD.print("  1 ", 262, 110); }
	if (mode1 == 1) { koeff_h = 3.879 * 4; myGLCD.print(" 0.5", 262, 110); }
	if (mode1 == 2) { koeff_h = 1.939 * 4; myGLCD.print("0.25", 264, 110); }
	if (mode1 == 3) { koeff_h = 0.969 * 4; myGLCD.print(" 0.1", 262, 110); }
}

void oscilloscope_time()   // В файл не пишет 
{
	uint32_t bgnBlock, endBlock;
	block_t block[BUFFER_BLOCK_COUNT];
	myGLCD.clrScr();
	myGLCD.setBackColor( 0, 0, 0);
	delay(500);
	buttons_right_time();     // Отобразить кнопки справа
	buttons_channel();        // Отобразить кнопки переключения входов
	DrawGrid();               // Отобразить сетку и нарисовать окантовку кнопок справа и внизу
	int xpos;
	int ypos1;
	int ypos2;
	int ypos_osc1_0;

	int ypos_osc2_0;
	
	int sec_osc = 0;
	int min_osc = 0;
	bool ind_start = false;
	StartSample = 0; 
	uint32_t logTime = 0;
	uint32_t SAMPLE_INTERVAL_MS = 250;
	int32_t diff;
	count_repeat = 0;

	for( xpos = 0; xpos < 239;	xpos ++)                    // Стереть старые данные

		{
			OldSample_osc[xpos][page] = 0;
		}

	myGLCD.setColor(VGA_LIME);
	myGLCD.fillRoundRect (1, 1, 60, 35);
	myGLCD.setColor (255, 0, 0);
	myGLCD.setBackColor(VGA_LIME);
	myGLCD.setFont(BigFont);
	myGLCD.print("ESC", 6, 9);
	myGLCD.setColor (255, 255, 255);
	myGLCD.drawRoundRect (1, 1, 60, 35);
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.fillRoundRect (40, 40, 200, 120);
	myGLCD.setBackColor(VGA_YELLOW);
	myGLCD.setColor (255, 0, 0);
	myGLCD.setFont(BigFont);
	myGLCD.print("CTAPT", 80, 72);
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setColor (255, 255,255);
	myGLCD.drawRoundRect (40, 40, 200, 120);
	StartSample = millis();

	while(1) 
	 {
				logTime = millis();
				if(logTime - StartSample > 1000)  // Прерывистая индикация надписи "START"
				{
					StartSample = millis();
					ind_start = !ind_start;
					 if (ind_start)
						{
							myGLCD.setColor(VGA_YELLOW);
							myGLCD.fillRoundRect (40, 40, 200, 120);
							myGLCD.setBackColor(VGA_YELLOW);
							myGLCD.setColor (255, 0, 0);
							myGLCD.setFont(BigFont);
							myGLCD.print("CTAPT", 80, 72);
							myGLCD.setBackColor( 0, 0, 255);
							myGLCD.setColor (255, 255,255);
							myGLCD.drawRoundRect (40, 40, 200, 120);
						}
					else
						{
							myGLCD.setColor(VGA_YELLOW);
							myGLCD.fillRoundRect (40, 40, 200, 120);
							myGLCD.setBackColor(VGA_YELLOW);
							myGLCD.setFont( BigFont);
							myGLCD.print("     ", 80, 72);
							myGLCD.setBackColor( 0, 0, 255);
							myGLCD.setColor (255, 255,255);
							myGLCD.drawRoundRect (40, 40, 200, 120);
						}
				}

		 strob_start = digitalRead(strob_pin);                      // Проверка входа внешнего запуска 
		  //if (!strob_start) 
			 //{
				// myGLCD.setColor(VGA_RED);
				// myGLCD.fillCircle(230,10,20);
			 //}
		 if (!strob_start) break;

		 if (myTouch.dataAvailable())
			{
				delay(10);
				myTouch.read();
				x_osc=myTouch.getX();
				y_osc=myTouch.getY();
				myGLCD.setBackColor( 0, 0, 255);
				myGLCD.setColor (255, 255,255);
				myGLCD.setFont( SmallFont);

				if ((x_osc>=1) && (x_osc<=60))                         //  Выход 
					{
						if ((y_osc>=1) && (y_osc<=35))                 // Delay row
						{
							waitForIt(1, 1, 60, 35);
							return;
						} 
					}

				if ((x_osc>=40) && (x_osc<=200))                       //  Выход из ожидания, Старт измерения
					{
						if ((y_osc>=40) && (y_osc<=120))               // Delay row
						{
							waitForIt(40, 40, 200, 120);
							break;
						} 
					}

			if ((x_osc>=250) && (x_osc<=284))                           // Боковые кнопки
			  {
				 
				  if ((y_osc>=1) && (y_osc<=40))  // Первая  период -
					  {
						waitForIt(250, 1, 318, 40);
						mode -- ;
						if (mode < 0) mode = 0;   
						if (mode == 0) {SAMPLE_INTERVAL_MS = 250;}
						if (mode == 1) {SAMPLE_INTERVAL_MS = 1500;}
						if (mode == 2) {SAMPLE_INTERVAL_MS = 3000;}
						if (mode == 3) {SAMPLE_INTERVAL_MS = 4500;}
						scale_time();
					  }

				 if ((y_osc>=45) && (y_osc<=85))                          // Вторая - усреднение показаний
					 {
						waitForIt(250, 45, 318, 85);
						if(Set_x == true) 
							{
								 Set_x = false;
								 myGLCD.print("     ", 265, 65);
							}
							else
							{
								Set_x = true;
								myGLCD.print(" /x  ", 265, 65);
							}
					 }
				 if ((y_osc>=90) && (y_osc<=130))                           // Третья - делитель
					 {
						waitForIt(250, 90, 318, 130);
						mode1 -- ;
						myGLCD.setColor( 0, 0, 0);
						myGLCD.fillRoundRect (1, 1,239, 159);
						myGLCD.setColor(VGA_LIME);
						myGLCD.fillRoundRect (1, 1, 60, 35);
						myGLCD.setColor (255, 0, 0);
						myGLCD.setBackColor(VGA_LIME);
						myGLCD.setFont(BigFont);
						myGLCD.print("ESC", 6, 9);
						myGLCD.setColor (255, 255, 255);
						myGLCD.setBackColor( 0, 0, 255);
						myGLCD.setFont( SmallFont);
						if (mode1 < 0) mode1 = 0;   
						if (mode1 == 0){ koeff_h = 7.759*4; myGLCD.print(" 1  ", 275, 110);}
						if (mode1 == 1){ koeff_h = 3.879*4; myGLCD.print("0.5 ", 275, 110);}
						if (mode1 == 2){ koeff_h = 1.939*4; myGLCD.print("0.25", 275, 110);}
						if (mode1 == 3){ koeff_h = 0.969*4; myGLCD.print("0.1 ", 275, 110);}
					DrawGrid();

					 }
		
		   }
				
			
			if ((x_osc>=284) && (x_osc<=318))  // Боковые кнопки
			  {
				  if ((y_osc>=1) && (y_osc<=40))  // Первая  период  +
				  {
					waitForIt(250, 1, 318, 40);
					mode ++ ;
					if (mode > 3) mode = 3;   
					if (mode == 0) {SAMPLE_INTERVAL_MS = 250;}
					if (mode == 1) {SAMPLE_INTERVAL_MS = 1500;}
					if (mode == 2) {SAMPLE_INTERVAL_MS = 3000;}
					if (mode == 3) {SAMPLE_INTERVAL_MS = 4500;}
					scale_time();
				  }

			 if ((y_osc>=45) && (y_osc<=85))  // Вторая 
				 {
					waitForIt(250, 45, 318, 85);
					  if(Set_x == true) 
						{
							 Set_x = false;
							 myGLCD.print("     ", 265, 65);
						}
					  else
						{
							Set_x = true;
							myGLCD.print(" /x  ", 265, 65);
						}
				 }
			 if ((y_osc>=90) && (y_osc<=130))  // Третья - делитель
				 {
					waitForIt(250, 90, 318, 130);
					mode1 ++ ;
					myGLCD.setColor( 0, 0, 0);
					myGLCD.fillRoundRect (1, 1,239, 159);
					myGLCD.setColor(VGA_LIME);
					myGLCD.fillRoundRect (1, 1, 60, 35);
					myGLCD.setColor (255, 0, 0);
					myGLCD.setBackColor(VGA_LIME);
					myGLCD.setFont(BigFont);
					myGLCD.print("ESC", 6, 9);
					myGLCD.setColor (255, 255, 255);
					myGLCD.setBackColor( 0, 0, 255);
					myGLCD.setFont( SmallFont);
					if (mode1 > 3) mode1 = 3;   
					if (mode1 == 0){ koeff_h = 7.759*4; myGLCD.print(" 1  ", 275, 110);}
					if (mode1 == 1){ koeff_h = 3.879*4; myGLCD.print("0.5 ", 275, 110);}
					if (mode1 == 2){ koeff_h = 1.939*4; myGLCD.print("0.25", 275, 110);}
					if (mode1 == 3){ koeff_h = 0.969*4; myGLCD.print("0.1 ", 275, 110);}
					DrawGrid();
				 }
		
		   }

		if ((x_osc>=250) && (x_osc<=318))  

			{

				 if ((y_osc>=135) && (y_osc<=175))  // Четвертая разрешение
				 {
					waitForIt(250, 135, 318, 175);
					sled = !sled;
					myGLCD.setBackColor( 0, 0, 255);
					myGLCD.setColor (255, 255,255);
					if (sled == true) myGLCD.print("  B\x9F\xA0 ", 257, 155);
					if (sled == false) myGLCD.print("O\xA4\x9F\xA0", 270, 155);
				 }


			if ((y_osc>=200) && (y_osc<=239))  //   Нижние кнопки  
				{
					waitForIt(250, 200, 318, 238);
					repeat = !repeat;
					myGLCD.setBackColor( 0, 0, 255);
					myGLCD.setColor (255, 255,255);
					if (repeat == true & count_repeat == 0)
						{
							myGLCD.print("  B\x9F\xA0 ", 257, 220);
						}
					if (repeat == true & count_repeat > 0)
						{
							if (repeat == true) myGLCD.print("       ", 257, 220);
							if (repeat == true) myGLCD.printNumI(count_repeat, 270, 220);
						}
					if (repeat == false) myGLCD.print("O\xA4\x9F\xA0", 270, 220);
				}
		  }

			 if ((y_osc>=205) && (y_osc<=239))                             // Нижние кнопки переключения входов
					{
						 touch_osc();
					}
		}
	}

	myGLCD.setColor(0, 0, 0);
	//myGLCD.fillRoundRect (40, 40, 200, 120);
	//						myGLCD.setColor( 0, 0, 0);
	myGLCD.fillRoundRect (1, 1,239, 159);
	DrawGrid1();

	// +++++++++++++++++   Начало измерений ++++++++++++++++++++++++

	myGLCD.setBackColor( 0, 0, 0);
	myGLCD.setColor(255,255,255);
	myGLCD.drawCircle(230, 10,20);
	myGLCD.setFont( BigFont);
	myGLCD.print("     ", 80, 72);
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info29,LEFT, 180);
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setFont( SmallFont);
	DrawGrid1();
	logTime = micros();
	count_repeat = 0;
		// Записать аналоговый сигнал в блок памяти
			StartSample = micros();
			ADC_CHER = Channel_x;    // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3    
	 do
	  {
		 if (sled == false)                       // Отображение следа предыдущего измерения.
			{
					myGLCD.clrScr();
					buttons_right_time();
					buttons_channel();
					myGLCD.setBackColor( 0, 0, 255);
					DrawGrid();
					myGLCD.setBackColor( 0, 0, 0);
					myGLCD.setFont( BigFont);
					myGLCD.setColor(VGA_LIME);
					myGLCD.print(txt_info29,LEFT, 180);

			}
		if (sled == true & count_repeat > 0)
			{
					myGLCD.clrScr();
					buttons_right_time();
					buttons_channel();
					myGLCD.setBackColor( 0, 0, 255);
					DrawGrid();
					myGLCD.setBackColor( 0, 0, 0);
					myGLCD.setFont( BigFont);
					myGLCD.setColor(VGA_LIME);
					myGLCD.print(txt_info29,LEFT, 180);

			  for( int xpos = 0; xpos < 239;	xpos ++)
					{
						if (Channel0)
							{
								if (xpos == 0)					// определить начальную позицию по Х 
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos][page] /koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][page] /koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+1);
										if (ypos_osc1_0 < 150)
											{
												myGLCD.print("0", 2, ypos_osc1_0+1);
											}
										else
											{
												myGLCD.print("0", 2, ypos_osc1_0-10);
											}
									}
								else
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos - 1][page] /koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][page] /koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+1);
									}
							}
					
					}
			}


		for( xpos = 0;	xpos < 240; xpos ++) 
			{
				if (!strob_start)
					{
						strob_start = digitalRead(strob_pin);                      // Проверка входа внешнего запуска 
							if (strob_start)
							{
								repeat = false;
									if (!strob_start) 
										{
											myGLCD.setColor(VGA_RED);
											myGLCD.fillCircle(227, 10,10);
										}
									else
										{
											myGLCD.setColor(255,255,255);
											myGLCD.drawCircle(227, 10,10);
										}
								break;
							}
					}


			 if (myTouch.dataAvailable())
				{
					delay(10);
					myTouch.read();
					x_osc=myTouch.getX();
					y_osc=myTouch.getY();
				
					if ((x_osc>=2) && (x_osc<=240))                     //  Останов измерения
						{
							if ((y_osc>=1) && (y_osc<=160))             // Delay row
							{
								waitForIt(2, 1, 240, 160);
								myGLCD.setBackColor( 0, 0, 0);
								myGLCD.setFont( BigFont);
								myGLCD.print("               ",LEFT, 180);
								myGLCD.print("CTO\x89", 100, 180);
								repeat = false;
								break;
							} 
						}
				}

			 logTime += 1000UL*SAMPLE_INTERVAL_MS;
			 do  // Измерение одной точки
				 {

			//	ADC_CHER = Channel_x;         // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3    
				ADC_CR = ADC_START ; 	      // Запустить преобразование
				 while (!(ADC_ISR_DRDY));     // Ожидать завершение преобразования 
				if (Channel0)
					{
						MaxAnalog0 = max(MaxAnalog0, ADC->ADC_CDR[7]);
						SrednAnalog0 += MaxAnalog0;
					}
					 SrednCount++;
					 delayMicroseconds(20);                  //
					 diff = micros() - logTime;
					 EndSample = micros();
					 if(EndSample - StartSample > 1000000 )
						 {
							StartSample  =   EndSample ;
							sec_osc++;                          // Подсчет секунд
							if (sec_osc >= 60)
								{
								  sec_osc = 0;
								  min_osc++;                    // Подсчет минут
								}
							myGLCD.setColor( VGA_YELLOW);
							myGLCD.setBackColor( 0, 0, 0);
							myGLCD.setFont( BigFont);
							myGLCD.printNumI(min_osc, 250, 180);
							myGLCD.print(":", 277, 180);
							myGLCD.print("  ", 287, 180);
							myGLCD.printNumI(sec_osc,287, 180);
						 }
				 } while (diff < 0);

				  if(Set_x == true)
					 {
						MaxAnalog0 =  SrednAnalog0 / SrednCount;
						SrednAnalog0 = 0;
						SrednCount = 0;
					 }
				 if (Channel0) Sample_osc[ xpos][0] = MaxAnalog0;
					MaxAnalog0 =  0;
			

				myGLCD.setColor( 0, 0, 0);
					if (xpos == 0)
						{
							myGLCD.drawLine (xpos + 1, 1, xpos + 1, 220);
							myGLCD.drawLine (xpos + 2, 1, xpos + 2, 220);
						}
					
				if (Channel0)
					{
						if (xpos == 0)					// определить начальную позицию по Х 
							{
								myGLCD.setColor( 255, 255, 255);
								ypos_osc1_0 = 255-(Sample_osc[ xpos][0]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][0]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( 255, 255, 255);
								ypos_osc1_0 = 255-(Sample_osc[ xpos - 1][0]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][0]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}
					OldSample_osc[xpos][page] = Sample_osc[xpos][0];
	   }

		count_repeat++;
		myGLCD.setFont( SmallFont);
		myGLCD.setBackColor(0, 0, 255);
		myGLCD.setColor(255, 255, 255);
		if (repeat == true) myGLCD.print("       ", 257, 220);
		if (repeat == true) myGLCD.printNumI(count_repeat, 270, 220);

	} while (repeat);

	koeff_h = 7.759*4;
	mode1 = 0;
	Trigger = 0;
	count_repeat = 0;
	StartSample = millis();
	myGLCD.setFont( BigFont);
	while (!myTouch.dataAvailable()){}
	delay(50);
	while (myTouch.dataAvailable()){}
	delay(50);
}

void buttons_right()  //  Правые кнопки  oscilloscope
{
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect (245, 1, 318, 40);             // Заполнить поле кнопки синим цветом
	myGLCD.fillRoundRect (245, 45, 318, 85);
	myGLCD.fillRoundRect (245, 90, 318, 130);
	myGLCD.fillRoundRect (245, 135, 318, 175);

	myGLCD.setBackColor(0, 0, 255);
	myGLCD.setFont(SmallFont);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(245, 1, 318, 40);             // Нарисовать обрамление кнопок белым цветом
	myGLCD.drawRoundRect(245, 45, 318, 85);
	myGLCD.drawRoundRect(245, 90, 318, 130);
	myGLCD.drawRoundRect(245, 135, 318, 175);

	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setColor (255, 255,255);

	myGLCD.setFont(BigFont);
	myGLCD.print("-  +", 248, 20);
	myGLCD.print("-  +", 248, 63);
	myGLCD.print("-  +", 248, 108);
	myGLCD.setFont(SmallFont);

	chench_mode(0);
	chench_tmode(0);
	chench_mode1(0);

	myGLCD.print("Delay", 260, 6);
	myGLCD.print("Trig.", 265, 50);
	myGLCD.print("V/del.", 265, 95);
}
void buttons_right_time()
{
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect (245, 1, 318, 40);
	myGLCD.fillRoundRect (245, 45, 318, 85);
	myGLCD.fillRoundRect (245, 90, 318, 130);
	myGLCD.fillRoundRect (245, 135, 318, 175);
	myGLCD.fillRoundRect (245, 200, 318, 239);

	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255,255);
	myGLCD.print("C\xA0""e\x99", 270, 140);                       //
	if (sled == true) myGLCD.print("  B\x9F\xA0 ", 257, 155);     //
	if (sled == false) myGLCD.print("O\xA4\x9F\xA0", 270, 155);
	myGLCD.print(txt_info30, 260, 205);
	if (repeat == true & count_repeat == 0)
		{
			myGLCD.print("  B\x9F\xA0 ", 257, 220);
		}
	if (repeat == true & count_repeat > 0)
		{
			if (repeat == true) myGLCD.print("       ", 257, 220);
			if (repeat == true) myGLCD.printNumI(count_repeat, 270, 220);
		}
	if (repeat == false) myGLCD.print("O\xA4\x9F\xA0", 270, 220);    // 

	if(Set_x == true)
	{
	   myGLCD.print("V Max", 265, 50);
	   myGLCD.print(" /x  ", 265, 65);
	}
	else
	{
	   myGLCD.print("V Max", 265, 50);
	   myGLCD.print("     ", 265, 65);
	}

	myGLCD.print("V/del.", 260, 95);
	myGLCD.print("-     +", 260, 110);
	if (mode1 == 0){ koeff_h = 7.759*4; myGLCD.print(" 1  ", 275, 110);}
	if (mode1 == 1){ koeff_h = 3.879*4; myGLCD.print("0.5 ", 275, 110);}
	if (mode1 == 2){ koeff_h = 1.939*4; myGLCD.print("0.25", 275, 110);}
	if (mode1 == 3){ koeff_h = 0.969*4; myGLCD.print("0.1 ", 275, 110);}
	scale_time();   // вывод цифровой шкалы
}
void scale_time()
{
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255, 255);
	myGLCD.print("Delay", 264, 5);
	myGLCD.print("-      +", 254, 20);
	if (mode == 0)myGLCD.print("1min", 269, 20);
	if (mode == 1)myGLCD.print("6min", 269, 20);
	if (mode == 2)myGLCD.print("12min", 266, 20);
	if (mode == 3)myGLCD.print("18min", 266, 20);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("0",3, 163);         // В начале шкалы
	if (mode == 0)                    // Остальная сетка
		{
			myGLCD.print("10", 35, 163);
			myGLCD.print("20", 75, 163);
			myGLCD.print("30", 115, 163);
			myGLCD.print("40", 155, 163);
			myGLCD.print("50", 195, 163);
			myGLCD.print("60", 230, 163);
		}
	if (mode == 1)
		{
			myGLCD.print(" 1 ", 32, 163);
			myGLCD.print(" 2 ", 72, 163);
			myGLCD.print(" 3 ", 112, 163);
			myGLCD.print(" 4 ", 152, 163);
			myGLCD.print(" 5 ", 192, 163);
			myGLCD.print(" 6", 230, 163);
		}
	if (mode == 2)
		{
			myGLCD.print(" 2 ", 32, 163);
			myGLCD.print(" 4 ", 72, 163);
			myGLCD.print(" 6 ", 112, 163);
			myGLCD.print(" 8 ", 152, 163);
			myGLCD.print("10", 195, 163);
			myGLCD.print("12", 230, 163);
		}
	if (mode == 3)
		{
			myGLCD.print(" 3 ", 32, 163);
			myGLCD.print(" 6 ", 72, 163);
			myGLCD.print(" 9 ", 112, 163);
			myGLCD.print("12", 155, 163);
			myGLCD.print("15", 195, 163);
			myGLCD.print("18", 230, 163);
		}
}
void buttons_channelNew()                   // Нижние кнопки переключения 
{
	myGLCD.setFont( SmallFont);

	// Кнопка № 1
	myGLCD.setColor(VGA_BLACK);                // Цвет поля кнопки
	myGLCD.setBackColor(VGA_BLACK);
	myGLCD.fillRoundRect(10, 210, 60, 239);
	myGLCD.setColor(VGA_WHITE);                // Цвет шрифта
	//myGLCD.print("<-", 28, 214);
	myGLCD.print("Up1", 25, 224);
	osc_line_off0 = true;

	// Кнопка № 2
	myGLCD.setColor(VGA_BLACK);                // Цвет поля кнопки
	myGLCD.setBackColor(VGA_BLACK);
	myGLCD.fillRoundRect(70, 210, 120, 239);
	myGLCD.setColor(VGA_WHITE);                // Цвет шрифта
	//myGLCD.print("->", 88, 214);
	myGLCD.print("Down1", 78, 224);

	// Кнопка № 3
	myGLCD.setColor(VGA_BLACK);                // Цвет поля кнопки
	myGLCD.setBackColor(VGA_BLACK);
	myGLCD.fillRoundRect(130, 210, 180, 239);
	myGLCD.setColor(VGA_WHITE);                // Цвет шрифта

//	myGLCD.print("Up2", 148, 214);
	myGLCD.print("Up2", 142, 224);

	//myGLCD.print("<-", 148, 214);
	//myGLCD.print("sec0,1", 134, 224);


	// Кнопка № 4
	myGLCD.setColor(VGA_BLACK);                // Цвет поля кнопки
	myGLCD.setBackColor(VGA_BLACK);
	myGLCD.fillRoundRect(190, 210, 240, 239);
	myGLCD.setColor(VGA_WHITE);                // Цвет шрифта
	//myGLCD.print("Off", 206, 214);
	myGLCD.print("Down2", 197, 224);
	//myGLCD.print("->", 206, 214);
	//myGLCD.print("sec0,1", 193, 224);


	myGLCD.setColor(255, 255, 255);                  // Цвет окантовки кнопки
	myGLCD.drawRoundRect (10, 210, 60, 239);         // Окантовка кнопки N1
	myGLCD.drawRoundRect (70, 210, 120, 239);        // Окантовка кнопки N2
	myGLCD.drawRoundRect (130, 210, 180, 239);       // Окантовка кнопки N3
	myGLCD.drawRoundRect (190, 210, 240, 239);       // Окантовка кнопки N4
}

void buttons_set_time_synhro()                   // Нижние кнопки переключения 
{
	myGLCD.setFont(SmallFont);

	// Кнопка № 1
	myGLCD.setColor(VGA_BLUE);                // Цвет поля кнопки
    myGLCD.setBackColor(VGA_BLUE);

	myGLCD.fillRoundRect(10, 210, 60, 239);
	myGLCD.setColor(VGA_WHITE);                // Цвет шрифта
	myGLCD.print("<-", 28, 220);

	// Кнопка № 2
	myGLCD.setColor(VGA_BLUE);                // Цвет поля кнопки
	myGLCD.setBackColor(VGA_BLUE);
	myGLCD.fillRoundRect(70, 210, 120, 239);
	myGLCD.setColor(VGA_WHITE);                // Цвет шрифта
    myGLCD.print("->", 88, 220);


	// Кнопка № 3
	myGLCD.setColor(VGA_BLUE);                // Цвет поля кнопки
	myGLCD.setBackColor(VGA_BLUE);
	myGLCD.fillRoundRect(130, 210, 180, 239);
	myGLCD.setColor(VGA_WHITE);                // Цвет шрифта
	myGLCD.print("<-", 148, 220);


	// Кнопка № 4
	myGLCD.setColor(VGA_BLUE);                // Цвет поля кнопки
	myGLCD.setBackColor(VGA_BLUE);
	myGLCD.fillRoundRect(190, 210, 240, 239);
	myGLCD.setColor(VGA_WHITE);                // Цвет шрифта
	myGLCD.print("->", 206, 220);




	myGLCD.setBackColor(VGA_BLACK);
	myGLCD.setColor(255, 255, 255);                  // Цвет окантовки кнопки
	myGLCD.drawRoundRect(10, 210, 60, 239);         // Окантовка кнопки N1
	myGLCD.drawRoundRect(70, 210, 120, 239);        // Окантовка кнопки N2
	myGLCD.drawRoundRect(130, 210, 180, 239);       // Окантовка кнопки N3
	myGLCD.drawRoundRect(190, 210, 240, 239);       // Окантовка кнопки N4
}

void tuning_mod()
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	//myGLCD.setFont(BigFont);
	//myGLCD.setColor(0, 0, 255);
	//myGLCD.fillRoundRect(30, 20 + (50 * 3), 290, 60 + (50 * 3));
	//myGLCD.setColor(255, 255, 255);
	//myGLCD.drawRoundRect(30, 20 + (50 * 3), 290, 60 + (50 * 3));
	//myGLCD.setBackColor(0, 0, 255);
	//myGLCD.print(txt_tuning_menu4, CENTER, 180);
	//myGLCD.setBackColor(0, 0, 0);
	//setup_radio_ping();                           // Настроить радиоканал
	delayMicroseconds(500);
	//myGLCD.setFont(BigFont);
	radio_send_command();                              //  Отправить радио синхро сигнал
	//digitalWrite(LED_PIN13, HIGH);
	//delay(100);
	//digitalWrite(LED_PIN13, LOW);
//	delayMicroseconds(250);
	myGLCD.setFont(BigFont);
	if (tim1 >= 730 && tim1 <= 745)
	{
		myGLCD.print("Synhro START!", CENTER, 80);
		Timer5.start();
		delay(1000);
		//StartSample = micros();                  // Записать время
		//while (true)
		//{
		//	if (micros() - StartSample >= 3000000) 
		//	{
		//		start_led = !start_led;
		//		digitalWrite(LED_PIN13, HIGH);
		//		StartSample = micros();
		//	}
		//	if (micros() - StartSample >= 50000)
		//	{
		//		start_led = !start_led;
		//		digitalWrite(LED_PIN13, LOW);
		//	}
		////	delay(20);
		//	//digitalWrite(LED_PIN13, LOW);
		//	if (myTouch.dataAvailable())
		//	{
		//		myTouch.read();
		//		int	x = myTouch.getX();
		//		int	y = myTouch.getY();

		//		if ((x >= 30) && (x <= 290))       // 
		//		{

		//			if ((y >= 170) && (y <= 220))  // Button: 4 "EXIT" Выход
		//			{
		//				waitForIt(30, 170, 290, 210);
		//				break;
		//			}
		//		}
		//	}
		//}
	}
	else
	{
		myGLCD.print("Synhro HET!", CENTER, 80);
		delay(1000);

	}
	//while (!myTouch.dataAvailable()) {}
	//while (myTouch.dataAvailable()) {}

}
void synhro_ware()                         // Сформировать начальный синхро импульс. Включить синхронизацию
{
	count_ms = 0;
	myGLCD.setFont(BigFont);
	start_enable = false;

	if (digitalRead(synhro_pin) != LOW)
	{
		myGLCD.setColor(VGA_RED);
		myGLCD.print("C""\x9D\xA2""xpo ""\xA2""e ""\xA3""o""\x99\x9F\xA0\xAE\xA7""e""\xA2", CENTER, 80);   // Синхро не подключен
		delay(2000);
	}
	else
	{
		StartSample = micros();                       // Записать время
		while (true)
		{
			if (micros() - StartSample >= 6000000)
			{
				myGLCD.setColor(VGA_RED);
				myGLCD.print("He""\xA4"" c""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\x9D", CENTER, 80);   // "Нет синхронизации"
				delay(2000);
				break;
			}
			if (digitalRead(synhro_pin) == HIGH)
			{
				while (true)
				{
					Timer5.start(set_timeSynhro);
					wiev_synhro();
					while (myTouch.dataAvailable()) {}
					break;
				}
				break;
			}
		}
	}
}
void wiev_synhro()
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor(255, 255, 255);
	myGLCD.setColor(VGA_LIME);
	myGLCD.print("C""\x9D\xA2""xpo OK!", CENTER, 80);                                  // Синхро ОК!
	buttons_set_time_synhro();
	int count_synhro = 0;
	unsigned long TrigSynhro = 0;
	long time_temp = 0;                                  // Для правильного отображения результатов измерения
	dTime = 25;
	scale_strob = 2;

	for (page = 0; page < page_max; page++)              // Стереть старые данные отображения и временных меток
	{
		for (xpos = 0; xpos < 240; xpos++)               // Стереть старые данные отображения и временных меток
		{
			OldSample_osc[xpos][page] = 0;               // Стереть старые данные отображения
			Synhro_osc[xpos][page][new_info] = 0;        // Стереть новые данные временных меток
			Synhro_osc[xpos][page][old_info] = 0;        // Стереть новые данные временных меток
		}
	}

	myGLCD.print("Start", 250, 10);                       //  

	dt = DS3231_clock.getDateTime();
	myGLCD.print("H:", 250, 25);                          //  
	myGLCD.printNumI(dt.hour, 280, 25,2);                      // Вывести на экран время час
	myGLCD.print("M:", 250, 40);                          //  
	myGLCD.printNumI(dt.minute, 280, 40,2);                      // Вывести на экран время мин
	myGLCD.print("S:", 250, 55);                          //  
	myGLCD.printNumI(dt.second, 280, 55,2);                      // Вывести на экран время сек

	unsigned long Start_unixtime = dt.unixtime;
	unsigned long Current_unixtime = 0;
	
	while (1)
	{
		bool synhro_Off = false;                                 // Признак синхронизации в исходное состояние   
		StartSample = micros();
		while (!start_enable)                                    // Ожидание времени начала синхроимпульса
		{
			if (micros() - StartSample > 4000000)                // Ожидаем синхроимпульс в течении 3 секунд
			{
				//synhro_Off = true;
				myGLCD.setFont(BigFont);
				myGLCD.setColor(VGA_YELLOW);
				myGLCD.print("He""\xA4", 100, 30);                   // Нет
				myGLCD.print("c""\x9D\xA2""xpo""\xA2\x9D\x9C""a""\xA6\x9D\x9D", 20, 50);   // синхронизации
				delay(1000);
				break;                                            // Завершить ожидание синхроимпульса
			}

			if (myTouch.dataAvailable())
			{
				delay(10);
				myTouch.read();
				x_osc = myTouch.getX();
				y_osc = myTouch.getY();

				if ((x_osc >= 2) && (x_osc <= 240))               // Выход из программы. Нажать на область экрана
				{
					if ((y_osc >= 1) && (y_osc <= 160))           // 
					{
						break;                                    //Остановить вывода на экран временных меток
					}                                             //
				}

				if ((y_osc >= 205) && (y_osc <= 239))                                // Нижние кнопки переключения. Подстройка периода синхроимпульсов
				{
					if ((x_osc >= 10) && (x_osc <= 60))                              //  Кнопка №1
					{
						waitForIt(10, 210, 60, 239);
						set_timeSynhro = i2c_eeprom_ulong_read(adr_set_timeSynhro);
						set_timeSynhro++;                                            // Увеличить период следования импульсов   
						i2c_eeprom_ulong_write(adr_set_timeSynhro, set_timeSynhro);  // Записать  время 
						Timer5.setPeriod(set_timeSynhro);
						myGLCD.setFont(BigFont);
						myGLCD.printNumI(set_timeSynhro, 10, 190);             // Вывести на экран время периода следования синхроимпульсов
					}
					if ((x_osc >= 70) && (x_osc <= 120))                             //  Кнопка №2
					{
						waitForIt(70, 210, 120, 239);
						set_timeSynhro = i2c_eeprom_ulong_read(adr_set_timeSynhro);
						set_timeSynhro--;                                            // Уменьшить период следования импульсов   
						i2c_eeprom_ulong_write(adr_set_timeSynhro, set_timeSynhro);  // Записать  время 
						Timer5.setPeriod(set_timeSynhro);
						myGLCD.setFont(BigFont);
						myGLCD.printNumI(set_timeSynhro, 10, 190);             // Вывести на экран время периода следования синхроимпульсов
					}
					//if ((x_osc >= 130) && (x_osc <= 180))            //  Кнопка №3
					//{
					//	waitForIt(130, 210, 180, 239);

					//}
					//if ((x_osc >= 190) && (x_osc <= 240))            //  Кнопка №4
					//{
					//	waitForIt(190, 210, 240, 239);
	
					//}
					myGLCD.printNumI(set_timeSynhro, 10, 190);             // Вывести на экран время периода следования синхроимпульсов
				}
			}
		}
	//	StartSample = micros();
	
		if (start_enable)
		{
			// Все нормально. Измеряем !!
			StartSample = micros();
			start_enable = false;                                                 // Время начала синхроимпульса пришло. Выставляем флаг старта в исходное для ожидания следующего синхро импульса
			Timer7.start(scale_strob * 1000);                                     // Включить формирование  временных меток на экране
			for (page = 0; page < page_max; page++)                               // Пишем несколько блоков памяти
			{
				for (xpos = 0; xpos < 240; xpos++)                                // Блоки по 240 байтов
				{
					Sample_osc[xpos][page] = 0;
					Sample_osc[xpos][page] = digitalRead(synhro_pin);             // Получить данные synhro_pin и записать в массив
					if ((Sample_osc[xpos][page] != LOW) && (trig_sin == false))   // Поиск превышения уровня порога
					{
						TrigSynhro = micros();                                    // Записать время срабатывания триггера порога
						trig_sin = true;                                          // установить флаг срабатывания триггера порога
						page_trig = page;                                         // Записываем номер блока в котором сработал триггер
					}
					delayMicroseconds(dTime);                                     // dTime временной интервал (задержка) развертки 
				}
    			if (trig_sin == true) break;                                      // Завершить сканирование на блоке в котором сработал порог
			}
			Timer7.stop();
			count_synhro++;
	    	myGLCD.setColor(0, 0, 0);  
			myGLCD.fillRoundRect(0, 0, 240, 176);                                 // Очистить область экрана для вывода временных меток. 
			DrawGrid1();
			myGLCD.setFont(BigFont);
			myGLCD.setColor(255, 255, 255);

			myGLCD.printNumI(set_timeSynhro, 10, 190);             // Вывести на экран время периода следования синхроимпульсов
			//myGLCD.printNumI(xpos_trig, 170, 190);               // Вывести на экран время точной подстройки периода следования синхроимпульсов(отключена функция)
			myGLCD.printNumI(count_synhro, 245, 193);              // Вывести на экран счетчик синхроимпульсов

			myGLCD.setFont(SmallFont);
			myGLCD.print("Current", 250, 80);                      //  
			dt = DS3231_clock.getDateTime();

			myGLCD.print("H:", 250, 95);                           //  
			myGLCD.printNumI(dt.hour, 280, 95, 2);                      // Вывести на экран время час
			myGLCD.print("M:", 250, 110);                          //  
			myGLCD.printNumI(dt.minute, 280, 110, 2);                     // Вывести на экран время мин
			myGLCD.print("S:", 250, 125);                          //  
			myGLCD.printNumI(dt.second, 280, 125, 2);                     // Вывести на экран время сек
			myGLCD.print("Duration", 250, 145);                          //  
			myGLCD.print("of time", 253 , 160);

			Current_unixtime = dt.unixtime;
			myGLCD.printNumI((Current_unixtime - Start_unixtime)/60,250, 175);                        // Вывести на экран время сек
			myGLCD.print("min", 295, 175);
			myGLCD.setFont(BigFont);

			//count_time();



			for (int xpos = 0; xpos < 239; xpos++)                                     // вывод на экран
			{


				// Нарисовать линию  осциллограммы
				//myGLCD.setColor(255, 255, 255);
	
				if (xpos == 119)
				{
					myGLCD.setColor(VGA_YELLOW);
					myGLCD.drawLine(xpos, 40, xpos, 160);
					myGLCD.drawLine(xpos + 1, 40, xpos + 1, 160);
					myGLCD.setColor(255, 255, 255);
				}

				if (Sample_osc[xpos][page_trig] == HIGH)
				{
					myGLCD.drawLine(xpos, 70, xpos, 160);
					myGLCD.drawLine(xpos + 1, 70, xpos + 1, 160);
				}

				if (Synhro_osc[xpos][page_trig][new_info] > 0)
				{
					myGLCD.setFont(SmallFont);
					myGLCD.setColor(VGA_WHITE);
					if (Synhro_osc[xpos][page_trig][new_info] == 4095)
					{
						myGLCD.drawLine(xpos, 120, xpos, 160);
					}
					else
					{
						myGLCD.setColor(255, 255, 255);
						if (xpos > 230)
						{
							myGLCD.printNumI(Synhro_osc[xpos][page_trig][new_info], xpos - 12, 165);
						}
						else
						{
							myGLCD.printNumI(Synhro_osc[xpos][page_trig][new_info], xpos, 165);

						}
					}
				}
				myGLCD.setColor(0, 0, 0);
				myGLCD.fillCircle(200, 10, 10);
				myGLCD.setColor(255, 255, 255);
				myGLCD.drawCircle(200, 10, 10);
			}
		}
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_osc = myTouch.getX();
			y_osc = myTouch.getY();
		
			if ((x_osc >= 2) && (x_osc <= 240))               // Выход из программы. Нажать на область экрана
			{
				if ((y_osc >= 1) && (y_osc <= 160))           // 
				{
					break;                                    //Остановить вывода на экран временных меток
				}                                             //
			}

			if ((y_osc >= 205) && (y_osc <= 239))                                // Нижние кнопки переключения. Подстройка периода синхроимпульсов
			{
				myGLCD.setFont(BigFont);
				if ((x_osc >= 10) && (x_osc <= 60))                              //  Кнопка №1
				{
					waitForIt(10, 210, 60, 239);
					set_timeSynhro = i2c_eeprom_ulong_read(adr_set_timeSynhro);
					set_timeSynhro++;                                            // Увеличить период следования импульсов   
					i2c_eeprom_ulong_write(adr_set_timeSynhro, set_timeSynhro);  // Записать  время 
					Timer5.setPeriod(set_timeSynhro);
					myGLCD.setFont(BigFont);
					myGLCD.printNumI(set_timeSynhro, 10, 190);             // Вывести на экран время периода следования синхроимпульсов
				}
				if ((x_osc >= 70) && (x_osc <= 120))                             //  Кнопка №2
				{
					waitForIt(70, 210, 120, 239);
					set_timeSynhro = i2c_eeprom_ulong_read(adr_set_timeSynhro);
					set_timeSynhro--;                                            // Уменьшить период следования импульсов   
					i2c_eeprom_ulong_write(adr_set_timeSynhro, set_timeSynhro);  // Записать  время 
					Timer5.setPeriod(set_timeSynhro);
					myGLCD.setFont(BigFont);
					myGLCD.printNumI(set_timeSynhro, 10, 190);             // Вывести на экран время периода следования синхроимпульсов
				}
				//if ((x_osc >= 130) && (x_osc <= 180))            //  Кнопка №3
				//{
				//	waitForIt(130, 210, 180, 239);
				//}
				//if ((x_osc >= 190) && (x_osc <= 240))            //  Кнопка №4
				//{
				//	waitForIt(190, 210, 240, 239);
				//}
			}
		}

		attachInterrupt(kn_red, volume_up, FALLING);
		attachInterrupt(kn_blue, volume_down, FALLING);
	}
	while (myTouch.dataAvailable()) {}
 }
void synhro_DS3231_clock()
{
	DS3231_clock.clearAlarm1();
	data_out[2] = 8;                                    // Отправить команду синхронизации часов
	data_out[12] = dt.year-2000;                        // 
	data_out[13] = dt.month;                            // 
	data_out[14] = dt.day;                              // 
	data_out[15] = dt.hour;                             //
	data_out[16] = dt.minute;                           // 
	radio_send_command();
	DS3231_clock.setDateTime(dt.year, dt.month, dt.day, dt.hour, dt.minute, 00);
}


// чтение
unsigned long i2c_eeprom_ulong_read(int addr)
{
	byte raw[4];
	for (byte i = 0; i < 4; i++) raw[i] = i2c_eeprom_read_byte(deviceaddress, addr + i);
	unsigned long &num = (unsigned long&)raw;
	return num;
}
// запись
void i2c_eeprom_ulong_write(int addr, unsigned long num)
{
	byte raw[4];
	(unsigned long&)raw = num;
	for (byte i = 0; i < 4; i++) i2c_eeprom_write_byte(deviceaddress, addr + i, raw[i]);
}

void touch_down()  //  Нижнее меню осциллографа
{
	delay(10);
	myTouch.read();
	x_osc = myTouch.getX();
	y_osc = myTouch.getY();
	myGLCD.setFont(SmallFont);
	unsigned long timeP;

	if ((y_osc >= 210) && (y_osc <= 239))                         //   Нижние кнопки
	{
		if ((x_osc >= 10) && (x_osc <= 60))                       //  Кн 1
		{
			waitForIt(10, 210, 60, 239);
			timeP = i2c_eeprom_ulong_read(adr_timePeriod);
			timeP += 1000000;
			if (timeP > 1000000 * 6)  timeP = 1000000 * 6;
			i2c_eeprom_ulong_write(adr_timePeriod, timeP);

		}
		if ((x_osc >= 70) && (x_osc <= 120))                       //  Кн 2
		{
			waitForIt(70, 210, 120, 239);
			timeP = i2c_eeprom_ulong_read(adr_timePeriod);
			timeP -=1000000;
			if (timeP < 1000000) timeP = 1000000;
			i2c_eeprom_ulong_write(adr_timePeriod, timeP);
		}
		if ((x_osc >= 130) && (x_osc <= 180))                       //  Вход 0
		{
			waitForIt(130, 210, 180, 239);
		}
		if ((x_osc >= 190) && (x_osc <= 240))                       //  Вход 0
		{
			waitForIt(190, 210, 240, 239);

		}
	}
}
void buttons_channel()                   // Нижние кнопки переключения 
{
	myGLCD.setFont(SmallFont);

	if (Channel0)
	{
		myGLCD.setColor(255, 255, 255);
		myGLCD.fillRoundRect(10, 200, 60, 205);
		myGLCD.setColor(VGA_LIME);
		myGLCD.setBackColor(VGA_LIME);
		myGLCD.fillRoundRect(10, 210, 60, 239);
		myGLCD.setColor(0, 0, 0);
		myGLCD.print("0", 32, 212);
		myGLCD.print("BXOD", 20, 226);
		osc_line_off0 = true;
	}
	else
	{
		myGLCD.setColor(0, 0, 0);
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.fillRoundRect(10, 200, 60, 205);   // Индикатор цвета линии
		myGLCD.fillRoundRect(10, 210, 60, 239);
		myGLCD.setColor(255, 255, 255);
		myGLCD.print("0", 32, 212);
		myGLCD.print("BXOD", 20, 226);
	}

	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(10, 210, 60, 239);
	myGLCD.drawRoundRect(70, 210, 120, 239);
	myGLCD.drawRoundRect(130, 210, 180, 239);
	myGLCD.drawRoundRect(190, 210, 240, 239);
}
void chench_Channel()
{
	//Подготовка номера аналогового сигнала, количества каналов и кода настройки АЦП
	Channel_x = 0;                                  // Аналоговый вход А0
	count_pin = 1;                                  // Количество входов
	Channel_x |= 0x80;                              // Сформировать код установки входов
	ADC_CHER = Channel_x;                           // Записать код настройки входов в АЦП
	SAMPLES_PER_BLOCK = DATA_DIM16 / count_pin;     // Размер выделенного буфера
}

void set_Channel()
{
	//Подготовка номера аналогового сигнала, количества каналов и кода настройки АЦП
	Channel_x = 0;                                  // Аналоговый вход А0
	count_pin = 1;                                  // Количество входов
	Channel_x |= 0x80;                              // Сформировать код установки входов
	ADC_CHER = Channel_x;                           // Записать код настройки входов в АЦП
	SAMPLES_PER_BLOCK = DATA_DIM16 / count_pin;     // Размер выделенного буфера
}

void DrawGrid()
{

  myGLCD.setColor(VGA_GREEN);                                                 // Цвет сетки                                 
  for (dgvh = 0; dgvh < 7; dgvh++)                                            // Нарисовать сетку
  {
	  myGLCD.drawLine(dgvh * 40, 0, dgvh * 40, 159);
	  if(dgvh < 5)
	  {
	    myGLCD.drawLine(0, dgvh * 40, 239, dgvh * 40);
      }
  }
	myGLCD.setColor(255, 255, 255);                                           // Белая окантовка кнопок справа
	myGLCD.drawRoundRect (245, 1, 318, 40);                                   // Нарисовать кнопоки справа
	myGLCD.drawRoundRect (245, 45, 318, 85);
	myGLCD.drawRoundRect (245, 90, 318, 130);
	myGLCD.drawRoundRect (245, 135, 318, 175);

	//myGLCD.drawRoundRect (10, 210, 60, 239);
	//myGLCD.drawRoundRect (70, 210, 120, 239);
	//myGLCD.drawRoundRect (130, 210, 180, 239);
	//myGLCD.drawRoundRect (190, 210, 240, 239);
	//myGLCD.drawRoundRect (250, 200, 318, 239);
	//myGLCD.setBackColor( 0, 0, 0);
	//myGLCD.setFont( SmallFont);
	//if (mode1 == 0)
	//	{				
	//		myGLCD.print("4", 241, 0);
	//		myGLCD.print("3", 241, 34);
	//		myGLCD.print("2", 241, 74);
	//		myGLCD.print("1", 241, 114);
	//		myGLCD.print("0", 241, 152);
	//	}
	//if (mode1 == 1)
	//	{
	//		myGLCD.print("2", 241, 0);
	//		myGLCD.print("1,5", 226, 34);
	//		myGLCD.print("1", 241, 74);
	//		myGLCD.print("0,5", 226, 114);
	//		myGLCD.print("0", 241, 152);
	//	}

	//if (mode1 == 2)
	//	{
	//		myGLCD.print("1", 241, 0);
	//		myGLCD.print("0,75", 218, 34);
	//		myGLCD.print("0,5", 226, 74);
	//		myGLCD.print("0,25", 218, 114);
	//		myGLCD.print("0", 241, 152);
	//	}
	//if (mode1 == 3)
	//	{
	//		myGLCD.print("0,4", 226, 0);
	//		myGLCD.print("0,3", 226, 34);
	//		myGLCD.print("0,2", 226, 74);
	//		myGLCD.print("0,1", 226, 114);
	//		myGLCD.print("0", 241, 152);
	//	}
	//if (!strob_start) 
	//	{
	//		myGLCD.setColor(VGA_RED);
	//		myGLCD.fillCircle(227,12,10);
	//	}
	//else
	//	{
	//		myGLCD.setColor(255,255,255);
	//		myGLCD.drawCircle(227,12,10);
	//	}
	myGLCD.setColor(255,255,255);

}
void DrawGrid1()
{
	myGLCD.setColor(0, 200, 0);                  
	for (dgvh = 0; dgvh < 7; dgvh++)                          // Нарисовать сетку
	{
		myGLCD.drawLine(dgvh * 40, 0, dgvh * 40, 159);
		if (dgvh < 5)
		{
			myGLCD.drawLine(0, dgvh * 40, 239, dgvh * 40);
		}
	}
	myGLCD.setColor(255, 255, 255);                           // Белая окантовка

	//if (!strob_start)                                         // Нарисовать круг - индикатор синхронизации
	//	{
	//		myGLCD.setColor(VGA_RED);
	//		myGLCD.fillCircle(227,12,10);
	//	}
	//else
	//	{
	//		myGLCD.setColor(255,255,255);
	//		myGLCD.drawCircle(227,12,10);
	//	}
	myGLCD.setColor(255,255,255);
}

void touch_osc()  //  Нижнее меню осциллографа
{
	delay(10);
	myTouch.read();
	x_osc=myTouch.getX();
	y_osc=myTouch.getY();
	myGLCD.setFont( SmallFont);

	if ((y_osc>=210) && (y_osc<=239))                         //   Нижние кнопки
	  {
		if ((x_osc>=10) && (x_osc<=60))                       //  Вход 0
			{
				waitForIt(10, 210, 60, 239);

				Channel0 = !Channel0;

				if (Channel0)
					{
						myGLCD.setColor( 255, 255, 255);
						myGLCD.fillRoundRect (10, 200, 60, 205);
						myGLCD.setColor(VGA_LIME);
						myGLCD.setBackColor( VGA_LIME);
						myGLCD.fillRoundRect (10, 210, 60, 239);
						myGLCD.setColor(0, 0, 0);
						myGLCD.print("0", 32, 212);
						myGLCD.print("BXOD", 20, 226);
						osc_line_off0 = true;
					}
				else
					{
						myGLCD.setColor(0,0,0);
						myGLCD.setBackColor( 0,0,0);
						myGLCD.fillRoundRect (10, 200, 60, 205);
						myGLCD.fillRoundRect (10, 210, 60, 239);
						myGLCD.setColor(255, 255, 255);
						myGLCD.drawRoundRect (10, 210, 60, 239);
						myGLCD.print("0", 32, 212);
						myGLCD.print("BXOD", 20, 226);
					}

				chench_Channel();
				MinAnalog0 = 4095;
				MaxAnalog0 = 0;
			}
	}
}

void switch_trig(int trig_x)
{
	if (Channel0)
	{
		Channel_trig = 0x80;
		myGLCD.print(" ON ", 270, 226);
		MinAnalog = MinAnalog0 ;
		MaxAnalog = MaxAnalog0 ;
	}
	else
	{
		myGLCD.print(" OFF", 270, 226);
	}
}
void trig_min_max(int trig_x)
{
	if (Channel0)
	{
		MinAnalog = MinAnalog0 ;
		MaxAnalog = MaxAnalog0 ;
	}
}

void measure_power()
{                                                 // Программа измерения напряжения питания с делителем 1/2 
											      // Установить резистивный делитель +15к общ 10к на разъем питания
	uint32_t logTime1 = 0;
	logTime1 = millis();
	if(logTime1 - StartSample > 500)              //  индикация 
		{
		StartSample = millis();
		int m_power = 0;
		float ind_power = 0;
		ADC_CHER = Analog_pinA3;                  // Подключить канал А3, разрядность 12
		ADC_CR = ADC_START ; 	                  // Запустить преобразование
		while (!(ADC_ISR_DRDY));                  // Ожидание конца преобразования
		m_power =  ADC->ADC_CDR[4];               // Считать данные с канала A3
		ind_power = m_power *(3.2 / 4096 * 2);    // Получить напряжение в вольтах
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.setFont(SmallSymbolFont);
		if (ind_power > 4.1 )
			{
				myGLCD.setColor(VGA_LIME);
				myGLCD.drawRoundRect (279,149, 319, 189);
				myGLCD.setColor(VGA_WHITE);
				myGLCD.print( "\x20", 295, 155);  
			}
		else if (ind_power > 4.0 && ind_power < 4.1 ) 
			{
				myGLCD.setColor(VGA_LIME);
				myGLCD.drawRoundRect (279,149, 319, 189);
				myGLCD.setColor(VGA_WHITE);
				myGLCD.print( "\x21", 295, 155);  
			}
		else if (ind_power > 3.9 && ind_power < 4.0 )
			{
				myGLCD.setColor(VGA_WHITE);
				myGLCD.drawRoundRect (279,149, 319, 189);
				myGLCD.setColor(VGA_WHITE);
				myGLCD.print( "\x22", 295, 155);  
			}
		else if (ind_power > 3.8 && ind_power < 3.9 )
			{
				myGLCD.setColor(VGA_YELLOW);
				myGLCD.drawRoundRect (279,149, 319, 189);
				myGLCD.setColor(VGA_WHITE);
				myGLCD.print( "\x23", 295, 155); 
			}
		else if (ind_power < 3.8 )
			{
				myGLCD.setColor(VGA_RED);
				myGLCD.drawRoundRect (279,149, 319, 189);
				myGLCD.setColor(VGA_WHITE);
				myGLCD.print( "\x24", 295, 155);  
			}
		myGLCD.setFont( SmallFont);
		myGLCD.setColor(VGA_WHITE);
		myGLCD.printNumF(ind_power,1, 289, 172); 
	}
}

void resistor(int resist, int valresist)
{
	resistance = valresist;
	switch (resist)
	{
	case 1:
		Wire.beginTransmission(address_AD5252);     // transmit to device
		Wire.write(byte(control_word1));            // sends instruction byte  
		Wire.write(resistance);                     // sends potentiometer value byte  
		Wire.endTransmission();                     // stop transmitting
		break;
	case 2:
		Wire.beginTransmission(address_AD5252);     // transmit to device
		Wire.write(byte(control_word2));            // sends instruction byte  
		Wire.write(resistance);                     // sends potentiometer value byte  
		Wire.endTransmission();                     // stop transmitting
		break;
	}
}
void setup_resistor()
{
	Wire.beginTransmission(address_AD5252);      // transmit to device
	Wire.write(byte(control_word1));             // sends instruction byte  
	Wire.write(0);                               // sends potentiometer value byte  
	Wire.endTransmission();                      // stop transmitting
	Wire.beginTransmission(address_AD5252);      // transmit to device
	Wire.write(byte(control_word2));             // sends instruction byte  
	Wire.write(0);                               // sends potentiometer value byte  
	Wire.endTransmission();                      // stop transmitting
}

//------------------------------------------------------------------------------
  
void Draw_menu_Radio()
{
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x = 0; x<4; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(30, 20 + (50 * x), 290, 60 + (50 * x));
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(30, 20 + (50 * x), 290, 60 + (50 * x));
	}
	myGLCD.print(txt_radio_menu1, CENTER, 30);     // 
	myGLCD.print(txt_radio_menu2, CENTER, 80);
	myGLCD.print(txt_radio_menu3, CENTER, 130);
	myGLCD.print(txt_radio_menu4, CENTER, 180);
}

void menu_radio()                            // Меню "Radio", вызывается из меню "Настройка"
{
	Draw_menu_Radio();
	while (true)
	{
		delay(10);
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			int	x = myTouch.getX();
			int	y = myTouch.getY();

			if ((x >= 30) && (x <= 290))       // 
			{
				if ((y >= 20) && (y <= 60))    // Button: 1  "Test ping"
				{
					waitForIt(30, 20, 290, 60);
					myGLCD.clrScr();
					radio_test_ping();
					Draw_menu_Radio();
				}
				if ((y >= 70) && (y <= 110))   // Button: 2 "Test transfer"
				{
					waitForIt(30, 70, 290, 110);
					myGLCD.clrScr();
					radio_transfer();
					Draw_menu_Radio();
				}
				if ((y >= 120) && (y <= 160))  // Button: 3 
				{
					waitForIt(30, 120, 290, 160);
					myGLCD.clrScr();
				//	radio_TX();
					Draw_menu_Radio();
				}
				if ((y >= 170) && (y <= 220))  // Button: 4 "EXIT" Выход
				{
					waitForIt(30, 170, 290, 210);
					break;
				}
			}
		}
	}
}

void radio_test_ping()
{
	detachInterrupt(kn_red);
	detachInterrupt(kn_blue);
	myGLCD.clrScr();
	Serial.println(F("Test ping."));
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print("TEST PING", CENTER, 10);
	myGLCD.print(txt_info11, CENTER, 221);            // Кнопка "ESC -> PUSH"
	myGLCD.setBackColor(0, 0, 0);
	setup_radio();
	role_test = role_ping_out;
	unsigned long tim1 = 0;
	volume_variant = 3;                               //  Установить уровень громкости 1 канала усилителя Трансмиттера

	while(1)
	{
		if (myTouch.dataAvailable())
		{
			delay(10);
			myTouch.read();
			x_osc = myTouch.getX();
			y_osc = myTouch.getY();

			if ((x_osc >= 2) && (x_osc <= 319))               //  Область экрана
			{
				if ((y_osc >= 1) && (y_osc <= 239))           // Delay row
				{
					sound1();
					break;
				}
			}
		}
		if (role_test == role_ping_out) 
		{
			radio.stopListening();                                  // Во-первых, перестаньте слушать, чтобы мы могли поговорить.
			myGLCD.print("Sending", 1, 40);
			myGLCD.print("    ", 125, 40);
			myGLCD.setColor(VGA_LIME);
			if (counter_test<10)
			{
				myGLCD.printNumI(counter_test, 155, 40);
			}
			else if (counter_test>9 && counter_test<100)
			{
				myGLCD.printNumI(counter_test, 155-16, 40);
			}
			else if (counter_test > 99)
			{
				myGLCD.printNumI(counter_test, 155 - 32, 40);
			}
			myGLCD.setColor(255, 255, 255);
			myGLCD.print("payload", 178, 40);
			if(kn!=0) set_volume(volume_variant, kn);

			data_out[0] = counter_test;
			data_out[1] = 1;                                       //1= Отправить команду ping звуковую посылку 
			data_out[2] = 1;                    //
			data_out[3] = 1;                    //
			data_out[4] = highByte(time_sound);                     //1= Отправить звуковую посылку 
			data_out[5] = lowByte(time_sound);                    //1= Отправить звуковую посылку 
			data_out[6] = highByte(freq_sound);                    //1= Отправить звуковую посылку 
			data_out[7] = lowByte(freq_sound);                   //1= Отправить звуковую посылку 
			data_out[8] = volume1;
			data_out[9] = volume2;
	
			
			//int value = 3000;
			//// разбираем
			//byte hi = highByte(value);
			//byte low = lowByte(value);

			//// тут мы эти hi,low можем сохраить, прочитать из eePROM

			//int value2 = (hi << 8) | low; // собираем как "настоящие программеры"
			//int value3 = word(hi, low); // или собираем как "ардуинщики"

			//int time_sound = 200;
			//int freq_sound = 1850;



			unsigned long time = micros();                          // Take the time, and send it.  This will block until complete   

			if (!radio.write(&data_out, sizeof(data_out)))
			//if (!radio.write(&data_out, 2)) 
			{
				Serial.println(F("failed."));
			}
			else 
			{
				if (!radio.available()) 
				{
					Serial.println(F("Blank Payload Received."));
				}
				else 
				{
					while (radio.isAckPayloadAvailable())
					{
						unsigned long tim = micros();
						radio.read(&data_in, sizeof(data_in));

					//	radio.read(&data_in, sizeof(data_in));
						tim1 = tim - time;
						myGLCD.print("Respons", 1, 60);
						myGLCD.print("    ", 125, 60);
						myGLCD.setColor(VGA_LIME);
						if (data_in[0]<10)
						{
							myGLCD.printNumI(data_in[0], 155, 60);
						}
						else if(data_in[0]>9&& data_in[0]<100)
						{
							myGLCD.printNumI(data_in[0], 155-16, 60);
						}
						else if (data_in[0] > 99)
						{
							myGLCD.printNumI(data_in[0], 155-32, 60);
						}
						myGLCD.setColor(255, 255, 255);
						myGLCD.print("round", 178, 60);
						myGLCD.print("Delay: ", 1, 80);
						myGLCD.print("     ", 100, 80);
						myGLCD.setColor(VGA_LIME);
						if (tim1<999)
						{
							myGLCD.printNumI(tim1, 155-32, 80);
						}
						else 
						{
							myGLCD.printNumI(tim1, 155-48, 80);
						}
						myGLCD.setColor(255, 255, 255);
						myGLCD.print("microsec", 178, 80);
						counter_test++;
					}
				}
			}
			attachInterrupt(kn_red, volume_up, FALLING);
			attachInterrupt(kn_blue, volume_down, FALLING);
			delay(1000);
		}
	}
	while(!myTouch.dataAvailable()){}
	delay(50);
	while(myTouch.dataAvailable()){}
}

void radio_transfer()                                       // test transfer
{
	detachInterrupt(kn_red);
	detachInterrupt(kn_blue);
    tim1 = 0;
	volume_variant = 3;                                     //  Установить уровень громкости 1 канала усилителя Трансмиттера
	radio.stopListening();                                  // Во-первых, перестаньте слушать, чтобы мы могли поговорить.
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setFont(SmallFont);
	if (kn != 0) set_volume(volume_variant, kn);
	data_out[0] = counter_test;
	data_out[1] = 1;                                        //  
	data_out[2] = 1;                                        // Отправить команду ping звуковую посылку 
	data_out[3] = 1;                                        //
	data_out[4] = highByte(time_sound);                     // Старший байт Отправить длительность звуковуй посылки
	data_out[5] = lowByte(time_sound);                      // Младший байт Отправить длительность звуковуй посылки
	data_out[6] = highByte(freq_sound);                     // Старший байт Отправить звуковую посылку 
	data_out[7] = lowByte(freq_sound);                      // Младший байт Отправить звуковую посылку 
	data_out[8] = volume1;
	data_out[9] = volume2;

	timeStartRadio = micros();                              // Записать время старта радио синхро импульса

	if (!radio.write(&data_out, sizeof(data_out)))
	{
	//	Serial.println(F("failed."));
		myGLCD.setColor(VGA_LIME);                          // Вывести на дисплей время ответа синхроимпульса
		myGLCD.print("     ", 90 - 40, 178);                // Вывести на дисплей время ответа синхроимпульса
		myGLCD.printNumI(0, 90 - 32, 178);                  // Вывести на дисплей время ответа синхроимпульса
		myGLCD.setColor(255, 255, 255);
	}
	else
	{
		if (!radio.available())
		{
    		//	Serial.println(F("Blank Payload Received."));
		}
		else
		{
			while (radio.isAckPayloadAvailable())
			{
				timeStopRadio = micros();                     // Записать время получения ответа.  
				radio.read(&data_in, sizeof(data_in));
				tim1 = timeStopRadio - timeStartRadio;        // Получить время ответа синхро импульса
				myGLCD.print("Delay: ", 5, 178);              // Вывести на дисплей время ответа синхроимпульса
				myGLCD.print("     ", 90-40, 178);            // Вывести на дисплей время ответа синхроимпульса
				myGLCD.setColor(VGA_LIME);                    // Вывести на дисплей время ответа синхроимпульса
				if (tim1 < 999)                               // Подравнять вывод чисел на дисплей 
				{
					myGLCD.printNumI(tim1, 90 - 32, 178);     // Вывести на дисплей время ответа синхроимпульса
				}
				else
				{
					myGLCD.printNumI(tim1, 90 - 40, 178);     // Вывести на дисплей время ответа синхроимпульса
				}
				myGLCD.setColor(255, 255, 255);
				myGLCD.print("microsec", 90, 178);            // Вывести на дисплей время ответа синхроимпульса
				counter_test++;
			}
		}
	}
	attachInterrupt(kn_red, volume_up, FALLING);
	attachInterrupt(kn_blue, volume_down, FALLING);
}
void radio_send_command()                                   // Синронизация модулей
{
	detachInterrupt(kn_red);
	detachInterrupt(kn_blue);
	tim1 = 0;
	volume_variant = 3;                                //  Установить уровень громкости 1 канала усилителя Трансмиттера
	radio.stopListening();                             // Во-первых, перестаньте слушать, чтобы мы могли поговорить.
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setFont(SmallFont);
	if (kn != 0) set_volume(volume_variant, kn);
	data_out[0] = counter_test;
	data_out[1] = 1;                                    //  
	//data_out[2] = 2;                                  // Команда задана ранее при вызове программы
	data_out[3] = 1;                                    //
	data_out[4] = highByte(time_sound);                 // Старший байт Отправить длительность звуковой посылки
	data_out[5] = lowByte(time_sound);                  // Младший байт Отправить длительность звуковой посылки
	data_out[6] = highByte(freq_sound);                 // Старший байт Отправить частоту звуковой посылки
	data_out[7] = lowByte(freq_sound);                  // Младший байт Отправить частоту звуковой посылки 
	data_out[8] = volume1;                              // 
	data_out[9] = volume2;                              // 

	timeStartRadio = micros();                            // Записать время старта радио синхро импульса

	if (!radio.write(&data_out, sizeof(data_out)))
	{
		//	Serial.println(F("failed."));
		myGLCD.setColor(VGA_LIME);                        // Вывести на дисплей время ответа синхроимпульса
		myGLCD.print("     ", 90 - 40, 178);              // Вывести на дисплей время ответа синхроимпульса
		myGLCD.printNumI(0, 90 - 32, 178);                // Вывести на дисплей время ответа синхроимпульса
		myGLCD.setColor(255, 255, 255);
	}
	else
	{
		if (!radio.available())
		{
			//	Serial.println(F("Blank Payload Received."));
		}
		else
		{
			while (radio.isAckPayloadAvailable())
			{
				timeStopRadio = micros();                     // Записать время получения ответа.  
				radio.read(&data_in, sizeof(data_in));
				tim1 = timeStopRadio - timeStartRadio;        // Получить время ответа синхро импульса
				myGLCD.print("Delay: ", 5, 178);              // Вывести на дисплей время ответа синхроимпульса
				myGLCD.print("     ", 90 - 40, 178);          // Вывести на дисплей время ответа синхроимпульса
				myGLCD.setColor(VGA_LIME);                    // Вывести на дисплей время ответа синхроимпульса
				if (tim1 < 999)                               // Подравнять вывод чисел на дисплей 
				{
					myGLCD.printNumI(tim1, 90 - 32, 178);     // Вывести на дисплей время ответа синхроимпульса
				}
				else
				{
					myGLCD.printNumI(tim1, 90 - 40, 178);     // Вывести на дисплей время ответа синхроимпульса
				}
				myGLCD.setColor(255, 255, 255);
				myGLCD.print("microsec", 90, 178);            // Вывести на дисплей время ответа синхроимпульса
				counter_test++;
			}
		}
	}
}

void setup_radio()
{
	radio.begin();
	radio.setAutoAck(1);                    // Ensure autoACK is enabled
	//radio.setPALevel(RF24_PA_MAX);
	radio.setPALevel(RF24_PA_HIGH);
	//radio.setPALevel(RF24_PA_LOW);           // Set PA LOW for this demonstration. We want the radio to be as lossy as possible for this example.
	radio.setDataRate(RF24_1MBPS);
//	radio.setDataRate(RF24_250KBPS);
	radio.enableAckPayload();               // Allow optional ack payloads
	radio.setRetries(0, 1);                 // Smallest time between retries, max no. of retries
	radio.setPayloadSize(sizeof(data_out));                // Here we are sending 1-byte payloads to test the call-response speed
	//radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed
	radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();                 // Start listening
	radio.printDetails();                   // Dump the configuration of the rf unit for debugging

	role = role_ping_out;                  // Become the primary transmitter (ping out)
	radio.openWritingPipe(pipes[0]);
	radio.openReadingPipe(1, pipes[1]);
}

void sound1()
{
	digitalWrite(sounder, HIGH);
	delay(100);
	digitalWrite(sounder, LOW);
	delay(100);
}
void vibro1()
{
	digitalWrite(vibro, HIGH);
	delay(100);
	digitalWrite(vibro, LOW);
	delay(100);
}
void volume_up()
{
	if (Interrupt_enable)
	{
		detachInterrupt(kn_red);
		detachInterrupt(kn_blue);
		kn = 1;
		Serial.println("Up");
		attachInterrupt(kn_red, volume_up, FALLING);
		attachInterrupt(kn_blue, volume_down, FALLING);
	}
}
void volume_down()
{
	if (Interrupt_enable)
	{
		detachInterrupt(kn_red);
		detachInterrupt(kn_blue);
		kn = 2;
		Serial.println("Down");
		attachInterrupt(kn_red, volume_up, FALLING);
		attachInterrupt(kn_blue, volume_down, FALLING);
	}
}
void set_volume(int reg_module, byte count_vol)
{
	/*
	reg_module = 1  Установить уровень громкости 1 канала усилителя Sound Test Base
	reg_module = 2  Установить уровень громкости 2 канала усилителя Sound Test Base
	reg_module = 3  Установить уровень громкости 1 канала усилителя Трансмиттера
	reg_module = 4  Установить уровень громкости 2 канала усилителя Трансмиттера (не задействован)
	reg_module = 5  Отобразить уровень громкости  каналов усилителя на Базе
	*/
	kn = 0;
	myGLCD.setFont(SmallFont);
	if (count_vol != 0)
	{
		byte b = 0;
		switch (reg_module)
		{
		case 1:
			if (count_vol == 1)
			{
				b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
				b++;
				if (b > 9) b = 9;
				i2c_eeprom_write_byte(deviceaddress, adr_count1_kn, b);
			}
			else if (count_vol == 2)
			{
				b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
				if (b > 0) b--;
			    if (b <= 0) b = 0;
				i2c_eeprom_write_byte(deviceaddress, adr_count1_kn, b);
			}
			resistor(1, 16 * koeff_volume[b]);
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print(proc_volume[b], 22, 214);
			break;
		case 2:
			if (count_vol == 1)
			{
				b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
				b++;
				if (b > 9) b = 9;
				i2c_eeprom_write_byte(deviceaddress, adr_count2_kn, b);
			}
			else if (count_vol == 2)
			{
				b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
				if (b > 0) b--;
				if (b <= 0) b = 0;
				i2c_eeprom_write_byte(deviceaddress, adr_count2_kn, b);
			}
			resistor(2, 16 * koeff_volume[b]);
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print(proc_volume[b], 143, 214);
			break;
		case 3:
			if (count_vol == 1)
			{
				b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
				b++;
				if (b > 9) b = 9;
				i2c_eeprom_write_byte(deviceaddress, adr_count3_kn, b);
			}
			else if (count_vol == 2)
			{
				b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);
				if (b > 0) b--;
				if (b <= 0) b = 0;
				i2c_eeprom_write_byte(deviceaddress, adr_count3_kn, b);
			}
			volume1 = 16 * koeff_volume[b];
			kn = 0;
			break;
		case 4:

			if (count_vol == 1)
			{
				b = i2c_eeprom_read_byte(deviceaddress, adr_count4_kn);
				b++;
				if (b > 9) b = 9;
				i2c_eeprom_write_byte(deviceaddress, adr_count4_kn, b);
			}
			else if (count_vol == 2)
			{
				b = i2c_eeprom_read_byte(deviceaddress, adr_count4_kn);
				if (b > 0) b --;
				if (b <= 0) b = 0;
				i2c_eeprom_write_byte(deviceaddress, adr_count4_kn, b);
			}
			volume2 = 16 * koeff_volume[b];
			kn = 0;
			break;
		case 5:
			b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print(proc_volume[b], 22, 214);
			b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);
			myGLCD.print(proc_volume[b], 143, 214);
			break;
		default:
			break;
		}
		//kn = 0;
	}
	//kn = 0;
}
void restore_volume()
{
	// Восстановить установки громкости электронного резистора
	int b = 0;
	b = i2c_eeprom_read_byte(deviceaddress, adr_count1_kn);     // Восстановить уровень громкости 1 канала усилителя Sound Test Base
	resistor(1, 16 * koeff_volume[b]);
	b = i2c_eeprom_read_byte(deviceaddress, adr_count2_kn);     // Восстановить уровень громкости 2 канала усилителя Sound Test Base
	resistor(2, 16 * koeff_volume[b]);
	b = i2c_eeprom_read_byte(deviceaddress, adr_count3_kn);     // Восстановить уровень громкости 1 канала усилителя Трансмиттера
	volume1 = 16 * koeff_volume[b];
	b = i2c_eeprom_read_byte(deviceaddress, adr_count4_kn);     // Восстановить уровень громкости 2 канала усилителя Трансмиттера (не задействован)
	volume2 = 16 * koeff_volume[b];

}

//+++++++++++++++++++++++  Настройки +++++++++++++++++++++++++++++++++++++
 
void i2c_eeprom_write_byte(int deviceaddress, unsigned int eeaddress, byte data)
{
	int rdata = data;
	Wire.beginTransmission(deviceaddress);
	Wire.write((int)(eeaddress >> 8)); // MSB
	Wire.write((int)(eeaddress & 0xFF)); // LSB
	Wire.write(rdata);
	Wire.endTransmission();
	delay(10);
}
byte i2c_eeprom_read_byte(int deviceaddress, unsigned int eeaddress) {
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
Wire.requestFrom(deviceaddress,length);
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
void i2c_test()
{
	Serial.println("--------  EEPROM Test  ---------");
	char somedata[] = "this data from the eeprom i2c"; // data to write
	i2c_eeprom_write_page(deviceaddress, 0, (byte *)somedata, sizeof(somedata)); // write to EEPROM
	delay(100); //add a small delay
	Serial.println("Written Done");
	delay(10);
	Serial.print("Read EERPOM:");
	byte b = i2c_eeprom_read_byte(deviceaddress, 0); // access the first address from the memory
	char addr = 0; //first address

	while (b != 0)
	{
		Serial.print((char)b); //print content to serial port
		if (b != somedata[addr])
		{
			break;
		}
		addr++; //increase address
		b = i2c_eeprom_read_byte(0x50, addr); //access an address from the memory
	}
	Serial.println();
	Serial.println();
}

void clean_mem()
{

	byte b = i2c_eeprom_read_byte(deviceaddress, 1023);                      // access the first address from the memory
	if (b != mem_start)
	{
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.print("O""\xA7\x9D""c""\xA4\x9F""a ""\xA3""a""\xA1\xAF\xA4\x9D", CENTER, 60);             // "Очистка памяти"
		for (int i = 0; i < 1024; i++)
		{
			i2c_eeprom_write_byte(deviceaddress, i, 0x00);
			delay(10);
		}
		i2c_eeprom_write_byte(deviceaddress,1023, mem_start);
		i2c_eeprom_ulong_write(adr_set_timeSynhro, set_timeSynhro);         // Записать  время 
		myGLCD.print("                 ", CENTER, 60);             // "Очистка памяти"
		myGLCD.setBackColor(0, 0, 255);
	}
}

void test_resistor()
{

	for (int i = 0; i < 255; i++)
	{
		resistor(1, i);                                    // Установить уровень сигнала
		resistor(2, i);                                    // Установить уровень сигнала
		delay(100);
	}


}
//
//void alarmFunction()
//{
//	isAlarm = false;
//	DS3231_clock.clearAlarm1();
//	dt = DS3231_clock.getDateTime();
//	myGLCD.setBackColor(0, 0, 0);                   // Синий фон кнопок
//	myGLCD.setColor(255, 255, 255);
//	myGLCD.setFont(SmallFont);
//	myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s - ", dt), 10, 0);
//	alarm_synhro++;
//	if (alarm_synhro >4)
//	{
//		alarm_synhro = 0;
//		alarm_count++;
//		myGLCD.print(DS3231_clock.dateFormat("s", dt), 190, 0);
//	}
//	isAlarm = true;
//}
boolean state;
//------------------------------------------------------------------------------

void setup(void) 
{
	if (ERROR_LED_PIN >= 0)
		{
			pinMode(ERROR_LED_PIN, OUTPUT);
		}
	Serial.begin(115200);
	printf_begin();
	Wire.begin();
	myGLCD.InitLCD();
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);                   // Синий фон кнопок
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(BigFont);
	pinMode(kn_red, INPUT);
	pinMode(kn_blue, INPUT);
	pinMode(intensityLCD, OUTPUT);
	pinMode(synhro_pin, INPUT);
	pinMode(LED_PIN13, OUTPUT);
	digitalWrite(LED_PIN13, LOW);
	digitalWrite(intensityLCD, LOW);
	pinMode(strob_pin, INPUT);
	digitalWrite(strob_pin, HIGH);
	digitalWrite(synhro_pin, HIGH);
	pinMode(alarm_pin, INPUT);
	digitalWrite(alarm_pin, HIGH);

	pinMode(vibro, OUTPUT);
	pinMode(sounder, OUTPUT);
	digitalWrite(vibro, LOW);
	digitalWrite(sounder, LOW);

	pinMode(kn_red, OUTPUT);
	pinMode(kn_blue, OUTPUT);
	digitalWrite(kn_red, HIGH);
	digitalWrite(kn_blue, HIGH);

	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);
	//myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
	ADC_MR |= 0x00000100 ; // ADC full speed


	chench_Channel();

	//adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
	Timer3.attachInterrupt(firstHandler);             // Timer3 - запись результатов измерения  в файл 
	Timer4.attachInterrupt(secondHandler);            // Timer4 - запись временных меток в файл
	Timer5.attachInterrupt(send_synhro);              // Timer5 - формирование временных меток синхронизации измерения
	Timer6.attachInterrupt(synhroHandler);            // Timer6 - формирование импульсов синхронизации измерения. Настройка
	Timer7.attachInterrupt(sevenHandler);             // Timer7 - запись временных меток  в массив для вывода на экран

	myGLCD.setBackColor(0, 0, 255);

	setup_resistor();                                    // Начальные установки резистора
	restore_volume();                                    // Восстановить настройки уровня громкости из памяти


	resistor(1, 80);                                    // Установить уровень сигнала
	resistor(2, 80);                                    // Установить уровень сигнала
	//resistor(1, volume1);                                // Установить уровень сигнала
	//resistor(2, volume2);                                // Установить уровень сигнала

	
//	i2c_eeprom_ulong_write(adr_set_timeSynhro, set_timeSynhro);             // Записать  время 
	//timePeriod = i2c_eeprom_ulong_read(adr_timePeriod);
	//set_timeSynhro = i2c_eeprom_ulong_read(adr_set_timeSynhro);               // Грубая подстройка синхро

	delay(500);
	kn = 0;
	setup_radio();
	sound1();
	delay(100);
	sound1();
	delay(100);
	vibro1();
	delay(100);
	vibro1();
	clean_mem();

	Serial.println();
	Serial.print("Pulse repetition period  = ");
	Serial.println(set_timeSynhro);
	Serial.println();

	DS3231_clock.begin();
	// Disarm alarms and clear alarms for this example, because alarms is battery backed.
	// Under normal conditions, the settings should be reset after power and restart microcontroller.
	DS3231_clock.armAlarm1(false);
	DS3231_clock.armAlarm2(false);
	DS3231_clock.clearAlarm1();
	DS3231_clock.clearAlarm2();

	DS3231_clock.setDateTime(__DATE__, __TIME__);
	//DS3231_clock.setDateTime(2017, 11, 15, 0, 0, 0);
	//	 disable 32kHz 
	//DS3231_clock.enable32kHz(false);


	//// Select output as rate to 1Hz
	//DS3231_clock.setOutput(DS3231_1HZ);

	//// Enable output
	//DS3231_clock.enableOutput(false);


	// Check config

	//if (DS3231_clock.isOutput())
	//{
	//	Serial.println("Oscilator is enabled");
	//}
	//else
	//{
	//	Serial.println("Oscilator is disabled");
	//}

	//switch (DS3231_clock.getOutput())
	//{
	//case DS3231_1HZ:     Serial.println("SQW = 1Hz"); break;
	//case DS3231_4096HZ:  Serial.println("SQW = 4096Hz"); break;
	//case DS3231_8192HZ:  Serial.println("SQW = 8192Hz"); break;
	//case DS3231_32768HZ: Serial.println("SQW = 32768Hz"); break;
	//default: Serial.println("SQW = Unknown"); break;
	//}



	//DS3231_clock.setAlarm1(0, 0, 0, 1, DS3231_EVERY_SECOND);   //DS3231_EVERY_SECOND //Каждую секунду
	//DS3231_clock.setAlarm1(0, 0, 0, 5, DS3231_MATCH_S);  // СОВПАДЕНИЕ 


	Serial.println(F("Setup Ok!"));

	myGLCD.setBackColor(0, 0, 0);                   // 
	myGLCD.setColor(255, 255, 255);
	myGLCD.setFont(SmallFont);
	//while (true)
	//{
	//	dt = DS3231_clock.getDateTime();
	//	if (oldsec != dt.second)
	//	{
	//		myGLCD.print(DS3231_clock.dateFormat("d-m-Y H:i:s", dt), 10, 0);
	//		oldsec = dt.second;
	//	}

	//	if (dt.second == 0|| dt.second == 10 || dt.second == 20 || dt.second == 30 || dt.second == 40 || dt.second == 50)
	//	{
	//		break;
	//	}
	//}


	//attachInterrupt(alarm_pin, alarmFunction, FALLING);
	attachInterrupt(kn_red, volume_up, FALLING);
	attachInterrupt(kn_blue, volume_down, FALLING);
	Interrupt_enable = true;
	draw_Start_Menu();
}

//------------------------------------------------------------------------------
void loop(void) 
{

	//draw_Start_Menu();
	Start_Menu();
	//AnalogClock();
	//dt = DS3231_clock.getDateTime();
	//Serial.println(DS3231_clock.dateFormat("d-m-Y H:i:s - l", dt));

	//if (isAlarm)
	//{
	//	//digitalWrite(alarmLED, alarmState);
	//	alarmState = !alarmState;
	//	DS3231_clock.clearAlarm1();
	//}

	//delay(1000);
}

