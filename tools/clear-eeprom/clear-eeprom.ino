#include <EEPROM.h>

void setup() {
  Serial.begin(115200);
  EEPROM.begin(128);  // o el tamaño que uses (128, 512, etc.)

  Serial.println("Borrando EEPROM...");
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0xFF);
  }
  EEPROM.commit();
  Serial.println("EEPROM borrada. Reinicia el ESP.");
}

void loop() {}
