#include <SoftwareSerial.h>
SoftwareSerial myserial(7, 8); //Define virtual serial port name as myseria,Rx is port 7, Tx is port 8
void setup()
{
  myserial.begin(115200); //Initialize virtual serial port
  Serial.begin(115200); //Initialize Arduino default serial port
}

void loop()
{
  while(1){
  while (myserial.available()) {
    char c = myserial.read();
    Serial.write(c);//if Serial received data, output it via mySerial.
    delay(1);
  }
  while(Serial.available()) {
    char c = Serial.read();
    myserial.write(c);//if myserial received data, output it via Serial.
    delay(1);
  }
}
}