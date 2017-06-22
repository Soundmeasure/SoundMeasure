
/**
* An Mirf example which copies back the data it recives.
*
* Pins:
* Hardware SPI:
* MISO -> 12
* MOSI -> 11
* SCK -> 13
*
* Configurable:
* CE -> 8
* CSN -> 7
*
*/

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <AH_AD9850.h>
#include <SdFat.h>

//byte data[16];                           //  A buffer to store the data. ����� ��� �������� ������.(default 16 max 32.)
//byte data[Mirf.payload];                //  A buffer to store the data. ����� ��� �������� ������.(default 16 max 32.)



//��������� ��������� ����������
#define CLK     8  // ���������� ������� ���������� ��������
#define FQUP    9  // ���������� ������� ���������� ��������
#define BitData 10 // ���������� ������� ���������� ��������
#define RESET   11 // ���������� ������� ���������� ��������
AH_AD9850 AD9850(CLK, FQUP, BitData, RESET);// ��������� ��������� ����������

											// serial output steam
ArduinoOutStream cout(Serial);





//
//void receiv_packet()
//{
//	// ���� ����� �������.  isSending ����� ��������������� ����� �������������, ����� �� ��������� �� true � false.
//	if (!Mirf.isSending() && Mirf.dataReady())
//	{
//		Serial.println("Got packet");
//		Mirf.getData(data);                     // Get load the packet into the buffer.
//		Mirf.setTADDR((byte *)"clie1");        // Set the send address.
//		Mirf.send(data);                       // Send the data back to the client.
//		delay(10);
//		Serial.println("Reply sent.");
//	}
//}




void setup() {
	Serial.begin(9600);

	Mirf.spi = &MirfHardwareSpi;            // Set the SPI Driver.
	Mirf.init();                            // Setup pins / SPI.
	Mirf.setRADDR((byte *)"serv1");         // Configure reciving address.
    Mirf.payload = sizeof(unsigned long);   // payload on client and server must be the same.
	Mirf.config();                          //  Write channel and payload config then power up reciver.

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// ��������� ��������� ����������  
	//AD9850.reset();                         //reset module
	//delay(1000);
	//AD9850.powerDown();                     //set signal output to LOW
	//AD9850.set_frequency(0, 0, 500);        //set power=UP, phase=0, 1kHz frequency 

	Serial.println("Listening...");
}

void loop() 
{
	byte data[Mirf.payload];                //  A buffer to store the data. ����� ��� �������� ������.(default 16 max 32.)
	// ���� ����� �������.  isSending ����� ��������������� ����� �������������, ����� �� ��������� �� true � false.
	if (!Mirf.isSending() && Mirf.dataReady())
	{
		Serial.println("Got packet");
		Mirf.getData(data);                     // Get load the packet into the buffer.
		Mirf.setTADDR((byte *)"clie1");        // Set the send address.
		Mirf.send(data);                       // Send the data back to the client.
		delay(10);
		Serial.println("Reply sent.");
	}
	//receiv_packet();
	
	//cout << data;

}