/*
  скетч для Nano 3.0 Atmega328  или ATmega32U4
  c использованием millis, ButtonWC,SW3
  с программной защитой от дребезга контакта .
  время мин и сек условны

  // все пункты должны быть в одном скетче

  А)  управление двумя реле ,одной кнопкой с led индикацией .    // переключение источников водоснабжения. задержка в 2сек используется для сброса давления в водопроводе при переключении ел.кранов
    LedЕСО - (аналог) LED на кнопке ЕСО по умолчанию не горит.
    ButtonECO - если кнопка ECO  нажата то включает на 5мин + реле R1 +R2 (с задержкой в 2 сек) и Led на кнопке ЕСО // кнопка ButtonECO ,без фиксации NO
    на последней минуте Led на кнопке ЕСО   начинает плавно мигать показывая что время 5 мин заканчивается.
    досрочное принудительное  выключение  R2 +R1 происходит удержанием   кнопки ECO   2 сек. это выключает  реле R2 +R1(с задержкой в 2 сек)   и Led на кнопке ЕСО

  Б) переключение источников водоснабжения реле R1 +R2 (с задержкой в 2 сек)  такое же как в А)
     управление сервомотором на 60* одной кнопкой с led индикацией // слив воды в бочке унитаза
    LedWC  - это (аналог) LED на кнопке WC по умолчанию  горит.
    ButtonWC - это кнопка WC   нажата серва поворачивается  на 50* ,задержка 2сек и возврат на 10*
     если кнопка нажата ButtonWC то включает на 3 мин + реле R1 +R2 (с задержкой в 2 сек)
    // переключение источников водоснабжения. задержка в 2сек используется для сброса давления в водопроводе при переключении ел.кранов
    Led на кнопке WC плавно мигает 3 мин .

  С)  pin SW1 HIGH вкл R3 на 30сек                                                                         // сигнал от датчика влажности вкл вентиляцию
  Д)  pin SW2 HIGH вкл R3 и R4 на 10 сек                                                                   // сигнал от датчика движения вкл вентиляцию и освещение
  Е)  pin SW3 HIGH вкл R4 на 90сек + вкл плавно(1сек) Led на 60сек если SW3 LOW выкл плавно(1сек) Led      // сигнал от датчика движения вкл   освещение  и подсветку (аналог) LED

*/

#include <Servo.h>
#include <SerialInput.h>
#include <EEPROM.h>
#include <EEPROM2.h>

#include "DHT.h"

#define DHTPIN    11                               // what digital pin we're connected to
#define Rele_R1   15                               // Реле R1  
#define Rele_R2   16                               // Реле R2
#define Rele_R3   17                               // Реле R3
#define Rele_R4   18                               // Реле R4
#define led_ECO    8                               // Светодиод на кнопке ECO
#define led_WC     5                               // Светодиод на кнопке WC
#define ButtonECO  7                               // Кнопка ECO
#define ButtonWC  10                               // Кнопка WC
#define SW1        2                               // SW1 HIGH вкл R3 на 30сек. Сигнал от датчика влажности вкл вентиляцию
#define SW2        3                               // pin SW2 HIGH вкл R3 и R4 на 10 сек. Сигнал от датчика движения вкл освещение и вентиляцию
#define SW3        4                               // pin SW3 HIGH вкл R4 на 90сек + вкл плавно(1сек) Led на 60сек если SW3 LOW выкл плавно(1сек) Led. Сигнал от датчика движения вкл освещение и подсветку (аналог) LED 
#define Led_light  6                               // Светодиод подсветки 
#define servo_tank 9                               // Сервопривод.   ШИМ: 3, 5, 6, 9, 10, и 11. Любой из выводов обеспечивает ШИМ с разрешением 8 бит при помощи функции analogWrite()

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

