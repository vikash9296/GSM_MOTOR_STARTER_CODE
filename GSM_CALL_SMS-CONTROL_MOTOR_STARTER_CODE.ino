#include <EEPROM.h>
#include <SoftwareSerial.h>

SoftwareSerial GSM(8, 9);

String phone_no[9] = {"+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx", "+91xxxxxxxxxx"};

String RxString = "";
char RxChar = ' ';
int Counter = 0;
String GSM_Nr = "";
String GSM_Msg = "";

#define Relay1 2
#define Relay2 3
#define ManualSwitch 6 // Define manual pull-up switch pin
#define GSM_RST 4      // Define GSM reset pin

bool pumpState = false; // Track the state of the pump (true = ON, false = OFF)

// Function prototypes
void initModule(String cmd, char *res, int t);
void deleteSMS(int index);
void sendSMS(String number, String msg);
void processCommand();
void handleManualTrigger();
void handleIncomingCall();

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

  // Load saved phone numbers from EEPROM
  for (int i = 0; i < 9; i++) {
    char temp[15];
    for (int j = 0; j < 14; j++) {
      temp[j] = EEPROM.read(i * 15 + j);
    }
    temp[14] = '\0'; // Null-terminate the string
    phone_no[i] = String(temp);
  }

  delay(2000); // Wait for 2 seconds before sending the SMS
  sendSMS(phone_no[0], "GSM POWER ON");

  delay(100);
}

void loop() {
  // Read incoming SMS
  RxString = "";
  Counter = 0;
  while (GSM.available()) {
    delay(1);
    RxChar = char(GSM.read());
    if (Counter < 200) {
      RxString.concat(RxChar);
      Counter++;
    }
  }

  if (Received(F("CMT:"))) processCommand();

  // Handle manual switch trigger
  handleManualTrigger();

  // Handle incoming call
  handleIncomingCall();
}

void processCommand() {
  GetSMS();

  // Process commands for relay control
  if (GSM_Msg == "#1") { // Turn Relay1 ON (Pump ON)
    digitalWrite(Relay1, LOW); // Activate relay
    delay(3000); // Relay stays active for 3 seconds
    digitalWrite(Relay1, HIGH); // Deactivate relay
    pumpState = true; // Update pump state to ON
    sendSMS(GSM_Nr, "PUMP ON");
  } else if (GSM_Msg == "#2") { // Turn Relay2 ON (Pump OFF)
    digitalWrite(Relay2, LOW); // Activate relay
    delay(3000); // Relay stays active for 3 seconds
    digitalWrite(Relay2, HIGH); // Deactivate relay
    pumpState = false; // Update pump state to OFF
    sendSMS(GSM_Nr, "PUMP OFF");
  } else if (GSM_Msg == "#7") { // Send current status
    String status = "Status:\n";
    status += "PUMP: " + String(pumpState ? "ON" : "OFF"); // Reflect the current pump state
    sendSMS(GSM_Nr, status);
  } else if (GSM_Msg == "#CLR") { // Clear EEPROM
    for (int i = 0; i < 135; i++) { // Clear the first 135 bytes (9 phone numbers × 15 bytes each)
      EEPROM.write(i, '\0'); // Write null characters to EEPROM
    }
    for (int i = 0; i < 9; i++) { // Reset phone numbers in memory
      phone_no[i] = "";
    }
    sendSMS(GSM_Nr, "Empty"); // Send confirmation SMS
  }

  // Process mobile number registration commands
  if (GSM_Msg.startsWith("#SPN")) { // Show all stored numbers
    String reply = "STORED NO:\n";
    for (int i = 0; i < 9; i++) {
      reply += phone_no[i] + "\n";
    }
    sendSMS(GSM_Nr, reply);
  } else if (GSM_Msg.startsWith("#1STN") || GSM_Msg.startsWith("#2STN") || GSM_Msg.startsWith("#3STN") ||
             GSM_Msg.startsWith("#4STN") || GSM_Msg.startsWith("#5STN") || GSM_Msg.startsWith("#6STN") ||
             GSM_Msg.startsWith("#7STN") || GSM_Msg.startsWith("#8STN") || GSM_Msg.startsWith("#9STN")) {
    int index = GSM_Msg.substring(1, 2).toInt() - 1; // Get the index from the command
    String newNumber = GSM_Msg.substring(5);

    if (index >= 0 && index < 9 && newNumber.length() > 0) {
      phone_no[index] = newNumber;

      // Save the updated number to EEPROM
      for (int j = 0; j < 14; j++) {
        EEPROM.write(index * 15 + j, j < newNumber.length() ? newNumber[j] : '\0');
      }

      sendSMS(GSM_Nr, "Number " + String(index + 1) + " Changed Successfully!");
    }
  }
}

void handleManualTrigger() {
  if (digitalRead(ManualSwitch) == LOW) { // Manual switch pressed (active LOW)
    delay(50); // Debounce delay
    digitalWrite(Relay2, LOW); // Activate relay
    delay(3000); // Relay stays active for 3 seconds
    digitalWrite(Relay2, HIGH); // Deactivate relay
    pumpState = false; // Update pump state to OFF
    sendSMS(phone_no[0], "TANK FULL PUMP OFF");
    Serial.println("Manual switch triggered. Relay 2 activated.");
    delay(300); // Prevent multiple triggers due to bouncing
  }
}

void handleIncomingCall() {
  if (RxString.indexOf("RING") != -1) {
    int ringCount = 0;

    // Count the number of rings
    while (GSM.available()) {
      delay(1000); // Wait for 1 second per ring
      if (RxString.indexOf("RING") != -1) {
        ringCount++;
      }
      if (ringCount >= (pumpState ? 2 : 4)) {
        break;
      }
    }

    // Reject the call
    GSM.println("ATH"); // Hang up the call
    delay(1000);

    // Trigger relay based on pump state
    if (!pumpState) {
      digitalWrite(Relay1, LOW); // Turn pump ON
      delay(3000);
      digitalWrite(Relay1, HIGH);
      pumpState = true;
      sendSMS(phone_no[0], "PUMP ON via CALL");
    } else {
      digitalWrite(Relay2, LOW); // Turn pump OFF
      delay(3000);
      digitalWrite(Relay2, HIGH);
      pumpState = false;
      sendSMS(phone_no[0], "PUMP OFF via CALL");
    }
  }
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
  int t2 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t2 + 1);
  t2 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t2 + 1);
  t2 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t2 + 1);
  t2 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t2 + 1);
  t2 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t2 + 1);
  t2 = GSM_Msg.indexOf('"');
  GSM_Msg.remove(0, t2 + 1);
  GSM_Msg.remove(0, 1);
  GSM_Msg.trim();

  Serial.print("Number:");
  Serial.println(GSM_Nr);
  Serial.print("SMS:");
  Serial.println(GSM_Msg);

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
      }
    }
    delay(100);
  }
}
