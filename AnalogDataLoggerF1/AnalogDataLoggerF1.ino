/**
 
 */

#define __SAM3X8E__

#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include "AnalogBinLogger.h"
#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <DueTimer.h>
#include <AH_AD9850.h>
#include "Wire.h"
#include <rtc_clock.h>
//#include <SD.h>





//+++++++++++++++++++++++ SD info ++++++++++++++++++++++++++
SdFile file;

File root;

SdFat sd;

SdBaseFile binFile;

Sd2Card card;

// Declare which fonts we will be using

extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];
extern uint8_t SmallSymbolFont[];

// Настройка монитора

UTFT myGLCD(ITDB32S,25,26,27,28);

UTouch        myTouch(6,5,4,3,2);
//
//// Finally we set up UTFT_Buttons :)
UTFT_Buttons  myButtons(&myGLCD, &myTouch);

boolean default_colors = true;
uint8_t menu_redraw_required = 0;

StdioStream csvStream;


//----------------------Конец  Настройки дисплея --------------------------------

//*********************Работа с именем файла ******************************
char file_name[13] ;
//char file_name_txt[5] = ".txt";
byte file_name_count = 0;
char str_day_file[3];
char str_day_file0[3];
char str_day_file10[3];
char str_mon_file[3];
char str_mon_file0[3];
char str_mon_file10[3];
char str_year_file[3];

char str_file_name_count[4];
char str_file_name_count0[4] = "0";
char str0[10];
char str1[10];
char str2[10];
char list_files_tab[200][13];
uint32_t size_files_tab[200] ;
int set_files = 0;

//**************************  Меню прибора ***************************************

const int clockCenterX=119;
const int clockCenterY=119;
int oldsec=0;
char* str[] = {"MON","TUE","WED","THU","FRI","SAT","SUN"};
char* str_mon[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

RTC_clock rtc_clock(XTAL);

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

int hh,mm,ss,dow,dd,mon,yyyy;


//******************Назначение переменных для хранения № опций меню (клавиш)****************************

 int but1, but2, but3, but4, but5, but6, but7, but8, but9, but10, butX, butY, but_m1, but_m2, but_m3, but_m4, but_m5, pressed_button;
 int m2 = 1; // Переменная номера меню
 
//=====================================================================================

	int x_kn = 30;  // Смещение кнопок по Х
	int dgvh;
	const int hpos = 95; //set 0v on horizontal  grid
	int port = 0;
	int x_osc,y_osc;
	int Input = 0;
	int Old_Input = 0;
	int Sample[254];
	int OldSample[254];
	int Sample_osc[254][4];
	int OldSample_osc[254][4];
	int PageSample_osc[240][10][4];
	unsigned long PageSample_Num[10];
	int Page_count = 0;
	int x_pos_count = 0;
	int YSample_osc[254][4];
	float VSample_osc[254][4];
	unsigned long LongFile = 0;
	float StartSample = 0; 
	float EndSample = 0;
	float koeff_h = 7.759*4;
	int MaxAnalog = 0;
	int MaxAnalog0 = 0;
	int MaxAnalog1 = 0;
	int MaxAnalog2 = 0;
	int MaxAnalog3 = 0;
	unsigned long SrednAnalog = 0;
	unsigned long SrednAnalog0 = 0;
	unsigned long SrednAnalog1 = 0;
	unsigned long SrednAnalog2 = 0;
	unsigned long SrednAnalog3 = 0;
	unsigned long SrednCount = 0;
	bool Set_x = false;
	bool osc_line_off0 = false;
	bool osc_line_off1 = false;
	bool osc_line_off2 = false;
	bool osc_line_off3 = false;
	bool repeat = false;
	int16_t count_repeat = 0;
	bool save_files = false;
	bool sled = false;
	int Set_ADC = 10;
	int MinAnalog = 500;
	int MinAnalog0 = 500;
	int MinAnalog1 = 500;
	int MinAnalog2 = 500;
	int MinAnalog3 = 500;
	int mode = 0;

	int mode1 = 0;             //Переключение чувствительности
	int dTime = 1;
	int tmode = 1;
	int t_in_mode = 0;
	int Trigger = 0;
	int SampleSize = 0;
	float SampleTime = 0;
	float v_const = 0.0008057;
	int x_measure = 0 ;              // Переменная для изменения частоты измерения источника питания
	bool strob_start = true;



 //***************** Назначение переменных для хранения текстов*****************************************************

char  txt_menu1_1[]          = "PE\x81\x86""CTPATOP";                                                       // "РЕГИСТРАТОР"
char  txt_menu1_2[]          = "CAMO\x89\x86""CE\x8C";                                                      // "САМОПИСЕЦ"
char  txt_menu1_3[]          = "PE\x81\x86""CT.+ CAMO\x89.";                                                // "РЕГИСТ. + САМОП."
char  txt_menu1_4[]          = "PA\x80OTA c SD";                                                            // "РАБОТА с SD"

char  txt_ADC_menu1[]        = "\x85""a\xA3\x9D""c\xAC \x99""a\xA2\xA2\xABx";                                                               //
char  txt_ADC_menu2[]        = "\x89poc\xA1o\xA4p \xA5""a\x9E\xA0""a";                                                                    //
char  txt_ADC_menu3[]        = "\x89""epe\x99""a\xA7""a \x97 KOM";                                                            //
char  txt_ADC_menu4[]        = "B\x91XO\x82";                                                                      //

char  txt_osc_menu1[]        = "Oc\xA6\x9D\xA0\xA0o\x98pa\xA5";                                                              //
char  txt_osc_menu2[]        = "Oc\xA6\x9D\xA0\xA0.1-18\xA1\x9D\xA2";                                                               //
char  txt_osc_menu3[]        = "O\xA8\x9d\x96\x9F\x9D";                                                                    //
char  txt_osc_menu4[]        = "B\x91XO\x82";           

char  txt_SD_menu1[]         = "\x89poc\xA1o\xA4p \xA5""a\x9E\xA0""a";                                                                 //
char  txt_SD_menu2[]         = "\x86\xA2\xA5o SD";                                                                   //
char  txt_SD_menu3[]         = "\x8Bop\xA1""a\xA4 SD";                                                                 //
char  txt_SD_menu4[]         = "B\x91XO\x82";           

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
char  txt_info18[]            = "Record time: "; 
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


void dateTime(uint16_t* date, uint16_t* time) // Программа записи времени и даты файла
{
  rtc_clock.get_time(&hh,&mm,&ss);
  rtc_clock.get_date(&dow,&dd,&mon,&yyyy);

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(yyyy, mon, dd);

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(hh, mm, ss);
}

//Настройка звукового генератора
#define CLK     8  // Назначение выводов генератора сигналов
#define FQUP    9  // Назначение выводов генератора сигналов
#define BitData 10 // Назначение выводов генератора сигналов
#define RESET   11 // Назначение выводов генератора сигналов
AH_AD9850 AD9850(CLK, FQUP, BitData, RESET);// настройка звукового генератора

// ADC speed one channel 480,000 samples/sec (no enable per read)
//           one channel 288,000 samples/sec (enable per read)

#define ADC_MR * (volatile unsigned int *) (0x400C0004) /*adc mode word*/
#define ADC_CR * (volatile unsigned int *) (0x400C0000) /*write a 2 to start convertion*/
#define ADC_ISR * (volatile unsigned int *) (0x400C0030) /*status reg -- bit 24 is data ready*/
#define ADC_ISR_DRDY 0x01000000

#define ADC_START 2
#define ADC_LCDR * (volatile unsigned int *) (0x400C0020) /*last converted low 12 bits*/
#define ADC_DATA 0x00000FFF 
#define ADC_STARTUP_FAST 12

#define ADC_CHER * (volatile unsigned int *) (0x400C0010) /*ADC Channel Enable Register  Только запись*/
#define ADC_CHSR * (volatile unsigned int *) (0x400C0018) /*ADC Channel Status Register  Только чтение */
//#define ADC_CDR0 * (volatile unsigned int *) (0x400C0050) /*ADC Channel Только чтение */
//#define ADC_ISR_EOC0 0x00000001

#define strob_pin A4    // Вход для запуска измерения

uint32_t ulChannel;

//------------------------------------------------------------------------------
// Analog pin number list for a sample. 

int Channel_x = 0;
int Channel_trig = 0;
bool Channel0 = true;
bool Channel1 = false;
bool Channel2 = false;
bool Channel3 = false;
int count_pin = 0;
int set_strob = 100;


const float SAMPLE_RATE = 5000;  // Must be 0.25 or greater.

// The interval between samples in seconds, SAMPLE_INTERVAL, may be set to a
// constant instead of being calculated from SAMPLE_RATE.  SAMPLE_RATE is not
// used in the code below.  For example, setting SAMPLE_INTERVAL = 2.0e-4
// will result in a 200 microsecond sample interval.
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
#define RECORD_EIGHT_BITS 0
//------------------------------------------------------------------------------
// Pin definitions.
//
// Digital pin to indicate an error, set to -1 if not used.
// The led blinks for fatal errors. The led goes on solid for SD write
// overrun errors and logging continues.
const int8_t ERROR_LED_PIN = 13;

// SD chip select pin.
const uint8_t SD_CS_PIN = 53;
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
ArduinoOutStream cout(Serial);

//++++++++++++++++++ SD Format ++++++++++++++++++++++++++++++++++
// Change spiSpeed to SPI_FULL_SPEED for better performance
// Use SPI_QUARTER_SPEED for even slower SPI bus speed
const uint8_t spiSpeed = SPI_HALF_SPEED;

//Sd2Card card;
uint32_t cardSizeBlocks;
uint16_t cardCapacityMB;

// cache for SD block
cache_t cache;

// MBR information
uint8_t partType;
uint32_t relSector;
uint32_t partSize;

// Fake disk geometry
uint8_t numberOfHeads;
uint8_t sectorsPerTrack;

// FAT parameters
uint16_t reservedSectors;
uint8_t sectorsPerCluster;
uint32_t fatStart;
uint32_t fatSize;
uint32_t dataStart;

// constants for file system structure
uint16_t const BU16 = 128;
uint16_t const BU32 = 8192;

//  strings needed in file system structures
char noName[] = "NO NAME    ";
char fat16str[] = "FAT16   ";
char fat32str[] = "FAT32   ";
//------------------------------------------------------------------------------
#define sdError(msg) sdError_F(F(msg))

void sdError_F(const __FlashStringHelper* str) {
  cout << F("error: ");
  cout << str << endl;
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("Error: ", CENTER, 80);
  myGLCD.print(str, CENTER, 120);
  if (card.errorCode()) {
	cout << F("SD error: ") << hex << int(card.errorCode());
	cout << ',' << int(card.errorData()) << dec << endl;
  }
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info11,CENTER, 200);
	myGLCD.setColor(255, 255, 255);
	while (!myTouch.dataAvailable()){}
	while (myTouch.dataAvailable()){}

 // while (1);
}
//------------------------------------------------------------------------------
#if DEBUG_PRINT
void debugPrint() 
{
  cout << F("FreeRam: ") << FreeRam() << endl;
  cout << F("partStart: ") << relSector << endl;
  cout << F("partSize: ") << partSize << endl;
  cout << F("reserved: ") << reservedSectors << endl;
  cout << F("fatStart: ") << fatStart << endl;
  cout << F("fatSize: ") << fatSize << endl;
  cout << F("dataStart: ") << dataStart << endl;
  cout << F("clusterCount: ");
  cout << ((relSector + partSize - dataStart)/sectorsPerCluster) << endl;
  cout << endl;
  cout << F("Heads: ") << int(numberOfHeads) << endl;
  cout << F("Sectors: ") << int(sectorsPerTrack) << endl;
  cout << F("Cylinders: ");
  cout << cardSizeBlocks/(numberOfHeads*sectorsPerTrack) << endl;
}
#endif  // DEBUG_PRINT
//------------------------------------------------------------------------------
// write cached block to the card
uint8_t writeCache(uint32_t lbn) {
  return card.writeBlock(lbn, cache.data);
}
//------------------------------------------------------------------------------
// initialize appropriate sizes for SD capacity
void initSizes()
{
  if (cardCapacityMB <= 6) {
	sdError("Card is too small.");
  } else if (cardCapacityMB <= 16) {
	sectorsPerCluster = 2;
  } else if (cardCapacityMB <= 32) {
	sectorsPerCluster = 4;
  } else if (cardCapacityMB <= 64) {
	sectorsPerCluster = 8;
  } else if (cardCapacityMB <= 128) {
	sectorsPerCluster = 16;
  } else if (cardCapacityMB <= 1024) {
	sectorsPerCluster = 32;
  } else if (cardCapacityMB <= 32768) {
	sectorsPerCluster = 64;
  } else {
	// SDXC cards
	sectorsPerCluster = 128;
  }

  cout << F("Blocks/Cluster: ") << int(sectorsPerCluster) << endl;
  // set fake disk geometry
  sectorsPerTrack = cardCapacityMB <= 256 ? 32 : 63;

  if (cardCapacityMB <= 16) {
	numberOfHeads = 2;
  } else if (cardCapacityMB <= 32) {
	numberOfHeads = 4;
  } else if (cardCapacityMB <= 128) {
	numberOfHeads = 8;
  } else if (cardCapacityMB <= 504) {
	numberOfHeads = 16;
  } else if (cardCapacityMB <= 1008) {
	numberOfHeads = 32;
  } else if (cardCapacityMB <= 2016) {
	numberOfHeads = 64;
  } else if (cardCapacityMB <= 4032) {
	numberOfHeads = 128;
  } else {
	numberOfHeads = 255;
  }
}
//------------------------------------------------------------------------------
// zero cache and optionally set the sector signature
void clearCache(uint8_t addSig) {
  memset(&cache, 0, sizeof(cache));
  if (addSig) {
	cache.mbr.mbrSig0 = BOOTSIG0;
	cache.mbr.mbrSig1 = BOOTSIG1;
  }
}
//------------------------------------------------------------------------------
// zero FAT and root dir area on SD
void clearFatDir(uint32_t bgn, uint32_t count) 
{
  clearCache(false);
  if (!card.writeStart(bgn, count)) {
	sdError("Clear FAT/DIR writeStart failed");
  }
  for (uint32_t i = 0; i < count; i++) {
	if ((i & 0XFF) == 0) {
	  cout << '.';
	}
	if (!card.writeData(cache.data)) {
	  sdError("Clear FAT/DIR writeData failed");
	}
  }
  if (!card.writeStop()) {
	sdError("Clear FAT/DIR writeStop failed");
  }
  cout << endl;
}
//------------------------------------------------------------------------------
// return cylinder number for a logical block number
uint16_t lbnToCylinder(uint32_t lbn) 
{
  return lbn / (numberOfHeads * sectorsPerTrack);
}
//------------------------------------------------------------------------------
// return head number for a logical block number
uint8_t lbnToHead(uint32_t lbn) 
{
  return (lbn % (numberOfHeads * sectorsPerTrack)) / sectorsPerTrack;
}
//------------------------------------------------------------------------------
// return sector number for a logical block number
uint8_t lbnToSector(uint32_t lbn) 
{
  return (lbn % sectorsPerTrack) + 1;
}
//------------------------------------------------------------------------------
// format and write the Master Boot Record
void writeMbr() 
{
  clearCache(true);
  part_t* p = cache.mbr.part;
  p->boot = 0;
  uint16_t c = lbnToCylinder(relSector);
  if (c > 1023) {
	sdError("MBR CHS");
  }
  p->beginCylinderHigh = c >> 8;
  p->beginCylinderLow = c & 0XFF;
  p->beginHead = lbnToHead(relSector);
  p->beginSector = lbnToSector(relSector);
  p->type = partType;
  uint32_t endLbn = relSector + partSize - 1;
  c = lbnToCylinder(endLbn);
  if (c <= 1023) {
	p->endCylinderHigh = c >> 8;
	p->endCylinderLow = c & 0XFF;
	p->endHead = lbnToHead(endLbn);
	p->endSector = lbnToSector(endLbn);
  } else {
	// Too big flag, c = 1023, h = 254, s = 63
	p->endCylinderHigh = 3;
	p->endCylinderLow = 255;
	p->endHead = 254;
	p->endSector = 63;
  }
  p->firstSector = relSector;
  p->totalSectors = partSize;
  if (!writeCache(0)) {
	sdError("write MBR");
  }
}
//------------------------------------------------------------------------------
// generate serial number from card size and micros since boot
uint32_t volSerialNumber() 
{
  return (cardSizeBlocks << 8) + micros();
}
//------------------------------------------------------------------------------
// format the SD as FAT16
void makeFat16() 
{
  uint32_t nc;
  for (dataStart = 2 * BU16;; dataStart += BU16) {
	nc = (cardSizeBlocks - dataStart)/sectorsPerCluster;
	fatSize = (nc + 2 + 255)/256;
	uint32_t r = BU16 + 1 + 2 * fatSize + 32;
	if (dataStart < r) {
	  continue;
	}
	relSector = dataStart - r + BU16;
	break;
  }
  // check valid cluster count for FAT16 volume
  if (nc < 4085 || nc >= 65525) {
	sdError("Bad cluster count");
  }
  reservedSectors = 1;
  fatStart = relSector + reservedSectors;
  partSize = nc * sectorsPerCluster + 2 * fatSize + reservedSectors + 32;
  if (partSize < 32680) {
	partType = 0X01;
  } else if (partSize < 65536) {
	partType = 0X04;
  } else {
	partType = 0X06;
  }
  // write MBR
  writeMbr();
  clearCache(true);
  fat_boot_t* pb = &cache.fbs;
  pb->jump[0] = 0XEB;
  pb->jump[1] = 0X00;
  pb->jump[2] = 0X90;
  for (uint8_t i = 0; i < sizeof(pb->oemId); i++) {
	pb->oemId[i] = ' ';
  }
  pb->bytesPerSector = 512;
  pb->sectorsPerCluster = sectorsPerCluster;
  pb->reservedSectorCount = reservedSectors;
  pb->fatCount = 2;
  pb->rootDirEntryCount = 512;
  pb->mediaType = 0XF8;
  pb->sectorsPerFat16 = fatSize;
  pb->sectorsPerTrack = sectorsPerTrack;
  pb->headCount = numberOfHeads;
  pb->hidddenSectors = relSector;
  pb->totalSectors32 = partSize;
  pb->driveNumber = 0X80;
  pb->bootSignature = EXTENDED_BOOT_SIG;
  pb->volumeSerialNumber = volSerialNumber();
  memcpy(pb->volumeLabel, noName, sizeof(pb->volumeLabel));
  memcpy(pb->fileSystemType, fat16str, sizeof(pb->fileSystemType));
  // write partition boot sector
  if (!writeCache(relSector)) {
	sdError("FAT16 write PBS failed");
  }
  // clear FAT and root directory
  clearFatDir(fatStart, dataStart - fatStart);
  clearCache(false);
  cache.fat16[0] = 0XFFF8;
  cache.fat16[1] = 0XFFFF;
  // write first block of FAT and backup for reserved clusters
  if (!writeCache(fatStart)
	  || !writeCache(fatStart + fatSize)) {
	sdError("FAT16 reserve failed");
  }
}
//------------------------------------------------------------------------------
// format the SD as FAT32
void makeFat32() 
{
  uint32_t nc;
  relSector = BU32;
  for (dataStart = 2 * BU32;; dataStart += BU32) {
	nc = (cardSizeBlocks - dataStart)/sectorsPerCluster;
	fatSize = (nc + 2 + 127)/128;
	uint32_t r = relSector + 9 + 2 * fatSize;
	if (dataStart >= r) {
	  break;
	}
  }
  // error if too few clusters in FAT32 volume
  if (nc < 65525) {
	sdError("Bad cluster count");
  }
  reservedSectors = dataStart - relSector - 2 * fatSize;
  fatStart = relSector + reservedSectors;
  partSize = nc * sectorsPerCluster + dataStart - relSector;
  // type depends on address of end sector
  // max CHS has lbn = 16450560 = 1024*255*63
  if ((relSector + partSize) <= 16450560) {
	// FAT32
	partType = 0X0B;
  } else {
	// FAT32 with INT 13
	partType = 0X0C;
  }
  writeMbr();
  clearCache(true);

  fat32_boot_t* pb = &cache.fbs32;
  pb->jump[0] = 0XEB;
  pb->jump[1] = 0X00;
  pb->jump[2] = 0X90;
  for (uint8_t i = 0; i < sizeof(pb->oemId); i++) {
	pb->oemId[i] = ' ';
  }
  pb->bytesPerSector = 512;
  pb->sectorsPerCluster = sectorsPerCluster;
  pb->reservedSectorCount = reservedSectors;
  pb->fatCount = 2;
  pb->mediaType = 0XF8;
  pb->sectorsPerTrack = sectorsPerTrack;
  pb->headCount = numberOfHeads;
  pb->hidddenSectors = relSector;
  pb->totalSectors32 = partSize;
  pb->sectorsPerFat32 = fatSize;
  pb->fat32RootCluster = 2;
  pb->fat32FSInfo = 1;
  pb->fat32BackBootBlock = 6;
  pb->driveNumber = 0X80;
  pb->bootSignature = EXTENDED_BOOT_SIG;
  pb->volumeSerialNumber = volSerialNumber();
  memcpy(pb->volumeLabel, noName, sizeof(pb->volumeLabel));
  memcpy(pb->fileSystemType, fat32str, sizeof(pb->fileSystemType));
  // write partition boot sector and backup
  if (!writeCache(relSector)
	  || !writeCache(relSector + 6)) {
	sdError("FAT32 write PBS failed");
  }
  clearCache(true);
  // write extra boot area and backup
  if (!writeCache(relSector + 2)
	  || !writeCache(relSector + 8)) {
	sdError("FAT32 PBS ext failed");
  }
  fat32_fsinfo_t* pf = &cache.fsinfo;
  pf->leadSignature = FSINFO_LEAD_SIG;
  pf->structSignature = FSINFO_STRUCT_SIG;
  pf->freeCount = 0XFFFFFFFF;
  pf->nextFree = 0XFFFFFFFF;
  // write FSINFO sector and backup
  if (!writeCache(relSector + 1)
	  || !writeCache(relSector + 7)) {
	sdError("FAT32 FSINFO failed");
  }
  clearFatDir(fatStart, 2 * fatSize + sectorsPerCluster);
  clearCache(false);
  cache.fat32[0] = 0x0FFFFFF8;
  cache.fat32[1] = 0x0FFFFFFF;
  cache.fat32[2] = 0x0FFFFFFF;
  // write first block of FAT and backup for reserved clusters
  if (!writeCache(fatStart)
	  || !writeCache(fatStart + fatSize)) {
	sdError("FAT32 reserve failed");
  }
}
//------------------------------------------------------------------------------
// flash erase all data
void eraseCard()
{
  cout << endl << F("Erasing\n");
  uint32_t firstBlock = 0;
  uint32_t lastBlock;
  uint16_t n = 0;

  do {
	lastBlock = firstBlock + ERASE_SIZE - 1;
	if (lastBlock >= cardSizeBlocks) {
	  lastBlock = cardSizeBlocks - 1;
	}
	if (!card.erase(firstBlock, lastBlock)) {
	  sdError("erase failed");
	}
	cout << '.';
	if ((n++)%32 == 31) {
	  cout << endl;
	}
	firstBlock += ERASE_SIZE;
  } while (firstBlock < cardSizeBlocks);
  cout << endl;

  if (!card.readBlock(0, cache.data)) {
	sdError("readBlock");
  }
  cout << hex << showbase << setfill('0') << internal;
  cout << F("All data set to ") << setw(4) << int(cache.data[0]) << endl;
  cout << dec << noshowbase << setfill(' ') << right;
  cout << F("Erase done\n");
}
//------------------------------------------------------------------------------
void formatCard() 
{
  cout << endl;
  cout << F("Formatting\n");
  initSizes();
  if (card.type() != SD_CARD_TYPE_SDHC) {
	cout << F("FAT16\n");
	makeFat16();
  } else {
	cout << F("FAT32\n");
	makeFat32();
  }
#if DEBUG_PRINT
  debugPrint();
#endif  // DEBUG_PRINT
  cout << F("Format done\n");
}
//-------------------SD info ---------------------------------------------------

