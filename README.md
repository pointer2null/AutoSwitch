# AutoSwitch
Auto switch to control vac when a tool starts

Small program for the ATTiny85.

It reads the value of the AC712 current sensor and uses that the control the power to an ancillery device - normally a vaccum for dust collection.

There is a delay before turn on, to prevent surges if both devices power on at the same time, and a small run on to clear any dust after the main too stops.

I've done this in the simplist way possible - without timers or sample windows.

Schematic etc to follow

There is a seial debug output and I'll probably complicate the whole design by allowing serial modification of default params lol.
