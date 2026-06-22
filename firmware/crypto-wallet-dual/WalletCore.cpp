#include "WalletCore.h"

WalletCore::WalletCore() : isUnlocked(false), keyGenerated(false), currentPIN(-1) {}

void WalletCore::begin() {
    EEPROM.begin(128);  // ESP32 EEPROM initialization
    loadPINFromEEPROM();

    if (EEPROM.read(32) != 0xFF) {
        loadKeyFromEEPROM();
        keyGenerated = true;
    }
}

int WalletCore::unlock(int pin) {
    loadPINFromEEPROM();
    if(!isUnlocked) {
      if (currentPIN == -1) {
        currentPIN = pin;
        savePINToEEPROM();
        if (!keyGenerated) {
            generateNewKey();
            saveKeyToEEPROM();
            keyGenerated = true;
        }
        isUnlocked = true;
        return 1;
      }
      if (currentPIN == pin) {
        isUnlocked = true;
        return 0;
      }
    }
    return -1;
}

void WalletCore::lock() {
    isUnlocked = false;
}

String WalletCore::getAddress() {
    if (!isUnlocked) return "LOCKED";

    uint8_t publicKey[64];
    generatePublicKey(publicKey);
    
    // CORRECT SHA256 usage for Crypto library
    SHA256 sha256;
    sha256.reset();
    sha256.update(publicKey, 64);
    
    uint8_t hash[32];
    sha256.finalize(hash, 32);  // Use finalize with output buffer

    String address = "0x";
    for (int i = 12; i < 32; i++) {
        if (hash[i] < 16) address += "0";
        address += String(hash[i], HEX);
    }
    
    return address;
}

String WalletCore::getPrivateKeyHex() {
    if (!isUnlocked) return "LOCKED";

    String privateKeyHex = "";
    for (int i = 0; i < 32; i++) {
        if (privateKey[i] < 16) privateKeyHex += "0";
        privateKeyHex += String(privateKey[i], HEX);
    }
    return privateKeyHex;
}

void WalletCore::factoryReset() {
    for (int i = 0; i < 128; i++) EEPROM.write(i, 0xFF);
    EEPROM.commit();  // ESP32 requires commit()
    keyGenerated = false;
    isUnlocked = false;
    currentPIN = -1;
}

bool WalletCore::isLocked() { return !isUnlocked; }
bool WalletCore::isKeyGenerated() { return keyGenerated; }

int WalletCore::getCurrentPIN() {
    return currentPIN;
}

bool WalletCore::hasSavedWiFi() const {
    return EEPROM.read(64) == 0x01;
}

String WalletCore::getSavedWiFiSSID() const {
    if (!hasSavedWiFi()) return "";
    int len = EEPROM.read(66);
    if (len <= 0 || len > 32) return "";
    String ssid = "";
    for (int i = 0; i < len; i++) {
        uint8_t c = EEPROM.read(67 + i);
        if (c == 0xFF) break;
        ssid += char(c);
    }
    return ssid;
}

String WalletCore::getSavedWiFiPassword() const {
    if (!hasSavedWiFi()) return "";
    int len = EEPROM.read(99);
    if (len < 0 || len > 28) return "";
    String password = "";
    for (int i = 0; i < len; i++) {
        uint8_t c = EEPROM.read(100 + i);
        if (c == 0xFF) break;
        password += char(c);
    }
    return password;
}

int WalletCore::getSavedWiFiSecurity() const {
    if (!hasSavedWiFi()) return -1;
    int security = EEPROM.read(65);
    return (security >= 0 && security < 5) ? security : -1;
}

void WalletCore::saveWiFiConfig(const String& ssid, const String& password, int securityType) {
    int ssidLen = min((int)ssid.length(), 32);
    int pwdLen = min((int)password.length(), 28);
    EEPROM.write(64, 0x01);
    EEPROM.write(65, securityType);
    EEPROM.write(66, ssidLen);
    for (int i = 0; i < 32; i++) {
        if (i < ssidLen) EEPROM.write(67 + i, ssid[i]);
        else EEPROM.write(67 + i, 0xFF);
    }
    EEPROM.write(99, pwdLen);
    for (int i = 0; i < 28; i++) {
        if (i < pwdLen) EEPROM.write(100 + i, password[i]);
        else EEPROM.write(100 + i, 0xFF);
    }
    EEPROM.commit();
}

void WalletCore::clearWiFiConfig() {
    for (int i = 64; i < 128; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
}

void WalletCore::loadPINFromEEPROM() {
    if (EEPROM.read(0) == 0xFF && EEPROM.read(1) == 0xFF) {
        currentPIN = -1;
    } else {
        currentPIN = EEPROM.read(0) | (EEPROM.read(1) << 8);
    }
}

void WalletCore::savePINToEEPROM() {
    EEPROM.write(0, currentPIN & 0xFF);        // byte bajo
    EEPROM.write(1, (currentPIN >> 8) & 0xFF); // byte alto
    EEPROM.commit();
}

void WalletCore::loadKeyFromEEPROM() {
    for (int i = 0; i < 32; i++) privateKey[i] = EEPROM.read(i + 32);
    // currentPIN=lo que sea
}

void WalletCore::saveKeyToEEPROM() {
    for (int i = 0; i < 32; i++) EEPROM.write(i + 32, privateKey[i]);
    EEPROM.commit();  // ESP32 requires commit()
}

void WalletCore::generateNewKey() {
    uint8_t entropy[128];
    
    // ESP32-specific functions
    uint64_t mac = ESP.getEfuseMac();
    memcpy(entropy, &mac, 8);

    for (int i = 0; i < 4; i++) {
        uint32_t rnd = esp_random();  // ESP32 hardware random
        memcpy(entropy + 8 + (i * 4), &rnd, 4);
    }

    uint32_t microsVal = micros();
    memcpy(entropy + 24, &microsVal, 4);

    for (int i = 0; i < 32; i++) {
        entropy[28 + i] = analogRead(36) & 0xFF;
        delay(2);
    }

    uint32_t heap = ESP.getFreeHeap();
    memcpy(entropy + 60, &heap, 4);

    // CORRECT SHA256 usage for Crypto library
    SHA256 sha256;
    sha256.reset();
    sha256.update(entropy, 64);
    sha256.finalize(privateKey, 32);  // Use finalize with output buffer
}

void WalletCore::generatePublicKey(uint8_t* pubkey) {
    // Double SHA256
    SHA256 sha1;
    sha1.reset();
    sha1.update(privateKey, 32);
    
    uint8_t hash1[32];
    sha1.finalize(hash1, 32);
    
    SHA256 sha2;
    sha2.reset();
    sha2.update(hash1, 32);
    
    uint8_t hash2[32];
    sha2.finalize(hash2, 32);
    
    memcpy(pubkey, hash1, 32);
    memcpy(pubkey + 32, hash2, 32);
}

bool WalletCore::changePIN(int oldPIN, int newPIN) {
    if (oldPIN != currentPIN) return false;
    if (newPIN < 0 || newPIN > 9999) return false;
    currentPIN = newPIN;
    savePINToEEPROM();
    return true;
}