// global for card size
uint32_t cardSize;

// global for card erase size
uint32_t eraseSize;
//------------------------------------------------------------------------------
// store error strings in flash
#define sdErrorMsg(msg) sdErrorMsg_F(F(msg));
void sdErrorMsg_F(const __FlashStringHelper* str) 
{
  cout << str << endl;
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("Error: ", CENTER, 80);
  myGLCD.print(str, CENTER, 120);

  if (sd.card()->errorCode()) {
	cout << F("SD errorCode: ");
	cout << hex << int(sd.card()->errorCode()) << endl;
	cout << F("SD errorData: ");
	cout << int(sd.card()->errorData()) << dec << endl;
  }
  delay(3000);
}
//------------------------------------------------------------------------------
uint8_t cidDmp() 
{
  cid_t cid;
  if (!sd.card()->readCID(&cid)) {
	sdErrorMsg("readCID failed");
	return false;
  }
  cout << F("\nManufacturer ID: ");
  cout << hex << int(cid.mid) << dec << endl;
  cout << F("OEM ID: ") << cid.oid[0] << cid.oid[1] << endl;
  cout << F("Product: ");
  for (uint8_t i = 0; i < 5; i++) 
  {
	cout << cid.pnm[i];
  }
  cout << F("\nVersion: ");
  cout << int(cid.prv_n) << '.' << int(cid.prv_m) << endl;
  cout << F("Serial number: ") << hex << cid.psn << dec << endl;
  cout << F("Manufacturing date: ");
  cout << int(cid.mdt_month) << '/';
  cout << (2000 + cid.mdt_year_low + 10 * cid.mdt_year_high) << endl;
  cout << endl;
  return true;
}
//------------------------------------------------------------------------------
uint8_t csdDmp() {
  csd_t csd;
  uint8_t eraseSingleBlock;
  if (!sd.card()->readCSD(&csd)) {
	sdErrorMsg("readCSD failed");
	return false;
  }
  if (csd.v1.csd_ver == 0) {
	eraseSingleBlock = csd.v1.erase_blk_en;
	eraseSize = (csd.v1.sector_size_high << 1) | csd.v1.sector_size_low;
  } else if (csd.v2.csd_ver == 1) {
	eraseSingleBlock = csd.v2.erase_blk_en;
	eraseSize = (csd.v2.sector_size_high << 1) | csd.v2.sector_size_low;
  } else {
	cout << F("csd version error\n");
	return false;
  }
  eraseSize++;
  cout << F("cardSize: ") << 0.000512*cardSize;
   myGLCD.print("cardSize: ", LEFT, 40);
  myGLCD.printNumI(0.000512*cardSize, RIGHT-60 , 40);
   myGLCD.print("MB", RIGHT, 40);

  cout << F(" MB (MB = 1,000,000 bytes)\n");

  cout << F("flashEraseSize: ") << int(eraseSize) << F(" blocks\n");
  cout << F("eraseSingleBlock: ");
  if (eraseSingleBlock) {
	cout << F("true\n");
  } else {
	cout << F("false\n");
  }
  return true;
}
//------------------------------------------------------------------------------
// print partition table
uint8_t partDmp() {
  cache_t *p = sd.vol()->cacheClear();
  if (!p) {
	sdErrorMsg("cacheClear failed");
	return false;
  }
  if (!sd.card()->readBlock(0, p->data)) {
	sdErrorMsg("read MBR failed");
	return false;
  }
  for (uint8_t ip = 1; ip < 5; ip++) {
	part_t *pt = &p->mbr.part[ip - 1];
	if ((pt->boot & 0X7F) != 0 || pt->firstSector > cardSize) {
	  cout << F("\nNo MBR. Assuming Super Floppy format.\n");
	  return true;
	}
  }
  cout << F("\nSD Partition Table\n");
  cout << F("part,boot,type,start,length\n");
  for (uint8_t ip = 1; ip < 5; ip++) {
	part_t *pt = &p->mbr.part[ip - 1];
	cout << int(ip) << ',' << hex << int(pt->boot) << ',' << int(pt->type);
	cout << dec << ',' << pt->firstSector <<',' << pt->totalSectors << endl;
  }
  return true;
}
//------------------------------------------------------------------------------
void volDmp() {
  cout << F("\nVolume is FAT") << int(sd.vol()->fatType()) << endl;
  myGLCD.print("Volume is      FAT", LEFT, 60);
  int volFAT = sd.vol()->fatType();
  myGLCD.printNumI(volFAT, RIGHT , 60);
  cout << F("blocksPerCluster: ") << int(sd.vol()->blocksPerCluster()) << endl;
  cout << F("clusterCount: ") << sd.vol()->clusterCount() << endl;
  cout << F("freeClusters: ");
  uint32_t volFree = sd.vol()->freeClusterCount();
  cout <<  volFree << endl;
  float fs = 0.000512*volFree*sd.vol()->blocksPerCluster();
  cout << F("freeSpace: ") << fs << F(" MB (MB = 1,000,000 bytes)\n");
   myGLCD.print("freeSpace: ", LEFT, 80);
  myGLCD.printNumI(fs, RIGHT-60 , 80);
   myGLCD.print("MB", RIGHT, 80);
  cout << F("fatStartBlock: ") << sd.vol()->fatStartBlock() << endl;
  cout << F("fatCount: ") << int(sd.vol()->fatCount()) << endl;
  cout << F("blocksPerFat: ") << sd.vol()->blocksPerFat() << endl;
  cout << F("rootDirStart: ") << sd.vol()->rootDirStart() << endl;
  cout << F("dataStartBlock: ") << sd.vol()->dataStartBlock() << endl;
  if (sd.vol()->dataStartBlock() % eraseSize) {
	cout << F("Data area is not aligned on flash erase boundaries!\n");
	cout << F("Download and use formatter from www.sdsd.card()->org/consumer!\n");
  }
}
void  SD_info()
{
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
 // myGLCD.print(txt_info12, CENTER, 2);
  delay(400);  // catch Due reset problem

 // uint32_t t = millis();
   uint32_t t = micros();
  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
 // if (!sd.cardBegin(SD_CS_PIN, SPI_HALF_SPEED)) 
  if (!sd.cardBegin(SD_CS_PIN, SPI_FULL_SPEED)) 
  {
	sdErrorMsg("\ncardBegin failed");
	myGLCD.print("cardBegin failed", LEFT, 100);
	//return;
  }
  t = micros() - t;
 //   t = millis() - t;
  cardSize = sd.card()->cardSize();
  if (cardSize == 0) {
	sdErrorMsg("cardSize failed");
	myGLCD.print("cardSize failed", LEFT, 120);
	//return;
  }
  cout << F("\ninit time: ") << t << " us" << endl;
  myGLCD.print("Init time : ", LEFT, 0);
  myGLCD.printNumI(t, RIGHT-60 , 0);
   myGLCD.print("us", RIGHT, 0);
  cout << F("\nCard type: ");
  myGLCD.print("Card type: ", LEFT, 20);
  switch (sd.card()->type()) {
  case SD_CARD_TYPE_SD1:
	cout << F("SD1\n");
	 myGLCD.print("SD1", RIGHT , 20);
	break;

  case SD_CARD_TYPE_SD2:
	cout << F("SD2\n");
	 myGLCD.print("SD2", RIGHT , 20);
	break;

  case SD_CARD_TYPE_SDHC:
	if (cardSize < 70000000) 
	{
	  cout << F("SDHC\n");
	   myGLCD.print("SDHC", RIGHT , 20);
	} else {
	  cout << F("SDXC\n");
	   myGLCD.print("SDXC", RIGHT , 20);
	}
	break;

  default:
	cout << F("Unknown\n");
	 myGLCD.print("Unknown", RIGHT , 20);
  }
  if (!cidDmp()) 
  {
	return;
  }
  if (!csdDmp()) 
  {
	return;
  }
  uint32_t ocr;
  if (!sd.card()->readOCR(&ocr)) 
  {
	sdErrorMsg("\nreadOCR failed");
	 myGLCD.print("readOCR failed", LEFT, 140);
	//return;
  }
  cout << F("OCR: ") << hex << ocr << dec << endl;
  if (!partDmp()) {
	return;
  }
  if (!sd.fsBegin()) {
	sdErrorMsg("\nFile System initialization failed.\n");
	myGLCD.print("File System failed", LEFT, 160);
	//return;
  }
  volDmp();

	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info11,CENTER, 200);
	myGLCD.setColor(255, 255, 255);
	while (!myTouch.dataAvailable()){}
	while (myTouch.dataAvailable()){}
//	Draw_menu_ADC1();

}

//------------------------------------------------------------------------------
// store error strings in flash
#define sdErrorMsg(msg) sdErrorMsg_P(PSTR(msg));
void sdErrorMsg_P(const char* str) 
{
  cout << pgm(str) << endl;
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.print("Error: ", CENTER, 80);
  myGLCD.print(str, CENTER, 120);
  if (card.errorCode()) {
	cout << pstr("SD errorCode: ");
	cout << hex << int(card.errorCode()) << endl;
	cout << pstr("SD errorData: ");
	cout << int(card.errorData()) << dec << endl;
  }
	delay(2000);
}
//------------------------------------------------------------------------------

char binName[13] = FILE_BASE_NAME "00.BIN";
char timeName[13] = FILE_BASE_NAME_TIME "00.CSV";
size_t SAMPLES_PER_BLOCK ;//= DATA_DIM16/PIN_COUNT; // 254 разделить на количество входов
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
// Interrupt Service Routines

// Pointer to current buffer.
block_t* isrBuf;

// Need new buffer if true. Необходим новый  буфер, если true
bool isrBufNeeded = true;

// overrun count
uint16_t isrOver = 0;

// Insure no timer events are missed.
volatile bool timerError = false;
volatile bool timerFlag = false;
//------------------------------------------------------------------------------
bool ledOn = false;
void firstHandler()
{
	 int time_start = micros();
	//ADC_CHER = Channel_x;    // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3    
	ADC_CR = ADC_START ; 	// Запустить преобразование

  // while (!(ADC_ISR & ADC_ISR_DRDY));

  if (isrBufNeeded && emptyHead == emptyTail)   //  Необходим новый  буфер, если true 
	  {
		// no buffers - count overrun нет буферов - рассчитывайте перерасход
		if (isrOver < 0XFFFF) isrOver++;
	
		// Avoid missed timer error. Избежать пропущенных ошибку таймера.
		timerFlag = false;
		return;
	  }

 // Check for buffer needed.  Проверьте буфера, необходимого.
  if (isrBufNeeded) 
	  {   
		// Remove buffer from empty queue. Удалить буфер из пустого очереди.
		isrBuf = emptyQueue[emptyTail];
		emptyTail = queueNext(emptyTail);
		isrBuf->count = 0;            // Счнтчик в 0
		isrBuf->overrun = isrOver;    // 
		isrBufNeeded = false;    
	  }
  // Store ADC data.
   //while (!(ADC_ISR & ADC_ISR_DRDY));
  while (!(ADC_ISR_DRDY));
	  //  int time_period = micros() - time_start;
	 //   isrBuf->data[isrBuf->count++] = time_period;
		if (Channel0) isrBuf->data[isrBuf->count++] = ADC->ADC_CDR[7];
		if (Channel1) isrBuf->data[isrBuf->count++] = ADC->ADC_CDR[6];
		if (Channel2) isrBuf->data[isrBuf->count++] = ADC->ADC_CDR[5];
		if (Channel3) isrBuf->data[isrBuf->count++] = ADC->ADC_CDR[4];
 
  if (isrBuf->count >= count_pin*SAMPLES_PER_BLOCK)  //  SAMPLES_PER_BLOCK -количество памяти выделенное на  один вход 
								 // SAMPLES_PER_BLOCK = DATA_DIM16/PIN_COUNT; // 254 разделить на количество входов
	  {
		// Put buffer isrIn full queue.  Положите буфер isrIn полной очереди.
		uint8_t tmp = fullHead;  // Avoid extra fetch of volatile fullHead.
		fullQueue[tmp] = (block_t*)isrBuf;
		fullHead = queueNext(tmp);
		// Set buffer needed and clear overruns.
		isrBufNeeded = true;
		isrOver = 0;
	  }
 
}
void secondHandler()
{
	Serial.println("[ - ] Second Handler!");
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
 // fatalBlink();
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
	
		if (Channel0 )
			{
				meta->pinNumber[i] = 0;
				i++;
			}
		if (Channel1 )
			{
				meta->pinNumber[i] = 1;
				i++;
			}
		
		if (Channel2 ) 
			{
				meta->pinNumber[i] = 2;
				i++;
			}

		if (Channel3 ) 
			{
			   meta->pinNumber[i] = 3;
			}

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
	//  Serial.print(F("Sample pins:"));
	/*  for (int i = 0; i < meta->pinCount; i++) 
	  {
		Serial.print(' ');
		Serial.print(meta->pinNumber[i], DEC);
	  }
 
	  Serial.println(); 
	  Serial.println(F("ADC bits: 12 "));
	  Serial.print(F("ADC interval usec: "));
	  Serial.println(set_strob);*/
	  //Serial.print(F("Sample Rate: "));
	  //Serial.println(sampleRate);  
	  //Serial.print(F("Sample interval usec: "));
	  //Serial.println(1000000.0/sampleRate, 4); 
}
void adcStart() {
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

void binaryToCsv() 
{
	uint8_t lastPct = 0;
	block_t buf;
	metadata_t* pm;
	uint32_t t0 = millis();
	char csvName[13];
 
	if (!binFile.isOpen()) 
		{
			//Serial.println(F("No current binary file"));
			myGLCD.clrScr();
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print("Error: ", CENTER, 80);
			myGLCD.print("No binary file", CENTER, 120);
			delay(2000);
			Draw_menu_ADC1();
			return;
		}
	binFile.rewind();
	if (!binFile.read(&buf , 512) == 512) error("Read metadata failed");
	// Create a new CSV file.
	strcpy(csvName, binName);
	strcpy_P(&csvName[BASE_NAME_SIZE + 3], PSTR("CSV"));

	if (!csvStream.fopen(csvName, "w")) 
		{
			error("open csvStream failed");  
		}
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info7,LEFT, 145);  //
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(csvName,RIGHT, 145);   // 
	myGLCD.setColor(255, 255, 255);
	pm = (metadata_t*)&buf;
	csvStream.print(F("File - "));
	csvStream.println(F(csvName));
	csvStream.print(F("Interval - "));
	float intervalMicros = set_strob;
	csvStream.print(intervalMicros, 4);
	csvStream.println(F(",usec"));
	csvStream.print(F("Step = "));
	csvStream.print(v_const, 8);
	csvStream.println(F(" volt"));
	csvStream.print(F("Data : "));
	rtc_clock.get_time(&hh,&mm,&ss);
	rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
	dow1=dow;
	sec = ss;       //Initialization time
	min = mm;
	hour = hh;
	date = dd;
	mon1 = mon;
	year = yyyy;

	csvStream.print(date);
	csvStream.print(F("/"));
	csvStream.print(mon1);
	csvStream.print(F("/"));
	csvStream.print(year);
	csvStream.print(F("   "));

	csvStream.print(hour);
	csvStream.print(F(":"));
	csvStream.print(min);
	csvStream.print(F(":"));
	csvStream.print(sec);
	csvStream.println(); 
	csvStream.print(F("@"));                  // Признак начала определения количества входов  
	for (uint8_t i = 0; i < pm->pinCount; i++) 
		{
			if (i) csvStream.putc(',');
			csvStream.print(F("pin "));
			csvStream.print(pm->pinNumber[i]);
		}
	csvStream.println(); 
	csvStream.println('#');                  // Признак начала данных
	myGLCD.setColor(255, 255, 255);
	myGLCD.print("Converting:",2, 165);      //
 
	uint32_t tPct = millis();

  while (!Serial.available() && binFile.read(&buf, 512) == 512) 
  {
	uint16_t i;
	if (buf.count == 0) break;
	//if (buf.overrun) 
	//{
	//  csvStream.print(F("OVERRUN,"));
	//  csvStream.println(buf.overrun);     
	//}
	for (uint16_t j = 0; j < buf.count; j += count_pin) 
	{
	  for (uint16_t i = 0; i < count_pin; i++) 
	  {
		if (i) csvStream.putc(',');
		csvStream.print(buf.data[i + j]);     
	  }
	  csvStream.println();
	}
	if ((millis() - tPct) > 1000) 
	{
	  uint8_t pct = binFile.curPosition()/(binFile.fileSize()/100);
	  if (pct != lastPct) 
	  {
		tPct = millis();
		lastPct = pct;
		myGLCD.setColor(VGA_YELLOW);
		myGLCD.printNumI(pct, 180, 165);       // 
		myGLCD.print(txt_info8,215, 165);     //
		myGLCD.setColor(255, 255, 255);
	  }
	}
	if (myTouch.dataAvailable()) break;
  }

	csvStream.println();                       // Признак  5555 окончания данных в файле
	csvStream.print('5');                      // Признак  5555 окончания данных в файле
	csvStream.print('5');                      // Признак  5555 окончания данных в файле
	csvStream.print('5');                      // Признак  5555 окончания данных в файле
	csvStream.print('5');                      // Признак  5555 окончания данных в файле
	csvStream.println(); 
	csvStream.print("Time measure = ");
	rtc_clock.get_time(&hh,&mm,&ss);
	rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
	dow1=dow;
	sec = ss;       //Initialization time
	min = mm;
	hour = hh;
	date = dd;
	mon1 = mon;
	year = yyyy;

	csvStream.print(date);
	csvStream.print(F("/"));
	csvStream.print(mon1);
	csvStream.print(F("/"));
	csvStream.print(year);
	csvStream.print(F("   "));

	csvStream.print(hour);
	csvStream.print(F(":"));
	csvStream.print(min);
	csvStream.print(F(":"));
	csvStream.print(sec);
	csvStream.println(); 

	csvStream.fclose();  
	//	binFile.remove();
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info9,2, 185);   
	myGLCD.setColor(VGA_YELLOW);   //
	myGLCD.printNumF((0.001*(millis() - t0)),2, 90, 185);// 
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info10, 210, 185);//
}
void checkOverrun() 
{
  bool headerPrinted = false;
  block_t buf;
  uint32_t bgnBlock, endBlock;
  uint32_t bn = 0;
  
  if (!binFile.isOpen()) 
  {
	//Serial.println(F("No current binary file"));
		myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
//	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255, 255);
	myGLCD.print("No binary file", CENTER, 120);
	delay(2000);
	return;
  }
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
	error("contiguousRange failed");
  }
  binFile.rewind();
 // Serial.println();
