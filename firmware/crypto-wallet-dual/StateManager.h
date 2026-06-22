#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include <Arduino.h>

class StateManager {
public:
    enum AppState {
        STATE_MENU,
        STATE_PIN_ENTRY,
        STATE_SHOW_MESSAGE,
        STATE_SHOW_ADDRESS,
        STATE_SHOW_PRIVATE_KEY,
        STATE_SHOW_INFO,
        STATE_WIFI_TEST,
        STATE_WIFI_MENU,
        STATE_WIFI_STRING_ENTRY,
        STATE_WIFI_SECURITY_SELECTION
    };

    struct AppData {
        // Estado general
        AppState currentState;
        AppState previousState;

        // Menú
        int selectedMenuItem;
        int previousMenuItem;

        // Wallet
        bool walletUnlocked;
        bool previousWalletState;

        // PIN
        int pinInput[4];
        int pinCursorPosition;
        bool pinDigitVisible[4];
        unsigned long pinDigitTime[4];

        // Modo de entrada
        String inputMode;

        // Mensajes
        String message;
        bool messageRequireButton;
    };

    StateManager();

    void initialize();
    void setState(AppState newState);
    void setSelectedItem(int item);
    void setWalletUnlocked(bool unlocked);
    void setPinInput(int pos, int pin);
    void setPinCursor(int index);
    void setPinDigitVisible(int index, bool visible);
    void setPinDigitTime(int index, unsigned long time);
    void setMessage(const String& message, bool requireButton);
    void setInputMode(const String& mode);

    void setFreshInput();

    bool hasStateChanged() const;
    bool hasMenuChanged() const;
    bool hasWalletStateChanged() const;

    int getPin() const;

    AppData getState() const { return currentData; }

private:
    AppData currentData;
};

#endif
