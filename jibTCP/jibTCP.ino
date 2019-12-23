// This code reads commands from serial port (USB wire) or W5500 ethernet shield and controls 3 stepper motors via TMC2130 drivers.
// This code runs on atmega 328p microcontroller (Arduino Uno or Nano)
/* todo:
 * adjustable acceleration
 * 16 bit speed commands
 * smooth transition from slow mode to fast mode
 * user shouldnt be able to switch to silent mode during homing
 * real acceleration setting instead of acl
 * jerk limit? increase acceleration resolution?
 * make trolley slow down before edges to limit sway
 * adjust sg_stall_value based on input voltage
 * POWER & TORQUE:
	 * adjust setCurrent, power_down_delay, microsteps etc.
	 * enable coolStep for power savings and less heating
	 * high torque mode for heavy lifting (and homing?), low torque for power savings
	 * use stallGuard value to limit speed to prevent motors stalling
 * ISR is not sending step pulses perfectly evenly when spinning many motors at the same time
*/

// TMC2130 pin connections
	/* You need to connect the SPI pins as follows for programming the TMC2130. If you have several TMC2130, they all must use these same pins.
		SDI --> D11
		SDO --> D12
		SCK --> D13
	En --> GND // enable (CFG6). I want driver always enabled, so connect EN --> GND
	Dir --> GND // direction can also be controlled through SPI, so to save pins, connect DIR --> GND
	Step (slew) --> D4 
	Step (trolley) --> D5
	Step (hook) --> D6
	And on top of that you need to also connect motor coils
		M1A and M1B to one coil and
		M2A and M2B to another coil
	Finally connect the power wires
		GND --> GND
		VIO --> 5V
		VM --> motor power supply (5 - 45 V) and > 100 µF capacitor */

// a motor can never spin too fast, right?
#pragma GCC optimize ("-O2") // https://www.instructables.com/id/Arduino-IDE-16x-compiler-optimisations-faster-code/

// This code uses libraries. These can be easily installed through Arduino IDE library manager by pressing CTRL + SHIFT + I
#include <TMC2130Stepper.h> // https://github.com/teemuatlut/TMC2130Stepper

// choose chip select pins for each stepper driver
TMC2130Stepper slew = TMC2130Stepper(A0);
TMC2130Stepper trolley = TMC2130Stepper(A1);
TMC2130Stepper hook = TMC2130Stepper(A2);

// led library
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel led(23, A4, NEO_GRB + NEO_KHZ800); // led count, led pin

#include <EEPROM.h> // for storing unique IP address for each arduino

#include <Ethernet.h>
EthernetServer server(10000); // port for controlling motor movement
EthernetServer serverLoc(10001); // port for reading motor positions
EthernetClient client;
EthernetClient clientLoc;

// global variables
volatile unsigned long
	kid[3]={0xFFFF00,0xFFFF00,0xFFFF00}, // CPU cycles to wait between steps for each motor
	boy[3]={0xFFFF00,0xFFFF00,0xFFFF00}; // CPU cycles left until the motor needs to be stepped again
volatile bool motOn[3]={0,0,0}; // which motors are spinning
volatile bool dir[3]={0,0,0}; // slew, trolley, hook direction
volatile long
	pos[3]={0,0,0}, // motor step positions
	posMax=2E9, posMin=-2E9, posTop=2E9;
volatile byte homing=0, homeSlew=0, homeTrolley=0;
unsigned long fast[3]; // motor max speeds
char spd[3]={0,0,0}, goal[3]={0,0,0};
bool ethernetConnected=0, serialActive=0;
String message;
unsigned long now; // current time in loop()
float acceleration[3]={0.01,0.02,0.03}; // slew, trolley, hook acceleration limits
