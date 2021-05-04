/*
The Volume Remote Controller receives commands via infrared and utilizes
a PT2257 to control the volume of an stereo line signal

Wiring µC ATMega328P:
 1 reset
 3 TXD
 7 VCC
 8 GND
11 IR receiver          (PD5)
12 IR Status LED        (PD6)
13 Stystem Status LED   (PD7)



Pinout IR receiver:
       ___
      /   \
  +---     ---+
  |  1  2  3  |
  +-----------+
     |  |  |

    1 Out
    2 Gnd
    3 VCC

    Anschluß eines IR-Empfängers an µC
    https://www.mikrocontroller.net/articles/IRMP


  Apple Remote White
  Up:     EF0B3660
  Down:   65E47B04
  Mute:   F80B4AFC
  Left:   99948518
  Right:  E7414F9A
  Preset: A87896B8



*/

#include <Arduino.h>

#include <AppController.h>

#define VERSION F("1.0")

int main(void) {
  init(); // Arduino

  Serial.begin(9600);
  Serial.print(F("~\nVolume Remote Control v"));
  Serial.println(VERSION);

  AppController appController;
  appController.init();

	return 0;
}

