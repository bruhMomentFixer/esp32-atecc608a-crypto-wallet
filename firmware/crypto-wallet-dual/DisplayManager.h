#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "StateManager.h"

class DisplayManager {
private:
    //Adafruit_ST7735* tft;
    bool mirrorToSerial = false;   // ✅ ahora es atributo de la clase
    
    /*struct ScreenState {
        int selectedItem;
        bool walletUnlocked;
        String statusText;
        bool operator!=(const ScreenState& other) const {
            return selectedItem != other.selectedItem || 
                   walletUnlocked != other.walletUnlocked ||
                   statusText != other.statusText;
        }
    } currentState, previousState;*/
    
    // Constants for 128x160 display
    static const int MENU_ITEM_HEIGHT = 12;
    static const int STATUS_BAR_HEIGHT = 16;
    static const int MAX_LINE_CHARS = 21;

public:
    Adafruit_ST7735* tft;
    DisplayManager(Adafruit_ST7735* display);
    void initialize();
    void setMirrorToSerial(bool enabled);  // ✅ ahora está dentro de la clase

    // Display methods
    void drawFullMenu(int selectedItem, const char** menuItems, int itemCount, bool walletUnlocked, const String& inputMode);
    void updateMenu(int selectedItem, int previousItem, const char** menuItems, int itemCount, bool walletUnlocked, const String& inputMode);
    void updateStatusBar(bool walletUnlocked, const String& inputMode);
    void updateMenuItem(int index, bool selected, const char* text, bool walletUnlocked);

    // PIN entry
    void showPinEntry(int currentInput, int cursorPos, const bool* digitVisible);
    void drawPinDigit(int position, int digit, bool visible, bool cursor);
    void drawPinCursor(int pos);
    void showWiFiTextEntry(const String& label, const int* slots, int slotCount, int cursorPos);
    void updateWiFiTextEntry(const int* slots, int slotCount, int cursorPos, int previousCursorPos, const int* previousSlots, int previousSlotCount);
    void clearWiFiSlot(int index);
    void showWiFiTestScreen(const String& step1, const String& step2, const String& step3, const String& footer);

    // Full screen messages
    void showMessage(const String& message, bool requireButton = true);
    void showAddress(const String& address);
    void showPrivateKey(const String& privateKey);
    void showInfo(bool walletUnlocked, bool keyGenerated, bool pinSet, bool wifiSaved, const String& inputMode);
    
    void redrawToSerial(int selectedItem, const char** menuItems, int itemCount, bool walletUnlocked, const String& inputMode);
    void refreshScreen(const StateManager::AppData& state, const char** menuItems, int menuItemCount, bool keyGenerated, bool pinSet, String activeInputMode, bool walletChanged, bool wifiSaved = false);

private:
    void clearRegion(int x, int y, int width, int height);
    void drawStatusBar(bool walletUnlocked, const String& inputMode);
    void drawMenuItems(const char** menuItems, int itemCount, int selectedItem, bool walletUnlocked);
    void drawWiFiSlot(int index, int slotValue, bool selected);
    void getWiFiSlotPosition(int index, int& x, int& y);
    String truncateText(const String& text, int maxChars);
};

#endif
