

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
#define DHT11     11                             // Датчик влажности

Servo myservo;                                   // create servo object to control a servo 
                                                 // twelve servo objects can be created on most boards
int pos   = 0;                                   // variable to store the servo position 
int pos0  = 0;                                   // variable to store the servo position 
int pos50 = 30;                                  // variable to store the servo position 
int pos10 = 6;                                   // variable to store the servo position 

bool ButECO = false;                             //
bool ButECO_Start = false;                       // Флаг запуска программы по команде кнопки  ButtonECO
bool ButWC  = false;
bool ButSW1 = false;
bool ButSW2 = false;
bool ButSW3 = false;

bool Rele2_Start = false;                          // Флаг включения реле №2


unsigned long timeECO            = 10000;          // 300000 Время включения реле №1 ( 5 минут)
unsigned long timeWC             = 2000;           // Увеличить до 5 минут
unsigned long Rele2_time         = 500;            // 2000 Время задержки включения реле№2 (2 секунды)
unsigned long time_flash_led_ECO = 2000;           // 60000 Время до окончания периода, включить мигание светодиода (60 секунд)
unsigned long time_push_ButECO   = 2000;           // 2000 Время удержания кнопки ButtonECO  (2 секунды)
unsigned long currentMillisECO   = 0;
unsigned long currentMillisWC    = 0;


class RelayControl                                  // Управление реле в многозадачном режиме  
{
	int relePin;
	long OnTime;
	long OffTime;

	int releState;
	unsigned long previousMillis;
public:
	RelayControl(int pin,  long on, long off)
	{
		relePin = pin;
		pinMode(relePin, OUTPUT);

		OnTime = on;
		OffTime = off;

		releState = LOW;
		previousMillis = 0;
	}

	void Update()
	{
       unsigned long currentMillis = millis();

	   if((releState == HIGH) && (currentMillis - previousMillis >= OnTime))
	   {
		   releState = LOW;
		   previousMillis = currentMillis;  
		   digitalWrite(relePin,releState);
	   }
	   else if ((releState == LOW) && (currentMillis - previousMillis >= OffTime))
	   {
		   releState = HIGH;
		   previousMillis = currentMillis;  
		   digitalWrite(relePin,releState); 
	   }
	}
};


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

void flash_led()
{
	 unsigned long currentMillis = millis();

	if(ButECO_Start==true && (currentMillis - (currentMillisECO) >= timeECO - time_flash_led_ECO))
	{
      // led1.Update();
	}
	
}





//class Sweeper                                    // Управление servo в многозадачном режиме  
//{
//Servo servo;
//int pos;
//int increment;
//int updateInterval;
//unsigned long lastUpdate;
//
//public:
//	Sweeper(int interval)
//	{
//		updateInterval = interval;
//		increment = 1;
//	}
//
//	void Attach(int pin)
//	{
//		servo.attach(pin);
//	}
//	void Detach()
//	{
//       servo.detach();
//	}
//  void Update()
//  {
//    if((millis() - lastUpdate) > updateInterval)
//	{
//      lastUpdate = millis();
//	  pos += increment;
//	  servo.write(pos);
//	  Serial.println(pos);
//	  if((pos >= 180) || (pos <= 0))
//	  {
//       increment = -increment;
//	  }
//	}
//  }
//};
//
//
//Sweeper sweeper1(10);


void UpdateECO()
{
	unsigned long currentMillis = millis();

	if((ButECO_Start == true) && (currentMillis - currentMillisECO >= timeECO ))
	{
		digitalWrite(Rele_R1,LOW);
		digitalWrite(Rele_R2,LOW);
		ButECO_Start = false;
		digitalWrite(led_ECO,LOW);
		Serial.println("ButtonECO Off");
	}
}

void UpdateRele2()
{
	unsigned long currentMillis = millis();
	if((Rele2_Start == true) && (currentMillis - currentMillisECO >= Rele2_time))
	{
		digitalWrite(Rele_R2,HIGH);
		Rele2_Start = false;
		Serial.println("Rele_R2 On");
	}
}

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
	   unsigned long currentMillis = millis();

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

	if (digitalRead(ButtonWC) == LOW)
	{
		if(ButWC == false)
		{
			ButWC = true;
			Serial.println("ButtonWC");
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



RelayControl ReleR1(Rele_R1,1000,400);
RelayControl ReleR2(Rele_R2,100,400);
RelayControl ReleR3(Rele_R3,100,400);
RelayControl ReleR4(Rele_R4,100,400);

Flasher led1(led_ECO, 200, 200);
//Flasher ledWC(led_WC, 350, 350);
//Flasher Ledlight(Led_light, 350, 350);

void setup() 
{
	Serial.begin(9600);
	pinMode(Rele_R1, OUTPUT);                    // Реле R1  
	pinMode(Rele_R2, OUTPUT);                    // Реле R2
	pinMode(Rele_R3, OUTPUT);                    // Реле R3
	pinMode(Rele_R4, OUTPUT);                    // Реле R4
	//digitalWrite(Rele_R1,HIGH);


	pinMode(led_ECO, OUTPUT);                    // Светодиод на кнопке ECO
	pinMode(led_WC,  OUTPUT);                    // Светодиод на кнопке WC
	pinMode(ButtonECO,INPUT);                    // Кнопка ECO
	pinMode(ButtonWC, INPUT);                    // Кнопка WC
	digitalWrite(ButtonECO,HIGH);
	digitalWrite(ButtonWC,HIGH);
 


	pinMode(SW1, INPUT);                         // SW1 HIGH вкл R3 на 30сек. Сигнал от датчика влажности вкл вентиляцию
	pinMode(SW2, INPUT);                         // pin SW2 HIGH вкл R3 и R4 на 10 сек. Сигнал от датчика движения вкл освещение и вентиляцию
	pinMode(SW3, INPUT);                         // pin SW3 HIGH вкл R4 на 90сек + вкл плавно(1сек) Led на 60сек если SW3 LOW выкл плавно(1сек) Led. Сигнал от датчика движения вкл освещение и подсветку (аналог) LED 
	pinMode(Led_light, OUTPUT);                  // Светодиод подсветки 
	//digitalWrite(Led_light,HIGH);

	myservo.attach(servo_tank);                  // attaches the servo on pin 9 to the servo object 
	Serial.println("Setup Ok!");
	//sweeper1.Attach(servo_tank);
}

void loop() 
{

	test_sensor();
	UpdateECO();
	UpdateRele2();
//	flash_led();

	unsigned long currentMillis = millis();
	if(ButECO_Start==true && (currentMillis - (currentMillisECO) >= timeECO - time_flash_led_ECO))
	{
       led1.Update();
	}
	
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
