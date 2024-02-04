#include "SoftwareSerial.h"

#define SIM800L_Tx 8    // TX pin of Arduino connected to RX pin of SIM800L
#define SIM800L_Rx 9    // RX pin of Arduino connected to TX pin of SIM800L
SoftwareSerial SIM800L(SIM800L_Tx, SIM800L_Rx);

#define RELAY1_PIN 2    // Change to relay module pin
#define RELAY2_PIN 3    // Change to relay module pin
#define MANUAL_SWITCH_PIN 6 // Manual switch pin

char dtmf_cmd;
bool call_flag = false;
bool manual_switch_triggered = false;

// Master phone number and other phone numbers
const char *masterNumber = "+919435601991"; // Example master phone number
const char *phoneNumbers[] = {
    "+919525927960", // Example phone number 1
    "+91xxxxxxxxxx", // Example phone number 2
    "+91xxxxxxxxxx", // Example phone number 3
    "+91xxxxxxxxxx"  // Example phone number 4
};

void update_relay();
void sendSMS(const char *message, const char *phoneNumber);

void setup()
{
    SIM800L.begin(9600);
    Serial.begin(9600);
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(MANUAL_SWITCH_PIN, INPUT_PULLUP); // Set manual switch pin as input with internal pull-up resistor
    digitalWrite(RELAY1_PIN, HIGH);           // Set relay pins to HIGH initially
    digitalWrite(RELAY2_PIN, HIGH);           // Set relay pins to HIGH initially

    // Initialize GSM module
    boolean gsm_Ready = 1;
    Serial.println("initializing GSM module");
    while (gsm_Ready > 0)
    {
        SIM800L.println("AT");
        Serial.println("AT");
        while (SIM800L.available())
        {
            if (SIM800L.find("OK") > 0)
                gsm_Ready = 0;
        }
        delay(2000);
    }
    Serial.println("AT READY");

    boolean ntw_Ready = 1;
    Serial.println("finding network");
    while (ntw_Ready > 0)
    {
        SIM800L.println("AT+CPIN?");
        Serial.println("AT+CPIN?");
        while (SIM800L.available())
        {
            if (SIM800L.find("+CPIN: READY") > 0)
                ntw_Ready = 0;
        }
        delay(2000);
    }
    Serial.println("NTW READY");

    boolean DTMF_Ready = 1;
    Serial.println("turning DTMF ON");
    while (DTMF_Ready > 0)
    {
        SIM800L.println("AT+DDET=1");
        Serial.println("AT+DDET=1");
        while (SIM800L.available())
        {
            if (SIM800L.find("OK") > 0)
                DTMF_Ready = 0;
        }
        delay(2000);
    }
    Serial.println("DTMF READY");

    // Send GSM power on SMS notification to all numbers
    const char* powerOnMessage = "GSM POWER ON";
    
    // Send to other numbers
    for (int i = 0; i < sizeof(phoneNumbers) / sizeof(phoneNumbers[0]); i++) {
        sendSMS(powerOnMessage, phoneNumbers[i]);
        delay(1000); // Delay between sending SMS to different numbers
    }

    // Send to master number
    sendSMS(powerOnMessage, masterNumber);
}

void loop()
{
    String gsm_data;
    int x = -1;

    while (SIM800L.available())
    {
        char c = SIM800L.read();
        gsm_data += c;
        delay(10);
    }

    // Check manual switch state
    if (digitalRead(MANUAL_SWITCH_PIN) == LOW)
    {
        if (!manual_switch_triggered)
        {
            manual_switch_triggered = true;
            sendSMS("TANK FULL PUMP OFF", masterNumber);
        }
        dtmf_cmd = '2'; // Set dtmf_cmd to trigger the same action as DTMF '2'
        update_relay();

        // Delay or debounce to avoid rapid switch detection
        delay(500); // Adjust delay time as per requirement
    }
    else
    {
        manual_switch_triggered = false; // Reset manual switch triggered flag
    }

    if (call_flag)
    {
        x = gsm_data.indexOf("DTMF");
        if (x > -1)
        {
            dtmf_cmd = gsm_data[x + 6];
            Serial.println(dtmf_cmd);
            update_relay();
        }

        x = gsm_data.indexOf("NO CARRIER");
        if (x > -1)
        {
            SIM800L.println("ATH");
            call_flag = false;
        }
    }
    else
    {
        // Check for incoming SMS
        if (gsm_data.indexOf("+CMTI:") > -1)
        {
            delay(100);
            SIM800L.println("AT+CMGR=1");
            delay(100);
            while (SIM800L.available())
            {
                gsm_data = SIM800L.readString();
            }
            if (gsm_data.indexOf("START") > -1)
            {
                dtmf_cmd = '1';
                update_relay();
            }
            else if (gsm_data.indexOf("STOP") > -1)
            {
                dtmf_cmd = '2';
                update_relay();
            }
        }
        else
        {
            x = gsm_data.indexOf("RING");
            if (x > -1)
            {
                delay(5000);
                SIM800L.println("ATA");
                call_flag = true;
            }
        }
    }
}

void update_relay()
{
    const unsigned long relay_duration = 1000; // 1 second
    unsigned long currentMillis = millis();

    if (dtmf_cmd == '1')
    {
        digitalWrite(RELAY1_PIN, LOW); // Turn on relay
        delay(relay_duration);          // Wait for relay_duration milliseconds
        digitalWrite(RELAY1_PIN, HIGH); // Turn off relay
        sendSMS("PUMP ON", masterNumber);
        Serial.println("PUMP ON");
    }

    if (dtmf_cmd == '2')
    {
        digitalWrite(RELAY2_PIN, LOW); // Turn on relay
        delay(relay_duration);          // Wait for relay_duration milliseconds
        digitalWrite(RELAY2_PIN, HIGH); // Turn off relay
        sendSMS("PUMP OFF", masterNumber);
        Serial.println("PUMP OFF");
    }
}

void sendSMS(const char *message, const char *phoneNumber)
{
    SIM800L.println("AT+CMGF=1");
    delay(1000);

    SIM800L.print("AT+CMGS=\"");
    SIM800L.print(phoneNumber);
    SIM800L.println("\"");
    delay(1000);
    SIM800L.print(message);
    delay(100);
    SIM800L.println((char)26);
    delay(1000);
}
