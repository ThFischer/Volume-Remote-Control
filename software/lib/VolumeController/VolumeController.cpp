#include "Arduino.h"
#include <Wire.h>

#include "VolumeController.h"

// See https://github.com/MCUdude/MiniCore/issues/172
#undef round

// The data sheet specifies 0x88 but Wire uses 7 bit addresses, therefore 0x88 is shifted by bit (0x44)
#define PT2257_ADDR  0x44
#define PT2257_LEFT  0b10000000
#define PT2257_RIGHT 0b00000000
#define PT2257_1DB   0b00100000
#define PT2257_10DB  0b00110000
#define PT2257_MUTE  0b01111000

#define VOLUME_MIN         0
#define VOLUME_MAX        79
#define VOLUME_STEP_SIZE  3
#define BALANCE_MIN      -20
#define BALANCE_MAX      +20

// Change it in case your balance shifts into the opposite direction:
#define SWAP_LEFT_RIGHT true

void VolumeController::init() {
  Wire.begin();
  preset(
    VOLUME_MIN + (VOLUME_MAX - VOLUME_MIN) / 4,
    (BALANCE_MIN + BALANCE_MAX) / 2,
    false
  );
};

void VolumeController::preset(uint8_t volume, int8_t balance, bool muted) {
  _muted = muted;
  _setBalance(balance);
  _setVolume(volume);
  _updatePT2257();
};

bool VolumeController::volumeUp() {
  // Recognizable volume change is 3dB:
  const bool changed = _setVolume((int16_t) _volume + VOLUME_STEP_SIZE);
  if (changed) _updatePT2257();
  return changed;
};

bool VolumeController::volumeDown() {
  // Since 79 / 3 is not an integer the steps are [0, 3, 6, ... 75, 78, 79]:
  const bool changed = _setVolume((int16_t) _volume == VOLUME_MAX ? VOLUME_MAX -1 : _volume - VOLUME_STEP_SIZE);
  if (changed) _updatePT2257();
  return changed;
};

bool VolumeController::balanceRight() {
  const bool changed = _setBalance((int16_t) _balance + 1);
  if (changed) _updatePT2257();
  return changed;
};

bool VolumeController::balanceLeft() {
  const bool changed = _setBalance((int16_t) _balance - 1);
  if (changed) _updatePT2257();
  return changed;
};

bool VolumeController::toggleMute() {
  _muted ^= 1;
  _updatePT2257();
  return _muted;
};

uint8_t VolumeController::getVolume() {
  return _volume;
};

int8_t VolumeController::getBalance() {
  return _balance;
};

bool VolumeController::getMute() {
  return _muted;
};

bool VolumeController::_setVolume(int16_t volume) {
  volume = _clamp(volume, VOLUME_MIN, VOLUME_MAX);
  if (volume != _volume) {
    _volume = volume;
    return true;
  }
  return false;
}

bool VolumeController::_setBalance(int16_t balance) {
  balance = _clamp(balance, BALANCE_MIN, BALANCE_MAX);
  if (balance != _balance) {
    _balance = balance;
    return true;
  }
  return false;
}

int16_t VolumeController::_clamp(int16_t value, int16_t min, int16_t max) {
  return value < min ? min : value > max ? max : value;
}

void VolumeController::_updatePT2257() {
  Serial.printf(
    "\n\nApply volume: %i, balance: %+d, mute: %s\n",
    _volume, _balance, _muted ? "true" : "false"
  );

  // Set volume for left and right channel separately:
  for (uint8_t i = 0; i < 2; i++) {
    // Compute the balance adjusted volume of the channel (i == 0: left, i == 1: right)
    int16_t volume = 0;
    if (!_muted) {
      volume = _clamp(
        _volume +
          round((float) _volume * _balance / (i ? BALANCE_MAX : BALANCE_MIN)) *
          (SWAP_LEFT_RIGHT ? -1 : +1),
        VOLUME_MIN, VOLUME_MAX
      );
    }
    // A volume value of 0 correspond to -79dB, volume value of 79 correspond to -0dB:
    uint8_t decibel = VOLUME_MAX - volume;
    uint8_t channel = i == 0 ? PT2257_LEFT : PT2257_RIGHT;
    Serial.printf("  %s: -%i dB\n", i ? "R" : "L", decibel);
    Serial.print("    x10: ");
    Serial.println(channel | PT2257_10DB | (decibel / 10), BIN);
    Serial.print("     x1: ");
    Serial.println(channel | PT2257_1DB | (decibel % 10), BIN);
    Wire.beginTransmission(PT2257_ADDR);
    // The most significant digit (e.g. 42 becomes 4):
    Wire.write(channel | PT2257_10DB | (decibel / 10));
    // The least significant decibel digit (eg. 42 becomes 2)::
    Wire.write(channel | PT2257_1DB | (decibel % 10));
    const uint8_t error = Wire.endTransmission();
    if (error) {
      Serial.printf("I2C error %i\n", error);
    };
  }
}
