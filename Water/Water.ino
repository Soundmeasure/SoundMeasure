

/*

скетч для Nano 3.0 Atmega328  или ATmega32U4  
c использованием millis 
c внешними прерываниями ,ButtonWC,SW3
с программной защитой от дребезга контакта .
время мин и сек условны 
вот примерный алгоритм для скетча .      

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
#include "DHT.h"


#define Rele_R1   15                             // Реле R1  
#define Rele_R2   16                             // Реле R2
#define Rele_R3   17                             // Реле R3
#define Rele_R4   18                             // Реле R4


#define led_ECO    8                             // Светодиод на кнопке ECO
#define led_WC     5                             // Светодиод на кнопке WC
#define ButtonECO  7                             // Кнопка ECO
#define ButtonWC  10                             // Кнопка WC

#define SW1        2                             // SW1 HIGH вкл R3 на 30сек. Сигнал от датчика влажности вкл вентиляцию
#define SW2        3                             // pin SW2 HIGH вкл R3 и R4 на 10 сек. Сигнал от датчика движения вкл освещение и вентиляцию
#define SW3        4                             // pin SW3 HIGH вкл R4 на 90сек + вкл плавно(1сек) Led на 60сек если SW3 LOW выкл плавно(1сек) Led. Сигнал от датчика движения вкл освещение и подсветку (аналог) LED 
#define Led_light  6                             // Светодиод подсветки 
#define servo_tank 9                             // Сервопривод.   ШИМ: 3, 5, 6, 9, 10, и 11. Любой из выводов обеспечивает ШИМ с разрешением 8 бит при помощи функции analogWrite()
#define DHTPIN     11                             // Датчик влажности

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);


Servo myservo;                                   // create servo object to control a servo 
                                                 // twelve servo objects can be created on most boards
int pos   = 0;                                   // variable to store the servo position 
int pos0  = 0;                                   // variable to store the servo position 
int pos50 = 30;                                  // variable to store the servo position 
int pos10 = 6;                                   // variable to store the servo position 

bool ButECO = false;                             //
bool ButECO_Start = false;                       // Флаг запуска программы по команде кнопки  ButtonECO
bool ButWC  = false;
bool ButWC_Start = false;                        // Флаг запуска программы по команде кнопки  ButtonWC
bool ButSW1 = false;
bool ButSW2 = false;
bool ButSW3 = false;

bool Rele2_Start = false;                          // Флаг включения реле №2


unsigned long timeECO            = 10000;          // 300000 Время включения реле №1 ( 5 минут)
unsigned long timeWC             = 10000;          // Увеличить до 3 минут
unsigned long Rele2_time         = 500;            // 2000 Время задержки включения реле№2 (2 секунды)
unsigned long time_flash_led_ECO = 2000;           // 60000 Время до окончания периода, включить мигание светодиода (60 секунд)
unsigned long time_push_ButECO   = 2000;           // 2000 Время удержания кнопки ButtonECO  (2 секунды)
unsigned long currentMillisECO   = 0;
unsigned long currentMillisWC    = 0;
unsigned long currentMillis      = 0;

class Flasher                                   // Управление светодиодами в многозадачном режиме  
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

	   if((ledState == HIGH) && (currentMillis - previousMillis >= OnTime))
	   {
		   ledState = LOW;
		   previousMillis = currentMillis;  
		   digitalWrite(ledPin,ledState);
	   }
	   else if ((ledState == LOW) && (currentMillis - previousMillis >= OffTime))
	   {
		   ledState = HIGH;
		   previousMillis = currentMillis;  
		   digitalWrite(ledPin,ledState);
	   }
	}
};


void UpdateECO()                           // Программа выполнения программы по нажатию кнопки ButtonECO
{
	currentMillis = millis();

	if((ButECO_Start == true) && (currentMillis - currentMillisECO >= timeECO))
	{
		digitalWrite(Rele_R1,LOW);
		digitalWrite(Rele_R2,LOW);
		ButECO_Start = false;
		digitalWrite(led_ECO,LOW);
		Serial.println("ButtonECO Off");
	}
}

void UpdateReleECO()                        // Программа выполнения программы по включению реле №2
{
	currentMillis = millis();

	if((Rele2_Start == true) && (currentMillis - currentMillisECO >= Rele2_time))
	{
		digitalWrite(Rele_R2,HIGH);
		Rele2_Start = false;
		Serial.println("Rele_R2 On");
	}
}


void UpdateWC()
{
	currentMillis = millis();
	if((ButWC_Start == true) && (currentMillis - currentMillisWC >= timeWC))
	{
		digitalWrite(Rele_R1,LOW);
		digitalWrite(Rele_R2,LOW);
		ButWC_Start = false;
		digitalWrite(led_WC,HIGH);
		Serial.println("ButtonWC Off");
	}
}

//void UpdateReleWC()                        // Программа выполнения программы по включению реле №2
//{
//	currentMillis = millis();
//
//	if((Rele2_Start == true) && (currentMillis - currentMillisWC >= Rele2_time))
//	{
//		digitalWrite(Rele_R2,HIGH);
//		Rele2_Start = false;
//		Serial.println("Rele_R2 On");
//	}
//}





void test_sensor()
{
	// ------------------------  Проверка нажатия кнопки ButtonECO -----------------------
	if (digitalRead(ButtonECO) == LOW && ButECO_Start != true )
	{
		if(ButECO == false)
		{
			ButECO = true;
			ButECO_Start = true;
			Rele2_Start = true;
			Serial.println("ButtonECO On");
			currentMillisECO = millis();
			digitalWrite(Rele_R1,HIGH);
			digitalWrite(led_ECO,HIGH);
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
		if((ButECO_Start == true) && (currentMillis - currentMillisECO >= time_push_ButECO))
		{
			digitalWrite(Rele_R1,LOW);
			digitalWrite(Rele_R2,LOW);
			ButECO_Start = false;
			digitalWrite(led_ECO,LOW);
			Serial.println("ButtonECO Off");
			while(digitalRead(ButtonECO) == LOW){}              // Ожидание отпускания кнопки ButtonECO для предотвращения запуска нового нажатия.
		}
	}

	//--------------------------  Проверка нажатия кнопки ButtonWC -----------------------
	if (digitalRead(ButtonWC) == LOW)
	{
		if(ButWC == false)
		{
			ButWC = true;
			ButWC_Start = true;
			Rele2_Start = true;
			Serial.println("ButtonWC");
			digitalWrite(Rele_R1,HIGH);
			currentMillisWC = millis();
		}
	}
	else
	{
		ButWC = false;
	}

//------------- проверка контактов датчиков ------------------------	
	if (digitalRead(SW1) == LOW)
	{
		if(ButSW1 == false)
		{
			ButSW1 = true;
			Serial.println("SW1");
		}
	}
	else
	{
		ButSW1 = false;
	}
		
	if (digitalRead(SW2) == LOW)
	{
		if(ButSW2 == false)
		{
			ButSW2 = true;
			Serial.println("SW2");
		}
	}
	else
	{
		ButSW2 = false;
	}
	
	if (digitalRead(SW3) == LOW)
	{
		if(ButSW3 == false)
		{
			ButSW3 = true;
			Serial.println("SW3");
		}
	}
	else
	{
		ButSW3 = false;
	}
}



Flasher led1(led_ECO, 200, 200);
Flasher led2(led_WC, 200, 200);
//Flasher Ledlight(Led_light, 350, 350);

void setup() 
{
	Serial.begin(9600);
	pinMode(Rele_R1, OUTPUT);                    // Реле R1  
	pinMode(Rele_R2, OUTPUT);                    // Реле R2
	pinMode(Rele_R3, OUTPUT);                    // Реле R3
	pinMode(Rele_R4, OUTPUT);                    // Реле R4

	pinMode(led_ECO, OUTPUT);                    // Светодиод на кнопке ECO
	pinMode(led_WC,  OUTPUT);                    // Светодиод на кнопке WC
	digitalWrite(led_WC,HIGH);                   // Включить светодиод на кнопке WC
	pinMode(ButtonECO,INPUT);                    // Кнопка ECO
	pinMode(ButtonWC, INPUT);                    // Кнопка WC
	digitalWrite(ButtonECO,HIGH);                // Установить высокий уровень на кнопке
	digitalWrite(ButtonWC,HIGH);                 // Установить высокий уровень на кнопке
 
	pinMode(SW1, INPUT);                         // SW1 HIGH вкл R3 на 30сек. Сигнал от датчика влажности вкл вентиляцию
	pinMode(SW2, INPUT);                         // pin SW2 HIGH вкл R3 и R4 на 10 сек. Сигнал от датчика движения вкл освещение и вентиляцию
	pinMode(SW3, INPUT);                         // pin SW3 HIGH вкл R4 на 90сек + вкл плавно(1сек) Led на 60сек если SW3 LOW выкл плавно(1сек) Led. Сигнал от датчика движения вкл освещение и подсветку (аналог) LED 
	digitalWrite(SW1,HIGH);                      // Установить высокий уровень на контакте
	digitalWrite(SW2,HIGH);                      // Установить высокий уровень на контакте
	digitalWrite(SW3,HIGH);                      // Установить высокий уровень на контакте


	pinMode(Led_light, OUTPUT);                  // Светодиод подсветки 

	myservo.attach(servo_tank);                  // attaches the servo on pin 9 to the servo object 
	dht.begin();

	Serial.println("Setup Ok!");
	//sweeper1.Attach(servo_tank);
}

void loop() 
{
	test_sensor();
	UpdateECO();
	UpdateReleECO();

	UpdateWC();
//	UpdateReleWC();

	currentMillis = millis();

	if(ButECO_Start==true && (currentMillis - (currentMillisECO) >= timeECO - time_flash_led_ECO))
	{
       led1.Update();
	}

	currentMillis = millis();

	if(ButWC_Start==true)
	{
       led2.Update();
	}

	
	// Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
	// float h = dht.readHumidity();
	// float t = dht.readTemperature();     // Read temperature as Celsius (the default)

	//myservo.write(pos0);              // tell servo to go to position in variable 'pos' 
	//Serial.println(pos0);
	//delay(2000);     
	//myservo.write(pos50);              // tell servo to go to position in variable 'pos' 
	//Serial.println(pos50);
	//delay(2000);     

	//myservo.write(pos10);              // tell servo to go to position in variable 'pos' 
	//Serial.println(pos10);
	//delay(2000);     
	 //myservo.write(0);              // tell servo to go to position in variable 'pos' 
	 //delay(1000);
  //for(pos = 0; pos <= 180; pos += 1) // goes from 0 degrees to 180 degrees 
  //{                                  // in steps of 1 degree 
  //  myservo.write(pos);              // tell servo to go to position in variable 'pos' 
  //  delay(25);                       // waits 15ms for the servo to reach the position 
  //} 

  //for(pos = 180; pos>=0; pos-=1)     // goes from 180 degrees to 0 degrees 
  //{                                
  //  myservo.write(pos);              // tell servo to go to position in variable 'pos' 
  //  delay(25);                       // waits 15ms for the servo to reach the position 
  //} 
  //	delay(2000);    
}
