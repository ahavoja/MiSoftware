import processing.net.*;

Client c;
String data;

void setup() {
  size(200, 200);
  background(50);
  fill(200);
  c = new Client(this,"10.1.1.1", 80); // Connect to server on port 80
}

int then = 0;
void draw() {
  int now = millis();
  if(now-then>1000){
    then=now;
    c.write("moi");
  }
  if (c.available() > 0) { // If there's incoming data from the client...
    data = c.readString(); // ...then grab it and print it
    print(data);
  }
}
