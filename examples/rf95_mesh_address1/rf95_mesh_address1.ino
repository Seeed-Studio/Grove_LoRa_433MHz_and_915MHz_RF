// rf95_mesh_address1.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, routed reliable messaging client
// with the RHMesh class.
// It is designed to work with the other examples rf95_mesh_address*
// Hint: you can simulate other network topologies by setting the 
// RH_TEST_NETWORK define in RHRouter.h
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


#include <RHMesh.h>

// Mesh has much greater memory requirements, and you may need to limit the
// max message length to prevent wierd crashes
#define RH_MESH_MAX_MESSAGE_LEN 50

// In this small artifical network of 4 nodes,
#define MESH1_ADDRESS 1
#define MESH2_ADDRESS 2
#define MESH3_ADDRESS 3
#define MESH4_ADDRESS 4


// Class to manage message delivery and receipt, using the driver declared above
RHMesh manager(driver, MESH1_ADDRESS);

void setup() 
{
  ShowSerial.begin(9600);
  if (!manager.init())
    ShowSerial.println("init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
}

uint8_t data[] = "Hello World!";
uint8_t back[] = "And hello back to you from mesh 1";
// Dont put this on the stack:
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];

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
        // Send a message to a mesh
        // It will be routed by the intermediate
        // nodes to the destination node, accorinding to the
        // routing tables in each node
        ShowSerial.print("Send message to mesh ");
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
                ShowSerial.println("No reply, are meshes running?");
            }
        }
        else ShowSerial.println("sendtoWait failed. Are the intermediate meshes running?");

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
