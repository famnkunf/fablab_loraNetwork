/* 
AT command documentation:
    https://docs.rakwireless.com/RUI3/Serial-Operating-Modes/AT-Command-Manual/#content
*/

#include <Wire.h>
#include "Adafruit_SGP30.h"
#include <cmath>


Adafruit_SGP30 sgp;
HardwareSerial mySerial1(1);
// Rx and Tx pin of esp32
int rxPin = 20;
int txPin = 21;


int lastsend = 0;  //Last send time
int lastjoin = 0;  //Last join time
int t_loop = 0; // Time a loop takes
bool cnt = false;
bool cnt2 = false;
char out[200];
char test[200];
int infrared = 0;
int old_infrared;
int co2, tvoc;
int old_co2, old_tvoc;
char temp[200];
char hum[200];
int t, h;
int old_t, old_h;
char t_c[20];
char h_c[20];

char data[200];
bool con;
bool avaiableForSend = true;

char event_tx_done[] = { 'T', 'X', '_', 'D', 'O', 'N', 'E', '\0' };

const char* band = "8";
const char* appkey = "AC1F09FFFE0AE30FAC1F09FFF8683172";


void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(txPin, OUTPUT);
  pinMode(rxPin, INPUT);
  pinMode(10, OUTPUT);    //Rak enable
  pinMode(4, OUTPUT);     // LED
  pinMode(1, OUTPUT);     // GNSS enable
  pinMode(7, INPUT);      // Infrared Sensor
  pinMode(0, INPUT);      // MQ sensor
  digitalWrite(4, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);            // wait for a second
  digitalWrite(4, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);

  digitalWrite(10, HIGH);  // Switch on RAK3172 LDO
  delay(1000);
  mySerial1.begin(115200, SERIAL_8N1, rxPin, txPin);
  delay(1000);


  Serial.println("\nSetting Up AT-Command...");
  delay(3000);
  mySerial1.write("AT\r\n");
  delay(100);
  mySerial1.write("AT+APPKEY=");
  mySerial1.write(appkey);
  mySerial1.write("\r\n");
  delay(100);
  mySerial1.write("AT+BAND=");
  mySerial1.write(band);
  mySerial1.write("\r\n");
  delay(100);
  mySerial1.write("AT+NJM=1\r\n");
  delay(100);
  mySerial1.write("AT+JOIN\r\n");
  delay(100);
  Serial.println("\nJoinning Network...");
  delay(100);
  mySerial1.write("ATC+SHT\r\n");
  // Sensor Initiate
  Wire.begin();
  if (!sgp.begin()) {
    Serial.println("SPG30 not found!");
  }
  sgp.IAQinit();
  sgp.setHumidity(12670);
  delay(5000);
  while (mySerial1.available()) {
    Serial.write(mySerial1.read());  // read it and send it out Serial (USB)
  }
  Serial.println("Setup Finished!");
  lastjoin = millis();
}