//  Serial.println(F("Checking overrun errors - type any character to stop"));
  if (!binFile.read(&buf , 512) == 512) {
	error("Read metadata failed");
  }
  bn++;
  while (binFile.read(&buf, 512) == 512) {
	if (buf.count == 0) break;
	if (buf.overrun) 
	{
	  if (!headerPrinted) 
	  {
		//Serial.println();
		//Serial.println(F("Overruns:"));
		//Serial.println(F("fileBlockNumber,sdBlockNumber,overrunCount"));
		headerPrinted = true;
	  }
	/*  Serial.print(bn);
	  Serial.print(',');
	  Serial.print(bgnBlock + bn);
	  Serial.print(',');
	  Serial.println(buf.overrun);*/
	}
	bn++;
  }
  if (!headerPrinted) {
	//Serial.println(F("No errors found"));
  } else {
	//Serial.println(F("Done"));
  }
}
void dumpData() 
{
	block_t buf;
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info12,CENTER, 40);

  if (!binFile.isOpen()) 
  {
	Serial.println(F("No current binary file"));
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("Error: ", CENTER, 80);
	myGLCD.print("No binary file", CENTER, 120);
	delay(2000);
	Draw_menu_ADC1();
	return;
  }
  binFile.rewind();
  if (binFile.read(&buf , 512) != 512) 
  {
	error("Read metadata failed");
  }
  Serial.println();
  Serial.println(F("Type any character to stop"));
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info11,CENTER, 200);
	myGLCD.setColor(255, 255, 255);
  delay(1000);
  while (!myTouch.dataAvailable() && binFile.read(&buf , 512) == 512) 
  {
	if (buf.count == 0) break;
	if (buf.overrun) 
	{
	  Serial.print(F("OVERRUN,"));
	  Serial.println(buf.overrun);
	}
	for (uint16_t i = 0; i < buf.count; i++) 
	{
	  Serial.print(buf.data[i], DEC);
	  if ((i+1)%count_pin) 
	  {
		Serial.print(',');
	  } else {
		Serial.println();
	  }
	}
  }
  Serial.println(F("Done"));
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(txt_info9,CENTER, 80);
	myGLCD.setColor(255, 255, 255);
	delay(500);
	while (myTouch.dataAvailable()){}
	Draw_menu_ADC1();


}
void dumpData_Osc()
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info12,CENTER, 40);
	block_t buf;
	uint32_t count = 0;
	uint32_t count1 = 0;
	koeff_h = 7.759*4;
	int xpos = 0;
	int ypos1;
	int ypos2;
	int kl[buf.count];         //Текущий блок

	if (!binFile.isOpen()) 
		{
			Serial.println(F("No current binary file"));
			return;
		}

	binFile.rewind();
	if (binFile.read(&buf , 512) != 512) 
	{
		error("Read metadata failed");
	}
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info11,CENTER, 200);
	myGLCD.setColor(255, 255, 255);
	delay(1000);
	myGLCD.clrScr();
	LongFile = 0;
	DrawGrid1();
	myGLCD.setColor(0, 0, 255);
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.fillRoundRect (250, 90, 310, 130);
	myGLCD.setColor( 255, 255, 255);
	myGLCD.drawRoundRect (250, 90, 310, 130);
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setColor( 255, 255, 255);
	myGLCD.setFont( SmallFont);
	myGLCD.print("V/del.", 260, 95);
	myGLCD.print("      ", 260, 110);
	if (mode1 == 0)myGLCD.print("1", 275, 110);
	if (mode1 == 1)myGLCD.print("0.5", 268, 110);
	if (mode1 == 2)myGLCD.print("0.2", 268, 110);
	if (mode1 == 3)myGLCD.print("0.1", 268, 110);

	while (binFile.read(&buf , 512) == 512) 
	{
		if (buf.count == 0) break;
		if (buf.overrun) 
			{
				Serial.print(F("OVERRUN,"));
				Serial.println(buf.overrun);
			}

		DrawGrid1();

		 if (myTouch.dataAvailable())
			{
				delay(10);
				myTouch.read();
				x_osc=myTouch.getX();
				y_osc=myTouch.getY();

				if ((x_osc>=2) && (x_osc<=240))  //  Delay Button
					{
						if ((y_osc>=1) && (y_osc<=160))  // Delay row
						{
							break;
						} 
					}
			if ((x_osc>=250) && (x_osc<=310))  //  Delay Button
			  {

				 if ((y_osc>=90) && (y_osc<=130))  // Port select   row
					 {
						waitForIt(250, 90, 310, 130);
						mode1 ++ ;
						myGLCD.clrScr();
						myGLCD.setColor(0, 0, 255);
						myGLCD.fillRoundRect (250, 90, 310, 130);
						myGLCD.setColor( 255, 255, 255);
						myGLCD.drawRoundRect (250, 90, 310, 130);
				//		buttons();
						if (mode1 > 3) mode1 = 0;   
						if (mode1 == 0) koeff_h = 7.759*4;
						if (mode1 == 1) koeff_h = 3.879*4;
						if (mode1 == 2) koeff_h = 1.939*4;
						if (mode1 == 3) koeff_h = 0.969*4;
					//	print_set();
						myGLCD.setBackColor( 0, 0, 255);
						myGLCD.setColor( 255, 255, 255);
						myGLCD.setFont( SmallFont);
						myGLCD.print("V/del.", 260, 95);
						myGLCD.print("      ", 260, 110);
						if (mode1 == 0)myGLCD.print("1", 275, 110);
						if (mode1 == 1)myGLCD.print("0.5", 268, 110);
						if (mode1 == 2)myGLCD.print("0.2", 268, 110);
						if (mode1 == 3)myGLCD.print("0.1", 268, 110);
					 }
			  }
		   }

		for (uint16_t i = 0; i < buf.count; i++) 
		{

				Sample[xpos] = buf.data[i];//.adc[0]; 
				xpos++;
			if(xpos == 240)
				{
				DrawGrid1();
				for( int xpos = 0; xpos < 239;	xpos ++)
					{
										// Erase previous display Стереть предыдущий экран
						myGLCD.setColor( 0, 0, 0);
						ypos1 = 255-(OldSample[ xpos + 1]/koeff_h) - hpos; 
						ypos2 = 255-(OldSample[ xpos + 2]/koeff_h) - hpos;

						if(ypos1<0) ypos1 = 0;
						if(ypos2<0) ypos2 = 0;
						if(ypos1>200) ypos1 = 200;
						if(ypos2>200) ypos2 = 200;
						myGLCD.drawLine (xpos + 1, ypos1, xpos + 2, ypos2);
						if (xpos == 0) myGLCD.drawLine (xpos + 1, 1, xpos + 1, 220);
						//Draw the new data
						myGLCD.setColor( 255, 255, 255);
						ypos1 = 255-(Sample[ xpos]/koeff_h) - hpos;
						ypos2 = 255-(Sample[ xpos + 1]/koeff_h)- hpos;

						if(ypos1<0) ypos1 = 0;
						if(ypos2<0) ypos2 = 0;
						if(ypos1>220) ypos1 = 200;
						if(ypos2>220) ypos2 = 200;
						myGLCD.drawLine (xpos, ypos1, xpos + 1, ypos2);
						OldSample[xpos] = Sample[ xpos];
					}
					xpos = 0;
					myGLCD.setFont( BigFont);
					myGLCD.setBackColor( 0, 0, 0);
					count1++;
					myGLCD.printNumI(count, RIGHT, 220);// 
					myGLCD.setColor(VGA_LIME);
					myGLCD.printNumI(count1*240, LEFT, 220);// 
				}
		}
	}
	koeff_h = 7.759*4;
	mode1 = 0;
	Trigger = 0;
	myGLCD.setFont( BigFont);
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(txt_info9,CENTER, 80);
	myGLCD.setColor(255, 255, 255);
	delay(500);
	while (myTouch.dataAvailable()){}

}
//------------------------------------------------------------------------------
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
						if (mode_strob == 0) {set_strob = 100;}
						if (mode_strob == 1) {set_strob = 250;}
						if (mode_strob == 2) {set_strob = 500;}
						if (mode_strob == 3) {set_strob = 1000;}
						if (mode_strob == 4) {set_strob = 5000;}
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
						if (mode_strob> 4) mode_strob = 4;   
						if (mode_strob == 0) {set_strob = 100;}
						if (mode_strob == 1) {set_strob = 250;}
						if (mode_strob == 2) {set_strob = 500;}
						if (mode_strob == 3) {set_strob = 1000;}
						if (mode_strob == 4) {set_strob = 5000;}
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
  // Clear screen
  myGLCD.clrScr();
  
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

  rtc_clock.get_time(&hh,&mm,&ss);
  rtc_clock.get_date(&dow,&dd,&mon,&yyyy);

 // clock_read();
  drawMin(mm);
  drawHour(hh, mm);
  drawSec(ss);
  oldsec=ss;

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
	rtc_clock.get_time(&hh,&mm,&ss);
	rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
	myGLCD.setFont(BigFont);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(255, 255, 255);
  myGLCD.print(str[dow-1], 256, 8);
  if (dd<10)
	myGLCD.printNumI(dd, 272, 28);
  else
	myGLCD.printNumI(dd, 264, 28);

  myGLCD.print(str_mon[mon-1], 256, 48);
  myGLCD.printNumI(yyyy, 248, 65);
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
   rtc_clock.get_time(&hh,&mm,&ss);
   rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
  //clock_read();
  
  while (true)
  {

	if (oldsec!=ss)
	{
	  if ((ss==0) and (mm==0) and (hh==0))
	  {
		clearDate();
		printDate();
	  }
	  if (ss==0)
	  {
		drawMin(mm);
		drawHour(hh, mm);
	  }
	  drawSec(ss);
	  oldsec=ss;
	}

	if (myTouch.dataAvailable())
	{
	  myTouch.read();
	  x=myTouch.getX();
	  y=myTouch.getY();
	  if (((y>=200) && (y<=239)) && ((x>=260) && (x<=319))) //установка часов
	  {
		myGLCD.setColor (255, 0, 0);
		myGLCD.drawRoundRect(260, 200, 319, 239);
		setClockRTC();
	  }

	  if (((y>=140) && (y<=180)) && ((x>=260) && (x<=319))) //Возврат
	  {
		myGLCD.setColor (255, 0, 0);
		myGLCD.drawRoundRect(260, 140, 319, 180);
		myGLCD.clrScr();
		myGLCD.setFont(BigFont);
		break;
	  }
	}
	
	delay(10);
	  rtc_clock.get_time(&hh,&mm,&ss);
	  rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
  }

}

void draw_Glav_Menu()
{
  but1 = myButtons.addButton( 10,  20, 250,  35, txt_menu1_1);
  but2 = myButtons.addButton( 10,  65, 250,  35, txt_menu1_2);
  but3 = myButtons.addButton( 10, 110, 250,  35, txt_menu1_3);
  but4 = myButtons.addButton( 10, 155, 250,  35, txt_menu1_4);
  butX = myButtons.addButton( 279, 199,  40,  40, "W", BUTTON_SYMBOL); // кнопка Часы 
  myGLCD.setColor(VGA_BLACK);
  myGLCD.setBackColor(VGA_WHITE);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myButtons.drawButtons();
}
void swichMenu() // Тексты меню в строках "txt....."
	
