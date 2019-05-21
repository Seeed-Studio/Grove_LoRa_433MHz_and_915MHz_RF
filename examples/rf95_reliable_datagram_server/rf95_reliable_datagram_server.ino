// rf95_reliable_datagram_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging server
// with the RHReliableDatagram class, using the RH_RF95 driver to control a RF95 radio.
// It is designed to work with the other example rf95_reliable_datagram_client
// Tested with Anarduino MiniWirelessLoRa

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#ifdef __AVR__
#include <SoftwareSerial.h>
SoftwareSerial SSerial(10, 11); // RX, TX
#define COMSerial SSerial
#define ShowSerial Serial 

RH_RF95<SoftwareSerial> driver(COMSerial);
#endif

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
#define COMSerial Serial1
#define ShowSerial SerialUSB 

RH_RF95<Uart> driver(COMSerial);
#endif

#ifdef ARDUINO_ARCH_STM32F4
#define COMSerial Serial
#define ShowSerial SerialUSB 

RH_RF95<HardwareSerial> driver(COMSerial);
#endif
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2


// Singleton instance of the radio driver
SoftwareSerial ss(10, 11);
RH_RF95 driver(ss);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);


void setup() 
{
  ShowSerial.begin(115200);
  if (!manager.init())ShowSerial.println("init failed");
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  //driver.setTxPower(23, false);
}

uint8_t data[] = "And hello back to you";
// Dont put this on the stack:
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

void loop()
{
  if(manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    
    if(manager.recvfromAck(buf, &len, &from))
    {
      ShowSerial.print("got request from : 0x");
      ShowSerial.print(from, HEX);
      ShowSerial.print(": ");
      ShowSerial.println((char*)buf);

      // Send a reply back to the originator client
      if(!manager.sendtoWait(data, sizeof(data), from))
        ShowSerial.println("sendtoWait failed");
    }
  }
}

