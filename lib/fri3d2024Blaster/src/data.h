
#ifndef DATA_H
#define DATA_H
#include <Arduino.h>

const int ir_bit_lenght = 32;
const int ir_start_high_time = 16;
const int ir_start_low_time = 8;
const int ir_zero_high_time = 1;
const int ir_zero_low_time = 1;
const int ir_one_high_time = 1;
const int ir_one_low_time = 3;
const int ir_stop_high_time = 1;
const int ir_stop_low_time = 1;
const int pulse_train_lenght =  2 + ir_bit_lenght * 2 + 2;
#define IR_IN1_PIN 4 // PB4

enum TeamColor : uint8_t
{
  eNoTeam = 0b000,
  eTeamRex = 0b001,
  eTeamGiggle = 0b010,
  eTeamBuzz = 0b100,
  eTeamYellow = eTeamRex | eTeamGiggle,
  eTeamMagenta = eTeamRex | eTeamBuzz,
  eTeamCyan = eTeamGiggle | eTeamBuzz,
  eTeamWhite = eTeamRex | eTeamGiggle | eTeamBuzz
};

enum Action : uint8_t
{
  eActionNone = 0,
  eActionDamage = 1,
  eActionHeal = 2,
};

union IrDataPacket
{
  uint32_t raw;
  struct
  {
    uint8_t channel: 1;
    uint8_t team: 3;
    uint8_t action: 2;
    uint8_t action_param: 4;
    uint16_t player_id: 12;
    uint8_t crc: 8;
  };
};


class DataReader
{
private:
  volatile uint32_t refTime;
  volatile bool oldState = 1;
  volatile uint32_t rawData;
  volatile uint8_t bitsRead;
  volatile bool dataReady;

public:
  void handlePinChange(bool state);
  void reset();           // clear buffer
  bool isDataReady();     // check buffer, if valid True, if invalid ResetBuffer
  uint32_t getPacket(); // return packet and reset; Dataclass then needs to calculate CRC
};

class _data
{
private:
  _data();
  DataReader ir1_reader;

  void enableReceive();
  void disableReceive();

public:
  IrDataPacket readIr();

  static _data &getInstance();
  uint32_t calculateCRC(uint32_t raw_packet);
  void receive_ISR(bool ir1); // function called by ISR
  void init();
};

extern _data &Data;
#endif