{
	 while(1) 
	   {
		 myButtons.setTextFont(BigFont);                      // Установить Большой шрифт кнопок  
		 measure_power();
			if (myTouch.dataAvailable() == true)              // Проверить нажатие кнопок
			  {
				pressed_button = myButtons.checkButtons();    // Если нажата - проверить что нажато
					 if (pressed_button==butX)                // Нажата вызов часы
						  {  
							 myGLCD.setFont( BigFont);
							 AnalogClock();
							 myGLCD.clrScr();
							 myButtons.drawButtons();         // Восстановить кнопки
						  }
	
				   //*****************  Меню №1  **************

				   if (pressed_button==but1)
					   {
							 Draw_menu_ADC1();
							 menu_ADC();
							 myGLCD.clrScr();
							 myButtons.drawButtons();;
					   }
	  
				   if (pressed_button==but2)
					   {
							Draw_menu_Osc();
							menu_Oscilloscope();
							myGLCD.clrScr();
							myButtons.drawButtons();
					   }
	  
				   if (pressed_button==but3)
					   {
							oscilloscope_file();
							myGLCD.clrScr();
							myButtons.drawButtons();
					   }
				   if (pressed_button==but4)
					   {
							Draw_menu_SD();
							menu_SD();
							myGLCD.clrScr();
							myButtons.drawButtons();
					   }

			 } 
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
//++++++++++++++++++++++++++ Конец меню прибора ++++++++++++++++++++++++
void Draw_menu_Osc()
{
	myGLCD.clrScr();
	myGLCD.setFont( BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x=0; x<4; x++)
		{
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect (30, 20+(50*x), 290,60+(50*x));
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+(50*x), 290,60+(50*x));
		}
	myGLCD.print( txt_osc_menu1, CENTER, 30);     // 
	myGLCD.print( txt_osc_menu2, CENTER, 80);   
	myGLCD.print( txt_osc_menu3, CENTER, 130);   
	myGLCD.print( txt_osc_menu4, CENTER, 180);      
}
void menu_Oscilloscope()   // Меню "Осциллоскопа", вызывается из меню "Самописец"
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
							Draw_menu_Osc();
						}
					if ((y>=70) && (y<=110))   // Button: 2 "Oscill_Time"
						{
							waitForIt(30, 70, 290, 110);
							myGLCD.clrScr();
							oscilloscope_time();
							Draw_menu_Osc();
						}
					if ((y>=120) && (y<=160))  // Button: 3 "checkOverrun"  Проверка ошибок
						{
							waitForIt(30, 120, 290, 160);
							myGLCD.clrScr();
							checkOverrun();
							Draw_menu_Osc();
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
void trigger()
{
	 ADC_CHER = Channel_trig;

	for(int tr = 0; tr < 1000; tr++)
	{
		ADC_CR = ADC_START ; 	// Запустить преобразование
		while (!(ADC_ISR_DRDY));
		switch (t_in_mode) 
			{
				case 1:
					Input = ADC->ADC_CDR[6];
					break;
				case 2:
					Input = ADC->ADC_CDR[5];
					break;
				case 3:
					Input = ADC->ADC_CDR[4];
					break;
				default: 
					Input = ADC->ADC_CDR[7];
			}
		// if (Input<Trigger) break;
		 if (Input< 15) break;
	}
	//delayMicroseconds(2);

	for(int tr = 0; tr < 1000; tr++)
	{
		 ADC_CR = ADC_START ; 	// Запустить преобразование
		 while (!(ADC_ISR_DRDY));
		 switch (t_in_mode) 
			{
				case 1:
					Input = ADC->ADC_CDR[6];
					break;
				case 2:
					Input = ADC->ADC_CDR[5];
					break;
				case 3:
					Input = ADC->ADC_CDR[4];
					break;
				default: 
					Input = ADC->ADC_CDR[7];
			}
	
		if (Input>Trigger) break;
		
	}

}

void oscilloscope()  // просмотр в реальном времени на большой скорости
{
	uint32_t bgnBlock, endBlock;
	block_t block[BUFFER_BLOCK_COUNT];
	myGLCD.clrScr();
	myGLCD.setBackColor( 0, 0, 0);
	delay(500);
	myGLCD.clrScr();
	buttons_right();
	buttons_channel();
	myGLCD.setBackColor( 0, 0, 0);
	myGLCD.setFont( BigFont);
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info29,LEFT, 180);
	int x_dTime;
	int xpos;
	int ypos1;
	int ypos2;


	int ypos_osc1_0;
	int ypos_osc1_1;
	int ypos_osc1_2;
	int ypos_osc1_3;

	int ypos_osc2_0;
	int ypos_osc2_1;
	int ypos_osc2_2;
	int ypos_osc2_3;

	for( xpos = 0; xpos < 239;	xpos ++) // Стереть старые данные

		{
			OldSample_osc[xpos][0] = 0;
			OldSample_osc[xpos][1] = 0;
			OldSample_osc[xpos][2] = 0;
			OldSample_osc[xpos][3] = 0;
		}

	while(1) 
	{
		 DrawGrid();
		 if (myTouch.dataAvailable())
			{
				delay(10);
				myTouch.read();
				x_osc=myTouch.getX();
				y_osc=myTouch.getY();

				if ((x_osc>=2) && (x_osc<=240))  //  Область экрана
					{
						if ((y_osc>=1) && (y_osc<=160))  // Delay row
						{
							break;
						} 
					}

				myGLCD.setBackColor( 0, 0, 255);
				myGLCD.setFont( SmallFont);
				myGLCD.setColor (255, 255,255);
				myGLCD.drawRoundRect (250, 1, 318, 40);
				myGLCD.drawRoundRect (250, 45, 318, 85);
				myGLCD.drawRoundRect (250, 90, 318, 130);
				myGLCD.drawRoundRect (250, 135, 318, 175);

			if ((x_osc>=250) && (x_osc<=284))  // Боковые кнопки
			  {
				  if ((y_osc>=1) && (y_osc<=40))  // Первая  период
				  {
					waitForIt(250, 1, 318, 40);
					mode -- ;
					if (mode < 0) mode = 0;   
					// Select delay times you can change values to suite your needs
					if (mode == 0) {dTime = 1;    x_dTime = 282;}
					if (mode == 1) {dTime = 10;   x_dTime = 278;}
					if (mode == 2) {dTime = 20;   x_dTime = 278;}
					if (mode == 3) {dTime = 50;   x_dTime = 278;}
					if (mode == 4) {dTime = 100;  x_dTime = 274;}
					if (mode == 5) {dTime = 200;  x_dTime = 274;}
					if (mode == 6) {dTime = 300;  x_dTime = 274;}
					if (mode == 7) {dTime = 500;  x_dTime = 274;}
					if (mode == 8) {dTime = 1000; x_dTime = 270;}
					if (mode == 9) {dTime = 5000; x_dTime = 270;}
					myGLCD.print("    ", 270, 22);
					myGLCD.printNumI(dTime, x_dTime, 22);
				  }

			 if ((y_osc>=45) && (y_osc<=85))  // Вторая - триггер
				 {
					waitForIt(250, 45, 318, 85);
					tmode --;
					if (tmode < 0)tmode = 0;
					if (tmode == 1){ Trigger = MinAnalog+10; myGLCD.print(" 0%  ", 268, 65);}
					if (tmode == 2){ Trigger = MaxAnalog/2;  myGLCD.print(" 50% ", 266, 65);}
					if (tmode == 3){ Trigger = MaxAnalog-10; myGLCD.print("100%", 270, 65);}
					if (tmode == 0)myGLCD.print(" Off ", 268, 65);

				 }
			 if ((y_osc>=90) && (y_osc<=130))  // Третья - делитель
				 {
					waitForIt(250, 90, 318, 130);
					mode1 -- ;
					myGLCD.setColor( 0, 0, 0);
					myGLCD.fillRoundRect (1, 1,239, 159);
					myGLCD.setColor (255, 255, 255);
					myGLCD.setBackColor( 0, 0, 255);
					myGLCD.setFont( SmallFont);
					if (mode1 < 0) mode1 = 0;   
					if (mode1 == 0){ koeff_h = 7.759*4; myGLCD.print(" 1  ", 275, 110);}
					if (mode1 == 1){ koeff_h = 3.879*4; myGLCD.print("0.5 ", 275, 110);}
					if (mode1 == 2){ koeff_h = 1.939*4; myGLCD.print("0.25", 275, 110);}
					if (mode1 == 3){ koeff_h = 0.969*4; myGLCD.print("0.1 ", 275, 110);}
				 }
			 if ((y_osc>=135) && (y_osc<=175))  // Четвертая разрешение
				 {

				 }
		   }
		
			if ((x_osc>=284) && (x_osc<=318))  // Боковые кнопки
			  {
				  if ((y_osc>=1) && (y_osc<=40))  // Первая  период
				  {
					waitForIt(250, 1, 318, 40);
					mode ++ ;
					if (mode > 9) mode = 9;   
					if (mode == 0) {dTime = 1;    x_dTime = 282;}
					if (mode == 1) {dTime = 10;   x_dTime = 278;}
					if (mode == 2) {dTime = 20;   x_dTime = 278;}
					if (mode == 3) {dTime = 50;   x_dTime = 278;}
					if (mode == 4) {dTime = 100;  x_dTime = 274;}
					if (mode == 5) {dTime = 200;  x_dTime = 274;}
					if (mode == 6) {dTime = 300;  x_dTime = 274;}
					if (mode == 7) {dTime = 500;  x_dTime = 274;}
					if (mode == 8) {dTime = 1000; x_dTime = 270;}
					if (mode == 9) {dTime = 5000; x_dTime = 270;}
					myGLCD.print("    ", 270, 22);
					myGLCD.printNumI(dTime, x_dTime, 22);
				  }

			 if ((y_osc>=45) && (y_osc<=85))  // Вторая - триггер
				 {
					waitForIt(250, 45, 318, 85);
					tmode ++;
					if (tmode > 3)tmode = 3;
					if (tmode == 1){ Trigger = MinAnalog+10; myGLCD.print(" 0%  ", 268, 65);}
					if (tmode == 2){ Trigger = MaxAnalog/2;  myGLCD.print(" 50% ", 266, 65);}
					if (tmode == 3){ Trigger = MaxAnalog-10; myGLCD.print("100%", 270, 65);}
					if (tmode == 0)myGLCD.print(" Off ", 268, 65);
				 }
			 if ((y_osc>=90) && (y_osc<=130))  // Третья - делитель
				 {
					waitForIt(250, 90, 318, 130);
					mode1 ++ ;
					myGLCD.setColor( 0, 0, 0);
					myGLCD.fillRoundRect (1, 1,239, 159);
					myGLCD.setColor (255, 255, 255);
					myGLCD.setBackColor( 0, 0, 255);
					myGLCD.setFont( SmallFont);
					if (mode1 > 3) mode1 = 3;   
					if (mode1 == 0){ koeff_h = 7.759*4; myGLCD.print(" 1  ", 275, 110);}
					if (mode1 == 1){ koeff_h = 3.879*4; myGLCD.print("0.5 ", 275, 110);}
					if (mode1 == 2){ koeff_h = 1.939*4; myGLCD.print("0.25", 275, 110);}
					if (mode1 == 3){ koeff_h = 0.969*4; myGLCD.print("0.1 ", 275, 110);}
				 }
			 if ((y_osc>=135) && (y_osc<=175))  // Четвертая разрешение
				 {
					waitForIt(250, 135, 318, 175);
				 }

		   }

		if ((x_osc>=250) && (x_osc<=318))  

			{
			if ((y_osc>=200) && (y_osc<=239))  //   Нижние кнопки  
				{
					waitForIt(250, 200, 318, 238);
					Channel_trig = 0;
					t_in_mode ++;
						if (t_in_mode > 3)
							{
								t_in_mode = 0;
							}
						switch_trig(t_in_mode);
						myGLCD.setBackColor( 0, 0, 255);
						myGLCD.setColor (255, 255,255);
						myGLCD.printNumI(t_in_mode, 282, 214);
				}
		  }

			 if ((y_osc>=205) && (y_osc<=239))  // Нижние кнопки переключения входов
					{
						 touch_osc();
					}
		}
		 trig_min_max(t_in_mode);
		 if (tmode>0) trigger();
	
		// Записать аналоговый сигнал в блок памяти
		StartSample = micros();
			ADC_CHER = Channel_x;    // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3    
		for( xpos = 0;	xpos < 240; xpos ++) 
			{
			//	ADC_CHER = Channel_x;    // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3    
				ADC_CR = ADC_START ; 	// Запустить преобразование
				 while (!(ADC_ISR_DRDY));
				if (Channel0)
					{
						Sample_osc[xpos][0] = ADC->ADC_CDR[7];
						MaxAnalog0 = max(MaxAnalog0, Sample_osc[xpos][0]);
						MinAnalog0 = min(MinAnalog0, Sample_osc[xpos][0]);
					}
				if (Channel1)
				   {
						Sample_osc[xpos][1] = ADC->ADC_CDR[6];
						MaxAnalog1 = max(MaxAnalog1, Sample_osc[xpos][1]);
						MinAnalog1 = min(MinAnalog1, Sample_osc[xpos][1]);
				   }
				if (Channel2)
					{
						Sample_osc[xpos][2] = ADC->ADC_CDR[5];
						MaxAnalog2 = max(MaxAnalog2, Sample_osc[xpos][2]);
						MinAnalog2 = min(MinAnalog2, Sample_osc[xpos][2]);
					}
				if (Channel3)
					{
						Sample_osc[xpos][3] = ADC->ADC_CDR[4];
						MaxAnalog3 = max(MaxAnalog3, Sample_osc[xpos][3]);
						MinAnalog3 = min(MinAnalog3, Sample_osc[xpos][3]);
					}
				delayMicroseconds(dTime); //dTime
			}
		EndSample = micros();
		DrawGrid();
  
		// 
		for( int xpos = 0; xpos < 239;	xpos ++)
			{
				//  Стереть предыдущий экран
				myGLCD.setColor( 0, 0, 0);
			
				if (Channel0 | osc_line_off0)
					{
						ypos_osc1_0 = 255-(OldSample_osc[ xpos + 1][0]/koeff_h) - hpos; 
						ypos_osc2_0 = 255-(OldSample_osc[ xpos + 2][0]/koeff_h) - hpos;
						if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
						if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
						if(ypos_osc1_0 > 220) ypos_osc1_0 = 220;
						if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
						myGLCD.drawLine (xpos + 1, ypos_osc1_0, xpos + 2, ypos_osc2_0);
						myGLCD.drawLine (xpos + 2, ypos_osc1_0+1, xpos + 3, ypos_osc2_0+1);

						if (xpos > 237 & Channel0 == false )
							{
								osc_line_off0 = false;
							}
					}
			
				if (Channel1|osc_line_off1)
					{
						ypos_osc1_1 = 255-(OldSample_osc[ xpos + 1][1]/koeff_h) - hpos; 
						ypos_osc2_1 = 255-(OldSample_osc[ xpos + 2][1]/koeff_h) - hpos;
						if(ypos_osc1_1 < 0) ypos_osc1_1 = 0;
						if(ypos_osc2_1 < 0) ypos_osc2_1 = 0;
						if(ypos_osc1_1 > 220) ypos_osc1_1 = 220;
						if(ypos_osc2_1 > 220) ypos_osc2_1 = 220;
						myGLCD.drawLine (xpos + 1, ypos_osc1_1, xpos + 2, ypos_osc2_1);
						myGLCD.drawLine (xpos + 2, ypos_osc1_1+1, xpos + 3, ypos_osc2_1+1);
						if (xpos > 237 & Channel1 == false )
							{
								osc_line_off1 = false;
							}
					}
			
				if (Channel2|osc_line_off2)
					{
						ypos_osc1_2 = 255-(OldSample_osc[ xpos + 1][2]/koeff_h) - hpos; 
						ypos_osc2_2 = 255-(OldSample_osc[ xpos + 2][2]/koeff_h) - hpos;
						if(ypos_osc1_2 < 0) ypos_osc1_2 = 0;
						if(ypos_osc2_2 < 0) ypos_osc2_2 = 0;
						if(ypos_osc1_2 > 220) ypos_osc1_2 = 220;
						if(ypos_osc2_2 > 220) ypos_osc2_2 = 220;
						myGLCD.setColor( 0, 0, 0);
						myGLCD.drawLine (xpos + 1, ypos_osc1_2, xpos + 2, ypos_osc2_2);
						myGLCD.drawLine (xpos + 2, ypos_osc1_2+1, xpos + 3, ypos_osc2_2+1);
						if (xpos > 237 & Channel2 == false )
							{
								osc_line_off2 = false;
							}
					}
			
				if (Channel3|osc_line_off3)
					{
						ypos_osc1_3 = 255-(OldSample_osc[ xpos + 1][3]/koeff_h) - hpos; 
						ypos_osc2_3 = 255-(OldSample_osc[ xpos + 2][3]/koeff_h) - hpos;
						if(ypos_osc1_3 < 0) ypos_osc1_3 = 0;
						if(ypos_osc2_3 < 0) ypos_osc2_3 = 0;
						if(ypos_osc1_3 > 220) ypos_osc1_3 = 220;
						if(ypos_osc2_3 > 220) ypos_osc2_3 = 220;
						myGLCD.drawLine (xpos + 1, ypos_osc1_3, xpos + 2, ypos_osc2_3);
						myGLCD.drawLine (xpos + 2, ypos_osc1_3+1, xpos + 3, ypos_osc2_3+1);
						if (xpos > 237 & Channel3 == false )
							{
								osc_line_off3 = false;
							}
					}

					if (xpos == 0)
						{
							myGLCD.drawLine (xpos + 1, 1, xpos + 1, 220);
							myGLCD.drawLine (xpos + 2, 1, xpos + 2, 220);
						}
					
				if (Channel0)
					{

						myGLCD.setColor( 255, 255, 255);
						ypos_osc1_0 = 255-(Sample_osc[ xpos][0]/koeff_h) - hpos;
						ypos_osc2_0 = 255-(Sample_osc[ xpos + 1][0]/koeff_h)- hpos;
						if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
						if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
						if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
						if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
						myGLCD.drawLine (xpos, ypos_osc1_0, xpos + 1, ypos_osc2_0);
						myGLCD.drawLine (xpos+1, ypos_osc1_0+1, xpos + 2, ypos_osc2_0+1);
					}

				if (Channel1)
					{
						myGLCD.setColor( VGA_YELLOW);
						ypos_osc1_1 = 255-(Sample_osc[ xpos][1]/koeff_h) - hpos;
						ypos_osc2_1 = 255-(Sample_osc[ xpos + 1][1]/koeff_h)- hpos;
						if(ypos_osc1_1 < 0) ypos_osc1_1 = 0;
						if(ypos_osc2_1 < 0) ypos_osc2_1 = 0;
						if(ypos_osc1_1 > 220) ypos_osc1_1  = 220;
						if(ypos_osc2_1 > 220) ypos_osc2_1 = 220;
						myGLCD.drawLine (xpos, ypos_osc1_1, xpos + 1, ypos_osc2_1);
						myGLCD.drawLine (xpos+1, ypos_osc1_1+1, xpos + 2, ypos_osc2_1+1);
					}
				
				if (Channel2)
					{
						//Draw the new data
						myGLCD.setColor( VGA_RED);
						ypos_osc1_2 = 255-(Sample_osc[ xpos][2]/koeff_h) - hpos;
						ypos_osc2_2 = 255-(Sample_osc[ xpos + 1][2]/koeff_h)- hpos;
						if(ypos_osc1_2 < 0) ypos_osc1_2 = 0;
						if(ypos_osc2_2 < 0) ypos_osc2_2 = 0;
						if(ypos_osc1_2 > 220) ypos_osc1_2  = 220;
						if(ypos_osc2_2 > 220) ypos_osc2_2 = 220;
						myGLCD.drawLine (xpos, ypos_osc1_2, xpos + 1, ypos_osc2_2);
						myGLCD.drawLine (xpos+1, ypos_osc1_2+1, xpos + 2, ypos_osc2_2+1);
					}
				
				if (Channel3)
					{
						myGLCD.setColor( VGA_BLUE);
						ypos_osc1_3 = 255-(Sample_osc[ xpos][3]/koeff_h) - hpos;
						ypos_osc2_3 = 255-(Sample_osc[ xpos + 1][3]/koeff_h)- hpos;
						if(ypos_osc1_3 < 0) ypos_osc1_3 = 0;
						if(ypos_osc2_3 < 0) ypos_osc2_3 = 0;
						if(ypos_osc1_3 > 220) ypos_osc1_3  = 220;
						if(ypos_osc2_3 > 220) ypos_osc2_3 = 220;
						myGLCD.drawLine (xpos, ypos_osc1_3, xpos + 1, ypos_osc2_3);
						myGLCD.drawLine (xpos+1, ypos_osc1_3+1, xpos + 2, ypos_osc2_3+1);
					}

					OldSample_osc[xpos][0] = Sample_osc[xpos][0];
					OldSample_osc[xpos][1] = Sample_osc[xpos][1];
					OldSample_osc[xpos][2] = Sample_osc[xpos][2];
					OldSample_osc[xpos][3] = Sample_osc[xpos][3];
			}
	}
koeff_h = 7.759*4;
mode1 = 0;
Trigger = 0;
StartSample = millis();
myGLCD.setFont( BigFont);
while (myTouch.dataAvailable()){}
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
	int ypos_osc1_1;
	int ypos_osc1_2;
	int ypos_osc1_3;
	int ypos_osc2_0;
	int ypos_osc2_1;
	int ypos_osc2_2;
	int ypos_osc2_3;

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
			OldSample_osc[xpos][0] = 0;
			OldSample_osc[xpos][1] = 0;
			OldSample_osc[xpos][2] = 0;
			OldSample_osc[xpos][3] = 0;
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
	myGLCD.drawCircle(230,10,20);
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
										ypos_osc1_0 = 255-(OldSample_osc[ xpos][0]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][0]/koeff_h)- hpos;
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
										ypos_osc1_0 = 255-(OldSample_osc[ xpos - 1][0]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][0]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+1);
									}
							}
						if (Channel1)
							{
								if (xpos == 0)					// определить начальную позицию по Х 
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos][1]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][1]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+1);
										if (ypos_osc1_0 < 150)
											{
												myGLCD.print("1", 2, ypos_osc1_0+1);
											}
										else
											{
												myGLCD.print("1", 2, ypos_osc1_0-10);
											}
									}
								else
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos - 1][1]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][1]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+1);
									}
							}
						if (Channel2)
							{
								if (xpos == 0)					// определить начальную позицию по Х 
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos][2]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][2]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+1);
										if (ypos_osc1_0 < 150)
											{
												myGLCD.print("2", 2, ypos_osc1_0+1);
											}
										else
											{
												myGLCD.print("2", 2, ypos_osc1_0-10);
											}
									}
								else
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos - 1][2]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][2]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+1);
									}
							}
						if (Channel3)
							{
								if (xpos == 0)					// определить начальную позицию по Х 
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos][3]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][3]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+1);
										if (ypos_osc1_0 < 150)
											{
												myGLCD.print("3", 2, ypos_osc1_0+1);
											}
										else
											{
												myGLCD.print("3", 2, ypos_osc1_0-10);
											}
									}
								else
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos - 1][3]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][3]/koeff_h)- hpos;
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
											myGLCD.fillCircle(227,12,10);
										}
									else
										{
											myGLCD.setColor(255,255,255);
											myGLCD.drawCircle(227,12,10);
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
				if (Channel1)
				   {
						MaxAnalog1 = max(MaxAnalog1, ADC->ADC_CDR[6]);
						SrednAnalog1 += MaxAnalog1;
				   }
				if (Channel2)
					{
						MaxAnalog2 = max(MaxAnalog2, ADC->ADC_CDR[5]);
						SrednAnalog2 += MaxAnalog2;
					}
				if (Channel3)
					{
						MaxAnalog3 = max(MaxAnalog3,  ADC->ADC_CDR[4]);
						SrednAnalog3 += MaxAnalog3;
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
						MaxAnalog1 =  SrednAnalog1 / SrednCount;
						MaxAnalog2 =  SrednAnalog2 / SrednCount;
						MaxAnalog3 =  SrednAnalog3 / SrednCount;
						SrednAnalog0 = 0;
						SrednAnalog1 = 0;
						SrednAnalog2 = 0;
						SrednAnalog3 = 0;
						SrednCount = 0;
					 }
				if (Channel0) Sample_osc[ xpos][0] = MaxAnalog0;
				if (Channel1) Sample_osc[ xpos][1] = MaxAnalog1;
				if (Channel2) Sample_osc[ xpos][2] = MaxAnalog2;
				if (Channel3) Sample_osc[ xpos][3] = MaxAnalog3;

					MaxAnalog0 =  0;
					MaxAnalog1 =  0;
					MaxAnalog2 =  0;
					MaxAnalog3 =  0;



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

				if (Channel1)
					{

						if (xpos == 0)
							{
								myGLCD.setColor( VGA_YELLOW);
								ypos_osc1_0 = 255-(Sample_osc[ xpos][1]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][1]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( VGA_YELLOW);
								ypos_osc1_0 = 255-(Sample_osc[ xpos - 1][1]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][1]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}
				
				if (Channel2)
					{

							if (xpos == 0)
							{
								myGLCD.setColor( VGA_RED);
								ypos_osc1_0 = 255-(Sample_osc[ xpos][2]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][2]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( VGA_RED);
								ypos_osc1_0 = 255-(Sample_osc[ xpos - 1][2]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][2]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}
				
				if (Channel3)
					{

						if (xpos == 0)
							{
								myGLCD.setColor( VGA_BLUE);
								ypos_osc1_0 = 255-(Sample_osc[ xpos][3]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][3]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( VGA_BLUE);
								ypos_osc1_0 = 255-(Sample_osc[ xpos - 1][3]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][3]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}

					OldSample_osc[xpos][0] = Sample_osc[xpos][0];
					OldSample_osc[xpos][1] = Sample_osc[xpos][1];
					OldSample_osc[xpos][2] = Sample_osc[xpos][2];
					OldSample_osc[xpos][3] = Sample_osc[xpos][3];
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
void oscilloscope_file()  // Пишет в файл
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
	int ypos_osc1_1;
	int ypos_osc1_2;
	int ypos_osc1_3;
	int ypos_osc2_0;
	int ypos_osc2_1;
	int ypos_osc2_2;
	int ypos_osc2_3;

	uint8_t sec_osc = 0;
	uint16_t min_osc = 0;
	bool ind_start = false;
	strob_start = true;
	char str_file[10];
	StartSample = 0; 
	uint32_t logTime = 0;
	uint32_t SAMPLE_INTERVAL_MS = 250;
	int32_t diff;
	count_repeat = 0;

	for( xpos = 0; xpos < 239;	xpos ++) // Стереть старые данные

		{
			OldSample_osc[xpos][0] = 0;
			OldSample_osc[xpos][1] = 0;
			OldSample_osc[xpos][2] = 0;
			OldSample_osc[xpos][3] = 0;
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

				if ((x_osc>=1) && (x_osc<=60))  //  Выход 
					{
						if ((y_osc>=1) && (y_osc<=35))  // Delay row
						{
							waitForIt(1, 1, 60, 35);
							return;
						} 
					}

				if ((x_osc>=40) && (x_osc<=200))  //  Выход из ожидания, Старт
					{
						if ((y_osc>=40) && (y_osc<=120))  // Delay row
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
					mode -- ;
					if (mode < 0) mode = 0;   
					if (mode == 0) {SAMPLE_INTERVAL_MS = 250;}
					if (mode == 1) {SAMPLE_INTERVAL_MS = 1500;}
					if (mode == 2) {SAMPLE_INTERVAL_MS = 3000;}
					if (mode == 3) {SAMPLE_INTERVAL_MS = 4500;}
					scale_time();
				  }

			 if ((y_osc>=45) && (y_osc<=85))  // Вторая - усреднение показаний
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
				
			
			if ((x_osc>=284) && (x_osc<=318))                          // Боковые кнопки
			  {
				  if ((y_osc>=1) && (y_osc<=40))                       // Первая  период  +
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

			 if ((y_osc>=45) && (y_osc<=85))                           // Вторая - триггер
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
			 if ((y_osc>=90) && (y_osc<=130))                         // Третья - делитель
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
				 if ((y_osc>=135) && (y_osc<=175))                                   // Четвертая разрешение отображения следа
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

			 if ((y_osc>=205) && (y_osc<=239))  // Нижние кнопки переключения входов
					{
						 touch_osc();
					}
		}
	}


// +++++++++++++++++++++++++ Работа с файлом +++++++++++++++++++++++++++
	metadata_t* pm;
	uint8_t lastPct = 0;
	uint32_t t0 = millis();
	char csvName[13];
	char csvNameTmp[13];
	char csvData[4];

	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setFont(BigFont);
	myGLCD.setColor( 0, 0, 0);
	myGLCD.fillRoundRect (2, 2,239, 159);
	myGLCD.setColor( 255,255,255);
	myGLCD.setBackColor( 0, 0, 0);
	preob_num_str();

   // Create a new CSV file.
	if (BASE_NAME_SIZE > 6) 
		{
			error("FILE_BASE_NAME too long");
		}
	while (sd.exists(timeName)) 
		{
		if (timeName[BASE_NAME_SIZE + 1] != '9') 
			{
				timeName[BASE_NAME_SIZE + 1]++;
			}
		else 
			{
				timeName[BASE_NAME_SIZE + 1] = '0';
				if (timeName[BASE_NAME_SIZE] == '9') 
					{
						error("Can't create file name");
					}
				timeName[BASE_NAME_SIZE]++;
			}
		}
	// Delete old tmp file.
	if (sd.exists(TMP_FILE_NAME)) 
		{
		myGLCD.print(txt_info13,LEFT, 135);              //
		if (!sd.remove(TMP_FILE_NAME)) 
			{
				error("Can't remove tmp file");
			}
		}
	myGLCD.print(txt_info27,10, 40);//
	strcpy(csvName, timeName);
	strcpy_P(&csvName[BASE_NAME_SIZE + 3], PSTR("TXT"));

	if (!csvStream.fopen(csvName, "w")) 
		{
			error("open csvStream failed");  
		}
 
	myGLCD.setFont(BigFont);
	myGLCD.print(txt_info7,10, 60);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(csvName,RIGHT, 60);// 
	strcpy(csvNameTmp, csvName);
	myGLCD.setColor(255, 255, 255);
	delay(2000);
	myGLCD.clrScr();
	csvStream.print(F("File - "));
	csvStream.println(F(csvName));
	csvStream.print(F("Interval - "));
	if (mode == 0) csvStream.print("1");
	if (mode == 1) csvStream.print("6");
	if (mode == 2) csvStream.print("12");
	if (mode == 3) csvStream.print("18");

	csvStream.println(F(" min"));

	csvStream.print(F("Step = "));
	csvStream.print(v_const, 8);
	csvStream.println(F(" volt"));
	csvStream.print(F("Data : "));
	rtc_clock.get_time(&hh,&mm,&ss);
	rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
	dow1=dow;
	sec = ss;       //Initialization time
	min = mm;
	hour = hh;
	date = dd;
	mon1 = mon;
	year = yyyy;

	csvStream.print(date);
	csvStream.print(F("/"));
	csvStream.print(mon1);
	csvStream.print(F("/"));
	csvStream.print(year);
	csvStream.print(F("   "));
	csvStream.print(hour);
	csvStream.print(F(":"));
	csvStream.print(min);
	csvStream.print(F(":"));
	csvStream.print(sec);
	csvStream.println(); 
	csvStream.println(); 

	adcInit((metadata_t*) &block[0]);            // Получение данных об используемых входах
	pm = (metadata_t*)&block;                    // Получение данных об используемых входах
	csvStream.println(); 
	csvStream.print(F("@"));   
	for (uint8_t i = 0; i < pm->pinCount; i++)     // Запись входов в файл
		{
			if (i) csvStream.putc(',');
			csvStream.print(F("pin "));
			csvStream.print(pm->pinNumber[i]);
		}
	csvStream.println(); 
	csvStream.println('#');       
	csvStream.println(); 
	uint32_t tPct = millis();


	// +++++++++++++++++   Начало измерений ++++++++++++++++++++++++

	myGLCD.setBackColor( 0, 0, 0);
	myGLCD.setFont(BigFont);
	myGLCD.print("     ", 80, 72);
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info29,LEFT, 180);
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setFont( SmallFont);
	DrawGrid1();
	logTime = micros();
	count_repeat = 0;
	uint32_t bn = 1;
	uint32_t overruns = 0;
	uint32_t count = 0;
	buttons_right_time();
	buttons_channel();
	myGLCD.setBackColor( 0, 0, 0);
	DrawGrid();

		// Записать аналоговый сигнал в блок памяти
			StartSample = micros();
			ADC_CHER = Channel_x;    // this is (1<<7) | (1<<6) for adc 7= A0, 6=A1 , 5=A2, 4 = A3  Установить аналоговые входа.  
	 do
	  {
		if (sled == true & count_repeat > 0)  // Нарисовать след от предыдущего измерения
			{

					myGLCD.setColor (0, 0, 0);
					myGLCD.fillRoundRect (1, 1, 239, 159);
					myGLCD.setFont( SmallFont);
					myGLCD.setBackColor( 0, 0, 255);
					DrawGrid();

			  for( int xpos = 0; xpos < 239;	xpos ++)             
					{
						if (Channel0)
							{
								if (xpos == 0)					// определить начальную позицию по Х 
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos][0]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][0]/koeff_h)- hpos;
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
										ypos_osc1_0 = 255-(OldSample_osc[ xpos - 1][0]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][0]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+1);
									}
							}
						if (Channel1)
							{
								if (xpos == 0)					// определить начальную позицию по Х 
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos][1]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][1]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+1);
										if (ypos_osc1_0 < 150)
											{
												myGLCD.print("1", 2, ypos_osc1_0+1);
											}
										else
											{
												myGLCD.print("1", 2, ypos_osc1_0-10);
											}
									}
								else
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos - 1][1]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][1]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+1);
									}
							}
						if (Channel2)
							{
								if (xpos == 0)					// определить начальную позицию по Х 
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos][2]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][2]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+1);
										if (ypos_osc1_0 < 150)
											{
												myGLCD.print("2", 2, ypos_osc1_0+1);
											}
										else
											{
												myGLCD.print("2", 2, ypos_osc1_0-10);
											}
									}
								else
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos - 1][2]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][2]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+1);
									}
							}
						if (Channel3)
							{
								if (xpos == 0)					// определить начальную позицию по Х 
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos][3]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][3]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+1);
										if (ypos_osc1_0 < 150)
											{
												myGLCD.print("3", 2, ypos_osc1_0+1);
											}
										else
											{
												myGLCD.print("3", 2, ypos_osc1_0-10);
											}
									}
								else
									{
										myGLCD.setColor(VGA_LIME);
										ypos_osc1_0 = 255-(OldSample_osc[ xpos - 1][3]/koeff_h) - hpos;
										ypos_osc2_0 = 255-(OldSample_osc[ xpos][3]/koeff_h)- hpos;
										if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
										if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
										if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
										if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
										myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+1);
									}
							}
					}
			}
		if(sled == false)
			{
				myGLCD.setColor (0, 0, 0);
				myGLCD.fillRoundRect (1, 1, 239, 159);
				myGLCD.setFont( SmallFont);
				myGLCD.setBackColor( 0, 0, 255);
				DrawGrid();
				myGLCD.setFont( BigFont);
			}
		myGLCD.setBackColor( 0, 0, 0);
		myGLCD.setFont(BigFont);
		myGLCD.setColor(VGA_LIME);
		myGLCD.print(txt_info29,LEFT, 180);    // "Stop->PUSH Disp"; 

		for( xpos = 0;	xpos < 240; xpos ++)   //  Старт измерения
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

			 if (myTouch.dataAvailable())      // Выход из программы измерения
				{
					delay(10);
					myTouch.read();
					x_osc=myTouch.getX();
					y_osc=myTouch.getY();
				
					if ((x_osc>=2) && (x_osc<=240))  //  Delay Button
						{
							if ((y_osc>=1) && (y_osc<=160))  // Delay row
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

				ADC_CR = ADC_START ; 	// Запустить преобразование
				 while (!(ADC_ISR_DRDY));

				if (Channel0)
					{
						MaxAnalog0 = max(MaxAnalog0, ADC->ADC_CDR[7]);
						SrednAnalog0 += MaxAnalog0;
					}
				if (Channel1)
				   {
						MaxAnalog1 = max(MaxAnalog1, ADC->ADC_CDR[6]);
						SrednAnalog1 += MaxAnalog1;
				   }
				if (Channel2)
					{
						MaxAnalog2 = max(MaxAnalog2, ADC->ADC_CDR[5]);
						SrednAnalog2 += MaxAnalog2;
					}
				if (Channel3)
					{
						MaxAnalog3 = max(MaxAnalog3,  ADC->ADC_CDR[4]);
						SrednAnalog3 += MaxAnalog3;
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
						MaxAnalog1 =  SrednAnalog1 / SrednCount;
						MaxAnalog2 =  SrednAnalog2 / SrednCount;
						MaxAnalog3 =  SrednAnalog3 / SrednCount;
						SrednAnalog0 = 0;
						SrednAnalog1 = 0;
						SrednAnalog2 = 0;
						SrednAnalog3 = 0;
						SrednCount = 0;
					 }

						if (Channel0)
						{
							Sample_osc[ xpos][0] = MaxAnalog0;
							itoa (MaxAnalog0,csvData, 10); // Преобразование  строку ( 10 - десятичный формат) 
							csvStream.fputs(csvData) ;
							if(Channel1 | Channel2 | Channel3) csvStream.putc(',');
						}
						if (Channel1)
						{
							Sample_osc[ xpos][1] = MaxAnalog1;
							itoa (MaxAnalog1,csvData, 10); // Преобразование  строку ( 10 - десятичный формат) 
							csvStream.fputs(csvData) ;
							if(Channel2 | Channel3) csvStream.putc(',');
						}
						if (Channel2)	
						{
							Sample_osc[ xpos][2] = MaxAnalog2;
							itoa (MaxAnalog2,csvData, 10); // Преобразование  строку ( 10 - десятичный формат) 
							csvStream.fputs(csvData) ;
							if(Channel3) csvStream.putc(',');
						}
						if (Channel3)	
						{
							Sample_osc[ xpos][3] = MaxAnalog3;
							itoa (MaxAnalog3,csvData, 10); // Преобразование  строку ( 10 - десятичный формат) 
							csvStream.fputs(csvData) ;
						}

					csvStream.println();

					MaxAnalog0 =  0;
					MaxAnalog1 =  0;
					MaxAnalog2 =  0;
					MaxAnalog3 =  0;
						
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

				if (Channel1)
					{

						if (xpos == 0)
							{
								myGLCD.setColor( VGA_YELLOW);
								ypos_osc1_0 = 255-(Sample_osc[ xpos][1]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][1]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( VGA_YELLOW);
								ypos_osc1_0 = 255-(Sample_osc[ xpos - 1][1]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][1]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}
				
				if (Channel2)
					{

							if (xpos == 0)
							{
								myGLCD.setColor( VGA_RED);
								ypos_osc1_0 = 255-(Sample_osc[ xpos][2]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][2]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( VGA_RED);
								ypos_osc1_0 = 255-(Sample_osc[ xpos - 1][2]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][2]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}
				
				if (Channel3)
					{

						if (xpos == 0)
							{
								myGLCD.setColor( VGA_BLUE);
								ypos_osc1_0 = 255-(Sample_osc[ xpos][3]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][3]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( VGA_BLUE);
								ypos_osc1_0 = 255-(Sample_osc[ xpos - 1][3]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(Sample_osc[ xpos][3]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}

					OldSample_osc[xpos][0] = Sample_osc[xpos][0];
					OldSample_osc[xpos][1] = Sample_osc[xpos][1];
					OldSample_osc[xpos][2] = Sample_osc[xpos][2];
					OldSample_osc[xpos][3] = Sample_osc[xpos][3];

	   }

	count_repeat++;
	myGLCD.setFont( SmallFont);
	myGLCD.setBackColor(0, 0, 255);
	myGLCD.setColor(255, 255, 255);
	if (repeat == true) myGLCD.print("       ", 257, 220);
	if (repeat == true) myGLCD.printNumI(count_repeat, 270, 220);
	csvStream.println();
	csvStream.print("repeat = ");
	csvStream.printDec(count_repeat);
	csvStream.println();

	} while (repeat);

	csvStream.println();                       // Признак  5555 окончания данных в файле
	csvStream.print('5');                      // Признак  5555 окончания данных в файле
	csvStream.print('5');                      // Признак  5555 окончания данных в файле
	csvStream.print('5');                      // Признак  5555 окончания данных в файле
	csvStream.print('5');                      // Признак  5555 окончания данных в файле
	csvStream.println(); 
	myGLCD.setFont( BigFont);
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("            ", 10, 140);
	myGLCD.print("Stop record", 40, 140);
	csvStream.print("Time measure = ");
	csvStream.print(min_osc);
	csvStream.print(":");
	csvStream.print(sec_osc);
	delay(1000);

	csvStream.fclose();  
	strob_start = true;
	myGLCD.clrScr();
	myGLCD.setBackColor( 0, 0, 0);
	myGLCD.setColor(255, 255, 255);
	myGLCD.print(txt_info6,CENTER, 5);//
	myGLCD.print(txt_info7,LEFT, 75);//
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(csvNameTmp,RIGHT, 75);// 
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info29,CENTER, 180);
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
	myGLCD.fillRoundRect (250, 1, 318, 40);
	myGLCD.fillRoundRect (250, 45, 318, 85);
	myGLCD.fillRoundRect (250, 90, 318, 130);
	myGLCD.fillRoundRect (250, 135, 318, 175);
	myGLCD.fillRoundRect (250, 200, 318, 239);

	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255,255);
	myGLCD.print("Delay", 265, 6);
	myGLCD.print("-      +", 255, 22);
	myGLCD.printNumI(dTime, 282, 22);
	myGLCD.print("Trig.", 270, 50);
	myGLCD.print("-      +", 255, 65);
	if (tmode == 0)myGLCD.print(" Off ", 268, 65);
	if (tmode == 1)myGLCD.print(" 0%  ", 268, 65);
	if (tmode == 2)myGLCD.print(" 50% ", 266, 65);
	if (tmode == 3)myGLCD.print(" 100%", 270, 65);

	myGLCD.print("V/del.", 265, 95);
	myGLCD.print("-      +", 255, 110);
	if (mode1 == 0){ koeff_h = 7.759*4; myGLCD.print(" 1  ", 275, 110);}
	if (mode1 == 1){ koeff_h = 3.879*4; myGLCD.print("0.5 ", 275, 110);}
	if (mode1 == 2){ koeff_h = 1.939*4; myGLCD.print("0.25", 275, 110);}
	if (mode1 == 3){ koeff_h = 0.969*4; myGLCD.print("0.1 ", 275, 110);}

	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setColor (255, 255,255);
	myGLCD.print("Synchro", 255, 202);
	switch_trig(t_in_mode);
	myGLCD.printNumI(t_in_mode, 282, 212);
}
void buttons_right_time()
{
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect (250, 1, 318, 40);
	myGLCD.fillRoundRect (250, 45, 318, 85);
	myGLCD.fillRoundRect (250, 90, 318, 130);
	myGLCD.fillRoundRect (250, 135, 318, 175);
	myGLCD.fillRoundRect (250, 200, 318, 239);

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
void buttons_channel()  // Нижние кнопки переключения входов
{
	myGLCD.setFont( SmallFont);

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
						myGLCD.fillRoundRect (10, 200, 60, 205);   // Индикатор цвета линии
						myGLCD.fillRoundRect (10, 210, 60, 239);
						myGLCD.setColor(255, 255, 255);
						myGLCD.print("0", 32, 212);
						myGLCD.print("BXOD", 20, 226);
					}

				if (Channel1)
					{
						myGLCD.setColor(VGA_YELLOW);
						myGLCD.fillRoundRect (70, 200, 120, 205);
						myGLCD.setColor(VGA_LIME);
						myGLCD.setBackColor( VGA_LIME);
						myGLCD.fillRoundRect (70, 210, 120, 239);
						myGLCD.setColor(0, 0, 0);
						myGLCD.print("1", 92, 212);
						myGLCD.print("BXOD", 80, 226);
						osc_line_off1 = true;
					}
				else
					{
						myGLCD.setColor(0,0,0);
						myGLCD.setBackColor( 0,0,0);
						myGLCD.fillRoundRect (70, 200, 120, 205);   // Индикатор цвета линии
						myGLCD.fillRoundRect (70, 210, 120, 239);
						myGLCD.setColor(255, 255, 255);
						myGLCD.print("1", 92, 212);
						myGLCD.print("BXOD", 80, 226);
					}

				if (Channel2)
					{
						myGLCD.setColor(VGA_RED);
						myGLCD.fillRoundRect (130, 200, 180, 205);
						myGLCD.setColor(VGA_LIME);
						myGLCD.setBackColor( VGA_LIME);
						myGLCD.fillRoundRect (130, 210, 180, 239);
						myGLCD.setColor(0, 0,0);
						myGLCD.print("2", 152, 212);
						myGLCD.print("BXOD", 140, 226);
						osc_line_off2 = true;
					}
				else
					{
						myGLCD.setColor(0,0,0);
						myGLCD.setBackColor( 0,0,0);
						myGLCD.fillRoundRect (130, 210, 180, 239);
						myGLCD.fillRoundRect (130, 200, 180, 205);   // Индикатор цвета линии
						myGLCD.setColor(255, 255, 255);
						myGLCD.print("2", 152, 212);
						myGLCD.print("BXOD", 140, 226);
					}

				if (Channel3)
					{
						myGLCD.setColor(VGA_BLUE);
						myGLCD.fillRoundRect (190, 200, 240, 205);
						myGLCD.setColor(VGA_LIME);
						myGLCD.setBackColor( VGA_LIME);
						myGLCD.fillRoundRect (190, 210, 240, 239);
						myGLCD.setColor(0, 0, 0);
						myGLCD.print("3", 212, 212);
						myGLCD.print("BXOD", 200, 226);
						osc_line_off3 = true;
					}
				else
					{
						myGLCD.setColor(0,0,0);
						myGLCD.setBackColor( 0,0,0);
						myGLCD.fillRoundRect (190, 210, 240, 239);
						myGLCD.fillRoundRect (190, 200, 240, 205);   // Индикатор цвета линии
						myGLCD.setColor(255, 255, 255);
						myGLCD.print("3", 212, 212);
						myGLCD.print("BXOD", 200, 226);
					}
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect (10, 210, 60, 239);
	myGLCD.drawRoundRect (70, 210, 120, 239);
	myGLCD.drawRoundRect (130, 210, 180, 239);
	myGLCD.drawRoundRect (190, 210, 240, 239);
}
void chench_Channel()
{
	//Подготовка номера аналогового сигнала, количества каналов и кода настройки АЦП
		   Channel_x = 0;
		   ADC_CHER = Channel_x;
		   count_pin = 0;
	 
		if (Channel0 )
			{
				Channel_x|=0x80;
				count_pin++;
			}
		if (Channel1 )
			{
				Channel_x|=0x40;
				count_pin++;
			}
		
		if (Channel2 ) 
			{
				Channel_x|=0x20;
				count_pin++;
			}

		if (Channel3) 
			{
				Channel_x|=0x10;
				count_pin++;
			}
		 ADC_CHER = Channel_x;
		 SAMPLES_PER_BLOCK = DATA_DIM16/count_pin;
}
void DrawGrid()
{

  myGLCD.setColor( 0, 200, 0);
  for(  dgvh = 0; dgvh < 5; dgvh ++)
  {
	  myGLCD.drawLine( dgvh * 40, 0, dgvh * 40, 160);
	  myGLCD.drawLine(  0, dgvh * 40, 240 ,dgvh * 40);
  }
	myGLCD.drawLine( 200, 0, 200, 160);
	myGLCD.drawLine( 240, 0, 240, 160);
	myGLCD.setColor(255, 255, 255);           // Белая окантовка
	myGLCD.drawRoundRect (250, 1, 318, 40);
	myGLCD.drawRoundRect (250, 45, 318, 85);
	myGLCD.drawRoundRect (250, 90, 318, 130);
	myGLCD.drawRoundRect (250, 135, 318, 175);

	myGLCD.drawRoundRect (10, 210, 60, 239);
	myGLCD.drawRoundRect (70, 210, 120, 239);
	myGLCD.drawRoundRect (130, 210, 180, 239);
	myGLCD.drawRoundRect (190, 210, 240, 239);
	myGLCD.drawRoundRect (250, 200, 318, 239);
	myGLCD.setBackColor( 0, 0, 0);
	myGLCD.setFont( SmallFont);
	if (mode1 == 0)
		{				
			myGLCD.print("4", 241, 0);
			myGLCD.print("3", 241, 34);
			myGLCD.print("2", 241, 74);
			myGLCD.print("1", 241, 114);
			myGLCD.print("0", 241, 152);
		}
	if (mode1 == 1)
		{
			myGLCD.print("2", 241, 0);
			myGLCD.print("1,5", 226, 34);
			myGLCD.print("1", 241, 74);
			myGLCD.print("0,5", 226, 114);
			myGLCD.print("0", 241, 152);
		}

	if (mode1 == 2)
		{
			myGLCD.print("1", 241, 0);
			myGLCD.print("0,75", 218, 34);
			myGLCD.print("0,5", 226, 74);
			myGLCD.print("0,25", 218, 114);
			myGLCD.print("0", 241, 152);
		}
	if (mode1 == 3)
		{
			myGLCD.print("0,4", 226, 0);
			myGLCD.print("0,3", 226, 34);
			myGLCD.print("0,2", 226, 74);
			myGLCD.print("0,1", 226, 114);
			myGLCD.print("0", 241, 152);
		}
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

}
void DrawGrid1()
{

 myGLCD.setColor( 0, 200, 0);
  for(  dgvh = 0; dgvh < 5; dgvh ++)
  {
	  myGLCD.drawLine( dgvh * 40, 0, dgvh * 40, 160);
	  myGLCD.drawLine(  0, dgvh * 40, 240 ,dgvh * 40);
  }
	myGLCD.drawLine( 200, 0, 200, 160);
	myGLCD.drawLine( 240, 0, 240, 160);
	//myGLCD.setColor(255, 255, 255);           // Белая окантовка

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
}

void Draw_menu_SD()
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
	myGLCD.print( txt_SD_menu1, CENTER, 30);     // 
	myGLCD.print( txt_SD_menu2, CENTER, 80);      
	myGLCD.print( txt_SD_menu3, CENTER, 130);     
	myGLCD.print( txt_SD_menu4, CENTER, 180);      
}
void menu_SD()
{
	while (Serial.read() >= 0) {} // Удалить все символы из буфера
	char c;
	while (true)
	  {
		delay(10);
		if (myTouch.dataAvailable())
			{
				myTouch.read();
				int	x=myTouch.getX();
				int	y=myTouch.getY();

				if ((x>=30) && (x<=290))       // Upper row
					{
					if ((y>=20) && (y<=60))    // Button: 1
						{
							waitForIt(30, 20, 290, 60);
							myGLCD.clrScr();
							root = sd.open("/");
							printDirectory(root, 0);
							Draw_menu_SD();
						}
					if ((y>=70) && (y<=110))   // Button: 2
						{
							waitForIt(30, 70, 290, 110);
							SD_info();
							Draw_menu_SD();
						}
					if ((y>=120) && (y<=160))  // Button: 3
						{
							waitForIt(30, 120, 290, 160);
							myGLCD.clrScr();
							menu_formatSD();
							Draw_menu_SD();
						}
					if ((y>=170) && (y<=220))  // Button: 4
						{
							waitForIt(30, 170, 290, 210);
							break;
						}
				}
			}
	   }
}

void Draw_menu_formatSD()
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 255);
	for (int x=0; x<4; x++)
		{
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect (30, 20+(50*x), 290,60+(50*x));
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+(50*x), 290,60+(50*x));
		}
	myGLCD.print( "\x89o\xA0\xA2o""e y\x99""a\xA0""e\xA2\x9D""e", CENTER, 30);     // 
	myGLCD.print( "\x8Bop\xA1""a\xA4\x9Dpo\x97""a\xA2\x9D""e", CENTER, 80);      
	myGLCD.print( "      ", CENTER, 130);     
	myGLCD.print( txt_SD_menu4, CENTER, 180);      
}
void menu_formatSD()
{
  if (!card.init(spiSpeed, SD_CS_PIN)) 
	  {
		cout << pstr(
		 "\nSD initialization failure!\n"
		 "Is the SD card inserted correctly?\n"
		 "Is chip select correct at the top of this sketch?\n");
		sdError("card.init failed");
		 myGLCD.print("File System failed", CENTER, 120);

	  }

  cardSizeBlocks = card.cardSize();
  if (cardSizeBlocks == 0) sdError("cardSize");
  cardCapacityMB = (cardSizeBlocks + 2047)/2048;

  cout << pstr("Card Size: ") << cardCapacityMB;
  cout << pstr(" MB, (MB = 1,048,576 bytes)") << endl;

  Draw_menu_formatSD();

		// discard any input
	//while (Serial.read() >= 0) {} // Удалить все символы из буфера

	char c;

	while (true)
		{
		delay(10);
		if (myTouch.dataAvailable())
			{
				myTouch.read();
				int	x=myTouch.getX();
				int	y=myTouch.getY();

				if ((x>=30) && (x<=290))       // Upper row
					{
					if ((y>=20) && (y<=60))    // Button: 1
						{
							waitForIt(30, 20, 290, 60);
							myGLCD.clrScr();
							eraseCard();
						Draw_menu_formatSD();
						}
					if ((y>=70) && (y<=110))   // Button: 2
						{
							waitForIt(30, 70, 290, 110);
							myGLCD.clrScr();
							formatCard();
						Draw_menu_formatSD();
						}
					if ((y>=120) && (y<=160))  // Button: 3
						{
							waitForIt(30, 120, 290, 160);
							myGLCD.clrScr();
					
							Draw_menu_formatSD();
						}
					if ((y>=170) && (y<=220))  // Button: 4
						{
							waitForIt(30, 170, 290, 210);
							break;
						}
				}
			}
	   }

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

		else if ((x_osc>=70) && (x_osc<=120))                    //  Вход 1
			{

				waitForIt(70, 210, 120, 239);

					Channel1 = !Channel1;

				if (Channel1)
					{
						myGLCD.setColor(VGA_YELLOW);
						myGLCD.fillRoundRect (70, 200, 120, 205);
						myGLCD.setColor(VGA_LIME);
						myGLCD.setBackColor( VGA_LIME);
						myGLCD.fillRoundRect (70, 210, 120, 239);
						myGLCD.setColor(0, 0, 0);
						myGLCD.print("1", 92, 212);
						myGLCD.print("BXOD", 80, 226);
						osc_line_off1 = true;
					}
				else
					{
						myGLCD.setColor(0,0,0);
						myGLCD.setBackColor( 0,0,0);
						myGLCD.fillRoundRect (70, 200, 120, 205);
						myGLCD.fillRoundRect (70, 210, 120, 239);
						myGLCD.setColor(255, 255, 255);
						myGLCD.drawRoundRect (70, 210, 120, 239);
						myGLCD.print("1", 92, 212);
						myGLCD.print("BXOD", 80, 226);
					}

				chench_Channel();
				MinAnalog1 = 4095;
				MaxAnalog1 = 0;
			}
		else if ((x_osc>=130) && (x_osc<=180))                    //  Вход 2
			{
				waitForIt(130, 210, 180, 239);

					Channel2 = !Channel2;

				if (Channel2)
					{
						myGLCD.setColor(VGA_RED);
						myGLCD.fillRoundRect (130, 200, 180, 205);
						myGLCD.setColor(VGA_LIME);
						myGLCD.setBackColor( VGA_LIME);
						myGLCD.fillRoundRect (130, 210, 180, 239);
						myGLCD.setColor(0, 0,0);
						myGLCD.print("2", 152, 212);
						myGLCD.print("BXOD", 140, 226);
						osc_line_off2 = true;
					}
				else
					{
						myGLCD.setColor(0,0,0);
						myGLCD.setBackColor( 0,0,0);
						myGLCD.fillRoundRect (130, 210, 180, 239);
						myGLCD.fillRoundRect (130, 200, 180, 205);
						myGLCD.setColor(255, 255, 255);
						myGLCD.drawRoundRect (130, 210, 180, 239);
						myGLCD.print("2", 152, 212);
						myGLCD.print("BXOD", 140, 226);
					}
				chench_Channel();
				MinAnalog2 = 4095;
				MaxAnalog2 = 0;
			}
		else if ((x_osc>=190) && (x_osc<=240))                     //  Вход 3
			{
				waitForIt(190, 210, 240, 239);

					Channel3 = !Channel3;

				if (Channel3)
					{
						myGLCD.setColor(VGA_BLUE);
						myGLCD.fillRoundRect (190, 200, 240, 205);
						myGLCD.setColor(VGA_LIME);
						myGLCD.setBackColor( VGA_LIME);
						myGLCD.fillRoundRect (190, 210, 240, 239);
						myGLCD.setColor(0, 0, 0);
						myGLCD.print("3", 212, 212);
						myGLCD.print("BXOD", 200, 226);
						osc_line_off3 = true;
					}
				else
					{
						myGLCD.setColor(0,0,0);
						myGLCD.setBackColor( 0,0,0);
						myGLCD.fillRoundRect (190, 210, 240, 239);
						myGLCD.fillRoundRect (190, 200, 240, 205);
						myGLCD.setColor(255, 255, 255);
						myGLCD.drawRoundRect (190, 210, 240, 239);
						myGLCD.print("3", 212, 212);
						myGLCD.print("BXOD", 200, 226);
					}

				chench_Channel();
				MinAnalog3 = 4095;
				MaxAnalog3 = 0;
			}
	}
}

