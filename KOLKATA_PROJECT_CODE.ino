#include <EEPROM.h>
#include <SoftwareSerial.h>

SoftwareSerial GSM(8, 9);

String phone_no1 = "+919435601991"; // Replace with your phone numbers
String phone_no2 = "+919525927960";

String RxString = "";
char RxChar = ' ';
int Counter = 0;
String GSM_Nr = "";
String GSM_Msg = "";

#define Relay1 2

int load1;
int lastState = 1; // Initially, assume the last state was OFF

void setup() {
  pinMode(Relay1, OUTPUT);
  digitalWrite(Relay1, HIGH); 

  Serial.begin(9600);
  GSM.begin(9600);

  Serial.println("Initializing....");
  initModule("AT","OK",1000);
  initModule("AT+CPIN?","READY",1000);
  initModule("AT+CMGF=1","OK",1000);
  initModule("AT+CNMI=2,2,0,0,0","OK",1000);
  Serial.println("Initialized Successfully");

  load1 = EEPROM.read(1);
  lastState = load1; // Set last state to loaded value
  relays();

  sendSMS(phone_no1, "GSM POWER ON");
  sendSMS(phone_no2, "GSM POWER ON"); // Send to other number, if required

  delay(100);
}

void loop() {
  RxString = "";
  Counter = 0;

  while(GSM.available()) {
    delay(1);
    RxChar = char(GSM.read());

    if (Counter < 200) {
      RxString.concat(RxChar);
      Counter = Counter + 1;
    }
  }

  if (Received(F("CMT:"))) GetSMS();

  if (GSM_Nr == phone_no1 || GSM_Nr == phone_no2) {
    if (GSM_Msg == "START") {
      load1 = 0;
      sendSMS(GSM_Nr, "PUMP ON");
      lastState = load1; // Update lastState with the current state
    }

    if (GSM_Msg == "STOP") {
      load1 = 1;
      sendSMS(GSM_Nr, "PUMP OFF");
      lastState = load1; // Update lastState with the current state
    }

    if (GSM_Msg == "STS") {
      if (lastState == 0) {
        sendSMS(GSM_Nr, "PUMP IS ON");
      } else {
        sendSMS(GSM_Nr, "PUMP IS OFF");
      }
    }

    eeprom_write();
    relays();
  }

  GSM_Nr = "";
  GSM_Msg = "";
}

void eeprom_write() {
  EEPROM.write(1, load1);
}

void relays() {
  digitalWrite(Relay1, load1);
}

void sendSMS(String number, String msg) {
  GSM.print("AT+CMGS=\""); GSM.print(number); GSM.println("\"\r\n");
  delay(500);
  GSM.println(msg);
  delay(500);
  GSM.write(byte(26));
  delay(5000);
}

void GetSMS() {
  GSM_Nr = RxString;
  int t1 = GSM_Nr.indexOf('"');
  GSM_Nr.remove(0, t1 + 1);
  t1 = GSM_Nr.indexOf('"');
  GSM_Nr.remove(t1);

  GSM_Msg = RxString;
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  t1 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t1 + 1);
  GSM_Msg.remove(0, 1);
  GSM_Msg.trim();

  Serial.print("Number:"); Serial.println(GSM_Nr);
  Serial.print("SMS:"); Serial.println(GSM_Msg);
}

boolean Received(String S) {
  return (RxString.indexOf(S) >= 0);
}

void initModule(String cmd, char *res, int t) {
  while(1) {
    Serial.println(cmd);
    GSM.println(cmd);
    delay(100);

    while(GSM.available() > 0) {
      if (GSM.find(res)) {
        Serial.println(res);
        delay(t);
        return;
      } else {
        Serial.println("Error");
      }
    }

    delay(t);
  }
}