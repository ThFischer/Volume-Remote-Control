#include "Arduino.h"
#include "AppController.h"

#define IRMP_PROTOCOL_NAMES   1    // Activate mapping of protocoll names for debugging
#define IRMP_INPUT_PIN        5    // PD5 (ATMega328P pin 11)
#define IRMP_FEEDBACK_LED_PIN 6    // PD6 (ATMega328P pin 12) (red LED)
#define CONTROLLER_LED_PIN    7    // PD7 (ATMega328P pin 13) (green LED)



//#include <irmpSelectMain15Protocols.h>
#define IRMP_SUPPORT_NEC_PROTOCOL               1


#include <irmp.c.h>
IRMP_DATA irmp_data;


void AppController::init() {
  _volumeController.init();

  pinMode(IRMP_FEEDBACK_LED_PIN, OUTPUT);
  pinMode(CONTROLLER_LED_PIN, OUTPUT);
  digitalWrite(IRMP_FEEDBACK_LED_PIN, LOW);
  digitalWrite(CONTROLLER_LED_PIN, LOW);

  irmp_init();
  irmp_irsnd_LEDFeedback(false);

  Serial.print(F("Ready to receive IR signals of protocol(s): "));
  irmp_print_active_protocols(&Serial);
  Serial.println();

  _configManager.restore();
  if (!_configManager.hasValidConfig()) {
    // No valid configuration was loaded, learn a remote:
    do {
      learnRemote(0);
    } while (!_configManager.hasValidConfig());
  } else {
    // A valid configuration was loaded.
    // If in the first 5 sec an IR command is received we assume that a new remote should be learned:
    Serial.println("Time to learn a new remote?");
    digitalWrite(CONTROLLER_LED_PIN, HIGH);
    uint32_t end = millis() + 5000;
    while (millis() < end) {
      if (irmp_get_data(&irmp_data) && !(irmp_data.flags & IRMP_FLAG_REPETITION)) {
        Serial.println("Learn a new remote was triggered");
        learnRemote(irmp_data.command);
        break;
      }
    }
  }
  digitalWrite(CONTROLLER_LED_PIN, LOW);
  irmp_irsnd_LEDFeedback(true);
  run();
};

/**
 * @param ignoredCommand To ignore this command when waiting for the first IR command.
 *        It's a protection in case the IR remote stucking in the couch with pressed button
 */
bool AppController::learnRemote(uint16_t invalidFirstCommand) {
  Serial.println("Learn the remote!");
  irmp_irsnd_LEDFeedback(false);
  uint16_t irAddress = 0;
  bool isLearningComplete = false;
  irCommands_t irCommands;
  for (uint8_t idx = 0; idx < NUMBER_OF_COMMANDS; idx++) {
    Serial.printf("  %8s: ", _configManager.getVolumeCommandName(idx));
    bool flipflop = 0;
    while (
      !irmp_get_data(&irmp_data) ||
      (irmp_data.flags & IRMP_FLAG_REPETITION)
    ) {
      if (idx == 0) {
        // If it is the very first command then we let the LEDs blinking alternately.
        // Start with the CONTROLLER_LED_PIN, otherwise an extra delay would be necessary after:
        blink(flipflop ? IRMP_FEEDBACK_LED_PIN : CONTROLLER_LED_PIN, 200, 0);
        flipflop ^= 1;
      }
    };
    if (idx == 0 && irmp_data.command == invalidFirstCommand) {
      Serial.println("\nLearning interrupted: Invalid first command!");
      break;
    }
    // Assure that we receive allways the same IR address (signals from same remote).
    // If not turn on the red LED and stop the learning:
    if (!irAddress) {
      irAddress = irmp_data.address;
    } else if (irAddress != irmp_data.address) {
      blink(IRMP_FEEDBACK_LED_PIN, 200, 15);
      break;
    }
    irCommands[idx] = irmp_data.command;
    Serial.printf("%08lX\n", irCommands[idx]);
    if (idx == NUMBER_OF_COMMANDS - 1) {
      blink(CONTROLLER_LED_PIN, 333, 3);
      isLearningComplete = true;
    } else {
      blink(CONTROLLER_LED_PIN, 1000, 0);
    }
    // Some remotes send more then one command per button.
    // Therefore all additional received commands are discarded:
    while (irmp_get_data(&irmp_data));
  }
  if (isLearningComplete) {
    if (_configManager.hasValidConfig()) {
      // Update only the IR address and commands:
      isLearningComplete = _configManager.store(irAddress, irCommands);
    } else {
      isLearningComplete = _configManager.store(
        irAddress,
        irCommands,
        _volumeController.getVolume(),
        _volumeController.getBalance()
      );
    }
  }
  irmp_irsnd_LEDFeedback(true);
  return isLearningComplete;
}