void loop() {
  t_loop = millis();
  while (1) {
    // Check the networks is joined, if not then try to rejoin.
    if (millis() - lastjoin >= 15000) {

      lastjoin = millis();
      Serial.println("\nChecking Network Status");
      mySerial1.write("AT+NJS=?\r\n");
      con = true;
      while (con) {
        if (mySerial1.available()) {
          Serial.println("mySerial1 is available.");
        }
        for (int i = 0; mySerial1.available(); i++) {
          char s = mySerial1.read();
          Serial.write(s);
          if (s == '1') {
            cnt = true;
            Serial.write("\nNetwork Status: ");
            Serial.print(cnt);
            Serial.write("\r\n");
            con = false;
          } else if (s == '0') {
            cnt = false;
            Serial.write("\nNetwork Status: ");
            Serial.print(cnt);
            Serial.write("\r\n");
            con = false;
          }
        }
      }
      if (cnt == false) {
        Serial.println("Rejoinning...");
        mySerial1.write("AT+JOIN\r\n");
        while (mySerial1.available() == 0)
          ;
        while (mySerial1.available()) {
          Serial.write(mySerial1.read());
        }
      }
    }
    if (cnt) {
      break;
    }
  }
  // Send after 1s
  if (millis() - lastsend >= 1000) {
    Serial.println("Getting Data from Sensor");
    mySerial1.write("ATC+TEMP=?\r\n");
    while (mySerial1.available() == 0);
    while (true) {
      if (mySerial1.available()) {
        char a = mySerial1.read();
        if (a > '0' && a <= '9') {
          t_c[0] = a;
          for (int i = 1; true; i++) {
            t_c[i] = mySerial1.read();
            if (t_c[i] == '.') {
              t_c[i] = '\0';
              break;
            }
          }
          break;
        }
      }
    }
    while (mySerial1.available()) {
      mySerial1.read();
    }
    t = combineToInt(t_c);
    mySerial1.write("ATC+HUM=?\r\n");
    while (mySerial1.available() == 0)
      ;
    while (true) {
      if (mySerial1.available()) {
        char a = mySerial1.read();
        if (a > '0' && a <= '9') {
          h_c[0] = a;
          for (int i = 1; true; i++) {
            h_c[i] = mySerial1.read();
            if (h_c[i] == '\n' || h_c[i] == '\r') {
              h_c[i] = '\0';
              break;
            }
          }
          break;
        }
      }
    }
    while (mySerial1.available()) {
      mySerial1.read();
    }
    h = combineToInt(h_c);
    sgp.setHumidity(calculateAbsoluteHumidity(h, t));
    if (sgp.IAQmeasure()) {
      tvoc = sgp.TVOC;
      co2 = sgp.eCO2;
    }
    if (digitalRead(7) == 0) {
      infrared = 1;
    } else {
      infrared = 0;
    }
    Serial.print("Temperature: ");
    Serial.println(t);
    Serial.print("Humidity: ");
    Serial.println(h);
    Serial.print("Co2: ");
    Serial.println(co2);
    Serial.print("tvoc: ");
    Serial.println(tvoc);
    Serial.print("Infrared: ");
    Serial.println(infrared);
    if (old_co2 != co2) {
      send(1, co2);
    }
    if (old_tvoc != tvoc) {
      send(2, tvoc);
    }
    if (old_infrared != infrared) {
      send(3, infrared);
    }
    if (old_t != t) {
      send(4, t);
    }
    if (old_h != h) {
      send(5, h);
    }

    lastsend = millis();
    Serial.print("\nA loop takes: ");
    Serial.print((millis() - t_loop)/1000);
    Serial.println("s");
    t_loop = millis();
  }



  //---------------------Manual AT command---------------------
  // if (mySerial1.available()) {
  //   Serial.write(mySerial1.read());
  // }
  while (Serial.available()) {
    mySerial1.write(Serial.read());
  }
  //---------------------                 ---------------------
  
}

// Compare 2 array
bool compareChar(char* a, char* b) {
  int la = sizeof(a);
  int lb = sizeof(b);
  if (la != lb) {
    return false;
  }
  for (int i = 0; i < la; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

char* convertToHex(int num) {
  static char hexString[50];  // Allocate static memory for the hex string
  int digitIndex = 0;
  if (num == 0) {
    hexString[digitIndex] = '0';
    digitIndex++;
    hexString[digitIndex] = '0';
    digitIndex++;
    hexString[digitIndex] = '\0';  // Terminate the string with a null character
    return hexString;
  }
  while (num > 0) {
    int digit = num % 16;
    if (digit < 10) {
      hexString[digitIndex] = digit + '0';
    } else {
      hexString[digitIndex] = digit - 10 + 'A';
    }

    digitIndex++;
    num /= 16;
  }

  if (digitIndex % 2 == 1) {
    hexString[digitIndex] = '0';
    digitIndex++;
  }
  // Reverse the hex string since it was built from the least significant digit first
  for (int i = 0; i < digitIndex / 2; i++) {
    char temp = hexString[i];
    hexString[i] = hexString[digitIndex - i - 1];
    hexString[digitIndex - i - 1] = temp;
  }


  hexString[digitIndex] = '\0';  // Terminate the string with a null character
  return hexString;
}
void send(int port, int data) {
  int i = 0;
  int port_c = port + '0';
  mySerial1.write("AT+SEND=");
  mySerial1.write(port_c);
  mySerial1.write(":");
  mySerial1.write(convertToHex(data));
  mySerial1.write("\r\n");
  delay(500);
  while (1) {
    if (mySerial1.available()) {
      char o = mySerial1.read();
      Serial.write(o);
      if (o != event_tx_done[i]) {
        i = 0;
      } else {
        i++;
      }
      if (i == 7) {
        break;
      }
    }
  }
}
int combineToInt(char* c) {
  int s = 0;
  int size = 0;
  for (int i = 0; c[i] != '\0'; i++) {
    size += 1;
  }
  for (int i = 0; i < size; i++) {
    s += (c[i] - '0') * pow(10, (size - i - 1));
  }
  return s;
}

double calculateAbsoluteHumidity(double relativeHumidity, double temperature) {
  double saturationVaporPressure = 610.78 * exp((17.275 * temperature) / (temperature + 237.3));
  return (relativeHumidity * saturationVaporPressure * 1000) / (461.5 * temperature * 100.0);
}
