# AutoSwitch
Auto switch to start a vac or dust collector when a tool starts.


Simple circuit based on the ATTiny85 PCU that monitors the current drawn by a tool, typically a wood working tool such as a saw or planer, and activates a dust collecter when that tool is switched on.

Current is senses with the ACS712 hall device and the ATTiny85 samples this and after a short delay fires the triac, via an opto isolator, to power on the dust collector.

A seperate relay provides a volt free set of contacts in case blast gates need controlling.

There is a serial port which allows tweaking of the trigger values and time delays.

