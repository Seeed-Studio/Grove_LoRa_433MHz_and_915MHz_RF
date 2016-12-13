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

#ifndef _RH_UART_Driver_H_
#define _RH_UART_Driver_H_


#include <Arduino.h>
#include <RHGenericDriver.h>
#include <SoftwareSerial.h>


#define DEFAULT_TIMEOUT 3

#define RH_WRITE_MASK 0x80


class RHUartDriver : public RHGenericDriver
{
public:

    RHUartDriver(SoftwareSerial& ss);
    
    virtual bool init();
    
    uint8_t uartAvailable(void);
    
    uint8_t uartRead(void);
    
    // 'W' + 'Reg' + 'Len' + 'Data'
    void uartTx(uint8_t reg, uint8_t* src, uint8_t len);
    
    // 'R' + 'Reg' + 'Len'
    void uartRx(uint8_t reg, uint8_t* dest, uint8_t len);
    
    uint8_t read(uint8_t reg);

    void write(uint8_t reg, uint8_t val);

    void burstRead(uint8_t reg, uint8_t* dest, uint8_t len);

    void burstWrite(uint8_t reg, uint8_t* src, uint8_t len);

protected:

    SoftwareSerial& _ss;

private:

    
    
};

#endif