Servo myservo;                                     // create servo object to control a servo
// twelve servo objects can be created on most boards
int num_button                           = 0;              //
int num_button34                         = 0;              //
int Rele34_time                          = 0;              //
bool ButECO                              = false;          //
bool ButECO_Start                        = false;          // Флаг запуска программы по команде кнопки  ButtonECO
bool ButWC                               = false;          //
bool ButWC_Start                         = false;          // Флаг запуска программы по команде кнопки  ButtonWC
bool ButSW1                              = false;          // Флаг запуска программы по команде SW1
bool ButSW2                              = false;          // Флаг запуска программы по команде SW2
bool ButSW3                              = false;          // Флаг запуска программы по команде SW3
bool ButHum                              = false;          // Флаг запуска программы по команде датчика влажности
bool Rele2_Start                         = false;          // Флаг включения реле №2
bool Rele1_Stop                          = false;          // Флаг выключения реле №1
bool Rele2_Stop                          = false;          // Флаг выключения реле №2
bool Rele34_Start                        = false;          // Флаг включения реле №3,4
bool lightOnOff                          = false;          // Флаг управления плавным включением/отключением света
bool lightmin                            = false;          // Флаг управления плавным отключением света
bool lightmax                            = false;          // Флаг управления плавным включением света
unsigned long timeECO                    = 10000;          // A 300000 Время включения реле №1 ( 5 минут) от кнопки ECO
unsigned long timeWC                     = 10000;          // B 180000 Время включения реле №1 ( 3 минуты) от кнопки WC
unsigned long Rele2_time                 = 500;            // C 2000 Время задержки включения реле№2 (2 секунды)
unsigned long time_flash_led_ECO         = 2000;           // P 60000 Время до окончания периода, включить мигание светодиода (60 секунд)
unsigned long time_push_ButECO           = 2000;           // E 2000 Время удержания кнопки ButtonECO  (2 секунды)
unsigned long SW1_time                   = 3000;           // F Время задержки по по комманде SW1
unsigned long SW2_time                   = 1000;           // G Время задержки по по комманде SW2
unsigned long SW3_time                   = 10000;          // H Время задержки по по комманде SW3
int pos0                                 = 0;              // J Параметры позиции сервопривода 0 градусов
int pos50                                = 30;             // K Параметры позиции сервопривода 50 градусов
unsigned int pos_time                    = 2000;           // L Время до возврата сервопривода в исходное положение
int ligh_speedECO                        = 200;            // M время скорости перестройки плавностью включения/отключения светодиода ECO
int ligh_speedWC                         = 200;            // N время скорости перестройки плавностью включения/отключения светодиода WC
int ligh_speed                           = 20;             // O время скорости перестройки плавностью включения/отключения света
int humidity_threshold                   = 30;             // T Порог влажности
int humidity_serial                      = 0;              // R Разрешение передачи информации о влажности в СОМ порт "0" - запретить, "1" - разрешить.
int humidity_On                          = 0;              // V Разрешение использования информации о влажности  "0" - запретить, "1" - разрешить.
unsigned long humidity_time              = 60000;          // W Интервал измерения влажности
//----------------- Параметры по умолчанию -------------------
// Адрес в EEPROM
const unsigned long c_timeECO            = 300000;         // 10  A 300000 Время включения реле №1 ( 5 минут) от кнопки ECO
const unsigned long c_timeWC             = 180000;         // 14  B 180000 Время включения реле №1 ( 3 минуты) от кнопки WC
const unsigned long c_Rele2_time         = 2000;           // 18  C 2000 Время задержки включения реле№2 (2 секунды)
const unsigned long c_time_flash_led_ECO = 60000;          // 22  P 60000 Время до окончания периода, включить мигание светодиода (60 секунд)
const unsigned long c_time_push_ButECO   = 2000;           // 26  E 2000 Время удержания кнопки ButtonECO  (2 секунды)
const unsigned long c_SW1_time           = 30000;          // 30  F Время задержки по комманде SW1
const unsigned long c_SW2_time           = 10000;          // 34  G Время задержки по комманде SW2
const unsigned long c_SW3_time           = 90000;          // 38  H Время задержки по комманде SW3
const int c_pos0                         = 0;              // 42  J Параметры позиции сервопривода 0 градусов
const int c_pos50                        = 30;             // 46  K Параметры позиции сервопривода 50 градусов
const unsigned int c_pos_time            = 1000;           // 50  L Время до возврата сервопривода в исходное положение
const int c_ligh_speedECO                = 200;            // 54  M время скорости перестройки плавностью включения/отключения светодиода ECO
const int c_ligh_speedWC                 = 200;            // 58  N время скорости перестройки плавностью включения/отключения светодиода WC
const int c_ligh_speed                   = 20;             // 62  O время скорости перестройки плавностью включения/отключения света
const int c_humidity_threshold           = 30;             // 66  T Порог влажности
const int c_humidity_serial              = 0;              // 70  R Разрешение передачи информации о влажности в СОМ порт "0" - запретить, "1" - разрешить.
const int c_humidity_On                  = 0;              // 74  V Разрешение использования информации о влажности  "0" - запретить, "1" - разрешить.
const unsigned long c_humidity_time      = 60000;          // 78  W Интервал измерения влажности

