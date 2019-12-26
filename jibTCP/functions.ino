void settings(){ // this function changes some settings of TMC2130
	// slewing driver settings
	slew.begin(); // Initiate pins and registeries
	slew.high_sense_R(1); // reference voltage for coil current sense resistors	1 = 0.18V			 0 = 0.32V
	slew.hold_current(1); // 0-31 standstill current per motor coil
	slew.run_current(8); // 0-31,		 0 = 30 mA per coil,		31 = 980 mA per coil
	slew.power_down_delay(30); // how long to wait after movement stops before reducing to hold current 0-255 = 0-4 seconds
	slew.hold_delay(3); // 0-15 how gradually it reduces to hold current. 0=fast change. 15=slow change.
	slew.stealthChop(1); // Enable extremely quiet stepping
	slew.standstill_mode(2);
	slew.stealth_autoscale(1);
	slew.microsteps(0); // we dont want any
	slew.interpolate(1); // automatic 256 x microstepping
	slew.double_edge_step(1); // step on both rising and falling edges
	slew.chopper_mode(0); // 0=spreadCycle 1=constant off time

	// trolleying driver settings
	trolley.begin();
	trolley.high_sense_R(1);
	trolley.hold_current(1);
	trolley.run_current(8);
	trolley.power_down_delay(30);
	trolley.hold_delay(3);
	trolley.stealthChop(1);
	trolley.standstill_mode(2);
	trolley.stealth_autoscale(1);
	trolley.microsteps(0);
	trolley.interpolate(1);
	trolley.double_edge_step(1);
	trolley.chopper_mode(0);
	trolley.diag1_stall(1);
	trolley.sg_stall_value(15);

	// hoisting driver settings
	hook.begin();
	hook.high_sense_R(1);
	hook.hold_current(1);
	hook.run_current(20); // increase this to be able to lift heavier loads
	hook.power_down_delay(30);
	hook.hold_delay(3);
	hook.stealthChop(1);
	hook.stealth_autoscale(1);
	hook.standstill_mode(2);
	hook.microsteps(0);
	hook.interpolate(1);
	hook.double_edge_step(1);
	hook.chopper_mode(0);
	hook.diag1_stall(1);
	hook.sg_stall_value(0); // todo adjust this to match NEMA23 1.5A motor
}

void silentMode(){
	silent=1;
	Serial.println(F("Silent mode"));
	slew.stealth_max_speed(30); // switch stealthChop off if motor spins fast enough (meaning if time between two steps is less than this)
	trolley.stealth_max_speed(10); // todo find how small this can be
	hook.stealth_max_speed(10);
	trolley.coolstep_min_speed(0); // set to zero so that it can run silently at higher speeds too
	hook.coolstep_min_speed(0);
}

void fastMode(){
	silent=0;
	Serial.println(F("Fast mode"));
	slew.stealth_max_speed(10000);
	trolley.stealth_max_speed(10000);
	hook.stealth_max_speed(10000);
	trolley.coolstep_min_speed(200); // disable stallGuard when going too slow for it to work reliably
	hook.coolstep_min_speed(400);
}

// calculates new speed for motor and limits its acceleration
// call this function in every iteration of loop(), once for each motor
// parameter: motor (0 or 1 or 2)
void calcSpeed(byte motor){
	static unsigned long viime[3]={now,now,now};
	const byte tuokio=now-viime[motor];
	const byte increment=tuokio*acceleration[motor];
	if(increment>0){
		viime[motor]=now;
		if(spd[motor]<goal[motor]){
			if(increment<goal[motor]-spd[motor]) spd[motor]+=increment;
			else spd[motor]=goal[motor];
		}
		else if(spd[motor]>goal[motor]){
			if(increment<spd[motor]-goal[motor]) spd[motor]-=increment;
			else spd[motor]=goal[motor];
		}
	}
}

// changes motor speed
// parameter: motor (0 or 1 or 2)
void setSpeed(byte motor){
	if (spd[motor]==0){
		cli();
		motOn[motor]=0; // dont step the motor
		kid[motor]=0xFFFF00; // longest possible step period
		sei();
	}else{
		unsigned long newKid=16000000UL/abs(spd[motor]); // how many CPU cycles to wait between steps
		if(newKid>0xFFFF00) newKid=0xFFFF00; // why would we even try to step slower than this
		cli();
		motOn[motor]=1;
		kid[motor]=newKid;
		sei();
	}
}

// function to set timer1 period. Stolen from https://www.pjrc.com/teensy/td_libs_TimerOne.html
inline void fox(unsigned long cycles){
	/*if(homeSlew>0){
		if(cycles<2400) cycles=2400; //cos analogRead(A6) is slow
	}
	else */if(cycles<700) cycles=700; // minimum cycles, cuz ISR takes some time too
	byte clockSelectBits;
	word period;
	if (cycles < 0x10000) {
		clockSelectBits = _BV(CS10);
		period = cycles;
	} else
	if (cycles < 0x10000 * 8) {
		clockSelectBits = _BV(CS11);
		period = cycles / 8;
	} else
	if (cycles < 0x10000 * 64) {
		clockSelectBits = _BV(CS11) | _BV(CS10);
		period = cycles / 64;
	} else
	if (cycles < 0x10000 * 256) {
		clockSelectBits = _BV(CS12);
		period = cycles / 256;
	} else {
		clockSelectBits = _BV(CS12);
		period = 0xFFFF;
	}
	ICR1 = period;
	TCCR1B = _BV(WGM13) | _BV(WGM12) | clockSelectBits; // mode 12. p136
	//TCNT1 = 0; // reset counter. p115
}