void switch_trig(int trig_x)
{
	switch (trig_x) 
					{
						case 1:
						 if (Channel1)
							{
								Channel_trig = 0x40;
								myGLCD.print(" ON ", 270, 226);
								MinAnalog = MinAnalog1 ;
								MaxAnalog = MaxAnalog1 ;
							}
						else
							{
								myGLCD.print(" OFF", 270, 226);
							}
						  break;
						case 2:

						if (Channel2)
							{
								Channel_trig = 0x20;
								myGLCD.print(" ON ", 270, 226);
								MinAnalog = MinAnalog2 ;
								MaxAnalog = MaxAnalog2 ;
							}
						else
							{
								myGLCD.print(" OFF", 270, 226);
							}
						  break;
						case 3:
						 if (Channel3)
							{
								Channel_trig = 0x10;
								myGLCD.print(" ON ", 270, 226);
								MinAnalog = MinAnalog3 ;
								MaxAnalog = MaxAnalog3 ;
							}
						else
							{
								myGLCD.print(" OFF", 270, 226);
							}
						  break;
						default: 

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

}
void trig_min_max(int trig_x)
{
	switch (trig_x) 
					{
						case 1:
						 if (Channel1)
							{
								MinAnalog = MinAnalog1 ;
								MaxAnalog = MaxAnalog1 ;
							}
						  break;
						case 2:

						if (Channel2)
							{
								MinAnalog = MinAnalog2 ;
								MaxAnalog = MaxAnalog2 ;
							}
						  break;
						case 3:
						 if (Channel3)
							{
								MinAnalog = MinAnalog3 ;
								MaxAnalog = MaxAnalog3 ;
							}
						  break;
						default: 

						 if (Channel0)
							{
								MinAnalog = MinAnalog0 ;
								MaxAnalog = MaxAnalog0 ;
							}

					}

}

void Draw_menu_ADC1()
{
	myGLCD.clrScr();
	myGLCD.setFont( BigFont);
	myGLCD.setBackColor(0, 0, 255);
	for (int x=0; x<4; x++)
		{
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect (30, 20+(50*x), 290,60+(50*x));
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (30, 20+(50*x), 290,60+(50*x));
		}
	myGLCD.print( txt_ADC_menu1, CENTER, 30);       // "Record data"
	myGLCD.print( txt_ADC_menu2, CENTER, 80);       // "List fales"
	myGLCD.print( txt_ADC_menu3, CENTER, 130);      // "Data to Serial"
	myGLCD.print( txt_ADC_menu4, CENTER, 180);      // "EXIT"
}
void menu_ADC()
{
//	while (Serial.read() >= 0) {} // Удалить все символы из буфера
	char c;

	while (true)
		{
		delay(10);
		if (myTouch.dataAvailable())
			{
				myTouch.read();
				int	x=myTouch.getX();
				int	y=myTouch.getY();

				if ((x>=30) && (x<=290))       // Upper row
					{
					if ((y>=20) && (y<=60))    // Button: 1
						{
							waitForIt(30, 20, 290, 60);
							logData();
						}
					if ((y>=70) && (y<=110))   // Button: 2
						{
							waitForIt(30, 70, 290, 110);
							root = sd.open("/");
							printDirectory(root, 0);
						}
					if ((y>=120) && (y<=160))  // Button: 3
						{
							waitForIt(30, 120, 290, 160);
							root = sd.open("/");
							print_serial(root, 0);
						}
					if ((y>=170) && (y<=220))  // Button: 4
						{
							waitForIt(30, 170, 290, 210);
							break;
						}
				}
			}
	   }
}

void preob_num_str() // Программа формирования имени файла, состоящего из текущей даты и счетчика файлов
{

	rtc_clock.get_time(&hh,&mm,&ss);
	rtc_clock.get_date(&dow,&dd,&mon,&yyyy);
	dow1=dow;
	sec = ss;       //Initialization time
	min = mm;
	hour = hh;
	date = dd;
	mon1 = mon;
	year = yyyy;

//	DateTime now = RTC.now();
	int year_temp = year-2000;

	itoa (year_temp,str_year_file, 10); // Преобразование даты год в строку ( 10 - десятичный формат) 

	
	if (mon1 <10)
		{
		   itoa (0,str_mon_file0, 10);                   //  Преобразование даты месяц  в строку ( 10 - десятичный формат) 
		   itoa (mon1,str_mon_file10, 10);        //  Преобразование числа в строку ( 10 - десятичный формат) 
		   sprintf(str_mon_file, "%s%s", str_mon_file0, str_mon_file10);  // Сложение 2 строк
		}
	else
		{
		   itoa (mon1,str_mon_file, 10);         // Преобразование числа в строку ( 10 - десятичный формат) 
		}


	if (date <10)
		{
		   itoa (0,str_day_file0, 10);                  // Преобразование числа в строку ( 10 - десятичный формат) 
		   itoa (date,str_day_file10, 10);         // Преобразование числа в строку ( 10 - десятичный формат) 
		   sprintf(str_day_file, "%s%s", str_day_file0, str_day_file10);  // Сложение 2 строк
		}
	else
		{
		itoa (date,str_day_file, 10);                   // Преобразование числа в строку ( 10 - десятичный формат) 
		}

		 
	if (file_name_count<10)
		{
			itoa (file_name_count,str0, 10);                 // Преобразование числа в строку ( 10 - десятичный формат) 
			sprintf(str_file_name_count, "%s%s", str_file_name_count0, str0);  // Сложение 2 строк
		}
	
	else
		{
			itoa (file_name_count,str_file_name_count, 10);  // Преобразование числа в строку ( 10 - десятичный формат) 
		}
	sprintf(str1, "%s%s",str_year_file, str_mon_file);       // Сложение 2 строк
	sprintf(str2, "%s%s",str1, str_day_file);                // Сложение 2 строк
	sprintf(timeName, "%s%s", str2, "00.TXT");                // Получение имени файла в file_name
	sprintf(binName, "%s%s", str2, "00.BIN");                // Получение имени файла в file_name
}

void printDirectory(File dir, int numTabs) 
{
	char* par;
	char ext_files[3];  
	char ext_bin[] = "BIN";
	bool view_on = true;
	int count_files = 1;
	int max_count_files = 1;
	int max_count_files1 = 1;
	int min_count_files = 1;
	int count_page = 1;
	int max_count_page = 1;
	int max_count_page1 = 1;
	int count_string = 0;
	int icount = 1;
	int icount_end = 1;
	int y_fcount_start = 1;
	int y_fcount_stop = 12;
	int y_fcount_step = 1;
	int old_fcount_start = 5;
	myGLCD.clrScr();
	myGLCD.setBackColor( 0, 0, 0);
	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255,255);

	 for( icount = 0 ;icount < 100; icount++) // Очистить память списка
	 {
		  for(int i = 0 ;i < 13; i ++)
		  {
			 list_files_tab[icount][i] = ' ';
			 size_files_tab[count_files] = 0;
		  }
	 }


   while(true)                              // Заполнить список файлов
   {
	 File entry =  dir.openNextFile();
	 if (! entry) 
		 {
		   // no more files
		   break;
		 }
	 entry.getName(list_files_tab[count_files], 13);
	 size_files_tab[count_files] = entry.size();
	 entry.close();
	 count_files++;
   }

			myGLCD.setFont( SmallFont);
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect (5, 214, 315, 239);          // Кнопка "ESC -> PUSH"
			myGLCD.fillRoundRect (90, 189, 130, 209);         // Кнопка "<<"
			myGLCD.fillRoundRect (140, 189, 180, 209);        // Вывод номера страницы
			myGLCD.fillRoundRect (190, 189, 230, 209);        // Кнопка ">>"
			myGLCD.fillRoundRect (150, 60, 300, 120);         // Кнопка "Просмотр файла"
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (5, 214, 315, 239);
			myGLCD.drawRoundRect (90, 189, 130, 209);
			myGLCD.drawRoundRect (140, 189, 180, 209);
			myGLCD.drawRoundRect (190, 189, 230, 209);
			myGLCD.drawRoundRect (2, 2, 318, 186);
			myGLCD.drawRoundRect (150, 60, 300, 120);        // Кнопка "Просмотр файла"
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print("Page N ",30, 193);
			myGLCD.setBackColor(0, 0, 255);
			myGLCD.print(txt_info11,CENTER, 221);            // Кнопка "ESC -> PUSH"
			myGLCD.setColor(VGA_YELLOW);
			myGLCD.print("<<",101, 193);
			myGLCD.print(">>",203, 193);
			myGLCD.setFont(BigFont);
			myGLCD.print("\x89poc\xA1o\xA4p",160, 70);       // Кнопка "Просмотр"
			myGLCD.print("\xA5""a\x9E\xA0""a",185, 90);      // Кнопка "файла"
			myGLCD.setFont( SmallFont);
			int count_str = 1;
			count_string = 0;

			 for( icount = 1;icount < count_files; icount++)  //Вывод списка файдов на экран
			   {
				   myGLCD.setBackColor(0, 0, 0);
				   myGLCD.setColor(255, 255, 255);
				   myGLCD.printNumI(icount,7, count_string+5);
				   myGLCD.print(list_files_tab[icount],35, count_string+5);
				   count_string +=15;
				   count_str ++;
				 
				   if ( count_str >12)
				   {
						if (icount != count_files-1)
						{
						   myGLCD.setColor(0, 0, 0);
						   myGLCD.fillRoundRect (3, 3, 140, 185);
						   count_string = 0;
						   count_str = 1;
						   count_page++;
						}
				   }
			   }
			max_count_files = count_files;
			max_count_page =  count_page;
			max_count_files1 = count_files;
			max_count_page1 =  count_page;
			myGLCD.setColor(VGA_YELLOW);
			myGLCD.setBackColor(0, 0, 255);
			// Вывод количества страниц списка файлов
			if (count_page < 10) 
				{
					myGLCD.print("    ",146, 193);
					myGLCD.printNumI(count_page,157, 193);
				}
			if (count_page >= 10 & count_page <100 )
				{
					myGLCD.print("    ",146, 193);
					myGLCD.printNumI(count_page,153, 193);
				}
			if (count_page >= 100 ) myGLCD.printNumI(count_page,148 , 193);

	while (true)
		{

		if (myTouch.dataAvailable())
			{
				myTouch.read();
				int	x=myTouch.getX();
				int	y=myTouch.getY();

				if ((y>=214) && (y<=239))            // 
					{
					if ((x>=95) && (x<=315))         // Выход
						{
							waitForIt(5, 214, 315, 239);
							break;
						}
					}

				if ((y>=189) && (y<=209))            // 
					{
					if ((x>= 90) && (x<=130))        // Кнопки перелистывания страниц "<<"
						{
							waitForIt(90, 189, 130, 209);
							myGLCD.setColor(VGA_YELLOW);
							myGLCD.setBackColor(0, 0, 255);
							count_page--;
							if (count_page < 1) count_page = 1;
							if (count_page < 10) 
								{
									myGLCD.setColor(VGA_YELLOW);
									myGLCD.print("    ",146, 193);
									myGLCD.printNumI(count_page,157, 193);
								}
							if (count_page >= 10 & count_page <100 )
								{
									myGLCD.setColor(VGA_YELLOW);
									myGLCD.print("    ",146, 193);
									myGLCD.printNumI(count_page,153, 193);
								}
							if (count_page >= 100 ) myGLCD.printNumI(count_page,148 , 193);

							max_count_files = count_page * 12;
							min_count_files = max_count_files - 12;
							if (min_count_files <0 ) min_count_files = 0;
							if (max_count_files > count_files ) max_count_files = count_files-1;
							count_string = 0;
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect (3, 3, 140, 185);
							for( icount = min_count_files+1; icount < max_count_files+1; icount++)
							   {
								   myGLCD.setBackColor(0, 0, 0);
								   myGLCD.setColor(255, 255, 255);
								   myGLCD.print("   ",7, count_string+5);
								   myGLCD.printNumI(icount,7, count_string+5);
								   myGLCD.print(list_files_tab[icount],35, count_string+5);
								   count_string +=15;
							   }
						}
					if ((x>=190) && (x<=230))     // Кнопки перелистывания страниц "<<"
						{
							waitForIt(190, 189, 230, 209);
							myGLCD.setColor(VGA_YELLOW);
							myGLCD.setBackColor(0, 0, 255);
							count_page++;
							if (count_page > max_count_page) count_page = max_count_page;
							if (count_page < 10) 
								{
									myGLCD.setColor(VGA_YELLOW);
									myGLCD.print("    ",146, 193);
									myGLCD.printNumI(count_page,157, 193);
								}
							if (count_page >= 10 & count_page <100 )
								{
									myGLCD.setColor(VGA_YELLOW);
									myGLCD.print("    ",146, 193);
									myGLCD.printNumI(count_page,153, 193);
								}
							if (count_page >= 100 ) myGLCD.printNumI(count_page,148 , 193);

							max_count_files = count_page * 12;
							min_count_files = max_count_files - 12;
							if (min_count_files < 0 ) min_count_files = 0;
							if (max_count_files > count_files ) max_count_files = count_files-1;
							count_string = 0;
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect (3, 3, 140, 185);
							for( icount = min_count_files+1; icount < max_count_files+1; icount++)
							   {
								   myGLCD.setBackColor(0, 0, 0);
								   myGLCD.setColor(255, 255, 255);
								   myGLCD.print("   ",7, count_string+5);
								   myGLCD.printNumI(icount,7, count_string+5);
								   myGLCD.print(list_files_tab[icount],35, count_string+5);
								   count_string +=15;
							   }
						}
					}

					 y_fcount_start = 5;

					if ((x>= 30) && (x<=136))            //  Выбор файла из списка
						{
							if (count_page == max_count_page1)
								{
									y_fcount_stop = 11- ( (max_count_page1 * 12) - max_count_files1);
								}
							else
							{
								y_fcount_stop = 12;
							}

							for(y_fcount_step = 0; y_fcount_step < y_fcount_stop; y_fcount_step++)
								{
									if ((y>=y_fcount_start) && (y<=y_fcount_start+12))         // 
										{
											myGLCD.setColor(0, 0, 0);
											myGLCD.setBackColor(0, 0, 0);
											myGLCD.drawRoundRect (30, old_fcount_start, 136, old_fcount_start+12);
											waitForIt(30, y_fcount_start, 136, y_fcount_start+12);
											old_fcount_start = y_fcount_start;
											set_files = ((count_page-1) * 12)+y_fcount_step+1;
											myGLCD.setColor(VGA_YELLOW);
											myGLCD.print(list_files_tab[set_files],170, 30);     // номер файла в позиции "set_files"

											ext_files[0] = list_files_tab[set_files][9];         // Исключить просмотр BIN файлов
											ext_files[1] = list_files_tab[set_files][10];
											ext_files[2] = list_files_tab[set_files][11];

										if (ext_files[0] == 'B' && ext_files[1] == 'I' && ext_files[2] == 'N' )
											{
												view_on = false;
												myGLCD.setFont(BigFont);
												myGLCD.print("He\x97o\x9C\xA1o\x9B""e\xA2",146, 130);     
												myGLCD.setFont( SmallFont);
											}
										else
											{
												view_on = true;
												myGLCD.setFont(BigFont);
												myGLCD.print("          ",146, 130);     // номер файла в позиции "set_files"
												myGLCD.setFont( SmallFont);
											}
					   
										}
									 y_fcount_start += 15;
								 }
						}

					if ((x>= 150) && (x<=300))                                 //  
						{
						  if ((y>= 60) && (y<=120))                            //  Выбор
							{
								waitForIt(150, 60, 300, 120);
								myGLCD.clrScr();
								if(view_on) readFile();                                    // Просмотр файла
								myGLCD.clrScr();
								myGLCD.setFont( SmallFont);
								myGLCD.setColor(0, 0, 255);
								myGLCD.fillRoundRect (5, 214, 315, 239);       // Кнопка "ESC -> PUSH"
								myGLCD.fillRoundRect (90, 189, 130, 209);      // Кнопка "<<"
								myGLCD.fillRoundRect (140, 189, 180, 209);     // Вывод номера страницы
								myGLCD.fillRoundRect (190, 189, 230, 209);     // Кнопка ">>"
								myGLCD.fillRoundRect (150, 60, 300, 120);      // Кнопка "Просмотр файла"
								myGLCD.setColor(255, 255, 255);
								myGLCD.drawRoundRect (5, 214, 315, 239);
								myGLCD.drawRoundRect (90, 189, 130, 209);
								myGLCD.drawRoundRect (140, 189, 180, 209);
								myGLCD.drawRoundRect (190, 189, 230, 209);
								myGLCD.drawRoundRect (2, 2, 318, 186);
								myGLCD.drawRoundRect (150, 60, 300, 120);      // Кнопка "Просмотр файла"
								myGLCD.setBackColor(0, 0, 0);
								myGLCD.print("Page N ",30, 193);
								myGLCD.setBackColor(0, 0, 255);
								myGLCD.print(txt_info11,CENTER, 221);          // Кнопка "ESC -> PUSH"
								myGLCD.setColor(VGA_YELLOW);
								myGLCD.print("<<",101, 193);
								myGLCD.print(">>",203, 193);
								myGLCD.setFont(BigFont);
								myGLCD.print("\x89poc\xA1o\xA4p",160, 70);     // Кнопка "Просмотр"
								myGLCD.print("\xA5""a\x9E\xA0""a",185, 90);    // Кнопка "файла"
								myGLCD.setFont( SmallFont);
								myGLCD.setColor(VGA_YELLOW);
								myGLCD.setBackColor(0, 0, 255);
								if (count_page > max_count_page) count_page = max_count_page;
								if (count_page < 10) 
									{
										myGLCD.setColor(VGA_YELLOW);
										myGLCD.print("    ",146, 193);
										myGLCD.printNumI(count_page,157, 193);
									}
								if (count_page >= 10 & count_page <100 )
									{
										myGLCD.setColor(VGA_YELLOW);
										myGLCD.print("    ",146, 193);
										myGLCD.printNumI(count_page,153, 193);
									}
								if (count_page >= 100 ) myGLCD.printNumI(count_page,148 , 193);

								max_count_files = count_page * 12;
								min_count_files = max_count_files - 12;
								if (min_count_files < 0 ) min_count_files = 0;
								if (max_count_files > count_files ) max_count_files = count_files-1;
								count_string = 0;
								myGLCD.setColor(0, 0, 0);
								myGLCD.fillRoundRect (3, 3, 140, 185);
								for( icount = min_count_files+1; icount < max_count_files+1; icount++)
								   {
									   myGLCD.setBackColor(0, 0, 0);
									   myGLCD.setColor(255, 255, 255);
									   myGLCD.print("   ",7, count_string+5);
									   myGLCD.printNumI(icount,7, count_string+5);
									   myGLCD.print(list_files_tab[icount],35, count_string+5);
									   count_string +=15;
								   }
							}
						}
				}
	   }
	Draw_menu_ADC1();
}