//------------------------------------------------------------
int lighN                                = 0;              // Переменная для хранения количества ступеней плавной перестройки
unsigned long currentMillisECO           = 0;              // Переменная для временного хранения текущего времени
unsigned long currentMillisWC            = 0;              // Переменная для временного хранения текущего времени
unsigned long currentMillis              = 0;              // Переменная для временного хранения текущего времени
unsigned long currentMillis34            = 0;              // Переменная для временного хранения текущего времени
unsigned long currentMillisRele1         = 0;              // Переменная для временного хранения текущего времени
unsigned long currentMillisDHT           = 0;              // Переменная для временного хранения текущего времени
int incomingByte = 0;                                      // переменная для хранения полученного байта
long int Number;
char c;

class Flasher                                      // Управление светодиодами в многозадачном режиме
{
    int ledPin;
    long OnTime;
    long OffTime;

    int ledState;
    unsigned long previousMillis;
  public:
    Flasher(int pin,  long on, long off)
    {
      ledPin = pin;
      pinMode(ledPin, OUTPUT);

      OnTime = on;
      OffTime = off;

      ledState = LOW;
      previousMillis = 0;
    }

    void Update()
    {
      unsigned long currentMillis = millis();

      if ((ledState == HIGH) && (currentMillis - previousMillis >= OnTime))
      {
        ledState = LOW;
        previousMillis = currentMillis;
        digitalWrite(ledPin, ledState);
      }
      else if ((ledState == LOW) && (currentMillis - previousMillis >= OffTime))
      {
        ledState = HIGH;
        previousMillis = currentMillis;
        digitalWrite(ledPin, ledState);
      }
    }
};
Flasher led1(led_ECO, ligh_speedECO, ligh_speedECO);
Flasher led2(led_WC, ligh_speedECO, ligh_speedECO);
void UpdateECO()                                   // Проверка окончания выполнения программы по нажатию кнопки ButtonECO
{
  if ((ButECO_Start == true) && (currentMillis - currentMillisECO >= timeECO))
  {
    //digitalWrite(Rele_R1,LOW);
    Rele1_Stop = true;
    currentMillisRele1 = millis();
    digitalWrite(Rele_R2, LOW);
    ButECO_Start = false;
    digitalWrite(led_ECO, LOW);
    Serial.println("ButtonECO Off");
  }
}
void UpdateWC()                                  // Проверка окончания выполнения программы по нажатию кнопки ButtonWC
{
  if ((ButWC_Start == true) && (currentMillis - currentMillisWC >= timeWC))
  {
    digitalWrite(Rele_R1, LOW);
    digitalWrite(Rele_R2, LOW);
    ButWC_Start = false;
    digitalWrite(led_WC, HIGH);
    Serial.println("ButtonWC Off");
  }
}
void UpdateReleECO()                              // Программа выполнения программы по включению реле №2
{
  if (num_button == 1)
  {
    if ((Rele2_Start == true) && (currentMillis - currentMillisECO >= Rele2_time))
    {
      digitalWrite(Rele_R2, HIGH);
      Rele2_Start = false;
      Serial.println("Rele_R2 On");
    }
  }
  else if (num_button == 2)
  {
    if ((Rele2_Start == true) && (currentMillis - currentMillisWC >= Rele2_time))
    {
      digitalWrite(Rele_R2, HIGH);
      Rele2_Start = false;
      Serial.println("Rele_R2 On");
    }
  }
}
void UpdateRele34()                              // Программа выполнения программы по включению реле №3,4
{
  if (num_button34 == 3)                       // Проверить подачу команды от SW1
  {
    if ((Rele34_Start == true) && (currentMillis - currentMillis34 >= Rele34_time) && SW1_time != 0)
    {
      digitalWrite(Rele_R3, LOW);
      Rele34_Start = false;
      Serial.println("Rele_R3 Off");
    }
  }
  else if (num_button34 == 4)                  // Проверить подачу команды от SW2
  {
    if ((Rele34_Start == true) && (currentMillis - currentMillis34 >= Rele34_time) && SW2_time != 0)
    {
      digitalWrite(Rele_R3, LOW);
      digitalWrite(Rele_R4, LOW);
      Rele34_Start = false;
      Serial.println("Rele_R3,4 Off");
    }
  }
  else if (num_button34 == 5)                  // Проверить подачу команды от SW3
  {
    if ((Rele34_Start == true) && (currentMillis - currentMillis34 >= Rele34_time) && SW3_time != 0)
    {
      digitalWrite(Rele_R4, LOW);
      lighN = 255;
      lightmin = true;
      lightOnOff = true;
      Rele34_Start = false;
      Serial.println("Rele_R3,4 Off");
    }
  }
}

