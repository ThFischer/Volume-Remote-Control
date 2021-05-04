/*
  Config.h - Library for read/write configuration into EEPROM
*/
#ifndef Config_Manager_h
#define Config_Manager_h

#include "Arduino.h"


#define NUMBER_OF_COMMANDS 6
enum VolumeCommand: uint8_t {
  Up = 0,
  Down,    // 1
  Mute,    // 2
  Left,    // 3
  Right,   // 4
  Preset,  // 5
  Unknown  // 6
};

typedef uint16_t irCommands_t[NUMBER_OF_COMMANDS];

typedef struct {
  uint8_t  volume;
  int8_t  balance;
} audioSettings_t;

typedef struct {
  uint16_t irAddress;
  irCommands_t irCommands;
  audioSettings_t audioSettings;
} configData_t;

class ConfigManager {
  public:
    bool store(uint16_t irAddress, irCommands_t irCommands);
    bool store(uint8_t volume, int8_t balance);
    bool store(uint16_t irAddress, irCommands_t irCommands, uint8_t volume, int8_t balance);
    bool restore();
    void getPreset(audioSettings_t *preset);
    bool setPreset(uint8_t volume, int8_t balance);
    bool hasValidConfig() {
      return _isValid;
    };
    VolumeCommand getCommand(uint16_t);
    const char* getVolumeCommandName(uint8_t idx);

  private:
    configData_t _configData;
    audioSettings_t _lastSettings;
    uint16_t _getChecksum(uint8_t *data, int count);
    bool _isValid;
    void logConfig();
};

#endif