# Laser Cutter firmware for Arduino

## Installation
Either compile it yourself or use the appropriate [release](https://github.com/SuperMonkeyRules/LaserCutter/releases/latest).

## Usage
It should accept commands from most GCODE senders so long as they are on the right port and serial baud (9600).

Or use: https://github.com/SuperMonkeyRules/WebCutter

## Configuration - Pins
The following pins are used as default pins in the code:

- **XmotorPUL**: GPIO pin 15
- **XmotorDIR**: GPIO pin 14
- **XmotorENA**: GPIO pin 13
- **YmotorPUL**: GPIO pin 16
- **YmotorDIR**: GPIO pin 17
- **YmotorENA**: GPIO pin 18
- **LaserCtrl**: GPIO pin 22 (Not set yet)

## Available commands
- **G0, G1**: Linear movement command.
  - Parameters: X, Y (mandatory)
    - X: X-axis position
    - Y: Y-axis position
  - Optional parameters:
    - S: Set brightness
    - F: Set feedrate
    - Z: Laser toggle
- **G2, G3**: Arc movement command.
  - Parameters: X, Y (mandatory), I, J (mandatory for G2/G3)
    - X: X-axis position
    - Y: Y-axis position
    - I: X-axis offset for arc center
    - J: Y-axis offset for arc center
  - Optional parameters:
    - S: Set brightness
    - F: Set feedrate
    - Z: Laser toggle
  - Additional information:
    - G2: Clockwise arc
    - G3: Counter-clockwise arc
- **G21**: Set units to millimeters (not implemented)
- **G28**: Homing command.
  - Additional information:
    - Moves to the home position (0, 0)
- **M0**: Stop command.
  - Optional parameters:
    - P: Delay time
- **M3**: Spindle on (laser on).
  - Optional parameters:
    - S: Set brightness
- **M5**: Spindle off (laser off).
  - Optional parameters:
    - S: Set brightness
- **M8**: Air assist on.
- **M9**: Air assist off.
- **M17**: Enable stepper motor outputs.
- **M18**: Disable stepper motor outputs.
- **M84**: Disable stepper motor outputs.
- **M92**: Set steps per unit (microstepping).
  - Parameters: X, Y
    - X: X-axis steps per unit
    - Y: Y-axis steps per unit
- **M114**: Get current position.
  - Additional information:
    - Returns the current position in millimeters.
- **M115**: Get firmware version.
  - Additional information:
    - Returns the firmware version.

## Credits
This code utilizes the following libraries:

- [Arduino](https://www.arduino.cc/)
- [AccelStepper](https://www.airspayce.com/mikem/arduino/AccelStepper/)
- [Keypad](https://playground.arduino.cc/Code/Keypad/)