void led_lightOnOff()                         // Программа плавного включения/выключения светодиода
{
  if (lightOnOff == true)                     // Проверить была ли команда на включение или отключение светодиодов
  {
    if (lightmax == true)                     // Да, была команда на включение. Программа плавного включения светодиода
    {
      analogWrite(Led_light, lighN);          // Записать длительность ШИМ в порт управления свечением светодиода 
      delay(ligh_speed);                      // Выдержать интервал между изменениями сигнала ШИМ
      lighN++;                                // Увеличить длительность ШИМ на 1 
      if (lighN >= 255)                       // Сигнал ШИМ достиг максимального значения.
      {
        analogWrite(Led_light, 255);          // Прекратить процедуру управления яркостью светодиода 
        lightOnOff = false;
        lightmax = false;
        lightmin = false;
      }
    }

    if (lightmin == true)                    // Да, была команда на выключение.Программа плавного выключения светодиода
    {
      analogWrite(Led_light, lighN);         // Записать длительность ШИМ в порт управления свечением светодиода 
      delay(ligh_speed);                     // Выдержать интервал между изменениями сигнала ШИМ
      lighN--;                               // Уменьшить длительность ШИМ на 1 
      if (lighN <= 0)                        // Сигнал ШИМ достиг минимального значения.
      {
        analogWrite(Led_light, 0);           // Прекратить процедуру управления яркостью светодиода  
        lightOnOff = false;
        lightmin = false;
        lightmax = false;
      }
    }
  }
}

void rele1_Off()                      // Программа задержки отключения реле №1
{
  if (Rele1_Stop == true && (currentMillis - currentMillisRele1 >= Rele2_time))
  {
    digitalWrite(Rele_R1, LOW);
    Rele1_Stop = false;
    Serial.println("ReleN1 Off");
  }
}

void rele2_Off()                      // Программа задержки отключения реле №2
{
  if (Rele2_Stop == true && (currentMillis - currentMillisRele1 >= Rele2_time))
  {
    digitalWrite(Rele_R2, LOW);
    Rele2_Stop = false;
    Serial.println("ReleN2 Off");
  }
}


