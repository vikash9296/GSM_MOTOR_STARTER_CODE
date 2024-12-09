#include <EEPROM.h>
#include <SoftwareSerial.h>

SoftwareSerial GSM(8, 9);

String phone_no[8] = {"+919435601991", "+919525927960", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx"};

String RxString = "";
char RxChar = ' ';
int Counter = 0;
String GSM_Nr = "";
String GSM_Msg = "";
char lastCommand = ' '; // Variable to not store the last command received in EEPROM

#define Relay1 2
#define Relay2 3
#define ManualSwitch 6 // Define manual pull-up switch pin
#define GSM_RST 4      // Define GSM reset pin

int load1, load2;
unsigned long startTime = 0;
unsigned long duration = 5000; // 5 seconds

bool manualSwitchState = false;

void initModule(String cmd, char *res, int t);
void deleteSMS(int index);

void setup() {
  pinMode(Relay1, OUTPUT);
  digitalWrite(Relay1, HIGH); // Ensure relay starts in OFF state
  pinMode(Relay2, OUTPUT);
  digitalWrite(Relay2, HIGH); // Ensure relay starts in OFF state

  pinMode(ManualSwitch, INPUT_PULLUP); // Set up the manual switch as input with internal pull-up resistor

  pinMode(GSM_RST, OUTPUT);
  digitalWrite(GSM_RST, HIGH); // Initialize GSM reset pin to HIGH (inactive)

  Serial.begin(9600);
  GSM.begin(9600);

  Serial.println("Initializing....");

  // Reset the GSM module
  digitalWrite(GSM_RST, LOW);
  delay(100);
  digitalWrite(GSM_RST, HIGH);

  initModule("AT", "OK", 1000);
  initModule("AT+CPIN?", "READY", 1000);
  initModule("AT+CMGF=1", "OK", 1000);
  initModule("AT+CNMI=2,2,0,0,0", "OK", 1000);
  Serial.println("Initialized Successfully");

  // Reset the load states to OFF on startup
  load1 = 0;
  load2 = 0;

  // Update EEPROM with the initial state
  eeprom_write();

  relays();

  delay(2000); // Wait for 2 seconds before sending the SMS
  sendSMS(phone_no[0], "GSM POWER ON");

  delay(100);
}

void loop() {
  // Read the state of the manual switch
  manualSwitchState = digitalRead(ManualSwitch);

  RxString = "";
  Counter = 0;
  while (GSM.available()) {
    delay(1);
    RxChar = char(GSM.read());
    if (Counter < 200) {
      RxString.concat(RxChar);
      Counter = Counter + 1;
    }
  }

  if (Received(F("CMT:"))) GetSMS();

  for (int i = 0; i < 5; ++i) {
    if (GSM_Nr == phone_no[i]) {
      if (GSM_Msg == "#1") {
        load1 = 1;
        startTime = millis();
        sendSMS(GSM_Nr, "PUMP ON");
        lastCommand = 'S';
        eeprom_write(); // Save state in EEPROM after modifying load1
        relays();
      } else if (GSM_Msg == "#2") {
        load2 = 1;
        startTime = millis();
        sendSMS(GSM_Nr, "PUMP OFF");
        lastCommand = 'P';
        eeprom_write(); // Save state in EEPROM after modifying load2
        relays();
      } else if (GSM_Msg == "#7") {
        char storedState = EEPROM.read(0);
        if (storedState == 'S') {
          sendSMS(GSM_Nr, "PUMP IS ON");
        } else if (storedState == 'P') {
          sendSMS(GSM_Nr, "PUMP IS OFF");
        } else {
          sendSMS(GSM_Nr, "PUMP IS OFF");
        }
      }
      eeprom_write();
    }
  }

  // Check if the relay duration has passed and turn off if necessary
  if (load1 == 1 && (millis() - startTime) >= duration) {
    load1 = 0;
    eeprom_write(); // Save state in EEPROM after modifying load1
    relays();
  }

  if (load2 == 1 && (millis() - startTime) >= duration) {
    load2 = 0;
    eeprom_write(); // Save state in EEPROM after modifying load2
    relays();
  }

  // Check if manual switch is activated to trigger load2
  if (manualSwitchState == LOW && lastCommand != 'M') {
    load2 = 1;
    startTime = millis();
    lastCommand = 'M'; // 'M' for manual switch trigger
    eeprom_write(); // Save state in EEPROM after modifying load2
    relays();
  } else if (manualSwitchState == HIGH && lastCommand == 'M') {
    if (load2 == 1) {
      sendSMS(phone_no[0], "TANK FULL PUMP OFF");
      load2 = 0; // Turn off the pump
      lastCommand = ' ';
      eeprom_write(); // Save state in EEPROM after modifying lastCommand
      relays();
    }
  }

  GSM_Nr = "";
  GSM_Msg = "";
}

void eeprom_write() {
  EEPROM.write(0, lastCommand); // Store the last command in EEPROM
  EEPROM.write(1, load1);
  EEPROM.write(2, load2);
}

void relays() {
  digitalWrite(Relay1, !load1); // Set relay1 according to load1 status (Active-low)
  digitalWrite(Relay2, !load2); // Set relay2 according to load2 status (Active-low)
}

void sendSMS(String number, String msg) {
  GSM.println("AT+CMGF=1"); // Set SMS mode to text
  delay(1000);
  GSM.print("AT+CMGS=\"");
  GSM.print(number);
  GSM.println("\"");
  delay(1000);
  GSM.print(msg);
  delay(100);
  GSM.write(26); // ASCII code of Ctrl+Z
  delay(1000);
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

  Serial.print("Number:");
  Serial.println(GSM_Nr);
  Serial.print("SMS:");
  Serial.println(GSM_Msg);

  // Process the SMS message

  // Delete the processed SMS message from SIM inbox
  if (RxString.startsWith("+CMT:")) {
    int index = RxString.substring(6, 8).toInt(); // Extract index of SMS message
    deleteSMS(index); // Call function to delete SMS
  }
}

void deleteSMS(int index) {
  GSM.println("AT+CMGD=" + String(index)); // Send command to delete SMS
  delay(1000);
}

boolean Received(String S) {
  return RxString.indexOf(S) >= 0;
}

void initModule(String cmd, char *res, int t) {
  while (1) {
    Serial.println(cmd);
    GSM.println(cmd);
    delay(100);
    while (GSM.available() > 0) {
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