void view_file()  //  Не применяется
{
	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print(txt_info12,CENTER, 40);
	block_t buf;
	uint32_t count = 0;
	uint32_t count1 = 0;
	koeff_h = 7.759*4;
	int xpos = 0;
	int ypos1;
	int ypos2;
	int kl[buf.count];         //Текущий блок

	root = sd.open(list_files_tab[set_files]);
		if (!root.isOpen()) 
		{
			Serial.println(F("No current root file"));
			return;
		}

	root.rewind();

	if (root.read(&buf , 512) != 512) 
	{
		error("Read metadata failed");
	}


	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info11,CENTER, 200);
	myGLCD.setColor(255, 255, 255);
	delay(1000);
	myGLCD.clrScr();
	LongFile = 0;
	DrawGrid1();
	myGLCD.setColor(0, 0, 255);
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.fillRoundRect (250, 90, 310, 130);
	myGLCD.setColor( 255, 255, 255);
	myGLCD.drawRoundRect (250, 90, 310, 130);
	myGLCD.setBackColor( 0, 0, 255);
	myGLCD.setColor( 255, 255, 255);
	myGLCD.setFont( SmallFont);
	myGLCD.print("V/del.", 260, 95);
	myGLCD.print("      ", 260, 110);
	if (mode1 == 0)myGLCD.print("1", 275, 110);
	if (mode1 == 1)myGLCD.print("0.5", 268, 110);
	if (mode1 == 2)myGLCD.print("0.2", 268, 110);
	if (mode1 == 3)myGLCD.print("0.1", 268, 110);

   while (root.read(&buf , 512) == 512) 
	{
		if (buf.count == 0) break;
		//if (buf.overrun) 
		//	{
		//		Serial.print(F("OVERRUN,"));
		//		Serial.println(buf.overrun);
		//	}

		DrawGrid1();

		 if (myTouch.dataAvailable())
			{
				delay(10);
				myTouch.read();
				x_osc=myTouch.getX();
				y_osc=myTouch.getY();

				if ((x_osc>=2) && (x_osc<=240))  //  Delay Button
					{
						if ((y_osc>=1) && (y_osc<=160))  // Delay row
						{
							break;
						} 
					}
			if ((x_osc>=250) && (x_osc<=310))  //  Delay Button
			  {

				 if ((y_osc>=90) && (y_osc<=130))  // Port select   row
					 {
						waitForIt(250, 90, 310, 130);
						mode1 ++ ;
						myGLCD.clrScr();
						myGLCD.setColor(0, 0, 255);
						myGLCD.fillRoundRect (250, 90, 310, 130);
						myGLCD.setColor( 255, 255, 255);
						myGLCD.drawRoundRect (250, 90, 310, 130);
				//		buttons();
						if (mode1 > 3) mode1 = 0;   
						if (mode1 == 0) koeff_h = 7.759*4;
						if (mode1 == 1) koeff_h = 3.879*4;
						if (mode1 == 2) koeff_h = 1.939*4;
						if (mode1 == 3) koeff_h = 0.969*4;
					//	print_set();
						myGLCD.setBackColor( 0, 0, 255);
						myGLCD.setColor( 255, 255, 255);
						myGLCD.setFont( SmallFont);
						myGLCD.print("V/del.", 260, 95);
						myGLCD.print("      ", 260, 110);
						if (mode1 == 0)myGLCD.print("1", 275, 110);
						if (mode1 == 1)myGLCD.print("0.5", 268, 110);
						if (mode1 == 2)myGLCD.print("0.2", 268, 110);
						if (mode1 == 3)myGLCD.print("0.1", 268, 110);
					 }
			  }
		   }

		for (uint16_t i = 0; i < buf.count; i++) 
		{

				Sample[xpos] = buf.data[i];//.adc[0]; 
				xpos++;
			if(xpos == 240)
				{
				DrawGrid1();
				for( int xpos = 0; xpos < 239;	xpos ++)
					{
						//  Стереть предыдущий экран
						myGLCD.setColor( 0, 0, 0);
						ypos1 = 255-(OldSample[ xpos + 1]/koeff_h) - hpos; 
						ypos2 = 255-(OldSample[ xpos + 2]/koeff_h) - hpos;

						if(ypos1<0) ypos1 = 0;
						if(ypos2<0) ypos2 = 0;
						if(ypos1>200) ypos1 = 200;
						if(ypos2>200) ypos2 = 200;
						myGLCD.drawLine (xpos + 1, ypos1, xpos + 2, ypos2);
						if (xpos == 0) myGLCD.drawLine (xpos + 1, 1, xpos + 1, 220);
						myGLCD.setColor( 255, 255, 255);
						ypos1 = 255-(Sample[ xpos]/koeff_h) - hpos;
						ypos2 = 255-(Sample[ xpos + 1]/koeff_h)- hpos;

						if(ypos1<0) ypos1 = 0;
						if(ypos2<0) ypos2 = 0;
						if(ypos1>220) ypos1 = 200;
						if(ypos2>220) ypos2 = 200;
						myGLCD.drawLine (xpos, ypos1, xpos + 1, ypos2);
						OldSample[xpos] = Sample[ xpos];
					}
					xpos = 0;
					myGLCD.setFont( BigFont);
					myGLCD.setBackColor( 0, 0, 0);
					count1++;
					myGLCD.printNumI(count, RIGHT, 220);// 
					myGLCD.setColor(VGA_LIME);
					myGLCD.printNumI(count1*240, LEFT, 220);// 
				}
		}
	}
	koeff_h = 7.759*4;
	mode1 = 0;
	Trigger = 0;
	myGLCD.setFont( BigFont);
	myGLCD.setColor(VGA_YELLOW);
	myGLCD.print(txt_info9,CENTER, 80);
	myGLCD.setColor(255, 255, 255);
	delay(500);
	while (myTouch.dataAvailable()){}

}

