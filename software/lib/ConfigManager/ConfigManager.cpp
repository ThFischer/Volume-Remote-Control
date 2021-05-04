#include "Arduino.h"
#include "ConfigManager.h"
#include <EEPROM.h>

#define EEPROM_START_ADDR 1 // The byte at address 0 may damaged!


bool ConfigManager::restore() {
  Serial.println("Restore config");
  EEPROM.get(EEPROM_START_ADDR, _configData);
  uint16_t storedChecksum = 0;
  EEPROM.get(EEPROM_START_ADDR + sizeof(_configData), storedChecksum);
  uint16_t checksum = _getChecksum((uint8_t *)&_configData, sizeof(_configData));
  _isValid = checksum == storedChecksum;
  if (!_isValid) {
    Serial.println("Invalid checksum!");
  } else {
    logConfig();
  }
  return _isValid;
}

uint16_t ConfigManager::_getChecksum(uint8_t *data, int count) {
  uint16_t sum1 = 0;
  uint16_t sum2 = 0;
  for (int i = 0; i < count; ++i) {
    sum1 = (sum1 + data[i]) % 0xFF;
    sum2 = (sum2 + sum1) % 0xFF;
  }
  return (sum2 << 8) | sum1;
};

bool ConfigManager::store(uint16_t irAddress, irCommands_t codes) {
  return store(
    irAddress,
    codes,
    _configData.audioSettings.volume,
    _configData.audioSettings.balance);
}

bool ConfigManager::store(uint8_t volume, int8_t balance) {
  return store(
    _configData.irAddress,
    _configData.irCommands,
    volume,
    balance
  );
}


bool ConfigManager::store(uint16_t irAddress, irCommands_t codes, uint8_t volume, int8_t balance) {
  Serial.println("Store config");
  _configData.irAddress = irAddress;
  for (uint8_t i = 0; i < NUMBER_OF_COMMANDS; i++) {
    _configData.irCommands[i] = codes[i];
  }
  _configData.audioSettings.volume = volume;
  _configData.audioSettings.balance = balance;
  EEPROM.put(EEPROM_START_ADDR, _configData);
  uint16_t checksum = _getChecksum((uint8_t *)&_configData, sizeof(_configData));
  EEPROM.put(EEPROM_START_ADDR + sizeof(_configData), checksum);
  return restore();
}


void ConfigManager::getPreset(audioSettings_t *preset) {
  preset->volume = _configData.audioSettings.volume;
  preset->balance = _configData.audioSettings.balance;
}


bool ConfigManager::setPreset(uint8_t volume, int8_t balance) {
  if (volume != _configData.audioSettings.volume || balance != _configData.audioSettings.balance) {
    return store(volume, balance);
  } else {
    return true;
  }
};


VolumeCommand ConfigManager::getCommand(uint16_t code) {
  uint8_t idx = 0;
  for (; idx < VolumeCommand::Unknown; idx++) {
    if (_configData.irCommands[idx] == code) {
      break;
    };
  }
  return (VolumeCommand) idx;
}


const char* ConfigManager::getVolumeCommandName(uint8_t idx) {
  return (
    idx == 0 ? "Up" :
    idx == 1 ? "Down" :
    idx == 2 ? "Mute" :
    idx == 3 ? "Left" :
    idx == 4 ? "Right" :
    idx == 5 ? "Preset" :
    "Unknown"
  );
}


void ConfigManager::logConfig() {
  Serial.printf("  %8s: %04lX\n", "Address", _configData.irAddress);
  for (uint8_t i = 0; i < NUMBER_OF_COMMANDS; i++) {
    Serial.printf("  %8s: %04X\n", getVolumeCommandName(i), _configData.irCommands[i]);
  }
  Serial.printf("  %8s: %03i\n", "Volume", _configData.audioSettings.volume);
  Serial.printf("  %8s: %03i\n", "Balance", _configData.audioSettings.balance);
  uint16_t checksum = _getChecksum((uint8_t *)&_configData, sizeof(_configData));
  Serial.printf("  %8s: %08lX\n", "Checksum", checksum);
}
