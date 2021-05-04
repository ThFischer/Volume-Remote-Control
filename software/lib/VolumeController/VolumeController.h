/*
  Config.h - Library for read/write configuration into EEPROM
*/
#ifndef Volume_Controller_h
#define Volume_Controller_h

#include "Arduino.h"

#define DEFAULT_VOLUME = 8;
#define DEFAULT_BALANCE = 0;

class VolumeController {

  public:

    /**
     * To initialize the VolumeController
     */
    void init();

    /**
     * To apply presets for volume, balance and muted
     */
    void preset(uint8_t volumeIdx, int8_t balance, bool isMuted);

    /**
     * To increase the volume by one 3 dB
     * @return Indicates whether the volume was increased (true) or the maximum level was already set (false)
     */
    bool volumeUp();

    /**
     * To decrease the volume by 3 dB
     * @return Indicates whether the volume was decreased (true) or the minimum level was already set (false)
     */
    bool volumeDown();

    /**
     * To shift the balance 1 level to the right
     * @return Indicates whether the balance was shifted (true) or the most right value was already set (false)
     */
    bool balanceRight();

    /**
     * To shift the balance 1 level to the left
     * @return Indicates whether the balance was shifted (true) or the most left value was already set (false)
     */
    bool balanceLeft();

    /**
     * To toggle the mute state on or off
     * @return indicates if the mute state is on
     */
    bool toggleMute();

    /**
     * To obtain the current volume value.
     * @return volume in range [0, 79] (-79dB ... 0dB)
     */
    uint8_t getVolume();

    /**
     * To obtain the current balance value.
     * @return volume in range [-20, +20] (full left ... full right)
     */
    int8_t getBalance();

    /**
     * To obtain the current mute state.
     * @return true if muted
     */
    bool getMute();

  private:
    uint8_t _volume;
    int8_t _balance;
    bool _muted;
    bool _setVolume(int16_t volume);
    bool _setBalance(int16_t balance);
    int16_t _clamp(int16_t value, int16_t min, int16_t max);
    void _updatePT2257();
};

#endif