void test_sensor()
{
  // ------------------------  Проверка нажатия кнопки ButtonECO -----------------------
  if (digitalRead(ButtonECO) == LOW && ButECO_Start != true )
  {
    if (ButECO == false)
    {
      ButECO       = true;
      ButECO_Start = true;
      num_button   = 1;
      Rele2_Start  = true;
      Serial.println("ButtonECO On");
      currentMillisECO = millis();
      digitalWrite(Rele_R1, HIGH);
      digitalWrite(led_ECO, HIGH);
    }
  }
  else
  {
    ButECO = false;
  }
  // ---------------- Отключение по удержанию кнопки ButtonECO в течении  2 секунд ---------------------------
  if (digitalRead(ButtonECO) == LOW && ButECO_Start == true)
  {
    currentMillis = millis();
    if ((ButECO_Start == true) && (currentMillis - currentMillisECO >= time_push_ButECO))
    {
      digitalWrite(Rele_R1, LOW);
	   Rele1_Stop = true;
     // digitalWrite(Rele_R2, LOW);      // Переделать. Вставить задержку 2 сек.
	  Rele2_Stop = true;                 // Вставить задержку 2 сек. отключения реле №2
      ButECO_Start = false;
      digitalWrite(led_ECO, LOW);
      Serial.println("ButtonECO Off");
      while (digitalRead(ButtonECO) == LOW) {}            // Ожидание отпускания кнопки ButtonECO для предотвращения запуска нового нажатия.
    }
  }
  //--------------------------  Проверка нажатия кнопки ButtonWC -----------------------
  if (digitalRead(ButtonWC) == LOW)
  {
    if (ButWC == false)
    {
      myservo.write(pos50);              // tell servo to go to position in variable 'pos'
      delay(pos_time);                   // waits 15ms for the servo to reach the position
      myservo.write(pos0);               // tell servo to go to position in variable 'pos'
      ButWC       = true;
      ButWC_Start = true;
      num_button  = 2;
      Rele2_Start = true;
      Serial.println("ButtonWC");
      digitalWrite(Rele_R1, HIGH);
      currentMillisWC = millis();
    }
  }
  else
  {
    ButWC = false;
  }
  //------------- проверка контактов датчиков ------------------------
  if (digitalRead(SW1) == LOW)                        // проверка контактов датчика SW1
  {
    if (ButSW1 == false)
    {
      ButSW1 = true;
      num_button34                 = 3;
      Rele34_time                  = SW1_time;
      Rele34_Start = true;
      digitalWrite(Rele_R3, HIGH);
      Serial.println("SW1 On");
      currentMillis34 = millis();
    }
  }
  else
  {
    ButSW1 = false;
  }

  if (digitalRead(SW1) == HIGH && SW1_time == 0 && Rele34_Start == true && num_button34 == 3)   // Отключение задержки времени на отключение релеТ3 от контактов датчика SW1
  {
    digitalWrite(Rele_R3, LOW);
    Rele34_Start = false;
    Serial.println("Rele_R3 Off");
  }


  if (digitalRead(SW2) == LOW)                        // проверка контактов датчика SW2
  {
    if (ButSW2 == false)
    {
      ButSW2                       = true;
      num_button34                 = 4;
      Rele34_time                  = SW2_time;
      Rele34_Start                 = true;
      digitalWrite(Rele_R3, HIGH);
      digitalWrite(Rele_R4, HIGH);
      Serial.println("SW2 On");
      currentMillis34 = millis();
    }
  }
  else
  {
    ButSW2 = false;
  }

  if (digitalRead(SW2) == HIGH && SW2_time == 0 && Rele34_Start == true && num_button34 == 4)                        // проверка контактов датчика SW1
  {
    digitalWrite(Rele_R3, LOW);
    digitalWrite(Rele_R4, LOW);
    Rele34_Start = false;
    Serial.println("Rele_R3,4 Off");
  }

  if (digitalRead(SW3) == LOW)                        // проверка контактов датчика SW3
  {
    if (ButSW3 == false)
    {
      ButSW3                       = true;
      num_button34                 = 5;
      Rele34_time                  = SW3_time;
      Rele34_Start                 = true;
      digitalWrite(Rele_R4, HIGH);
      lighN = 0;
      lightmax = true;
      lightOnOff = true;
      currentMillis34 = millis();
      Serial.println("SW3");
    }
  }
  else
  {
    ButSW3 = false;
  }

  if (digitalRead(SW3) == HIGH && SW3_time == 0 && Rele34_Start == true && num_button34 == 5)                        // проверка контактов датчика SW1
  {
    digitalWrite(Rele_R4, LOW);
    lighN = 255;
    lightmin = true;
    lightOnOff = true;
    Rele34_Start = false;
    Serial.println("Rele_R3,4 Off");
  }


}