void stopMotors(){
	goal[0]=0; goal[1]=0; goal[2]=0;
	spd[0]=0; spd[1]=0; spd[2]=0;
	setSpeed(0); setSpeed(1); setSpeed(2);
	homing=0; homeTrolley=0; homeSlew=0;
	posMax=2E9; posMin=-2E9; posTop=2E9;
	Serial.println(F("Stop motors"));
}

void larsonScanner(){
	static char pos = 0, dir = 1; // Position, direction of "eye"
 
	// Draw 5 pixels centered on pos.	setPixelColor() will clip any
	// pixels off the ends of the strip, we don't need to watch for that.
	if(serialActive){
		led.setPixelColor(pos - 2, 0x0000FF);
		led.setPixelColor(pos - 1, 0x00FFFF);
		led.setPixelColor(pos    , 0x00FF00);
		led.setPixelColor(pos + 1, 0xFFFF00);
		led.setPixelColor(pos + 2, 0xFF0000);
	}else if(ethernetConnected){
		led.setPixelColor(pos - 2, 0x001000);
		led.setPixelColor(pos - 1, 0x008000);
		led.setPixelColor(pos    , 0x00FF00); // Center pixel is brightest
		led.setPixelColor(pos + 1, 0x008000);
		led.setPixelColor(pos + 2, 0x001000);
	}else{
		led.setPixelColor(pos - 2, 0x100000); // Dark red
		led.setPixelColor(pos - 1, 0x800000); // Medium red
		led.setPixelColor(pos    , 0xFF0000); // Center pixel is brightest
		led.setPixelColor(pos + 1, 0x800000); // Medium red
		led.setPixelColor(pos + 2, 0x100000); // Dark red
	}
	led.show();
 
	// Rather than being sneaky and erasing just the tail pixel,
	// it's easier to erase it all and draw a new one next time.
	for(char j=-2; j<= 2; j++) led.setPixelColor(pos+j, 0);
 
	// Bounce off ends of strip
	pos += dir;
	if(pos < 0) {
		pos = 1;
		dir = -dir;
	} else if(pos >= led.numPixels()) {
		pos = led.numPixels() - 2;
		dir = -dir;
	}
}

// Unlike its Arduino counterpart, this does not wait that ADC is ready. It makes using analogRead() inside interrupt so much faster.
int analogRead(byte pin){
	if (pin >= 14) pin -= 14; // allow for channel or pin numbers
	if (pin>7) return -1;
	static int result[8]={0};
	static byte lastPin=0;
	if(bit_is_clear(ADCSRA, ADSC)){ // ADSC is cleared when the conversion finishes.
		result[lastPin] = ADCL | (ADCH << 8);
		ADMUX = (DEFAULT << 6) | (pin & 0x07);
		lastPin=pin;
		bitSet(ADCSRA, ADSC); // start the conversion
	}
	return result[pin];
}

// Read accelerations from memory
void readAccels(){
	Serial.print(F("Accels:"));
	word accel;
	for(byte i=0; i<3; i++){
		EEPROM.get(4+i*2,accel);
		acceleration[i]=accel/100.;
		Serial.print("  ");
		Serial.print(accel); // in units of 10 steps/(s^2)
	}
	Serial.println();
}

void setup() {
	DDRD |= 0b01110000; // step pins outputs
	Serial.begin(250000); // Set baud rate in serial monitor
	// let's enable timer1 to time the step pulses. See p113 here https://www.sparkfun.com/datasheets/Components/SMD/ATMega328.pdf
	TCCR1A=B00000000; //p134
	TIMSK1=B00100001; //p139
	fox(1000);
	pinMode(A3,INPUT_PULLUP); // slack detector
	pinMode(8,INPUT_PULLUP); // diag1 trolley
	pinMode(9,INPUT_PULLUP); // diag1 hook
	fastMode();
	led.begin();
	readAccels();

	// Ethernet stuff
	Ethernet.init(10); // Ethernet shield CS pin
	//EEPROM.update(0,3); // you can use this line to update unique IP address
	const byte myIP=EEPROM.read(0);
	if(myIP==171) led.updateLength(47); // because jib number one has more leds
	const byte mac[] = {0xA3, 0xAD, 0xBE, 0x16, 0x47, myIP}; // a3:ad:be:16:47:ab
	IPAddress ip(192,168,EEPROM.read(1),myIP); // 192.168.0.171
	Ethernet.begin(mac, ip);
	if(Ethernet.hardwareStatus()==EthernetNoHardware) Serial.println(F("Ethernet shield not found. :("));
	else if(Ethernet.linkStatus()==LinkOFF) Serial.println(F("Ethernet cable not connected. :("));
	Serial.print(F("My IP is "));
	Serial.println(Ethernet.localIP());
	server.begin();
	serverLoc.begin();
}

/*EEPROM map:
*bytes 0,1,2,3 reserved for IP-address
*4,5 slew acceleration
*6,7 trolley acceleration
*8,9 hook acceleration
*/
