#include <EEPROM.h>

void setup() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0); // Write 0 to each address in the EEPROM
  }
}

void loop() {
  // Nothing to do here for clearing the EEPROM, as it's done in the setup
}