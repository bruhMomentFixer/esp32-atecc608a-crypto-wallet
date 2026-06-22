#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <format>

// Hardware configuration
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define JOY_Y    13
#define JOY_BTN  15

// Include modular headers
#include "DisplayManager.h"
#include "InputManager.h"
#include "InputFacade.h"
#include "StateManager.h"
#include "WalletCore.h"

// Global instances
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
DisplayManager displayManager(&tft);
InputManager inputManager(JOY_Y, JOY_BTN);
InputFacade inputFacade(&inputManager);
StateManager stateManager;
WalletCore wallet;

// Menu configuration
const char* baseMenu[] = {"Unlock", "Address", "Balance", "Export", "Change PIN", "Info", "Wi-Fi", "Reset"};
const int menuItemCount = 8;
const int pinLength = 4;

const char* wifiMenuOptions[] = {"Test Internet", "Saved WiFi", "New WiFi", "Back"};
const int wifiMenuItemCount = 4;
const char* wifiSecurityOptions[] = {"WPA2-PSK", "WPA3", "WPA/WPA2", "WEP", "Open"};
const int wifiSecurityOptionCount = 5;

enum ChangePinPhase {
    CHANGE_PIN_IDLE,
    CHANGE_PIN_NEW_FIRST,
    CHANGE_PIN_NEW_CONFIRM
};

enum ResetPhase {
    RESET_IDLE,
    RESET_CONFIRM
};

enum WifiState {
    WIFI_STATE_IDLE,
    WIFI_STATE_MENU,
    WIFI_STATE_STRING_ENTRY,
    WIFI_STATE_SECURITY_SELECTION
};

ChangePinPhase changePinPhase = CHANGE_PIN_IDLE;
int pendingNewPIN = -1;

ResetPhase resetPhase = RESET_IDLE;
int resetCode = -1;

WifiState wifiState = WIFI_STATE_IDLE;
int wifiMenuSelectedItem = 0;
int wifiMenuPreviousItem = -1;
int wifiSecuritySelectedItem = 0;
int wifiSecurityPreviousItem = -1;

const int WIFI_INPUT_MAX_SLOTS = 32;
int wifiEditSlots[WIFI_INPUT_MAX_SLOTS];
int wifiEditLen = 1;
int wifiEditCursor = 0;
int wifiPendingSecurity = 0;
String wifiPendingSSID = "";
String wifiPendingPassword = "";
String wifiEntryLabel = "";

bool wifiTextEntryNeedsRedraw = true;
int prevWifiEditSlots[WIFI_INPUT_MAX_SLOTS];
int prevWifiEditLen = 0;
int prevWifiEditCursor = -1;
String prevWifiEntryLabel = "";

const char wifiSpecialChars[] = " !@#$%^&*()-_=+[]{};:'\",.<>/?\\|`~";


// ---------- INITIALIZATION ----------
void initializeSystem() {
    Serial.begin(115200);
    
    displayManager.initialize();
    displayManager.setMirrorToSerial(true);
    inputManager.initialize();
    inputFacade.initialize();
    stateManager.initialize();
    wallet.begin();

    // Limpiar buffer serial (descartar bytes residuales)
    while (Serial.available()) Serial.read();

    // Calibrar joystick y evitar inputs iniciales
    inputManager.calibrate();
    inputManager.suppressInitialInputs(500); // ignorar inputs 500 ms

    // asegurar selección inicial
    stateManager.setSelectedItem(0);
  
    displayManager.showMessage("CRYPTO WALLET\n\rPRO MODE", false);
    delay(1000);

    auto s = stateManager.getState();
    displayManager.drawFullMenu(
        s.selectedMenuItem,
        baseMenu,
        menuItemCount,
        s.walletUnlocked,
        s.inputMode
    );
}
// ------------------------------------


// ---------- MENU HELPERS ------------
const char** getMenuOptions(bool walletUnlocked) {
    static const char* lockedMenu[] = {"Unlock", "Address", "Balance", "Export", "Change PIN", "Info", "Wi-Fi", "Reset"};
    static const char* unlockedMenu[] = {"Lock", "Address", "Balance", "Export", "Change PIN", "Info", "Wi-Fi", "Reset"};
    return walletUnlocked ? unlockedMenu : lockedMenu;
}

