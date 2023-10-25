# cx16_vera_recover
A nice way to recover your Commander X16 VERA using an Arduino Nano ESP32

With this code I could recover my own CX16 VERA using an Arduino Nano ESP32.
I don't wish anyone to brick their VERA but with this tool you can recover.

The process how to do this will be documented here.

I've built 2 version of it.
- A synchronous web server implementation using Arduino IDE 2.
- An asynchronous web server implementation using PlatformIO. Reason of platform IO is the necessity to be abe to drop index.html and style.css files (maybe later more) as part of the project. And PlatformIO has built-in facilities to do exactly that.

# WORK IN PROGRESS #
