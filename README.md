proton_pack_arduino
===================

This project contains (or will contain) all of the circuit diagrams and code used for the lighting and audio effects in my proton pack replica.

The lighting patterns are largely controlled by an Arduino UNO (v.3) driving two pairs of shift registers (74HC595N). One pair controls the 'power cell' lighting pattern and the other controls the thrower bargraph and has two LED's dedicated to a firing strobe. The circuits are effectively identical to those in the Arduino 'Shiftout' example: http://www.arduino.cc/en/Tutorial/ShiftOut

The cyclotron lights are controlled by a simple 4017 decade counter and a 555 timer. However, it would be straightforward to have the speed controlled by the arduino.

Sound effects come from an Adafruit waveshield:
https://www.adafruit.com/products/94

A 6W TDA1517 amplifier attached to a small, 8cm diameter speaker outputs the sound effects at a reasonable volume.


All of my circuits are built on stripboard, I'll include diagrams if you want to replicate the circuits in this way.