void serialEvent()
{
  c = tolower(Serial.read());
  do {
    delay(10);
  } while (Serial.read() >= 0);


  if (c == 'i' && 'I')
  {
    print_info();                                // Вывод параметров в СОМ порт
  }
  else if (c == 'u' && 'U')
  {
    print_infoU();                                // Вывод параметров в СОМ порт
  }
  else if (c == 'd' && 'D')
  {
    Serial.print("Save default ... ");
    save_Default();                              // Запись в EEPROM параметров по умолчанию
    read_param_EEPROM();
    Serial.println(" Ok!");
    Serial.println();
  }
  else if (c == 'a' && 'A')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(10, numberIn * 1000);
    EEPROM_read(10, timeECO);
  }
  else if (c == 'b' && 'B')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(14, numberIn * 1000);
    EEPROM_read(14, timeWC);
  }
  else if (c == 'c' && 'C')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(18, numberIn * 1000);
    EEPROM_read(18, Rele2_time);
  }
  else if (c == 'p' && 'P')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(22, numberIn * 1000);
    EEPROM_read(22, time_flash_led_ECO);
  }
  else if (c == 'e' && 'E')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(26, numberIn * 1000);
    EEPROM_read(26, time_push_ButECO);
  }
  else if (c == 'f' && 'F')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(30, numberIn * 1000);
    EEPROM_read(30, SW1_time);
  }
  else if (c == 'g' && 'G')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(34, numberIn * 1000);
    EEPROM_read(34, SW2_time);
  }
  else if (c == 'h' && 'H')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(38, numberIn * 1000);
    EEPROM_read(38, SW3_time);
  }
  else if (c == 'j' && 'J')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(42, numberIn);
    EEPROM_read(42, pos0);
  }
  else if (c == 'k' && 'K')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(46, numberIn);
    EEPROM_read(46, pos50);
  }
  else if (c == 'l' && 'L')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(50, numberIn * 1000);
    EEPROM_read(50, pos_time);
  }
  else if (c == 'm' && 'M')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(54, numberIn);
    EEPROM_read(54, ligh_speedECO);
  }
  else if (c == 'n' && 'N')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(58, numberIn);
    EEPROM_read(58, ligh_speedWC);
  }
  else if (c == 'o' && 'O')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(62, numberIn);
    EEPROM_read(62, ligh_speed);
  }
  else if (c == 't' && 'T')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(66, numberIn);
    EEPROM_read(66, humidity_threshold);
  }
  else if (c == 'r' && 'R')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(70, numberIn);
    EEPROM_read(70, humidity_serial);
  }
  else if (c == 'v' && 'V')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(74, numberIn);
    EEPROM_read(74, humidity_On);
  }
  else if (c == 'w' && 'W')
  {
    unsigned long numberIn =  input_serial();
    EEPROM_write(78, numberIn * 1000);
    EEPROM_read(78, humidity_time);
  }
  else
  {
    Serial.println(F("Invalid entry"));
    Serial.println();
    Serial.println("->");
  }
}

int input_serial()              // Программа ввода параметров с СОМ порта
{
  Serial.print("Enter number: ");
  Serial.print(c);
  Serial.print(" => ");
  Number = SerialInput.InputNumber();

  if (SerialInput.NumberEntered)
  {
    Serial.print("You entered: ");
    Serial.println(Number, DEC);
  }
  else
  {
    Serial.println("You didn't entered anything");
  }
  Serial.println();
  Serial.println("->");
  return Number;
}

void print_info()
{
  Serial.print("A  timeECO   sec.  - ");
  Serial.println(timeECO / 1000);
  Serial.print("B  timeWC    sec.  - ");
  Serial.println(timeWC / 1000);
  Serial.print("C  Rele2_time sec. - ");
  Serial.println(Rele2_time / 1000);
  Serial.print("P  time_flash_led_ECO sec. - ");
  Serial.println(time_flash_led_ECO / 1000);
  Serial.print("E  time_push_ButECO   sec. - ");
  Serial.println(time_push_ButECO / 1000);
  Serial.print("F  SW1_time sec. - ");
  Serial.println(SW1_time / 1000);
  Serial.print("G  SW2_time sec. - ");
  Serial.println(SW2_time / 1000);
  Serial.print("H  SW3_time sec. - ");
  Serial.println(SW3_time / 1000);
  Serial.print("J  pos0  grad.   - ");
  Serial.println(pos0);
  Serial.print("K  pos50  grad.  - ");
  Serial.println(pos50);
  Serial.print("L  pos_time sec. - ");
  Serial.println(pos_time / 1000);
  Serial.print("M  ligh_speedECO - ");
  Serial.println(ligh_speedECO);
  Serial.print("N  ligh_speedWC  - ");
  Serial.println(ligh_speedWC);
  Serial.print("O  ligh_speed    - ");
  Serial.println(ligh_speed );
  Serial.print("T  humidity_threshold - ");
  Serial.println(humidity_threshold);
  Serial.print("R  humidity_serial    - ");
  Serial.println(humidity_serial);
  Serial.print("V  humidity_On    - ");
  Serial.println(humidity_On );
  Serial.print("W  humidity_time  - ");
  Serial.println(humidity_time / 1000 );
  Serial.println();
  Serial.println("->");
}