int advanceWifiChar(int current, bool up) {
    if (current == -1) {
        if (up) return 'a';
        return wifiSpecialChars[strlen(wifiSpecialChars) - 1];
    }

    if (current >= 'a' && current <= 'z') {
        if (up) {
            return (current == 'z') ? 'A' : (current + 1);
        }
        return (current == 'a') ? -1 : (current - 1);
    }

    if (current >= 'A' && current <= 'Z') {
        if (up) {
            return (current == 'Z') ? '0' : (current + 1);
        }
        return (current == 'A') ? 'z' : (current - 1);
    }

    if (current >= '0' && current <= '9') {
        if (up) {
            return wifiSpecialChars[0];
        }
        return (current == '0') ? 'Z' : (current - 1);
    }

    // special chars range
    const int specialLen = strlen(wifiSpecialChars);
    for (int i = 0; i < specialLen; i++) {
        if (current == wifiSpecialChars[i]) {
            if (up) {
                return (i == specialLen - 1) ? -1 : wifiSpecialChars[i + 1];
            }
            return (i == 0) ? '9' : wifiSpecialChars[i - 1];
        }
    }

    return -1;
}

String getWifiInputString() {
    String result = "";
    for (int i = 0; i < wifiEditLen; i++) {
        if (wifiEditSlots[i] == -1) break;
        result += char(wifiEditSlots[i]);
    }
    return result;
}

bool wifiInputHasNonEmpty() {
    for (int i = 0; i < wifiEditLen; i++) {
        if (wifiEditSlots[i] != -1) return true;
    }
    return false;
}

void wifiInputReset() {
    wifiEditLen = 1;
    wifiEditCursor = 0;
    for (int i = 0; i < WIFI_INPUT_MAX_SLOTS; i++) {
        wifiEditSlots[i] = -1;
    }
}

String wifiSecurityLabel(int securityType) {
    if (securityType >= 0 && securityType < wifiSecurityOptionCount) {
        return String(wifiSecurityOptions[securityType]);
    }
    return "Unknown";
}

String getSavedWiFiInfo() {
    if (!wallet.hasSavedWiFi()) {
        return "No saved Wi-Fi connection";
    }
    String info = "SSID: " + wallet.getSavedWiFiSSID() + "\n";
    int security = wallet.getSavedWiFiSecurity();
    info += "Security: " + wifiSecurityLabel(security) + "\n";
    info += "Password: [hidden]";
    return info;
}

bool testSavedWiFi(String& result) {
    if (!wallet.hasSavedWiFi()) {
        result = "No saved Wi-Fi connection";
        return false;
    }

    String ssid = wallet.getSavedWiFiSSID();
    String password = wallet.getSavedWiFiPassword();
    int security = wallet.getSavedWiFiSecurity();

    WiFi.mode(WIFI_STA);
    if (security == 4) {
        WiFi.begin(ssid.c_str());
    } else {
        WiFi.begin(ssid.c_str(), password.c_str());
    }

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(200);
    }

    if (WiFi.status() != WL_CONNECTED) {
        result = "Wi-Fi connection failed";
        WiFi.disconnect(true);
        return false;
    }

    WiFiClient client;
    bool connected = client.connect(IPAddress(8, 8, 8, 8), 53);
    if (!connected) {
        result = "Internet ping failed";
        client.stop();
        WiFi.disconnect(true);
        return false;
    }
    client.stop();

    result = "Internet connection OK";
    WiFi.disconnect(true);
    return true;
}

bool pollWiFiTestAbort(unsigned long timeoutMs) {
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        auto input = inputFacade.processInputs();
        if (input.event == InputManager::NAV_SELECT) {
            return true;
        }
        delay(50);
    }
    return false;
}

void waitWiFiTestFinal() {
    while (true) {
        auto input = inputFacade.processInputs();
        if (input.event == InputManager::NAV_SELECT) {
            break;
        }
        delay(50);
    }
}

