# EventFramework #

## Overview ##

This is a facility intended to provide an event framework for
Arduino sketches. In other words wait for or monitor when things happen
like a GPIO input change or timer elapsed and execute a function
bound to the 'event.'

## Requirements ##

Arduino IDE (to build for Arduino)
Eclipse (to build test on PC)

## Procedure ##

Install in a location suitable for Arduino libraries. (details TBD) there
is a test project that can be executed on the Arduino UNO.

For testing on a PC, open the project in Eclipse and run. It would be very 
cool if a standard test framework were employed but that is not the case. Sad!

## Examples ##

Used in MusicGame (not yet in Github)

## Errata ##

This is a bit of a git mish-mash. The original project was in
EventFramework and had a .cpp extension.  In order to compile with the
Arduino tool chain, you have to use a .ino extension. But the Arduino
tool chain will happily include any .cpp and .h files in the directory
that holds the .ino file. And it throws up in its mouth on directories
that include a '-' so to avoid both problems, the cpp file is sym linked
to the ino file (e.g. ../TEV/TEV.ino. Probably some naming policies in
the Arduino dev kit also working there.
