//Keyboard control for crane through TCP:
//  Arrow keys = slewing and trolleying
//  A and Z = hoisting
//  H = home motor positions
//  F = fast mode
//  S = silent mode
//  Space = emergency stop

import processing.net.*;

Client c;
Client cr;
PFont f;
boolean TCP=true;
void setup() {
  size(300, 200);
  f = createFont("Consolas",16,true);
  println("hello");
  try{
    c = new Client(this,"192.168.0.174", 10000); // crane IP and port for control
    cr = new Client(this,"192.168.0.174", 10001); // port to read motor positions
  }catch(Exception e){
    TCP=false;
    println("Can not connect");
  }
}

String data;
int sendTime=0, receiveTime=0;
byte slew=0, trolley=0, hook=0, settings=0;
byte[] speeds=new byte[3];
void draw() {
  int now = millis();
  if(now-sendTime>50){
    background(0);
    textFont(f);
    fill(255);
    text("hi",10,50);
    sendTime=now;
    speeds[0]=slew;
    speeds[1]=trolley;
    speeds[2]=hook;
    if(TCP) c.write(speeds);
  }
  if(TCP){
    if(now-receiveTime>200){
      receiveTime=now;
      cr.write(1); // send something to crane so it sends position data back
    }
    if (cr.available() > 0) { // print position data
      data = cr.readString();
      println(data);
    }
  }
}

// keyboard control for crane
void keyPressed() {
  if (keyCode == ' '){ // emergency stop
    settings = 4;
    if(TCP) c.write(settings);
    settings = 0;
  }
  else{ // movement control
    if (keyCode == UP) trolley=-126;
    else if (keyCode == DOWN) trolley=126;
    if (keyCode == LEFT) slew=126;
    else if (keyCode == RIGHT) slew=-126;
    if (keyCode == 'A') hook=126;
    else if (keyCode == 'Z') hook=-126;
  }
}

void keyReleased() { // stop movement
  if (keyCode == UP || keyCode == DOWN) trolley=0;
  if (keyCode == LEFT || keyCode == RIGHT) slew=0;
  if (keyCode == 'A' || keyCode == 'Z') hook=0;
  if (keyCode == 'H') {
    settings = 2;
    if(TCP) c.write(settings);
    settings = 0;
  }
  else if (keyCode == 'S') {
    settings = 1;
    if(TCP) c.write(settings);
  }
  else if (keyCode == 'F') {
    settings = 0;
    if(TCP) c.write(settings);
  }
}