bool isWifiNetworkNearby(const String& ssid) {
    int n = WiFi.scanNetworks();
    if (n < 0) {
        return false;
    }
    bool found = false;
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == ssid) {
            found = true;
            break;
        }
    }
    WiFi.scanDelete();
    return found;
}

void runWiFiInternetTest() {
    displayManager.showWiFiTestScreen("Initializing Wi-Fi hardware...", "Scanning hardware...", "", "ENTER to cancel");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    if (pollWiFiTestAbort(500)) {
        return;
    }

    int networks = WiFi.scanNetworks();
    if (networks < 0) {
        displayManager.showWiFiTestScreen("Initializing Wi-Fi hardware...", "Hardware init failed", "", "ENTER to return");
        waitWiFiTestFinal();
        return;
    }
    WiFi.scanDelete();

    displayManager.showWiFiTestScreen("Initializing Wi-Fi hardware...", "Hardware init OK", String(networks) + " networks found", "ENTER to cancel");
    if (pollWiFiTestAbort(500)) {
        return;
    }

    if (!wallet.hasSavedWiFi()) {
        displayManager.showWiFiTestScreen("Authenticating...", "No saved Wi-Fi config", "", "ENTER to return");
        waitWiFiTestFinal();
        return;
    }

    String ssid = wallet.getSavedWiFiSSID();
    String password = wallet.getSavedWiFiPassword();
    int security = wallet.getSavedWiFiSecurity();

    displayManager.showWiFiTestScreen("Authenticating...", "Looking for: " + ssid, "", "ENTER to cancel");
    if (pollWiFiTestAbort(500)) {
        return;
    }

    if (!isWifiNetworkNearby(ssid)) {
        displayManager.showWiFiTestScreen("Authenticating...", "SSID not found nearby", "", "ENTER to return");
        waitWiFiTestFinal();
        return;
    }

    displayManager.showWiFiTestScreen("Authenticating...", "SSID found", "", "ENTER to cancel");
    if (pollWiFiTestAbort(500)) {
        return;
    }

    if (security == 4) {
        WiFi.begin(ssid.c_str());
    } else {
        WiFi.begin(ssid.c_str(), password.c_str());
    }

    unsigned long authStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - authStart < 10000) {
        if (pollWiFiTestAbort(100)) {
            WiFi.disconnect(true);
            return;
        }
        delay(50);
    }

    if (WiFi.status() != WL_CONNECTED) {
        displayManager.showWiFiTestScreen("Authenticating...", "Authentication failed", "", "ENTER to return");
        waitWiFiTestFinal();
        WiFi.disconnect(true);
        return;
    }

    displayManager.showWiFiTestScreen("Getting IP address...", WiFi.localIP().toString(), "", "ENTER to cancel");
    if (pollWiFiTestAbort(500)) {
        WiFi.disconnect(true);
        return;
    }

    int successCount = 0;
    for (int attempt = 1; attempt <= 10; attempt++) {
        displayManager.showWiFiTestScreen("Pinging 8.8.8.8", "Packet " + String(attempt) + "/10", "Success: " + String(successCount), "ENTER to cancel");
        WiFiClient client;
        if (client.connect(IPAddress(8, 8, 8, 8), 53)) {
            successCount++;
            client.stop();
        }
        if (pollWiFiTestAbort(300)) {
            WiFi.disconnect(true);
            return;
        }
        delay(200);
    }

    int successRate = (successCount * 100) / 10;
    displayManager.showWiFiTestScreen("Ping complete", "Success rate: " + String(successRate) + "%", String(successCount) + "/10 packets", "ENTER to return");
    waitWiFiTestFinal();
    WiFi.disconnect(true);
}

void renderWifiTextEntry() {
    if (wifiTextEntryNeedsRedraw || prevWifiEntryLabel != wifiEntryLabel || prevWifiEditCursor == -1) {
        displayManager.showWiFiTextEntry(wifiEntryLabel, wifiEditSlots, wifiEditLen, wifiEditCursor);
        wifiTextEntryNeedsRedraw = false;
    } else {
        displayManager.updateWiFiTextEntry(wifiEditSlots, wifiEditLen, wifiEditCursor, prevWifiEditCursor, prevWifiEditSlots, prevWifiEditLen);
    }

    prevWifiEntryLabel = wifiEntryLabel;
    prevWifiEditLen = wifiEditLen;
    prevWifiEditCursor = wifiEditCursor;
    for (int i = 0; i < WIFI_INPUT_MAX_SLOTS; i++) {
        prevWifiEditSlots[i] = wifiEditSlots[i];
    }
}