//------------------------------------------------------------------------------
// read and print CSV file
void readFile()
{
	// Программа чтения данных из файла и отображения на дисплее
	int data;                    // Дата для сиволов
	int data1;                   // Дата1 для цифровой информации
	uint32_t step_file = 0;      // Не применяется
	int pin_fcount = 0;          // Обход (переключение) используемых входов
	int max_pin_fcount = 0;      // Максимальное количество применяемых входов 
	bool start_pin = false;      // Признак разрешения определения используемых входов
	bool start_mod = false;      //
	bool stop_mod = false;       //
	bool stop_view = true;       //
	bool stop_return = false;    // Признак завершения программы
	uint32_t  File_size;         // переменная хранения размера файла 
	Page_count = 0;              // Счетчик страниц (по 240 точек)
	int Page_count_temp = 0;     // Счетчик страниц (по 240 точек) (Временный)
	x_pos_count = 0;             // Счетчик позиции по оси Х
	Channel0 = false;            // Сбросить признак включения канала 
	Channel1 = false;            // Сбросить признак включения канала 
	Channel2 = false;            // Сбросить признак включения канала 
	Channel3 = false;            // Сбросить признак включения канала 
	DrawGrid();                  // Отобразить сетку
	char chanel_base[4];         // База для хранения применяемых входов

	for (int p = 0; p < 10; p++)  // Очистить память хранения страниц 
		{
			for (int x = 0; x < 240; x++)
			{
				PageSample_osc[x][p][0] = 0;
				PageSample_osc[x][p][1] = 0;
				PageSample_osc[x][p][2] = 0;
				PageSample_osc[x][p][3] = 0;
			}

			PageSample_Num[p] = 0;
		}

	root = sd.open(list_files_tab[set_files]);                                                     // Открыть выбранный файл
	if (!root.isOpen())                                                                            // Прверка на ошибку открытия файла
		{
			Serial.println(F("No current root file"));
			return;
		}

	myGLCD.setFont(BigFont);                                                                       // Подписать кнопки управления просмотром 
	myGLCD.print("ESC", 260 , 13);                                                                 // Выход из просмотра
	myGLCD.print("<=", 265 , 56);                                                                  // Уменьшить номер страницы
	myGLCD.print("\x89\x8A""CK", 253 , 102);                                                       // "Пуск" Старт просмотра
	myGLCD.print("=>", 268 , 147);                                                                 // Увеличить номер страницы
//	myGLCD.print("CTO\x89", 253 , 211);                                                            // "Стоп" остановить просмотр

	root.rewind();                                                                                 // Установить в начало
	File_size = root.fileSize();                                                                   // Получить размер файла 

	myGLCD.setFont( SmallFont);
	myGLCD.print("Pa\x9C\xA1""ep \xA5""a\x9E\xA0""a", 5, 168);                                     // "Размер файла"
	myGLCD.print("\x89o\x9C. \x97 \xA5""a\x9E\xA0""e             ", 5, 183);                       // "Поз. в файле"
	myGLCD.setFont(BigFont);
	myGLCD.printNumI(File_size, 105, 165);                                                         //  Отобразить размер файла
	myGLCD.print("C\xA4p", 270, 180);                                                              // "Стр"

	while ((data = root.read()) >= 0)                                                              //
		{
			step_file = root.position();  
			if (data =='@' ) start_pin = true;                                                     // Определение включенных входов
				if (start_pin == true && start_mod == false )                                      // Начало измерения по символу @
					{                                                                              // Номера входов записываются в chanel_base[]
						if (data =='0')                                                            // По окончании работы программы  max_pin_fcount содержит количество входов
							{                                                                      // По окончании работы программы в chanel_base[] прописаны применяемые входа
								Channel0 = true;
								chanel_base[max_pin_fcount] = 0;
								max_pin_fcount ++;
							}
						else if (data =='1') 
							{
								Channel1 = true;
								chanel_base[max_pin_fcount] = 1;
								max_pin_fcount ++;
							}
						else if (data =='2') 
							{
								Channel2 = true;
								chanel_base[max_pin_fcount] = 2;
								max_pin_fcount ++;
							}
						else if (data =='3')
							{
								Channel3 = true;
								chanel_base[max_pin_fcount] = 3;
								max_pin_fcount ++;
							}
					}

			if (data =='#' ) 
				{
					start_mod = true;                                                                 // Разрешить получение цифровых данных из файла
					start_pin = false;                                                                // Запретить определение используемых входов 
					buttons_channel();                                                                // Отобразить кнопки переключения входов
				}

			if (stop_return == true) break;
			if (start_mod == true && stop_mod == false )                                              // Получение разрешено, признак окончания не обнаружен   
				{
				   data1 = root.parseInt();                                                           // Получить цифровые данные 
					if (data1 != 5555)                                                                //  Поиск окончания данных, признак окончания данных (5555) не обнаружен
						{
							PageSample_osc[x_pos_count][Page_count][chanel_base[pin_fcount]] = data1; // Записать данные в буфер страниц (один канал)

							pin_fcount++;                                                             // Переключить на следующий канал
							if (pin_fcount>max_pin_fcount-1)                                          // Проверка на достижение последнего канала
								{
									pin_fcount=0;                                                     // Установить  в начало первый по очереди канал
									x_pos_count++;                                                    // Установить следующую позицию по оси Х
									PageSample_Num[Page_count] = step_file;                           // Координаты страницы в файле
									if(x_pos_count > 239)                                             // Достигнута последняя позиция, Все данные записаны, Можно отобразить страницу
										{
											x_pos_count = 0;                                          // Установить в начало 
											Page_count_temp = Page_count; 

										do {
											if (myTouch.dataAvailable())                              // Проверить нажатие клавиши
												{
													delay(10);
													myTouch.read();
													x_osc=myTouch.getX();
													y_osc=myTouch.getY();
												if ((x_osc>=250) && (x_osc<=318))                    // Боковые кнопки
													{
														if ((y_osc>=1) && (y_osc<=40))               // Первая  "Выход"
															{
																waitForIt(250, 1, 318, 40);
																stop_return = true;                  // Подготовить выход из первого цикла
																break;                               // Выйти из второго цикла
															}
														if ((y_osc>=45) && (y_osc<=85))              // Вторая - "<="
															{
																waitForIt(250, 45, 318, 85);
																Page_count_temp--;
																if (Page_count_temp < 0) Page_count_temp = 9;
																myGLCD.printNumI(Page_count_temp, 250, 180);                  // Отобразить № страницы
																myGLCD.setColor(0, 0, 0);
																myGLCD.fillRoundRect (105, 180, 200, 195);                    // Очистить зону вывода
																myGLCD.setColor(255, 255, 255);
																myGLCD.printNumI(PageSample_Num[Page_count_temp], 105, 180);  // 
																view_read_file(Page_count_temp);                              // Вызвать программу отображения информации ??
															}
														if ((y_osc>=90) && (y_osc<=130))                                      // Третья - "Пуск"
															{
																waitForIt(250, 90, 318, 130);
																stop_view = true;
															}
														if ((y_osc>=135) && (y_osc<=175))                                     // Четвертая "=>"
															{
																waitForIt(250, 135, 318, 175);
																Page_count_temp++;
																if (Page_count_temp > 9) Page_count_temp = 0;
																myGLCD.printNumI(Page_count_temp, 250, 180);
																myGLCD.setColor(0, 0, 0);
																myGLCD.fillRoundRect (105, 180, 200, 195);                    // Очистить зону вывода
																myGLCD.setColor(255, 255, 255);
																myGLCD.printNumI(PageSample_Num[Page_count_temp], 105, 180);
																view_read_file(Page_count_temp);                              // Вызвать программу отображения информации ??
															}
													}	
													if ((x_osc>=2) && (x_osc<=240))                                           //  Область экрана кнопки  "Стоп"
														{
															if ((y_osc>=1) && (y_osc<=160))                                   // 
															{
																stop_view = false;
															} 
														}
												}
										  } while (!stop_view);
											
										//----------------------------------------
										  if (stop_view == true)                                                             // Если просмотр разрешен, начать росмотр страницы
												{
													myGLCD.setFont(BigFont);
													myGLCD.setBackColor(0, 0, 0);
													myGLCD.setColor(255, 255, 255);
													myGLCD.printNumI(Page_count, 250, 180);                                  // Номер страницы
													myGLCD.setColor(0, 0, 0);
													myGLCD.fillRoundRect (105, 180, 200, 195);                               // Очистить зону вывода
													myGLCD.setColor(255, 255, 255);
													myGLCD.printNumI(PageSample_Num[Page_count], 105, 180);                  // 
													view_read_file(Page_count);                                              // Вызвать программу отображения информации ??
												}
							
											Page_count++;                                                                    // Установить следующую страницу
											if(Page_count>9) Page_count = 0;                                                 // Не больше 10 страниц
										}
								}
						}                                          // Завершение программы поиска.  Признак окончания данных (5555)  обнаружен
					else
						{
							start_mod = false;
						}
				}
		 }
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.setColor (0, 0, 0);                                     // Стереть надписи неиспользованных кнопок
	myGLCD.print("\x89\x8A""CK", 253 , 102);                       // "Пуск" Старт просмотра
	myGLCD.print("CTO\x89", 253 , 211);                            // "Стоп" остановить просмотр

	myGLCD.setColor (255, 255, 255);
	
	myGLCD.drawLine(250, 90, 318, 130);                            // Перечеркнуть неиспользуемые кнопки
	myGLCD.drawLine(250, 130, 318, 90);                            // Перечеркнуть неиспользуемые кнопки

	myGLCD.drawLine(250, 200, 318, 239);                           // Перечеркнуть неиспользуемые кнопки
	myGLCD.drawLine(250, 239, 318, 200);                           // Перечеркнуть неиспользуемые кнопки


	 while(1)
	 {
		if (myTouch.dataAvailable())                               // Проверить нажатие клавиши
			{
				delay(10);
				myTouch.read();
				x_osc=myTouch.getX();
				y_osc=myTouch.getY();
			if ((x_osc>=250) && (x_osc<=318))                     // Боковые кнопки
				{
					if ((y_osc>=1) && (y_osc<=40))                // Первая  "Выход"
						{
							waitForIt(250, 1, 318, 40);
							break;                                // Выйти из второго цикла
						}
					if ((y_osc>=45) && (y_osc<=85))               // Вторая - "<="
						{
							waitForIt(250, 45, 318, 85);
							Page_count_temp--;
							if (Page_count_temp < 0) Page_count_temp = 9;
							myGLCD.printNumI(Page_count_temp, 250, 180);                  // Отобразить № страницы
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect (105, 180, 200, 195);                    // Очистить зону вывода
							myGLCD.setColor(255, 255, 255);
							myGLCD.printNumI(PageSample_Num[Page_count_temp], 105, 180);  // 
							view_read_file(Page_count_temp);                              // Вызвать программу отображения информации ??
						}

					if ((y_osc>=135) && (y_osc<=175))                                     // Четвертая "=>"
						{
							waitForIt(250, 135, 318, 175);
							Page_count_temp++;
							if (Page_count_temp > 9) Page_count_temp = 0;
							myGLCD.printNumI(Page_count_temp, 250, 180);
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect (105, 180, 200, 195);                    // Очистить зону вывода
							myGLCD.setColor(255, 255, 255);
							myGLCD.printNumI(PageSample_Num[Page_count_temp], 105, 180);
							view_read_file(Page_count_temp);                              // Вызвать программу отображения информации ??
						}
				}			
			}
		}

	root.close();
}

