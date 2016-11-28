
#include <SPI.h>
#include <SoftwareSerial.h>


#define DIO0_INT 0 // INT0, D2.
#define DIO1_INT 1 // INT1, D3.
#define DIO2_PIN 5 // D5
#define CS_PIN   10 // SPI select pin number.


char command = 0;
unsigned char reg = 0;
int len = 0;
unsigned char buffer[255] = {0, };

int count = 0;

unsigned char intFlag = 0;

static bool dio2_states = 0;


void setup()
{
    Serial.begin(57600);
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    
    attachInterrupt(DIO0_INT, isr0, RISING);
    attachInterrupt(DIO1_INT, isr1, RISING);
    pinMode(DIO2_PIN, INPUT);
    
    SPI.begin(); 
}

void loop()
{
    int i;
    
    if(Serial.available())
    {
        if(count == 0)
        {
            char c = (char)Serial.read();
            if(c == 'W' || c == 'R')
            {
                command = c;
                count ++;    
            }
        }
        else if(count == 1)
        {
            reg = Serial.read();
            count ++;
        }
        else if(count == 2)
        {
            len = Serial.read();
            count ++;
        }
        else if(command == 'W' && count > 2 && count < (len + 3))
        {
            buffer[count - 3] = Serial.read();
            count ++;
        }
    }
    
    if(command == 'W' && count >= (len + 3)) // 
    {
        digitalWrite(CS_PIN, LOW);
        SPI.transfer(reg);
        for(i = 0; i < len; i ++)SPI.transfer(buffer[i]);
        digitalWrite(CS_PIN, HIGH);

        command = 0;
        reg = 0;
        len = 0;
        memset(buffer, 0, 255);
        count = 0;
    }
    else if(command == 'R' && count >= 3)
    {
        digitalWrite(CS_PIN, LOW);
        SPI.transfer(reg);
        for(i = 0; i < len; i ++)buffer[i] = SPI.transfer(0);
        digitalWrite(CS_PIN, HIGH);
        for(i = 0; i < len; i ++)Serial.write(buffer[i]);
        
        command = 0;
        reg = 0;
        len = 0;
        memset(buffer, 0, 255);
        count = 0;
    }
    
    if(dio2_states != digitalRead(DIO2_PIN))
    {
        dio2_states = !dio2_states;
        if(dio2_states)intFlag = 1;
    }
    
    if(intFlag)
    {
        intFlag = 0;
        Serial.write('I');
    }
}

void isr0()
{
    intFlag = 1;
}

void isr1()
{
    intFlag = 1;
}