bool wifiInputIsEmptySlot(int index) {
    return index >= 0 && index < wifiEditLen && wifiEditSlots[index] == -1;
}
// ------------------------------------


// ---------- MENU STATE --------------
void handleMenuState() {
  
    // 1) Leer input PRIMERO
    auto inputRaw = inputFacade.processInputs();

    InputManager::InputData input;
    input.event = inputRaw.event;
    input.digit = inputRaw.digit;

    // Estado antes del input
    auto before = stateManager.getState();
    const char** menuOptions = getMenuOptions(before.walletUnlocked);

    // 2) Aplicar input (modifica StateManager)

    if (stateManager.getState().currentState == StateManager::STATE_MENU && input.event != InputManager::NO_EVENT) {            
        stateManager.setWalletUnlocked(!wallet.isLocked());
        switch (input.event) {
            case InputManager::NAV_UP:
                stateManager.setSelectedItem(((before.selectedMenuItem - 1) % menuItemCount + menuItemCount) % menuItemCount);
                displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                break;

            case InputManager::NAV_DOWN:
                stateManager.setSelectedItem(((before.selectedMenuItem + 1) % menuItemCount + menuItemCount) % menuItemCount);
                displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                break;

            case InputManager::NAV_SELECT:
                switch (before.selectedMenuItem) {
                    case 4:
                        if (stateManager.getState().walletUnlocked) {
                            changePinPhase = CHANGE_PIN_NEW_FIRST;
                            stateManager.setFreshInput();
                            stateManager.setPinCursor(0);
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            stateManager.setMessage("Change PIN\nEnter NEW PIN", true);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_PIN_ENTRY);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        } else {
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            stateManager.setMessage("Unlock wallet first", true);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        }
                        break;
                    case 1:
                        if (stateManager.getState().walletUnlocked) {
                            String address = wallet.getAddress();
                            stateManager.setMessage(address, true);
                            stateManager.setState(StateManager::STATE_SHOW_ADDRESS);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        } else {
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            stateManager.setMessage("Unlock wallet first to view address", true);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        }
                        break;
                    case 2:
                        if (stateManager.getState().walletUnlocked) {
                            String result;
                            if (testSavedWiFi(result)) {
                                stateManager.setMessage("Balance support not implemented\nInternet OK", true);
                            } else {
                                stateManager.setMessage(result, true);
                            }
                        } else {
                            stateManager.setMessage("Unlock wallet first to check balance", true);
                        }
                        stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        waitForConfirmation();
                        stateManager.setState(StateManager::STATE_MENU);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        break;
                    case 3:
                        if (stateManager.getState().walletUnlocked) {
                            String privateKey = wallet.getPrivateKeyHex();
                            stateManager.setMessage(privateKey, true);
                            stateManager.setState(StateManager::STATE_SHOW_PRIVATE_KEY);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        } else {
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            stateManager.setMessage("Unlock wallet first to export private key", true);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        }
                        break;
                    case 5:
                        stateManager.setState(StateManager::STATE_SHOW_INFO);
                        displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged(), wallet.hasSavedWiFi());
                        waitForConfirmation();
                        stateManager.setState(StateManager::STATE_MENU);
                        displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged(), wallet.hasSavedWiFi());
                        break;
                    case 6:
                        if (stateManager.getState().walletUnlocked) {
                            stateManager.setState(StateManager::STATE_WIFI_MENU);
                            wifiMenuSelectedItem = 0;
                            wifiMenuPreviousItem = 0;
                            displayManager.drawFullMenu(wifiMenuSelectedItem, wifiMenuOptions, wifiMenuItemCount, true, stateManager.getState().inputMode);
                        } else {
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            stateManager.setMessage("Unlock wallet first", true);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        }
                        break;
                    case 7:
                        if (stateManager.getState().walletUnlocked) {
                            resetCode = random(1000, 10000);
                            resetPhase = RESET_CONFIRM;
                            stateManager.setFreshInput();
                            stateManager.setPinCursor(0);
                            stateManager.setMessage("Enter " + String(resetCode) + " to confirm", true);
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_PIN_ENTRY);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        } else {
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            stateManager.setMessage("Unlock wallet first", true);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        }
                        break;
                    case 0:
                        if (stateManager.getState().walletUnlocked) {
                            wallet.lock();
                            stateManager.setWalletUnlocked(false);
                            stateManager.setMessage("Wallet Locked !!!", true);
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        } else {
                            stateManager.setState(StateManager::STATE_PIN_ENTRY);
                            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        }
                        break;
                    //case 
                }
                break;

            default:
                break;
        }
    } else if (stateManager.getState().currentState == StateManager::STATE_WIFI_MENU) {
        if (input.event != InputManager::NO_EVENT) {
            switch (input.event) {
                case InputManager::NAV_UP: {
                    int previous = wifiMenuSelectedItem;
                    wifiMenuSelectedItem = ((wifiMenuSelectedItem - 1) % wifiMenuItemCount + wifiMenuItemCount) % wifiMenuItemCount;
                    displayManager.updateMenu(wifiMenuSelectedItem, previous, wifiMenuOptions, wifiMenuItemCount, true, stateManager.getState().inputMode);
                    wifiMenuPreviousItem = previous;
                    break;
                }
                case InputManager::NAV_DOWN: {
                    int previous = wifiMenuSelectedItem;
                    wifiMenuSelectedItem = ((wifiMenuSelectedItem + 1) % wifiMenuItemCount + wifiMenuItemCount) % wifiMenuItemCount;
                    displayManager.updateMenu(wifiMenuSelectedItem, previous, wifiMenuOptions, wifiMenuItemCount, true, stateManager.getState().inputMode);
                    wifiMenuPreviousItem = previous;
                    break;
                }

                case InputManager::NAV_SELECT: {
                    switch (wifiMenuSelectedItem) {
                        case 0: {
                            runWiFiInternetTest();
                            stateManager.setState(StateManager::STATE_WIFI_MENU);
                            displayManager.drawFullMenu(wifiMenuSelectedItem, wifiMenuOptions, wifiMenuItemCount, true, stateManager.getState().inputMode);
                            break;
                        }
                        case 1: {
                            stateManager.setMessage(getSavedWiFiInfo(), true);
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_WIFI_MENU);
                            displayManager.drawFullMenu(wifiMenuSelectedItem, wifiMenuOptions, wifiMenuItemCount, true, stateManager.getState().inputMode);
                            break;
                        }
                        case 2:
                            wifiInputReset();
                            wifiPendingSSID = "";
                            wifiPendingPassword = "";
                            wifiEntryLabel = "Enter SSID";
                            stateManager.setState(StateManager::STATE_WIFI_STRING_ENTRY);
                            wifiTextEntryNeedsRedraw = true;
                            renderWifiTextEntry();
                            break;
                        case 3:
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            break;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    } else if (stateManager.getState().currentState == StateManager::STATE_WIFI_SECURITY_SELECTION) {
        if (input.event != InputManager::NO_EVENT) {
            switch (input.event) {
                case InputManager::NAV_UP: {
                    int previous = wifiSecuritySelectedItem;
                    wifiSecuritySelectedItem = ((wifiSecuritySelectedItem - 1) % wifiSecurityOptionCount + wifiSecurityOptionCount) % wifiSecurityOptionCount;
                    displayManager.updateMenu(wifiSecuritySelectedItem, previous, wifiSecurityOptions, wifiSecurityOptionCount, true, stateManager.getState().inputMode);
                    break;
                }
                case InputManager::NAV_DOWN: {
                    int previous = wifiSecuritySelectedItem;
                    wifiSecuritySelectedItem = ((wifiSecuritySelectedItem + 1) % wifiSecurityOptionCount + wifiSecurityOptionCount) % wifiSecurityOptionCount;
                    displayManager.updateMenu(wifiSecuritySelectedItem, previous, wifiSecurityOptions, wifiSecurityOptionCount, true, stateManager.getState().inputMode);
                    break;
                }
                case InputManager::NAV_SELECT:
                    wifiPendingSecurity = wifiSecuritySelectedItem;
                    wifiInputReset();
                    wifiPendingPassword = "";
                    wifiEntryLabel = "Enter password";
                    stateManager.setState(StateManager::STATE_WIFI_STRING_ENTRY);
                    wifiTextEntryNeedsRedraw = true;
                    renderWifiTextEntry();
                    break;
                default:
                    break;
            }
        }
    } else if (stateManager.getState().currentState == StateManager::STATE_WIFI_STRING_ENTRY) {
        int currentChar = wifiEditSlots[wifiEditCursor];
        switch (input.event) {
            case InputManager::NAV_UP:
                wifiEditSlots[wifiEditCursor] = advanceWifiChar(currentChar, true);
                if (wifiEditSlots[wifiEditCursor] == -1 && wifiEditCursor < wifiEditLen - 1) {
                    wifiEditLen = wifiEditCursor + 1;
                }
                renderWifiTextEntry();
                break;
            case InputManager::NAV_DOWN:
                wifiEditSlots[wifiEditCursor] = advanceWifiChar(currentChar, false);
                if (wifiEditSlots[wifiEditCursor] == -1 && wifiEditCursor < wifiEditLen - 1) {
                    wifiEditLen = wifiEditCursor + 1;
                }
                renderWifiTextEntry();
                break;
            case InputManager::NAV_LEFT:
                if (wifiEditCursor > 0) {
                    wifiEditCursor--;
                }
                renderWifiTextEntry();
                break;
            case InputManager::NAV_RIGHT:
                if (wifiEditCursor < wifiEditLen - 1) {
                    wifiEditCursor++;
                } else if (wifiEditSlots[wifiEditCursor] != -1 && wifiEditLen < WIFI_INPUT_MAX_SLOTS) {
                    wifiEditLen++;
                    wifiEditCursor++;
                    wifiEditSlots[wifiEditCursor] = -1;
                }
                renderWifiTextEntry();
                break;
            case InputManager::NAV_SELECT: {
                String entry = getWifiInputString();
                bool allowEmptyPassword = (wifiEntryLabel == "Enter password" && wifiPendingSecurity == 4);
                if (entry.length() == 0 && !(wifiEntryLabel == "Enter password" && allowEmptyPassword)) {
                    stateManager.setMessage("Input cannot be empty", true);
                    stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                    displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                    waitForConfirmation();
                    stateManager.setState(StateManager::STATE_WIFI_MENU);
                    displayManager.drawFullMenu(wifiMenuSelectedItem, wifiMenuOptions, wifiMenuItemCount, true, stateManager.getState().inputMode);
                } else if (wifiEntryLabel == "Enter SSID") {
                    wifiPendingSSID = entry;
                    wifiSecuritySelectedItem = 0;
                    stateManager.setState(StateManager::STATE_WIFI_SECURITY_SELECTION);
                    displayManager.drawFullMenu(wifiSecuritySelectedItem, wifiSecurityOptions, wifiSecurityOptionCount, true, stateManager.getState().inputMode);
                } else {
                    wifiPendingPassword = entry;
                    wallet.saveWiFiConfig(wifiPendingSSID, wifiPendingPassword, wifiPendingSecurity);
                    stateManager.setMessage("Wi-Fi saved", true);
                    stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                    displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                    waitForConfirmation();
                    stateManager.setState(StateManager::STATE_WIFI_MENU);
                    displayManager.drawFullMenu(wifiMenuSelectedItem, wifiMenuOptions, wifiMenuItemCount, true, stateManager.getState().inputMode);
                }
                break;
            }
            default:
                break;
        }
    } else if (stateManager.getState().currentState == StateManager::STATE_PIN_ENTRY) {
        int digit;
        switch (input.event) {
            case InputManager::NAV_UP: {
                int current = before.pinInput[before.pinCursorPosition];
                int next;
                if (current == -1) {
                    next = 0;         // * -> 0
                } else if (current == 9) {
                    next = -1;        // 9 -> *
                } else {
                    next = current + 1; // 0..8 -> 1..9
                }

                stateManager.setPinInput(before.pinCursorPosition, next);
                stateManager.setPinDigitVisible(before.pinCursorPosition, true);
                displayManager.refreshScreen(
                    stateManager.getState(),
                    baseMenu,
                    menuItemCount,
                    true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged()
                );
                break;
            }
            case InputManager::NAV_DOWN: {
                int current = before.pinInput[before.pinCursorPosition];
                int next;
                if (current == -1) {
                    next = 9;         // * -> 9
                } else if (current == 0) {
                    next = -1;        // 0 -> *
                } else {
                    next = current - 1; // 1..9 -> 0..8
                }

                stateManager.setPinInput(before.pinCursorPosition, next);
                stateManager.setPinDigitVisible(before.pinCursorPosition, true);
                displayManager.refreshScreen(
                    stateManager.getState(),
                    baseMenu,
                    menuItemCount,
                    true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged()
                );
                break;
            }
            case InputManager::NAV_RIGHT:
                stateManager.setPinCursor(((before.pinCursorPosition + 1) % pinLength + pinLength) % pinLength);
                displayManager.drawPinCursor(((before.pinCursorPosition + 1) % pinLength + pinLength) % pinLength);
                break;
            case InputManager::NAV_LEFT:
                stateManager.setPinCursor(((before.pinCursorPosition - 1) % pinLength + pinLength) % pinLength);
                displayManager.drawPinCursor(((before.pinCursorPosition - 1) % pinLength + pinLength) % pinLength);
                break;
            case InputManager::NAV_SELECT: {
                if (stateManager.getPin() == -1) {
                    stateManager.setMessage("Still Locked [+1W]", true);
                    stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                    displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                    waitForConfirmation();
                    stateManager.setState(StateManager::STATE_PIN_ENTRY);
                    displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                } else if (changePinPhase == CHANGE_PIN_NEW_FIRST) {
                    pendingNewPIN = stateManager.getPin();
                    changePinPhase = CHANGE_PIN_NEW_CONFIRM;
                    stateManager.setFreshInput();
                    stateManager.setPinCursor(0);
                    stateManager.setMessage("Confirm new PIN", true);
                    stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                    displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                    waitForConfirmation();
                    stateManager.setState(StateManager::STATE_PIN_ENTRY);
                    displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                } else if (changePinPhase == CHANGE_PIN_NEW_CONFIRM) {
                    int confirmedPIN = stateManager.getPin();
                    if (confirmedPIN != pendingNewPIN) {
                        changePinPhase = CHANGE_PIN_NEW_FIRST;
                        stateManager.setMessage("PIN mismatch, try again", true);
                        stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        waitForConfirmation();
                        stateManager.setFreshInput();
                        stateManager.setPinCursor(0);
                        stateManager.setState(StateManager::STATE_PIN_ENTRY);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                    } else {
                        wallet.changePIN(wallet.getCurrentPIN(), pendingNewPIN);
                        wallet.lock();
                        stateManager.setWalletUnlocked(false);
                        changePinPhase = CHANGE_PIN_IDLE;
                        stateManager.setMessage("PIN has changed, locking wallet", true);
                        stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        waitForConfirmation();
                        stateManager.setState(StateManager::STATE_MENU);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                    }
                } else if (resetPhase == RESET_CONFIRM) {
                    int entered = stateManager.getPin();
                    if (entered == resetCode) {
                        wallet.factoryReset();
                        resetPhase = RESET_IDLE;
                        stateManager.setMessage("Wallet has been reset", true);
                        stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        waitForConfirmation();
                        stateManager.setState(StateManager::STATE_MENU);
                        stateManager.setWalletUnlocked(false);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                    } else {
                        resetPhase = RESET_IDLE;
                        stateManager.setMessage("Reset cancelled", true);
                        stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                        waitForConfirmation();
                        stateManager.setState(StateManager::STATE_MENU);
                        displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                    }
                } else {
                    // Entrada normal para desbloquear
                    switch (wallet.unlock(stateManager.getPin())) {
                        case -1:
                            stateManager.setMessage("Still Locked [+1W]", true);
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            break;
                        case 0:
                            stateManager.setMessage("Wallet Unlocked !!!", true);
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            stateManager.setWalletUnlocked(true);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            break;
                        case 1:
                            stateManager.setMessage("PIN saved: " + String(stateManager.getPin()) + "\n\nPress ENTER 5 times to continue", false);
                            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            waitForConfirmation();
                            waitForConfirmation();
                            waitForConfirmation();
                            waitForConfirmation();
                            waitForConfirmation();
                            stateManager.setState(StateManager::STATE_MENU);
                            stateManager.setWalletUnlocked(true);
                            displayManager.refreshScreen(stateManager.getState(), baseMenu, menuItemCount, true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
                            break;
                        default:
                            break;
                    }
                }

                stateManager.setFreshInput();
                stateManager.setPinCursor(0);
                break;
            }
            default:
                break;
        }
    }
    stateManager.setState(stateManager.getState().currentState);
}

// ---------- MENU ACTIONS ------------
void executeMenuAction(const String& action) {
    auto state = stateManager.getState();
    if (action == "Address") {
        if (state.walletUnlocked) {
            String address = wallet.getAddress();
            // setState 
            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
            waitForConfirmation();
            stateManager.setState(StateManager::STATE_MENU);
            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
        } else {
            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
            //displayManager.showMessage("Wallet locked!\nUnlock first to view address");
            stateManager.setMessage("Wallet locked!\nUnlock first to view address",true);
            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
            //waitfor confirm
            stateManager.setState(StateManager::STATE_MENU);
            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
        }
    } 
    else if (action == "Export") {
        if (state.walletUnlocked) {
            String privateKey = wallet.getPrivateKeyHex();
            displayManager.showPrivateKey(privateKey);
            waitForConfirmation();
            stateManager.setState(StateManager::STATE_MENU);
        } else {
            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
            //displayManager.showMessage("Wallet locked!\nUnlock first to view address");
            stateManager.setMessage("Wallet locked!\nUnlock first to export private key",true);
            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
            //waitfor confirm
            stateManager.setState(StateManager::STATE_MENU);
            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
        }
    } 
    else if (action == "Balance") {
        if (state.walletUnlocked) {
            displayManager.showMessage("Balance check\nRequires WiFi connection\n\nConnect to internet for\nreal-time balance data");
            waitForConfirmation();
            stateManager.setState(StateManager::STATE_MENU);
        } else {
            stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
            //displayManager.showMessage("Wallet locked!\nUnlock first to view address");
            stateManager.setMessage("Wallet locked!\nUnlock first to check the balance",true);
            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
            //waitfor confirm
            stateManager.setState(StateManager::STATE_MENU);
            displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
        }
    } 
    else if (action == "Change PIN") {
        displayManager.showMessage(state.walletUnlocked ? "Change PIN feature\nComing soon" : "Unlock wallet first");
        stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
    } 
    else if (action == "Info") {
        stateManager.setState(StateManager::STATE_SHOW_INFO);
        displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
        waitForConfirmation();
        stateManager.setState(StateManager::STATE_MENU);
        displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
    } 
    else if (action == "Reset") {
        displayManager.showMessage(state.walletUnlocked ? "Reset is now functional" : "Unlock wallet first");
        stateManager.setState(StateManager::STATE_SHOW_MESSAGE);
        displayManager.refreshScreen(stateManager.getState(),baseMenu,menuItemCount,true, wallet.getCurrentPIN() != -1, stateManager.getState().inputMode, stateManager.hasWalletStateChanged());
    }
}

// ---------- WAIT FOR CONFIRMATION ----
void waitForConfirmation() {
    unsigned long start = millis();
    while (millis() - start < 30000) {
        auto input = inputFacade.processInputs();
        if (input.event == InputManager::NAV_SELECT) return;
        delay(50);
    }
}

// ---------- MAIN LOOP ---------------
void setup() {
    initializeSystem();
    stateManager.setState(StateManager::STATE_MENU);
}

void loop() {
    stateManager.setInputMode(inputFacade.getActiveInputMode());
    
    handleMenuState();

    delay(16);
}
// ------------------------------------