void AppController::run() {
  uint32_t firstPreset = 0;
  uint32_t lastPreset = 0;
  bool isPresetStored = false;
  Serial.println("Controller is up and running!");
  digitalWrite(CONTROLLER_LED_PIN, LOW);
  irmp_irsnd_LEDFeedback(true);
  uint32_t lastTimestamp = 0;
  while (true) {
    uint32_t timestamp = millis();
    if (irmp_get_data(&irmp_data)) {
      // Prevent unintended long pressed buttons:
      if (timestamp - lastTimestamp > 130) {
        VolumeCommand volumeCommand = _configManager.getCommand(irmp_data.command);
        bool isRepetition = irmp_data.flags & IRMP_FLAG_REPETITION;

        Serial.println(_configManager.getVolumeCommandName(volumeCommand));
        switch (volumeCommand) {
          case VolumeCommand::Up: {
            _volumeController.volumeUp();
            break;
          }
          case VolumeCommand::Down: {
            _volumeController.volumeDown();
            break;
          }
          case VolumeCommand::Mute: {
            if (!isRepetition) {
              _volumeController.toggleMute();
            }
            break;
          }
          case VolumeCommand::Left: {
            _volumeController.balanceLeft();
            break;
          }
          case VolumeCommand::Right: {
            _volumeController.balanceRight();
            break;
          }
          case VolumeCommand::Preset: {
            if (!firstPreset) {
              firstPreset = timestamp;
              isPresetStored = false;
            }
            lastPreset = timestamp;
            break;
          }
          case VolumeCommand::Unknown: {
            // ignore unknown commands
          }
        }
        // printCommand(timestamp - lastTimestamp);
        lastTimestamp = timestamp;

        if (volumeCommand != VolumeCommand::Preset) {
          firstPreset = 0;
          lastPreset = 0;
          isPresetStored = false;
        }
      }
    } else {
      // Handle preset commands delayed:
      if (firstPreset) {
        uint32_t duration = lastPreset - firstPreset;
        if (duration < 1000L) {
          if (timestamp - lastPreset > 500L) {
            Serial.println("GET PRESET\n");
            Serial.printf("F "); Serial.println(firstPreset);
            Serial.printf("L "); Serial.println(lastPreset);
            Serial.printf("D "); Serial.println(duration);
            Serial.printf("P "); Serial.println(timestamp - lastPreset);
            audioSettings_t preset;
            _configManager.getPreset(&preset);
            _volumeController.preset(preset.volume, preset.balance, false);
            firstPreset = 0;
            lastPreset = 0;
          }
        } else {
          // Preset button was pressed for more then 3 seconds?
          if (duration > 3000L && !isPresetStored) {
            Serial.printf("SET PRESET\n", firstPreset);
            Serial.printf("  %0i5 firstPreset\n", firstPreset);
            Serial.printf("  %0i5 lastPreset\n", lastPreset);
            Serial.printf("  %0i5 timestamp\n", timestamp);
            _configManager.setPreset(_volumeController.getVolume(), _volumeController.getBalance());
            isPresetStored = true;
            blink(CONTROLLER_LED_PIN, 666, 0);
          }
        }
      }
    }
  }
}

/**
 * Before calling this method assure that the IRMP LED feedback is deactivated (see irmp_irsnd_LEDFeedback)!
 * If the IRMP LED should be activated, the deactivation must happen around 200ms before!
 */
void AppController::blink(uint8_t ledPin, uint32_t period, uint16_t recurrences) {
  digitalWrite(ledPin == IRMP_FEEDBACK_LED_PIN ? CONTROLLER_LED_PIN : IRMP_FEEDBACK_LED_PIN, LOW);
  uint16_t loops = recurrences * 2;
  for (uint16_t cycle = 0; cycle <= loops; cycle++) {
    digitalWrite(ledPin, cycle % 2 ? LOW : HIGH);
    delay(period);
  }
  digitalWrite(ledPin, LOW);
}

void AppController::printCommand(uint32_t time) {
  // A single printf call for all values showed only gibberish.
  // I guess because sending was interrupted by IRMP interrupts.
  // Therefor single print commands are used!
  Serial.print(" PRT:");
  irmp_print_protocol_name(&Serial, irmp_data.protocol);
  Serial.print(" CMD:");
  Serial.printf("%0X4", irmp_data.command);
  Serial.print(" REP:");
  Serial.print(irmp_data.flags & IRMP_FLAG_REPETITION ? "Y" : "N");
  Serial.print(" REL:");
  Serial.print(irmp_data.flags & IRMP_FLAG_RELEASE ? "Y" : "N");
  Serial.print(" ADR:");
  Serial.printf("%0X4", irmp_data.address);
  Serial.print("  ");
  Serial.print(time);
  Serial.println(" ms");
}
