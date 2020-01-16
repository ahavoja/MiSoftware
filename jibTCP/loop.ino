void loop() {
	now=millis();
	
	// slew acceleration
	calcSpeed(0);
	static bool newDir0=0;
	if(spd[0]>0) newDir0=1; else
	if(spd[0]<0) newDir0=0;
	if(newDir0!=dir[0]){
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
	if(spd[1]>0){
		newDir1=1;
		spd[1]=min(decelerationTrol*sqrt(posMax-posTrolley)+3,spd[1]);
	}
	else if(spd[1]<0){
		newDir1=0;
		spd[1]=-min(decelerationTrol*sqrt(posTrolley-posMin)+3,-spd[1]);
	}
	if(newDir1!=dir[1]){
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
	if(spd[2]<0 && (PINC&8)==0){ // slack detection
		hookHitGround=1;
		spd[2]=0;
	}
	static bool newDir2=0;
	if(spd[2]>0){
		newDir2=1;
		spd[2]=min(decelerationHook*sqrt(posTop-posHook)+3,spd[2]);
	}
	else if(spd[2]<0) newDir2=0;
	if(newDir2!=dir[2]){
		hook.shaft_dir(newDir2);
		dir[2]=newDir2;
	}
	setSpeed(2);

	// Ethernet stuff
	if(ethernetConnected){
		if(serialActive){
			ethernetConnected=0;
			Serial.println(F("Serial active --> Ethernet closed"));
		}
		if(Ethernet.linkStatus()==LinkOFF){
			ethernetConnected=0;
			Serial.println(F("Ethernet cable unplugged"));
		}
		if(!client.connected()){
			static unsigned long connTime=now;
			if(now-connTime>200){
				connTime=now;
				//Serial.println(F("Connecting to 192.168.10.50"));
				const byte serverIP[]={192,168,10,50};
				if(client.connect(serverIP,3232)){
					Serial.write('C'); // connected
					client.print(1);
				}else Serial.write('F'); // failed to connect
			}
		}
	}
	else if(!serialActive && Ethernet.linkStatus()==LinkON){
		if(ethernetBegun==0){
			Serial.println(F("Connecting DHCP..."));
			if(Ethernet.begin(mac)) ethernetBegun=1;
			else Serial.println(F("DHCP fail"));
		}
		if(ethernetBegun){
			ethernetConnected=1;
			Serial.print(F("My IP is "));
			Serial.println(Ethernet.localIP());
			Serial.print(F("My MAC is"));
			for(byte x=0; x<6; x++){
				Serial.print(" ");
				Serial.print(mac[x],HEX);
			}
			Serial.println();
		}
	}
	if(ethernetConnected){
		// receive commands from PC through Ethernet
		if(client.available()){
			byte in=client.read();
			interpretByte(in);
		}
	}

	// receive commands from PC through USB
	static unsigned long timeSerial=0;
	if(Serial.available()){
		timeSerial = now;
		interpretByte(Serial.read());
	}

	// disable larson scanner whenever motors turn
	serialActive = now-timeSerial<1000?1:0;
	static unsigned long ant=0;
	if(spd[0]==0 && spd[1]==0 && spd[2]==0){
		static bool lightOld=0;
		if(light){
			if(lightOld==0){
				led.fill(0xFFFFFF);
				led.show();
			}
		}else{
			if(lightOld==1){
				led.fill(0x000000);
				led.show();
			}
			if(now-ant>100){
				ant=now;
				larsonScanner();
			}
		}
		lightOld=light;
	}

	// stop motors if no speed commands are received
	if(now - timeReceived > 1000){
		receptionActive=0;
		goal[0]=0; goal[1]=0; goal[2]=0;
	}else receptionActive=1;
	
	if(homing>0) home();
	
	static unsigned long owl=0;
	if(now-owl>1000){
		owl=now;
		printDebug();
	}
}