void print_infoU()
{
  Serial.println("*** Default settings ***");
  Serial.print("A  timeECO   sec.  - ");
  Serial.println(c_timeECO / 1000);
  Serial.print("B  timeWC    sec.  - ");
  Serial.println(c_timeWC / 1000);
  Serial.print("C  Rele2_time sec. - ");
  Serial.println(c_Rele2_time / 1000);
  Serial.print("P  time_flash_led_ECO sec. - ");
  Serial.println(c_time_flash_led_ECO / 1000);
  Serial.print("E  time_push_ButECO   sec. - ");
  Serial.println(c_time_push_ButECO / 1000);
  Serial.print("F  SW1_time sec. - ");
  Serial.println(c_SW1_time / 1000);
  Serial.print("G  SW2_time sec. - ");
  Serial.println(c_SW2_time / 1000);
  Serial.print("H  SW3_time sec. - ");
  Serial.println(c_SW3_time / 1000);
  Serial.print("J  pos0  grad.   - ");
  Serial.println(c_pos0);
  Serial.print("K  pos50  grad.  - ");
  Serial.println(c_pos50);
  Serial.print("L  pos_time sec. - ");
  Serial.println(c_pos_time / 1000);
  Serial.print("M  ligh_speedECO - ");
  Serial.println(c_ligh_speedECO);
  Serial.print("N  ligh_speedWC  - ");
  Serial.println(c_ligh_speedWC);
  Serial.print("O  ligh_speed    - ");
  Serial.println(c_ligh_speed );
  Serial.print("T  humidity_threshold - ");
  Serial.println(c_humidity_threshold);
  Serial.print("R  humidity_serial    - ");
  Serial.println(c_humidity_serial);
  Serial.print("V  humidity_On    - ");
  Serial.println(c_humidity_On );
  Serial.print("W  humidity_time  - ");
  Serial.println(c_humidity_time / 1000 );
  Serial.println();
  Serial.println("->");
}

void save_Default()
{
  EEPROM_write(10, c_timeECO);
  EEPROM_write(14, c_timeWC);
  EEPROM_write(18, c_Rele2_time);
  EEPROM_write(22, c_time_flash_led_ECO);
  EEPROM_write(26, c_time_push_ButECO);
  EEPROM_write(30, c_SW1_time);
  EEPROM_write(34, c_SW2_time);
  EEPROM_write(38, c_SW3_time);
  EEPROM_write(42, c_pos0);
  EEPROM_write(46, c_pos50);
  EEPROM_write(50, c_pos_time);
  EEPROM_write(54, c_ligh_speedECO);
  EEPROM_write(58, c_ligh_speedWC);
  EEPROM_write(62, c_ligh_speed);
  EEPROM_write(66, c_humidity_threshold);
  EEPROM_write(70, c_humidity_serial);
  EEPROM_write(74, c_humidity_On );
  EEPROM_write(78, c_humidity_time );
}

void read_param_EEPROM()
{
  EEPROM_read(10, timeECO);
  EEPROM_read(14, timeWC);
  EEPROM_read(18, Rele2_time);
  EEPROM_read(22, time_flash_led_ECO);
  EEPROM_read(26, time_push_ButECO);
  EEPROM_read(30, SW1_time);
  EEPROM_read(34, SW2_time);
  EEPROM_read(38, SW3_time);
  EEPROM_read(42, pos0);
  EEPROM_read(46, pos50);
  EEPROM_read(50, pos_time);
  EEPROM_read(54, ligh_speedECO);
  EEPROM_read(58, ligh_speedWC);
  EEPROM_read(62, ligh_speed);
  EEPROM_read(66, humidity_threshold);
  EEPROM_read(70, humidity_serial);
  EEPROM_read(74, humidity_On );
  EEPROM_read(78, humidity_time );
}
void clear_eeprom()
{
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
}
void ini_eeprom()
{
  EEPROM.write(0, 3);
  save_Default();
}

