// rf95_reliable_datagram_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_RF95 driver to control a RF95 radio.
// It is designed to work with the other example rf95_reliable_datagram_server
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

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350) ||  defined(ARDUINO_XIAO_RA4M1) 
    #include <SoftwareSerial.h>
    SoftwareSerial SSerial(D7, D6); // RX, TX
    #define COMSerial SSerial
    #define ShowSerial Serial

    RH_RF95<SoftwareSerial> driver(COMSerial);
#endif

#if  defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32S3)
    #define COMSerial Serial1
    #define ShowSerial Serial

    RH_RF95<HardwareSerial> driver(COMSerial);
#endif


#ifdef SEEED_XIAO_M0
    #define COMSerial Serial1
    #define ShowSerial Serial

    RH_RF95<Uart> driver(COMSerial);
#elif defined(ARDUINO_SAMD_VARIANT_COMPLIANCE)
    #define COMSerial Serial1
    #define ShowSerial SerialUSB

    RH_RF95<Uart> driver(COMSerial);
#endif

#ifdef ARDUINO_ARCH_STM32F4
    #define COMSerial Serial
    #define ShowSerial SerialUSB

    RH_RF95<HardwareSerial> driver(COMSerial);
#endif

#if defined(NRF52840_XXAA)
    #define COMSerial Serial1
    #define ShowSerial Serial

    RH_RF95<Uart> driver(COMSerial);
#endif

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2




// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);


void setup() {
    ShowSerial.begin(115200);
    if (!manager.init()) {
        ShowSerial.println("init failed");
    }

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
    // you can set transmitter powers from 5 to 23 dBm:
    //driver.setTxPower(23, false);
}

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

void loop() {
    ShowSerial.println("Sending to rf95_reliable_datagram_server");

    // Send a message to manager_server
    if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS)) {
        // Now wait for a reply from the server
        uint8_t len = sizeof(buf);
        uint8_t from;

        if (manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
            ShowSerial.print("got reply from : 0x");
            ShowSerial.print(from, HEX);
            ShowSerial.print(": ");
            ShowSerial.println((char*)buf);
        } else {
            ShowSerial.println("No reply, is rf95_reliable_datagram_server running?");
        }
    } else {
        ShowSerial.println("sendtoWait failed");
    }

    delay(500);
}

