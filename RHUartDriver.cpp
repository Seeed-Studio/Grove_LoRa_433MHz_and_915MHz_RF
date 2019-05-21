/*
 * RHUartDriver.h
 * A library for Grove - Lora 433MHz/470MHz RF or Grove - Lora 868MHz/915MHz RF
 *
 * Copyright (c) 2015 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : JY.W
 * Modified Time: 2016-07-15
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <RHUartDriver.h>

template <typename T>
RHUartDriver<T>::RHUartDriver(T& ss)
    :
    _ss(ss)
{
}

template <typename T>
bool RHUartDriver<T>::init()
{
    _ss.begin(57600);
 //   _ss.listen();
    
    return true;
}

template <typename T>
uint8_t RHUartDriver<T>::uartAvailable(void)
{
    return _ss.available();    
}

template <typename T>
uint8_t RHUartDriver<T>::uartRead(void)
{
    return _ss.read();    
}

template <typename T>
void RHUartDriver<T>::uartTx(uint8_t reg, uint8_t* src, uint8_t len)
{    
    _ss.write('W');
    _ss.write(reg);
    _ss.write(len);
    for(int i = 0; i < len; i ++)_ss.write(*(src + i));
}
    
template <typename T>
void RHUartDriver<T>::uartRx(uint8_t reg, uint8_t* dest, uint8_t len)
{   
    _ss.write('R');
    _ss.write(reg);
    _ss.write(len);
    
    unsigned long timerStart,timerEnd;
    int i = 0;
    timerStart = millis();
    while(1)
    {
        if(_ss.available())
        {
            *(dest + i) = _ss.read();
            i ++;
            if(i >= len)break;
        }
        
        timerEnd = millis();
        if(timerEnd - timerStart > 1000 * DEFAULT_TIMEOUT) break;
    }  
}

template <typename T>  
uint8_t RHUartDriver<T>::read(uint8_t reg)
{
    uint8_t val = 0;
    uartRx(reg & ~RH_WRITE_MASK, &val, 1);
    return val;
}

template <typename T>
void RHUartDriver<T>::write(uint8_t reg, uint8_t val)
{
    uartTx(reg | RH_WRITE_MASK, &val, 1);
}

template <typename T>
void RHUartDriver<T>::burstRead(uint8_t reg, uint8_t* dest, uint8_t len)
{
    uartRx(reg & ~RH_WRITE_MASK, dest, len);
}

template <typename T>
void RHUartDriver<T>::burstWrite(uint8_t reg, uint8_t* src, uint8_t len)
{
    uartTx(reg | RH_WRITE_MASK, src, len);
}


#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
template class RHUartDriver<Uart>;
#endif
template class RHUartDriver<HardwareSerial>;

#ifdef __AVR__
#include <SoftwareSerial.h>
template class RHUartDriver<SoftwareSerial>;
#endif