
#include <RH_RF95.h>


// These are indexed by the values of ModemConfigChoice
// Stored in flash (program) memory to save SRAM
PROGMEM static const RH_RF95::ModemConfig MODEM_CONFIG_TABLE[] =
{
    //  1d,     1e,      26
    { 0x72,   0x74,    0x00}, // Bw125Cr45Sf128 (the chip default)
    { 0x92,   0x74,    0x00}, // Bw500Cr45Sf128
    { 0x48,   0x94,    0x00}, // Bw31_25Cr48Sf512
    { 0x78,   0xc4,    0x00}, // Bw125Cr48Sf4096
    
};

RH_RF95::RH_RF95(SoftwareSerial& ss)
    :
    RHUartDriver(ss),
    _rxBufValid(0)
{
}

bool RH_RF95::init()
{
    RHUartDriver::init();
    
    // Set sleep mode, so we can also set LORA mode:
    write(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_SLEEP | RH_RF95_LONG_RANGE_MODE);
    delay(10); // Wait for sleep mode to take over from say, CAD
    
    // Check we are in sleep mode, with LORA set
    if (read(RH_RF95_REG_01_OP_MODE) != (RH_RF95_MODE_SLEEP | RH_RF95_LONG_RANGE_MODE))
    {
        Serial.println(read(RH_RF95_REG_01_OP_MODE), HEX);
        return false; // No device present?
    }

    // Set up FIFO
    // We configure so that we can use the entire 256 byte FIFO for either receive
    // or transmit, but not both at the same time
    write(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    write(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0);

    // Packet format is preamble + explicit-header + payload + crc
    // Explicit Header Mode
    // payload is TO + FROM + ID + FLAGS + message data
    // RX mode is implmented with RXCONTINUOUS
    // max message data length is 255 - 4 = 251 octets

    setModeIdle();

    // Set up default configuration
    // No Sync Words in LORA mode.
    setModemConfig(Bw125Cr45Sf128); // Radio default
    //setModemConfig(Bw125Cr48Sf4096); // slow and reliable?
    setPreambleLength(8); // Default is 8
    // An innocuous ISM frequency, same as RF22's
    //setFrequency(434.0);
    setFrequency(868.0);
    // Lowish power
    setTxPower(13);

    return true;
}

// C++ level interrupt handler for this instance
// LORA is unusual in that it has several interrupt lines, and not a single, combined one.
// On MiniWirelessLoRa, only one of the several interrupt lines (DI0) from the RFM95 is usefuly 
// connnected to the processor.
// We use this to get RxDone and TxDone interrupts
void RH_RF95::handleInterrupt()
{
    // Read the interrupt register
    uint8_t irq_flags = read(RH_RF95_REG_12_IRQ_FLAGS);
    if (_mode == RHModeRx && irq_flags & (RH_RF95_RX_TIMEOUT | RH_RF95_PAYLOAD_CRC_ERROR))
    {
        _rxBad++;
    }
    else if (_mode == RHModeRx && irq_flags & RH_RF95_RX_DONE)
    {
        // Have received a packet
        uint8_t len = read(RH_RF95_REG_13_RX_NB_BYTES);

        // Reset the fifo read ptr to the beginning of the packet
        write(RH_RF95_REG_0D_FIFO_ADDR_PTR, read(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR));
        burstRead(RH_RF95_REG_00_FIFO, _buf, len);
        _bufLen = len;
        write(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // Clear all IRQ flags

        // Remember the RSSI of this packet
        // this is according to the doc, but is it really correct?
        // weakest receiveable signals are reported RSSI at about -66
        _lastRssi = read(RH_RF95_REG_1A_PKT_RSSI_VALUE) - 137;

        // We have received a message.
        validateRxBuf(); 
        if (_rxBufValid)setModeIdle(); // Got one 
    }
    else if (_mode == RHModeTx && irq_flags & RH_RF95_TX_DONE)
    {
        _txGood++;
        setModeIdle();
    }
    
    write(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // Clear all IRQ flags
}

// Check whether the latest received message is complete and uncorrupted
void RH_RF95::validateRxBuf()
{
    if (_bufLen < 4)return; // Too short to be a real message

    // Extract the 4 headers
    _rxHeaderTo    = _buf[0];
    _rxHeaderFrom  = _buf[1];
    _rxHeaderId    = _buf[2];
    _rxHeaderFlags = _buf[3];
    
    if (_promiscuous || _rxHeaderTo == _thisAddress || _rxHeaderTo == RH_BROADCAST_ADDRESS)
    {
        _rxGood++;
        _rxBufValid = true;
    }
}

bool RH_RF95::available()
{
    if (uartAvailable())
    {
        if (uartRead() == 'I')
        {
            handleInterrupt();
        }
    }
    
    if (_mode == RHModeTx)return false;
    setModeRx();

    return _rxBufValid; // Will be set by the interrupt handler when a good message is received
}

void RH_RF95::clearRxBuf()
{
    _rxBufValid = false;
    _bufLen = 0;
}

bool RH_RF95::recv(uint8_t* buf, uint8_t* len)
{
    if (!available())
	return false;
    if (buf && len)
    {
        // Skip the 4 headers that are at the beginning of the rxBuf
        if (*len > _bufLen-RH_RF95_HEADER_LEN)*len = _bufLen-RH_RF95_HEADER_LEN;
        memcpy(buf, _buf+RH_RF95_HEADER_LEN, *len);
    }
    clearRxBuf(); // This message accepted and cleared
    return true;
}

bool RH_RF95::send(uint8_t* data, uint8_t len)
{
    if (len > RH_RF95_MAX_MESSAGE_LEN)
	return false;

    waitPacketSent(); // Make sure we dont interrupt an outgoing message
    setModeIdle();

    // Position at the beginning of the FIFO
    write(RH_RF95_REG_0D_FIFO_ADDR_PTR, 0);
    
    // The headers
    write(RH_RF95_REG_00_FIFO, _txHeaderTo);
    write(RH_RF95_REG_00_FIFO, _txHeaderFrom);
    write(RH_RF95_REG_00_FIFO, _txHeaderId);
    write(RH_RF95_REG_00_FIFO, _txHeaderFlags);
    
    // The message data
    burstWrite(RH_RF95_REG_00_FIFO, data, len);
    write(RH_RF95_REG_22_PAYLOAD_LENGTH, len + RH_RF95_HEADER_LEN);

    setModeTx(); // Start the transmitter
    // when Tx is done, interruptHandler will fire and radio mode will return to STANDBY
    
    //write(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // Clear all IRQ flags
    return true;
}

bool RH_RF95::printRegisters()
{
#ifdef RH_HAVE_SERIAL
    uint8_t registers[] = { 0x01, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x014, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27};

    uint8_t i;
    for (i = 0; i < sizeof(registers); i++)
    {
        Serial.print(registers[i], HEX);
        Serial.print(": ");
        Serial.println(read(registers[i]), HEX);
    }
#endif
    return true;
}

uint8_t RH_RF95::maxMessageLength()
{
    return RH_RF95_MAX_MESSAGE_LEN;
}

bool RH_RF95::setFrequency(float centre)
{
    // Frf = FRF / FSTEP
    uint32_t frf = (centre * 1000000.0) / RH_RF95_FSTEP;
    write(RH_RF95_REG_06_FRF_MSB, (frf >> 16) & 0xff);
    write(RH_RF95_REG_07_FRF_MID, (frf >> 8) & 0xff);
    write(RH_RF95_REG_08_FRF_LSB, frf & 0xff);

    return true;
}

void RH_RF95::setModeIdle()
{
    if (_mode != RHModeIdle)
    {
        write(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_STDBY);
        _mode = RHModeIdle;
    }
}

bool RH_RF95::sleep()
{
    if (_mode != RHModeSleep)
    {
        write(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_SLEEP);
        _mode = RHModeSleep;
    }
    return true;
}

void RH_RF95::setModeRx()
{
    if (_mode != RHModeRx)
    {
        write(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_RXCONTINUOUS);
        write(RH_RF95_REG_40_DIO_MAPPING1, 0x00); // Interrupt on RxDone
        _mode = RHModeRx;
    }
}

void RH_RF95::setModeTx()
{
    if (_mode != RHModeTx)
    {
        write(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_TX);
        write(RH_RF95_REG_40_DIO_MAPPING1, 0x40); // Interrupt on TxDone
        _mode = RHModeTx;
    }
}

void RH_RF95::setTxPower(int8_t power, bool useRFO)
{
    // Sigh, different behaviours depending on whther the module use PA_BOOST or the RFO pin
    // for the transmitter output
    if (useRFO)
    {
        if (power > 14)power = 14;
        if (power < -1)power = -1;
        write(RH_RF95_REG_09_PA_CONFIG, RH_RF95_MAX_POWER | (power + 1));
    }
    else
    {
        if (power > 23)power = 23;
        if (power < 5)power = 5;

        // For RH_RF95_PA_DAC_ENABLE, manual says '+20dBm on PA_BOOST when OutputPower=0xf'
        // RH_RF95_PA_DAC_ENABLE actually adds about 3dBm to all power levels. We will us it
        // for 21 and 23dBm
        if (power > 20)
        {
            write(RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_ENABLE);
            power -= 3;
        }
        else
        {
            write(RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_DISABLE);
        }

        // RFM95/96/97/98 does not have RFO pins connected to anything. Only PA_BOOST
        // pin is connected, so must use PA_BOOST
        // Pout = 2 + OutputPower.
        // The documentation is pretty confusing on this topic: PaSelect says the max power is 20dBm,
        // but OutputPower claims it would be 17dBm.
        // My measurements show 20dBm is correct
        write(RH_RF95_REG_09_PA_CONFIG, RH_RF95_PA_SELECT | (power-5));
    }
}

// Sets registers from a canned modem configuration structure
void RH_RF95::setModemRegisters(const ModemConfig* config)
{
    write(RH_RF95_REG_1D_MODEM_CONFIG1, config->reg_1d);
    write(RH_RF95_REG_1E_MODEM_CONFIG2, config->reg_1e);
    write(RH_RF95_REG_26_MODEM_CONFIG3, config->reg_26);
}

// Set one of the canned FSK Modem configs
// Returns true if its a valid choice
bool RH_RF95::setModemConfig(ModemConfigChoice index)
{
    if (index > (signed int)(sizeof(MODEM_CONFIG_TABLE) / sizeof(ModemConfig)))return false;

    ModemConfig cfg;
    memcpy_P(&cfg, &MODEM_CONFIG_TABLE[index], sizeof(RH_RF95::ModemConfig));
    setModemRegisters(&cfg);

    return true;
}

void RH_RF95::setPreambleLength(uint16_t bytes)
{
    write(RH_RF95_REG_20_PREAMBLE_MSB, bytes >> 8);
    write(RH_RF95_REG_21_PREAMBLE_LSB, bytes & 0xff);
}
