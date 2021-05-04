#ifndef IR_Controller_h
#define IR_Controller_h

#include "Arduino.h"
#include "ConfigManager.h"
#include "VolumeController.h"


class AppController {
  public:
    void init();
    void waitForIRCommand();
    bool learnRemote(uint16_t invalidFirstCommand);
    void run();
    void blink(uint8_t ledPin, uint32_t period, uint16_t recurrences);
  private:
    void printCommand(uint32_t time);
    ConfigManager _configManager;
    VolumeController _volumeController;
};

#endif