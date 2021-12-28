# AutoSwitch

Auto switch to start a vac or dust collector when a tool starts.

Simple circuit based on the ATTiny85 PCU that monitors the current drawn by a tool, typically a wood working tool such as a saw or planer, and activates a dust collecter when that tool is switched on.

Current is sensed with the ACS712 hall device and the ATTiny85 samples this and after a short delay fires the triac, via an opto isolator, to power on the dust collector.

A seperate relay provides a volt free set of contacts in case blast gates need controlling.

There is a serial port which allows tweaking of the trigger values and time delays.

## Operation

The AtTiny85 uses it's onboard DAC to sample the analogue output from the hall sensor. If this value is above the adcThreshold value, a counter is incremented on each poll, up to the max counter value. If the hall value is below this threshold the counter is decremented. When the counter exceed the on count value and the hall value is above the threadhold, the output triac is switched on. When the count drops below the off count value and the hall value is below the threashold, the triac is switched off. To give a clean shutoff the count value is also set to zero at this point.

## Serial interface

A vac switch doesn't need a seial interface, but I was bored and it got one!

Use the default serial settings: 9600 baud, Data 8, Stop 1, no parity and Xon/Xoff.

A list of commands should be displayed.

- get          : get all config values
- current [n]  : get or set the tool current threshold value
- on [n]       : get or set the on trigger value
- off [n]      : get or set the off trigger value
- count [n]    : get or set the max count value
- period [n]   : get or set the poll period (ms)
- def          : set the default values
- cal          : auto set the current threshold
- watch        : watch the current system values (x to stop)
- fon|foff     : force switch on/off

These allow the internal values to be modified to tweak the behaviour. Values are store in the internal EEPROM.

The **on** value must always be greater than zero and less than the off value.

The **off** value must always be greater that the on value and less than the count max value.

The **count** value must always be greater than the off vlaue.
