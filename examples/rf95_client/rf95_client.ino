// rf95_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf95_server
// Tested with Anarduino MiniWirelessLoRa

#include <RH_RF95.h>

#ifdef __AVR__  
    #include <SoftwareSerial.h>
    SoftwareSerial SSerial(5, 6); // RX, TX
    #define COMSerial SSerial
    #define ShowSerial Serial

    RH_RF95<SoftwareSerial> rf95(COMSerial);
#endif

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350) ||  defined(ARDUINO_XIAO_RA4M1) 
    #include <SoftwareSerial.h>
    SoftwareSerial SSerial(D7, D6); // RX, TX
    #define COMSerial SSerial
    #define ShowSerial Serial

    RH_RF95<SoftwareSerial> rf95(COMSerial);
#endif

#if  defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32S3)
    #define COMSerial Serial1
    #define ShowSerial Serial

    RH_RF95<HardwareSerial> rf95(COMSerial);
#endif


#ifdef SEEED_XIAO_M0
    #define COMSerial Serial1
    #define ShowSerial Serial

    RH_RF95<Uart> rf95(COMSerial);
#elif defined(ARDUINO_SAMD_VARIANT_COMPLIANCE)
    #define COMSerial Serial1
    #define ShowSerial SerialUSB

    RH_RF95<Uart> rf95(COMSerial);
#endif

#ifdef ARDUINO_ARCH_STM32F4
    #define COMSerial Serial
    #define ShowSerial SerialUSB

    RH_RF95<HardwareSerial> rf95(COMSerial);
#endif

#if defined(NRF52840_XXAA)
    #define COMSerial Serial1
    #define ShowSerial Serial

    RH_RF95<Uart> rf95(COMSerial);
#endif




void setup() {
    ShowSerial.begin(115200);
    ShowSerial.println("RF95 client test.");

    if (!rf95.init()) {
        ShowSerial.println("init failed");
        while (1);
    }

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
    // you can set transmitter powers from 5 to 23 dBm:
    //rf95.setTxPower(13, false);

    rf95.setFrequency(434.0);
}

void loop() {
    ShowSerial.println("Sending to rf95_server");
    // Send a message to rf95_server
    uint8_t data[] = "Hello World!";
    rf95.send(data, sizeof(data));

    rf95.waitPacketSent();

    // Now wait for a reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.waitAvailableTimeout(3000)) {
        // Should be a reply message for us now
        if (rf95.recv(buf, &len)) {
            ShowSerial.print("got reply: ");
            ShowSerial.println((char*)buf);
        } else {
            ShowSerial.println("recv failed");
        }
    } else {
        ShowSerial.println("No reply, is rf95_server running?");
    }

    delay(1000);
}


