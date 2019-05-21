// rf95_router_address4.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, routed reliable messaging server
// with the RHRouter class.
// It is designed to work with the other examples rf95_router_address*

#include <RHRouter.h>
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
// In this small artifical network of 4 nodes,
// messages are routed via intermediate nodes to their destination
// node. All nodes can act as routers
// ROUTER1_ADDRESS <-> ROUTER2_ADDRESS <-> ROUTER3_ADDRESS <-> ROUTER4_ADDRESS
#define ROUTER1_ADDRESS 1
#define ROUTER2_ADDRESS 2
#define ROUTER3_ADDRESS 3
#define ROUTER4_ADDRESS 4



// Class to manage message delivery and receipt, using the driver declared above
RHRouter manager(driver, ROUTER4_ADDRESS);

void setup() 
{
    ShowSerial.begin(9600);
    if(!manager.init())ShowSerial.println("init failed");
    // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36

    // Manually define the routes for this network
    manager.addRouteTo(ROUTER1_ADDRESS, ROUTER3_ADDRESS);
    manager.addRouteTo(ROUTER2_ADDRESS, ROUTER3_ADDRESS);
    manager.addRouteTo(ROUTER3_ADDRESS, ROUTER3_ADDRESS);
}

uint8_t data[] = "Hello World!";
uint8_t back[] = "And hello back to you from router 4";
// Dont put this on the stack:
uint8_t buf[RH_ROUTER_MAX_MESSAGE_LEN];

void loop()
{
    char addr;
    uint8_t len = sizeof(buf);
    uint8_t from;
    
    if(ShowSerial.available())
    {
        addr = ShowSerial.read();
    }
    
    if(addr > '0' && addr <= '4')
    {
        // Send a message to a router
        // It will be routed by the intermediate
        // nodes to the destination node, accorinding to the
        // routing tables in each node
        ShowSerial.print("Send message to router ");
        ShowSerial.println(addr - '0');
        if(manager.sendtoWait(data, sizeof(data), addr - '0') == RH_ROUTER_ERROR_NONE)
        {
            // It has been reliably delivered to the next node.
            // Now wait for a reply from the ultimate router
            memset(buf, 0, len);
            if(manager.recvfromAckTimeout(buf, &len, 3000, &from))
            {
                ShowSerial.print("got reply from : 0x");
                ShowSerial.print(from, HEX);
                ShowSerial.print(": ");
                ShowSerial.println((char*)buf);
            }
            else
            {
                ShowSerial.println("No reply, are routers running?");
            }
        }
        else ShowSerial.println("sendtoWait failed. Are the intermediate routers running?");

        addr = 0;     
    }

    memset(buf, 0, len);
    if(manager.recvfromAck(buf, &len, &from))
    {
        ShowSerial.print("got message from : 0x");
        ShowSerial.print(from, HEX);
        ShowSerial.print(": ");
        ShowSerial.println((char*)buf);
        
        // Send a reply back to the originator router
        if(manager.sendtoWait(back, sizeof(back), from) != RH_ROUTER_ERROR_NONE)
        ShowSerial.println("sendtoWait failed");
    }
}
