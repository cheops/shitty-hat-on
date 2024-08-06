#include "data.h"
#include <Arduino.h>



IrDataPacket::IrDataPacket(){};
IrDataPacket::IrDataPacket(uint32_t raw){this->raw=raw;};

uint32_t IrDataPacket::get_raw()         { return this->raw; }
uint8_t IrDataPacket::get_channel()      { return (this->raw & 0b00000000000000000000000000000001) >> 0; }
uint8_t IrDataPacket::get_team()         { return (this->raw & 0b00000000000000000000000000001110) >> 1; }
uint8_t IrDataPacket::get_action()       { return (this->raw & 0b00000000000000000000000000110000) >> 4; }
uint8_t IrDataPacket::get_action_param() { return (this->raw & 0b00000000000000000000001111000000) >> 6; }
uint16_t IrDataPacket::get_player_id()   { return (this->raw & 0b00000000001111111111110000000000) >> 10; }
uint8_t IrDataPacket::get_crc()          { return (this->raw & 0b00111111110000000000000000000000) >> 22; }

void IrDataPacket::set_raw(uint32_t raw)                  { this->raw = raw; }
void IrDataPacket::set_channel(uint8_t channel)           { this->raw &= ~(0b1 << 0); this->raw |= (channel & 0b1) << 0;}
void IrDataPacket::set_team(uint8_t team)                 { this->raw &= ~(0b111 << 1); this->raw |= (team & 0b111) << 1;}
void IrDataPacket::set_action(uint8_t action)             { this->raw &= ~(0b11 << 4); this->raw |= (action & 0b11) << 4;}
void IrDataPacket::set_action_param(uint8_t action_param) { this->raw &= ~(0b1111 << 6); this->raw |= (action_param & 0b1111) << 6;}
void IrDataPacket::set_player_id(uint16_t player_id)      { this->raw &= ~(0b111111111111 << 10); this->raw |= (player_id & 0b111111111111) << 10;}
void IrDataPacket::set_crc(uint8_t crc)                   { this->raw &= ~(0b11111111 << 22); this->raw |= (crc & 0b11111111) << 22;}
void IrDataPacket::set_unused(uint8_t unused)             { this->raw &= ~(0b11 << 30); this->raw |= (unused & 0b11) << 30;}


/* #region DataReader */
void DataReader::handlePinChange(bool state)
{
    if (dataReady)
        return; // don't read more data until the "buffer" is empty
    if (state == oldState)
        return;       // if the state didn't change then don't do anything. this happens if an other pin caused the interrupt
    oldState = state; // update the oldState value so we can detect the next pin change.
    if (state)
        return;               // we are looking for a rising edge, but the signal is inverted so a falling edge is what we want.
    uint32_t time = micros(); // check time passed since boot up.
    uint32_t delta_time = time - refTime;
    refTime = time;

    /* if delta_time == 4500 set Ack state to true
       ack state resets after a send
    */

    /* Check total pulse length (rising to rising edge) allow for some deviation*/
    if (delta_time > (uint32_t)(13500 * 0.8) && delta_time < (uint32_t)(13500 / 0.8))
    {
        bitsRead = 1;
        rawData = 0;
        return;
    }
    if (bitsRead == 0)
        return;

    if (delta_time > (uint32_t)(2250 * 0.8) && delta_time < (uint32_t)(2250 / 0.8))
    {
        rawData = rawData >> 1; // make room for an extra bit
        rawData |= 0x80000000;  // set left bit high
        if (++bitsRead == (ir_bit_lenght + 1))
        {
            dataReady = 1;
        }
    }
    else if (delta_time > (uint32_t)(1120 * 0.8) && delta_time < (uint32_t)(1120 / 0.8))
    {
        rawData = rawData >> 1; // make room for an extra bit
        if (++bitsRead == (ir_bit_lenght + 1))
        {
            dataReady = 1;
        }
    }
}
void DataReader::reset()
{
    rawData = 0;
    bitsRead = 0;
    dataReady = 0;
}
bool DataReader::isDataReady()
{
    return dataReady;
}

uint32_t DataReader::getPacket()
{
    uint32_t p;
    p = rawData;
    reset();
    return p;
}
/* #endregion */

/* #region Data */

// Private
_data::_data()
{
    ir1_reader.reset();

    enableReceive();
}

void _data::enableReceive()
{
    // Enable Pin Change Interrupt for PB4 (PCINT4)
    // Set the PCIE0 bit in the GIMSK register to enable PCINT0..PCINT5 interrupts
    GIMSK |= (1 << PCIE);

    // Enable the specific pin change interrupt for PB4 (PCINT4)
    // Set the PCINT4 bit in the PCMSK register to enable the pin change interrupt for PB4
    //   PCMSK |= (1 << PCINT4);
    PCMSK |= 0b00010000; // turn on PCINT4

    pinMode(IR_IN1_PIN, INPUT);
}

void _data::disableReceive()
{
    PCMSK &= ~0b00010000; // turn off PCINT4
    ir1_reader.reset();
}

// Public
IrDataPacket _data::readIr() // add overload to bypass command type validation?
{
    if (ir1_reader.isDataReady())
    {
        IrDataPacket p(ir1_reader.getPacket());
        p.set_raw(calculateCRC(p.get_raw()));
        if (p.get_crc() == 0)
        {
            return p;
        }
    }

    return IrDataPacket(0);
}

_data &_data::getInstance()
{
    static _data data;
    return data;
}

void _data::init()
{
}

uint32_t _data::calculateCRC(uint32_t raw_packet)
{
    uint32_t raw = raw_packet;
    uint32_t checksum = ((raw << 2) & 0b10000000111111110111111100) ^
                        ((raw << 1) & 0b01111111100000000111111110) ^
                        ((raw << 0) & 0b00000000111111111111111111) ^
                        ((raw >> 1) & 0b00000000100000000000000000) ^
                        ((raw >> 2) & 0b00000000011111110000000000) ^
                        ((raw >> 3) & 0b00000000111111111000000000) ^
                        ((raw >> 4) & 0b00000011100000001111111100) ^
                        ((raw >> 5) & 0b00000000111111111000000010);
    checksum = checksum ^ (checksum >> 8) ^ (checksum >> 16) ^ (checksum >> 24);
    checksum = checksum & 0xFF;
    raw ^= checksum << 24;
    return raw;
}

void _data::receive_ISR(bool ir1)
{
    ir1_reader.handlePinChange(ir1);
}

/* #endregion */

_data &Data = Data.getInstance();

// this is the attiny85 interrupt vector for external pin change (configured for PCINT4)
ISR(PCINT0_vect)
{
    // check the status of the pin and pass it to the receive_ISR
    // bool IR1 = PINB & (1 << PB4);
    bool IR1 = (PINB & 0b00010000);

    Data.receive_ISR(IR1);
}
