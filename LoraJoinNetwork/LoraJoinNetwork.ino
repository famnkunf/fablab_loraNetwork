/* 
AT command documentation:
    https://docs.rakwireless.com/RUI3/Serial-Operating-Modes/AT-Command-Manual/#content
*/

HardwareSerial mySerial1(1);
// Rx and Tx pin of esp32
int rxPin = 20;
int txPin = 21;


int lastsend = 0; //Last send time
int lastjoin = 0; //Last join time
char out[200]; 
char test[200];

void setup()
{
  Serial.begin(115200);
  delay(1000);
  
  pinMode(txPin, OUTPUT);
  pinMode(rxPin, INPUT);
  pinMode(10, OUTPUT); //Rak enable
  pinMode(4, OUTPUT); // LED
  pinMode(1, OUTPUT); // GNSS enable
  digitalWrite(4, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(4, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);  

  digitalWrite(10, HIGH); // Switch on RAK3172 LDO
  delay(1000);
  mySerial1.begin(115200, SERIAL_8N1, rxPin, txPin);
  delay(1000);  
    while (mySerial1.available()){
      Serial.write(mySerial1.read()); // read it and send it out Serial (USB)
    }
  
  Serial.println("Setting Up AT-Command...");
  delay(20);
  mySerial1.write("AT+JOIN\r\n");
  delay(20);
  Serial.println("Joinning Network...");
  delay(20);
}

void loop()
{
  // Check the networks is joined, if not then try to rejoin.
  if (millis()- lastjoin>=60000){
    lastjoin = millis();
    Serial.println("Checking Network Status");
    mySerial1.write("AT+NJS=?\r\n");
    delay(100);
    if (mySerial1.available()){
      for(int i=0; i<200; i++){
        out[i] = mySerial1.read();
      }
      strncpy(test, "AT+NJS=0\r\n", sizeof(test));
      if (compareChar(out, test)){
        Serial.write(out);
        mySerial1.write("AT+JOIN\r\n");
        memset(&out, 0, 200);
      }else{
        Serial.println("Network Status: Joined")
      }
    }
  }
 //---------------------Manual AT command---------------------
  if (mySerial1.available()){
    Serial.write(mySerial1.read());
  }
  if (Serial.available()){
    mySerial1.write(Serial.read());
  }
 //---------------------                 ---------------------
}

// Compare 2 array
bool compareChar(char* a, char* b){
  int la = sizeof(a);
  int lb = sizeof(b);
  if (la != lb){
    return false;
  }
  for (int i=0; i< la; i++){
    if (a[i] != b[i]){
      return false;
    }
  }
  return true;
}
