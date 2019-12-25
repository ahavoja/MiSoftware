void loop() {
	now=millis();
	
	// slew acceleration
	calcSpeed(0);
	static bool newDir0=0;
	if(spd[0]>0) newDir0=1; else
	if(spd[0]<0) newDir0=0;
	if (newDir0!=dir[0]){
		slew.shaft_dir(newDir0);
		dir[0]=newDir0;
	}
	setSpeed(0);

	// trolley acceleration
	calcSpeed(1);
	cli();
	const long posTrolley=pos[1];
	sei();
	if(posTrolley>=posMax && spd[1]>0 || posTrolley<=posMin && spd[1]<0) spd[1]=0;
	static bool newDir1=0;
	if(spd[1]>0) newDir1=1; else
	if(spd[1]<0) newDir1=0;
	if (newDir1!=dir[1]){
		trolley.shaft_dir(newDir1);
		dir[1]=newDir1;
	}
	setSpeed(1);

	// hook acceleration
	calcSpeed(2);
	cli();
	const long posHook=pos[2];
	sei();
	if(posHook>=posTop && spd[2]>0) spd[2]=0;
	static bool newDir2=0;
	if(spd[2]>0) newDir2=1; else
	if(spd[2]<0) newDir2=0;
	if (newDir2!=dir[2]){
		hook.shaft_dir(newDir2);
		dir[2]=newDir2;
	}
	if(newDir2==0 && (PINC&8)==0){ // slack detection
		spd[2]=0;
		goal[2]=0;
	}
	setSpeed(2);

	// Ethernet stuff
	static unsigned long timeReceived=0;
	if(ethernetConnected){
		if(serialActive){
			ethernetConnected=0;
			Serial.println(F("Serial active --> Ethernet closed. :)"));
		}
		if(Ethernet.linkStatus()==LinkOFF){
			Serial.println(F("Ethernet cable unplugged. :("));
			ethernetConnected=0;
		}
	}
	else if(!serialActive && Ethernet.linkStatus()==LinkON){
		Serial.println(F("Ethernet activated. :)"));
		ethernetConnected=1;
	}
	if(ethernetConnected){
		// receive commands from PC through Ethernet
		client = server.available();
		if(client.available()>=3){ // 3 speed commands received
			//Serial.print("Received");
			timeReceived=now;
			for(byte zoo=0; zoo<3; zoo++){
				const char buff=client.read();
				if(homing==0) goal[zoo]=buff; // set new speed goals for motors
				//Serial.write(' ');
				//Serial.print(goal[zoo],DEC);
			}
			//Serial.println();
		}

		// receive settings
		else if(client.available()>=1){
			const char wax=client.read();
			Serial.print("Settings ");
			Serial.println(wax,BIN);
			if(wax == 4){
				stopMotors();
			}else{
				if(wax == 2) homing=1;
				else{
					if(wax == 1) silentMode();
					else fastMode();
				}
			}
		}

		// Send motor positions to PC
		clientLoc = serverLoc.available();
		if(clientLoc.available()){
			while(clientLoc.available()) clientLoc.read(); // empty buffer
			//Serial.println("Transmitting");
			long positron[3];
			for(byte i=0; i<3; i++){ // copy motor positions to buffer
				cli();
				positron[i]=pos[i];
				sei();
			}
			//float beef = positron[0]/200.0/(103/121+26)/53*9; // gear ratio for slew
			//float beef = positron[0]*3.162075E-5; // slew in revolutions
			float beef = positron[0]*1.9867909E-4; // slew in radians
			message = String(beef,3);
			message += ';';
			beef = 668-positron[1]*0.2; // trolley position from tower centerline mm
			message += String(beef,0);
			message += ';';
			beef = positron[2]*0.337075; // hook position from trolley bottom mm
			message += String(beef,0);
			clientLoc.print(message+"\n");
		}
	}

	/*// receive commands from PC through USB
	static unsigned long timeSerial=0;
	if(Serial.available()){
		timeReceived = now;
		timeSerial = now;
		static byte job=255;
		const char wax=Serial.read();
		if(wax==127 && homing==0) job=0; // speed packet start character is 127
		else if(wax==-127) job=4; // -127 indicates that next byte will be settings
		else if(job<3){ // or else it must be a speed command -126 to 126
			if(job==0) goal[0]=wax; else
			if(job==1) goal[1]=wax; else
			if(job==2) goal[2]=wax;
			++job;
		}
		else if(job==4){ // decode settings byte
			if(wax & 4){
				stopMotors();
			}else{
				if(wax & 2) homing=1;
				else{
					if(wax & 1) silentMode();
					else fastMode();
				}
			}
			++job;
		}
	}*/

	static unsigned long timeSerial=0;
	if(Serial.available()){
		timeReceived = now;
		timeSerial = now;
		static byte job=255;
		const long wax=Serial.parseInt();
		goal[0]=wax;
		while(Serial.available()) Serial.read();
	}

	// disable larson scanner whenever motors turn
	static bool serialOld=serialActive;
	serialActive = now-timeSerial<1000?1:0;
	static unsigned long ant=0;
	if((serialActive>serialOld) || (spd[0]==0 && spd[1]==0 && spd[2]==0 && now-ant>40)){
		ant=now;
		larsonScanner();
		serialOld=serialActive;
	}

	/*// stop motors if no speed commands are received
	if(now - timeReceived > 1000){
		goal[0]=0; goal[1]=0; goal[2]=0;
	}*/
	
	if(homing>0) home();
	
	static unsigned long owl=0;
	if(now-owl>1000){
		owl=now;
		printDebug();
	}
}
