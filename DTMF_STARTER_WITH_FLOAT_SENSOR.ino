#include "SoftwareSerial.h"

#define SIM800L_Tx 8  // TX pin of Arduino connected to RX pin of SIM800L
#define SIM800L_Rx 9  // RX pin of Arduino connected to TX pin of SIM800L
SoftwareSerial SIM800L(SIM800L_Tx, SIM800L_Rx);

#define RELAY1_PIN 2 // Change to relay module pin
#define RELAY2_PIN 3 // Change to relay module pin
#define MANUAL_SWITCH_PIN 6 // Manual switch pin

char dtmf_cmd;
bool call_flag = false;

// Array to hold mobile numbers
const char* phoneNumbers[] = {
  "+919435601991", // Example phone number 1
  "+919263669666", // Example phone number 2
  "+919525927960", // Example phone number 3
  "+919876543210", // Example phone number 4
  "+911234567890"  // Example phone number 5
};

void init_gsm();
void update_relay();
void sendSMS(const char* message);

void setup() {
  SIM800L.begin(9600);
  Serial.begin(9600);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(MANUAL_SWITCH_PIN, INPUT_PULLUP); // Set manual switch pin as input with internal pull-up resistor
  digitalWrite(RELAY1_PIN, HIGH); // Set relay pins to HIGH initially
  digitalWrite(RELAY2_PIN, HIGH); // Set relay pins to HIGH initially
  init_gsm();
}

void loop() {
  String gsm_data;
  int x = -1;

  while (SIM800L.available()) {
    char c = SIM800L.read();
    gsm_data += c;
    delay(10);
  }

  // Check manual switch state
  if (digitalRead(MANUAL_SWITCH_PIN) == LOW) {
    dtmf_cmd = '2'; // Set dtmf_cmd to trigger the same action as DTMF '2'
    update_relay();
    sendSMS("TANK FULL PUMP OFF");
    // Delay or debounce to avoid rapid switch detection
    delay(1000); // Adjust delay time as per requirement
  }

  if (call_flag) {
    x = gsm_data.indexOf("DTMF");
    if (x > -1) {
      dtmf_cmd = gsm_data[x + 6];
      Serial.println(dtmf_cmd);
      update_relay();
    }

    x = gsm_data.indexOf("NO CARRIER");
    if (x > -1) {
      SIM800L.println("ATH");
      call_flag = false;
    }
  } else {
    // Check for incoming SMS
    if (gsm_data.indexOf("+CMTI:") > -1) {
      delay(100);
      SIM800L.println("AT+CMGR=1");
      delay(100);
      while (SIM800L.available()) {
        gsm_data = SIM800L.readString();
      }
      if (gsm_data.indexOf("START") > -1) {
        dtmf_cmd = '1';
        update_relay();
      } else if (gsm_data.indexOf("STOP") > -1) {
        dtmf_cmd = '2';
        update_relay();
      }
    } else {
      x = gsm_data.indexOf("RING");
      if (x > -1) {
        delay(5000);
        SIM800L.println("ATA");
        call_flag = true;
      }
    }
  }
}

void init_gsm() {
  boolean gsm_Ready = 1;
  Serial.println("initializing GSM module");
  while (gsm_Ready > 0) {
    SIM800L.println("AT");
    Serial.println("AT");
    while (SIM800L.available()) {
      if (SIM800L.find("OK") > 0)
        gsm_Ready = 0;
    }
    delay(2000);
  }
  Serial.println("AT READY");

  boolean ntw_Ready = 1;
  Serial.println("finding network");
  while (ntw_Ready > 0) {
    SIM800L.println("AT+CPIN?");
    Serial.println("AT+CPIN?");
    while (SIM800L.available()) {
      if (SIM800L.find("+CPIN: READY") > 0)
        ntw_Ready = 0;
    }
    delay(2000);
  }
  Serial.println("NTW READY");

  boolean DTMF_Ready = 1;
  Serial.println("turning DTMF ON");
  while (DTMF_Ready > 0) {
    SIM800L.println("AT+DDET=1");
    Serial.println("AT+DDET=1");
    while (SIM800L.available()) {
      if (SIM800L.find("OK") > 0)
        DTMF_Ready = 0;
    }
    delay(2000);
  }
  Serial.println("DTMF READY");

  // Send GSM power on SMS notification
  sendSMS("GSM POWER ON");
}

void update_relay() {
  const unsigned long relay_duration = 1000; // 1 second
  unsigned long currentMillis = millis();

  if (dtmf_cmd == '1') {
    digitalWrite(RELAY1_PIN, LOW); // Turn on relay
    delay(relay_duration); // Wait for relay_duration milliseconds
    digitalWrite(RELAY1_PIN, HIGH); // Turn off relay
    sendSMS("PUMP ON");
    Serial.println("PUMP ON");
  }

  if (dtmf_cmd == '2') {
    digitalWrite(RELAY2_PIN, LOW); // Turn on relay
    delay(relay_duration); // Wait for relay_duration milliseconds
    digitalWrite(RELAY2_PIN, HIGH); // Turn off relay
    sendSMS("PUMP OFF");
    Serial.println("PUMP OFF");
  }
}

void sendSMS(const char* message) {
  SIM800L.println("AT+CMGF=1");
  delay(1000);

  // Iterate over phone numbers and send SMS to each
  for (int i = 0; i < 5; i++) {
    SIM800L.print("AT+CMGS=\"");
    SIM800L.print(phoneNumbers[i]);
    SIM800L.println("\"");
    delay(1000);
    SIM800L.print(message);
    delay(100);
    SIM800L.println((char)26);
    delay(1000);
  }
}
