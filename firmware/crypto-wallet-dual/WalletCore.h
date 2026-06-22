#ifndef WALLET_CORE_H
#define WALLET_CORE_H

#include <Arduino.h>
#include <EEPROM.h>

// Use available SHA256 implementation instead of mbedtls
#include <Crypto.h>
#include <SHA256.h>

class WalletCore {
private:
    uint8_t privateKey[32];
    bool isUnlocked;
    bool keyGenerated;
    int currentPIN;

public:
    WalletCore();
    void begin();
    int unlock(int pin);
    void lock();
    bool changePIN(int oldPIN, int newPIN);
    bool hasSavedWiFi() const;
    String getSavedWiFiSSID() const;
    String getSavedWiFiPassword() const;
    int getSavedWiFiSecurity() const;
    void saveWiFiConfig(const String& ssid, const String& password, int securityType);
    void clearWiFiConfig();
    int getCurrentPIN();
    String getAddress();
    String getPrivateKeyHex();
    void factoryReset();
    bool isLocked();
    bool isKeyGenerated();

private:
    void loadPINFromEEPROM();
    void savePINToEEPROM();
    void loadKeyFromEEPROM();
    void saveKeyToEEPROM();
    void generateNewKey();
    void generatePublicKey(uint8_t* pubkey);
};

#endif