void meassure_dht()
{
  if (currentMillis - currentMillisDHT >= humidity_time)
  {
    currentMillisDHT = millis();                    // Записать текущее время
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    if (h > humidity_threshold)            // При превышении влажности включить реле№3
    {
      if (ButHum  == false)
      {
        ButHum  = true;
        digitalWrite(Rele_R3, HIGH);
        Serial.println("ReleN3 On");
        currentMillis34 = millis();
      }

    }
    else
    {
      if (ButHum  == true)   // При понижении влажности отключить реле№3, если оно было включено
      {
        digitalWrite(Rele_R3, LOW);
        Serial.println("ReleN3 Off");
      }
      ButHum = false;
    }

    if (humidity_serial == 1)
    {
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println(" %\t");
      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.println(" *C ");
    }
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(Rele_R1, OUTPUT);                    // Реле R1
  pinMode(Rele_R2, OUTPUT);                    // Реле R2
  pinMode(Rele_R3, OUTPUT);                    // Реле R3
  pinMode(Rele_R4, OUTPUT);                    // Реле R4

  pinMode(led_ECO, OUTPUT);                    // Светодиод на кнопке ECO
  pinMode(led_WC,  OUTPUT);                    // Светодиод на кнопке WC
  digitalWrite(led_WC, HIGH);                  // Включить светодиод на кнопке WC
  pinMode(ButtonECO, INPUT);                   // Кнопка ECO
  pinMode(ButtonWC, INPUT);                    // Кнопка WC
  digitalWrite(ButtonECO, HIGH);               // Установить высокий уровень на кнопке
  digitalWrite(ButtonWC, HIGH);                // Установить высокий уровень на кнопке

  pinMode(SW1, INPUT);                         // SW1 HIGH вкл R3 на 30сек. Сигнал от датчика влажности вкл вентиляцию
  pinMode(SW2, INPUT);                         // pin SW2 HIGH вкл R3 и R4 на 10 сек. Сигнал от датчика движения вкл освещение и вентиляцию
  pinMode(SW3, INPUT);                         // pin SW3 HIGH вкл R4 на 90сек + вкл плавно(1сек) Led на 60сек если SW3 LOW выкл плавно(1сек) Led. Сигнал от датчика движения вкл освещение и подсветку (аналог) LED
  digitalWrite(SW1, HIGH);                     // Установить высокий уровень на контакте
  digitalWrite(SW2, HIGH);                     // Установить высокий уровень на контакте
  digitalWrite(SW3, HIGH);                     // Установить высокий уровень на контакте

  pinMode(Led_light, OUTPUT);                  // Светодиод подсветки
  digitalWrite(Led_light, LOW);                // Светодиод подсветки отключить
  myservo.attach(servo_tank);                  // attaches the servo on pin 9 to the servo object
  myservo.write(pos0);                         // tell servo to go to position in variable 'pos'
  // инициализация настроек из EEPROM
  dht.begin();

  if (EEPROM.read(0) == 255)
  {
    clear_eeprom();
    ini_eeprom();
  }
  read_param_EEPROM();
  Serial.println("Setup Ok!");                 // Успешное завершение начальной настройки.
  Serial.println();
  Serial.println("    Enter the character");
  Serial.println(" I     - Parameter information");
  Serial.println(" U     - Default information");
  Serial.println(" D     - Save default");
  Serial.println(" A...W - Change parameter");
  Serial.println();
  Serial.println("->");
}

void loop()
{
  test_sensor();                               // Проверить нажатие кнопок
  currentMillis = millis();                    // Записать текущее время
  UpdateECO();                                 // Проверить окончание выполнения по кнопке ECO
  UpdateWC();                                  // Проверить окончание выполнения по кнопке WC
  UpdateReleECO();                             // Проверить окончание отключения реле № 1,2
  UpdateRele34();                              // Проверить окончание отключения реле № 3,4
  led_lightOnOff();                            // Выполнить плавное включение/отключение светодиода подсветки
  rele1_Off();                                 // Отключить Реле№1 с задержкой
  rele2_Off();                                 // Отключить Реле№2 с задержкой
  if (ButECO_Start == true && (currentMillis - (currentMillisECO) >= timeECO - time_flash_led_ECO)) // Мигание светодиода ECO
  {
    led1.Update();
  }
  if (ButWC_Start == true)                     // Мигание светодиода SW
  {
    led2.Update();
  }
  if (humidity_On != 0)
  {
    meassure_dht();
  }
}
