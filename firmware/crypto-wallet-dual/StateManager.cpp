#include "StateManager.h"

StateManager::StateManager() {
    currentData.currentState = STATE_MENU;
    currentData.previousState = STATE_MENU;
    currentData.selectedMenuItem = 0;
    currentData.previousMenuItem = -1;
    currentData.walletUnlocked = false;
    currentData.previousWalletState = false;
    currentData.pinCursorPosition = 0;
    currentData.inputMode = "TERMINAL+JOY";
    currentData.message = "";
    currentData.messageRequireButton = true;

    for (int i = 0; i < 4; i++) {
        currentData.pinInput[i] = -1;
        currentData.pinDigitVisible[i] = true;
        currentData.pinDigitTime[i] = 0;
    }
}

void StateManager::initialize() {
    // No se requiere inicialización adicional por ahora
}

void StateManager::setState(AppState newState) {
    currentData.previousState = currentData.currentState;
    currentData.currentState = newState;
}

void StateManager::setSelectedItem(int item) {
    currentData.previousMenuItem = currentData.selectedMenuItem;
    currentData.selectedMenuItem = item;
}

void StateManager::setWalletUnlocked(bool unlocked) {
    currentData.previousWalletState = currentData.walletUnlocked;
    currentData.walletUnlocked = unlocked;
}

void StateManager::setPinInput(int pos, int pin) {
    currentData.pinInput[pos] = pin;
}

void StateManager::setPinCursor(int index) {
    currentData.pinCursorPosition = index;
}

void StateManager::setPinDigitVisible(int index, bool visible) {
    if (index >= 0 && index < 4) {
        currentData.pinDigitVisible[index] = visible;
    }
}

void StateManager::setPinDigitTime(int index, unsigned long time) {
    if (index >= 0 && index < 4) {
        currentData.pinDigitTime[index] = time;
    }
}

void StateManager::setMessage(const String& message, bool requireButton) {
    currentData.message = message;
    currentData.messageRequireButton = requireButton;
}

void StateManager::setInputMode(const String& mode) {
    currentData.inputMode = mode;
}

bool StateManager::hasStateChanged() const {
    return currentData.currentState != currentData.previousState;
}

bool StateManager::hasMenuChanged() const {
    return currentData.selectedMenuItem != currentData.previousMenuItem;
}

bool StateManager::hasWalletStateChanged() const {
    return currentData.walletUnlocked != currentData.previousWalletState;
}

int StateManager::getPin() const {
    for (int i=0;i<4;i++) {
        if (currentData.pinInput[i]==-1) {
            return -1;
        }
    }
    return (1000*currentData.pinInput[0] + 
        100*currentData.pinInput[1] + 
        10*currentData.pinInput[2] + 
        currentData.pinInput[3]);
}

void StateManager::setFreshInput() {
    for (int i=0;i<4;i++) {
        setPinInput(i,-1);
    }
}