void view_read_file(int view_page)
{
	int xpos;
	int ypos1;
	int ypos2;
	int ypos_osc1_0;
	int ypos_osc1_1;
	int ypos_osc1_2;
	int ypos_osc1_3;
	int ypos_osc2_0;
	int ypos_osc2_1;
	int ypos_osc2_2;
	int ypos_osc2_3;
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRoundRect (1, 1,239, 159);
	DrawGrid1();

	for (xpos = 0; xpos < 239; xpos++)
		 {
			if (xpos == 0)
				{
					myGLCD.setColor( 0, 0, 0);
					myGLCD.drawLine (xpos + 1, 1, xpos + 1, 220);
					myGLCD.drawLine (xpos + 2, 1, xpos + 2, 220);
				}

			if (Channel0)
					{
						if (xpos == 0)					// определить начальную позицию по Х 
							{
								myGLCD.setColor( 255, 255, 255);
								ypos_osc1_0 = 255-(PageSample_osc[xpos][view_page][0]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(PageSample_osc[xpos][view_page][0]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( 255, 255, 255);
								ypos_osc1_0 = 255-(PageSample_osc[xpos-1][view_page][0]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(PageSample_osc[xpos][view_page][0]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}

				if (Channel1)
					{

						if (xpos == 0)
							{
								myGLCD.setColor( VGA_YELLOW);
								ypos_osc1_0 = 255-(PageSample_osc[xpos][view_page][1]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(PageSample_osc[xpos][view_page][1]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( VGA_YELLOW);
								ypos_osc1_0 = 255-(PageSample_osc[xpos-1][view_page][1]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(PageSample_osc[xpos][view_page][1]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}
				
				if (Channel2)
					{

							if (xpos == 0)
							{
								myGLCD.setColor( VGA_RED);
								ypos_osc1_0 = 255-(PageSample_osc[xpos][view_page][2]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(PageSample_osc[xpos][view_page][2]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( VGA_RED);
								ypos_osc1_0 = 255-(PageSample_osc[xpos-1][view_page][2]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(PageSample_osc[xpos][view_page][2]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}
				
				if (Channel3)
					{

						if (xpos == 0)
							{
								myGLCD.setColor( VGA_BLUE);
								ypos_osc1_0 = 255-(PageSample_osc[xpos][view_page][3]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(PageSample_osc[xpos][view_page][3]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos , ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
						else
							{
								myGLCD.setColor( VGA_BLUE);
								ypos_osc1_0 = 255-(PageSample_osc[xpos-1][view_page][3]/koeff_h) - hpos;
								ypos_osc2_0 = 255-(PageSample_osc[xpos][view_page][3]/koeff_h)- hpos;
								if(ypos_osc1_0 < 0) ypos_osc1_0 = 0;
								if(ypos_osc2_0 < 0) ypos_osc2_0 = 0;
								if(ypos_osc1_0 > 220) ypos_osc1_0  = 220;
								if(ypos_osc2_0 > 220) ypos_osc2_0 = 220;
								myGLCD.drawLine (xpos - 1, ypos_osc1_0, xpos, ypos_osc2_0+2);
							}
					}
		 }
		 delay(100);
	//while (!myTouch.dataAvailable()){}
	//delay(50);
	//while (myTouch.dataAvailable()){}
}
void measure_power()
{                                            // Программа измерения напряжения питания с делителем 1/3 
											 // Установить резистивный делитель +15к общ 10к на разъем питания
		uint32_t logTime1 = 0;
		logTime1 = millis();
		if(logTime1 - StartSample > 500)  //  индикация 
		  {
			StartSample = millis();
			int m_power = 0;
			float ind_power = 0;
			ADC_CHER = 0x04;                         // Подключить канал А5, разрядность 12
			ADC_CR = ADC_START ; 	                 // Запустить преобразование
			while (!(ADC_ISR_DRDY));                 // Ожидание конца преобразования
			m_power =  ADC->ADC_CDR[2];              // Считать данные с канала А5
			ind_power = m_power * 0.0008056*3;       // Получить напряжение в вольтах
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.setFont(SmallSymbolFont);
			if (ind_power > 8 )
				{
					myGLCD.setColor(VGA_LIME);
					myGLCD.drawRoundRect (279,149, 319, 189);
					myGLCD.setColor(VGA_WHITE);
					myGLCD.print( "\x20", 295, 155);  
				}
			else if (ind_power > 7 && ind_power < 8 ) 
				{
					myGLCD.setColor(VGA_LIME);
					myGLCD.drawRoundRect (279,149, 319, 189);
					myGLCD.setColor(VGA_WHITE);
					myGLCD.print( "\x21", 295, 155);  
				}
			else if (ind_power > 6 && ind_power < 7 )
				{
					myGLCD.setColor(VGA_WHITE);
					myGLCD.drawRoundRect (279,149, 319, 189);
					myGLCD.setColor(VGA_WHITE);
					myGLCD.print( "\x22", 295, 155);  
				}
			else if (ind_power > 5 && ind_power < 6 )
				{
					myGLCD.setColor(VGA_YELLOW);
					myGLCD.drawRoundRect (279,149, 319, 189);
					myGLCD.setColor(VGA_WHITE);
					myGLCD.print( "\x23", 295, 155); 
				}
			else if (ind_power < 5 )
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

void print_serial(File dir, int numTabs) 
{
	// Программа выбора файла для передачи в КОМ порт
	char* par;
	int count_files = 1;
	int max_count_files = 1;
	int max_count_files1 = 1;
	int min_count_files = 1;
	int count_page = 1;
	int max_count_page = 1;
	int max_count_page1 = 1;
	int count_string = 0;
	int icount = 1;
	int icount_end = 1;
	int y_fcount_start = 1;
	int y_fcount_stop = 12;
	int y_fcount_step = 1;
	int old_fcount_start = 5;
	myGLCD.clrScr();
	myGLCD.setBackColor( 0, 0, 0);
	myGLCD.setFont( SmallFont);
	myGLCD.setColor (255, 255,255);

	 for( icount = 0 ;icount < 100; icount++) // Очистить память списка
	 {
		  for(int i = 0 ;i < 13; i ++)
		  {
			 list_files_tab[icount][i] = ' ';
			 size_files_tab[count_files] = 0;
		  }
	 }


   while(true)                              // Заполнить список файлов
   {
	 File entry =  dir.openNextFile();
	 if (! entry) 
		 {
		   // no more files
		   break;
		 }
	 entry.getName(list_files_tab[count_files], 13);
	 size_files_tab[count_files] = entry.size();
	 entry.close();
	 count_files++;
   }

			myGLCD.setFont( SmallFont);
			myGLCD.setColor(0, 0, 255);
			myGLCD.fillRoundRect (5, 214, 315, 239);          // Кнопка "ESC -> PUSH"
			myGLCD.fillRoundRect (90, 189, 130, 209);         // Кнопка "<<"
			myGLCD.fillRoundRect (140, 189, 180, 209);        // Вывод номера страницы
			myGLCD.fillRoundRect (190, 189, 230, 209);        // Кнопка ">>"
			myGLCD.fillRoundRect (150, 60, 300, 120);         // Кнопка "Отправить файл"
			myGLCD.setColor(255, 255, 255);
			myGLCD.drawRoundRect (5, 214, 315, 239);
			myGLCD.drawRoundRect (90, 189, 130, 209);
			myGLCD.drawRoundRect (140, 189, 180, 209);
			myGLCD.drawRoundRect (190, 189, 230, 209);
			myGLCD.drawRoundRect (2, 2, 318, 186);
			myGLCD.drawRoundRect (150, 60, 300, 120);        // Кнопка "Отправить файл"
			myGLCD.setBackColor(0, 0, 0);
			myGLCD.print("Page N ",30, 193);
			myGLCD.setBackColor(0, 0, 255);
			myGLCD.print(txt_info11,CENTER, 221);            // Кнопка "ESC -> PUSH"
			myGLCD.setColor(VGA_YELLOW);
			myGLCD.print("<<",101, 193);
			myGLCD.print(">>",203, 193);
			myGLCD.setFont(BigFont);
			myGLCD.print("O\xA4\xA3pa\x97\x9D\xA4\xAC",153, 70);           // Кнопка "Отправить файл"
			myGLCD.print("\xA5""a\x9E\xA0",195, 90);                       // Кнопка "Отправить файл"
			myGLCD.setFont( SmallFont);
			int count_str = 1;
			count_string = 0;

			 for( icount = 1;icount < count_files; icount++)  //Вывод списка файлов на экран
			   {
				   myGLCD.setBackColor(0, 0, 0);
				   myGLCD.setColor(255, 255, 255);
				   myGLCD.printNumI(icount,7, count_string+5);
				   myGLCD.print(list_files_tab[icount],35, count_string+5);
				   count_string +=15;
				   count_str ++;
				 
				   if ( count_str >12)
				   {
						if (icount != count_files-1)
						{
						   myGLCD.setColor(0, 0, 0);
						   myGLCD.fillRoundRect (3, 3, 140, 185);
						   count_string = 0;
						   count_str = 1;
						   count_page++;
						}
				   }
			   }
			max_count_files = count_files;
			max_count_page =  count_page;
			max_count_files1 = count_files;
			max_count_page1 =  count_page;
			myGLCD.setColor(VGA_YELLOW);
			myGLCD.setBackColor(0, 0, 255);
			// Вывод количества страниц списка файлов
			if (count_page < 10) 
				{
					myGLCD.print("    ",146, 193);
					myGLCD.printNumI(count_page,157, 193);
				}
			if (count_page >= 10 & count_page <100 )
				{
					myGLCD.print("    ",146, 193);
					myGLCD.printNumI(count_page,153, 193);
				}
			if (count_page >= 100 ) myGLCD.printNumI(count_page,148 , 193);

	while (true)
		{

		if (myTouch.dataAvailable())
			{
				myTouch.read();
				int	x=myTouch.getX();
				int	y=myTouch.getY();

				if ((y>=214) && (y<=239))            // 
					{
					if ((x>=95) && (x<=315))         // Выход
						{
							waitForIt(5, 214, 315, 239);
							break;
						}
					}

				if ((y>=189) && (y<=209))            // 
					{
					if ((x>= 90) && (x<=130))        // Кнопки перелистывания страниц "<<"
						{
							waitForIt(90, 189, 130, 209);
							myGLCD.setColor(VGA_YELLOW);
							myGLCD.setBackColor(0, 0, 255);
							count_page--;
							if (count_page < 1) count_page = 1;
							if (count_page < 10) 
								{
									myGLCD.setColor(VGA_YELLOW);
									myGLCD.print("    ",146, 193);
									myGLCD.printNumI(count_page,157, 193);
								}
							if (count_page >= 10 & count_page <100 )
								{
									myGLCD.setColor(VGA_YELLOW);
									myGLCD.print("    ",146, 193);
									myGLCD.printNumI(count_page,153, 193);
								}
							if (count_page >= 100 ) myGLCD.printNumI(count_page,148 , 193);

							max_count_files = count_page * 12;
							min_count_files = max_count_files - 12;
							if (min_count_files <0 ) min_count_files = 0;
							if (max_count_files > count_files ) max_count_files = count_files-1;
							count_string = 0;
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect (3, 3, 140, 185);
							for( icount = min_count_files+1; icount < max_count_files+1; icount++)
							   {
								   myGLCD.setBackColor(0, 0, 0);
								   myGLCD.setColor(255, 255, 255);
								   myGLCD.print("   ",7, count_string+5);
								   myGLCD.printNumI(icount,7, count_string+5);
								   myGLCD.print(list_files_tab[icount],35, count_string+5);
								   count_string +=15;
							   }
						}
					if ((x>=190) && (x<=230))     // Кнопки перелистывания страниц "<<"
						{
							waitForIt(190, 189, 230, 209);
							myGLCD.setColor(VGA_YELLOW);
							myGLCD.setBackColor(0, 0, 255);
							count_page++;
							if (count_page > max_count_page) count_page = max_count_page;
							if (count_page < 10) 
								{
									myGLCD.setColor(VGA_YELLOW);
									myGLCD.print("    ",146, 193);
									myGLCD.printNumI(count_page,157, 193);
								}
							if (count_page >= 10 & count_page <100 )
								{
									myGLCD.setColor(VGA_YELLOW);
									myGLCD.print("    ",146, 193);
									myGLCD.printNumI(count_page,153, 193);
								}
							if (count_page >= 100 ) myGLCD.printNumI(count_page,148 , 193);

							max_count_files = count_page * 12;
							min_count_files = max_count_files - 12;
							if (min_count_files < 0 ) min_count_files = 0;
							if (max_count_files > count_files ) max_count_files = count_files-1;
							count_string = 0;
							myGLCD.setColor(0, 0, 0);
							myGLCD.fillRoundRect (3, 3, 140, 185);
							for( icount = min_count_files+1; icount < max_count_files+1; icount++)
							   {
								   myGLCD.setBackColor(0, 0, 0);
								   myGLCD.setColor(255, 255, 255);
								   myGLCD.print("   ",7, count_string+5);
								   myGLCD.printNumI(icount,7, count_string+5);
								   myGLCD.print(list_files_tab[icount],35, count_string+5);
								   count_string +=15;
							   }
						}
					}

					 y_fcount_start = 5;

					if ((x>= 30) && (x<=136))            //  Выбор файла из списка
						{
							if (count_page == max_count_page1)
								{
									y_fcount_stop = 11- ( (max_count_page1 * 12) - max_count_files1);
								}
							else
							{
								y_fcount_stop = 12;
							}

							for(y_fcount_step = 0; y_fcount_step < y_fcount_stop; y_fcount_step++)
								{
									if ((y>=y_fcount_start) && (y<=y_fcount_start+12))         // 
										{
											myGLCD.setColor(0, 0, 0);
											myGLCD.setBackColor(0, 0, 0);
											myGLCD.drawRoundRect (30, old_fcount_start, 136, old_fcount_start+12);
											waitForIt(30, y_fcount_start, 136, y_fcount_start+12);
											old_fcount_start = y_fcount_start;
											set_files = ((count_page-1) * 12)+y_fcount_step+1;
											myGLCD.setColor(VGA_YELLOW);
											myGLCD.print(list_files_tab[set_files],170, 30);     // номер файла в позиции "set_files"
										}
									 y_fcount_start += 15;
								 }
						}

					if ((x>= 150) && (x<=300))                                                   //  
						{
						  if ((y>= 60) && (y<=120))                                              //  Выбор
							{
								waitForIt(150, 60, 300, 120);
								myGLCD.clrScr();
								file_serial();                                                   // Вызов программы передачи файла
								myGLCD.clrScr();
								myGLCD.setFont( SmallFont);
								myGLCD.setColor(0, 0, 255);
								myGLCD.fillRoundRect (5, 214, 315, 239);                         // Кнопка "ESC -> PUSH"
								myGLCD.fillRoundRect (90, 189, 130, 209);                        // Кнопка "<<"
								myGLCD.fillRoundRect (140, 189, 180, 209);                       // Вывод номера страницы
								myGLCD.fillRoundRect (190, 189, 230, 209);                       // Кнопка ">>"
								myGLCD.fillRoundRect (150, 60, 300, 120);                        // Кнопка "Отправить файл"
								myGLCD.setColor(255, 255, 255);
								myGLCD.drawRoundRect (5, 214, 315, 239);
								myGLCD.drawRoundRect (90, 189, 130, 209);
								myGLCD.drawRoundRect (140, 189, 180, 209);
								myGLCD.drawRoundRect (190, 189, 230, 209);
								myGLCD.drawRoundRect (2, 2, 318, 186);
								myGLCD.drawRoundRect (150, 60, 300, 120);                        // Кнопка "Отправить файл"
								myGLCD.setBackColor(0, 0, 0);
								myGLCD.print("Page N ",30, 193);
								myGLCD.setBackColor(0, 0, 255);
								myGLCD.print(txt_info11,CENTER, 221);                            // Кнопка "ESC -> PUSH"
								myGLCD.setColor(VGA_YELLOW);
								myGLCD.print("<<",101, 193);
								myGLCD.print(">>",203, 193);
								myGLCD.setFont(BigFont);
								myGLCD.print("O\xA4\xA3pa\x97\x9D\xA4\xAC",153, 70);            // Кнопка "Отправить файл"
								myGLCD.print("\xA5""a\x9E\xA0",195, 90);                        // Кнопка "Отправить файл"
								myGLCD.setFont( SmallFont);			
								myGLCD.setColor(VGA_YELLOW);
								myGLCD.setBackColor(0, 0, 255);
								if (count_page > max_count_page) count_page = max_count_page;
								if (count_page < 10) 
									{
										myGLCD.setColor(VGA_YELLOW);
										myGLCD.print("    ",146, 193);
										myGLCD.printNumI(count_page,157, 193);
									}
								if (count_page >= 10 & count_page <100 )
									{
										myGLCD.setColor(VGA_YELLOW);
										myGLCD.print("    ",146, 193);
										myGLCD.printNumI(count_page,153, 193);
									}
								if (count_page >= 100 ) myGLCD.printNumI(count_page,148 , 193);

								max_count_files = count_page * 12;
								min_count_files = max_count_files - 12;
								if (min_count_files < 0 ) min_count_files = 0;
								if (max_count_files > count_files ) max_count_files = count_files-1;
								count_string = 0;
								myGLCD.setColor(0, 0, 0);
								myGLCD.fillRoundRect (3, 3, 140, 185);
								for( icount = min_count_files+1; icount < max_count_files+1; icount++)
								   {
									   myGLCD.setBackColor(0, 0, 0);
									   myGLCD.setColor(255, 255, 255);
									   myGLCD.print("   ",7, count_string+5);
									   myGLCD.printNumI(icount,7, count_string+5);
									   myGLCD.print(list_files_tab[icount],35, count_string+5);
									   count_string +=15;
								   }
							}
						}
				}
	   }
	Draw_menu_ADC1();
}

void file_serial()
{
		// Программа чтения данных из файла и отображения на дисплее
	int data;                    // Дата для сиволов
	uint32_t step_file = 0;      // Не применяется
	uint32_t  File_size;         // переменная хранения размера файла 

	myGLCD.clrScr();
	myGLCD.setBackColor(0, 0, 0);

	root = sd.open(list_files_tab[set_files]);                                                     // Открыть выбранный файл
	File_size = root.fileSize();                                                                   // Получить размер файла 

	myGLCD.setFont( SmallFont);
	myGLCD.print("Pa\x9C\xA1""ep \xA5""a\x9E\xA0""a", 5, 168);                                     // "Размер файла"
	myGLCD.print("\x89o\x9C. \x97 \xA5""a\x9E\xA0""e             ", 5, 183);                       // "Поз. в файле"
	myGLCD.setFont(BigFont);
	myGLCD.print("\x89""epe\x99""a\xA7""a \xA5""a\x9E\xA0""a", CENTER, 60);                        // "Размер файла"
	myGLCD.print("\x97 COM \xA3op\xA4", CENTER, 80);                                               // "Размер файла"
	myGLCD.printNumI(File_size, 105, 165);                                                         //  Отобразить размер файла
	myGLCD.setColor(VGA_LIME);
	myGLCD.print(txt_info11,CENTER, 200);
	myGLCD.setColor(255,255,255);
	if (!root.isOpen())                                                                            // Прверка на ошибку открытия файла
		{
			Serial.println(F("No current root file"));
			myGLCD.print("No current file",CENTER, 100);
			return;
		}

	root.rewind();       
	
	while ((data = root.read()) >= 0 && !myTouch.dataAvailable())      
	{
		Serial.write(data);
		step_file = root.position();  
		myGLCD.printNumI(step_file, 105, 180);  
	}
   if ((data = root.read()) >= 0)	while (myTouch.dataAvailable()) {};     
  // delay(500);
	
}
//------------------------------------------------------------------------------
void setup(void) 
{
	if (ERROR_LED_PIN >= 0)
		{
			pinMode(ERROR_LED_PIN, OUTPUT);
		}
	Serial.begin(115200);
	Serial.print(F("FreeRam: "));
	Serial.println(FreeRam());
	myGLCD.InitLCD();
	myGLCD.clrScr();
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);

	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);
	//myTouch.setPrecision(PREC_HI);
	myButtons.setTextFont(BigFont);
	myButtons.setSymbolFont(Dingbats1_XL);
	// initialize file system.
	if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED)) 
	  {
		sd.initErrorPrint();
		myGLCD.setBackColor(0, 0, 0);
		myGLCD.setColor(255, 100, 0);
		myGLCD.print("Can't access SD card",CENTER, 40);
		myGLCD.print("Do not reformat",CENTER, 70);
		myGLCD.print("SD card problem?",CENTER, 100);
		myGLCD.setColor(VGA_LIME);
		myGLCD.print(txt_info11,CENTER, 200);
		myGLCD.setColor(255, 255, 255);
		while (myTouch.dataAvailable()){}
		delay(50);
		while (!myTouch.dataAvailable()){}
		delay(50);
		myGLCD.clrScr();
		myGLCD.print("Run Setup", CENTER,120);
	  }

	ADC_MR |= 0x00000100 ; // ADC full speed

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// Настройка звукового генератора  
	AD9850.reset();                    //reset module
	delay(1000);
	AD9850.powerDown();                //set signal output to LOW
	AD9850.set_frequency(0,0,500);    //set power=UP, phase=0, 1kHz frequency 

	chench_Channel();

	//adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
	Timer3.attachInterrupt(firstHandler); // Every 50us
	Timer4.attachInterrupt(secondHandler).setFrequency(1);
	rtc_clock.init();
	rtc_clock.set_time(__TIME__);
	rtc_clock.set_date(__DATE__);
	SdFile::dateTimeCallback(dateTime); 
	//++++++++++++++++ SD info ++++++++++++++++++++++++++++++
	
	// use uppercase in hex and use 0X base prefix
	cout << uppercase << showbase << endl;
	// pstr stores strings in flash to save RAM

//	cout << pstr("SdFat version: ") << SD_FAT_VERSION << endl;
	myGLCD.setBackColor(0, 0, 255);
	preob_num_str();
	pinMode(strob_pin, INPUT);
	digitalWrite(strob_pin, HIGH);
	Serial.println(F("Setup Ok!"));
}
//------------------------------------------------------------------------------
void loop(void) 
{
	draw_Glav_Menu();
	swichMenu();
}
