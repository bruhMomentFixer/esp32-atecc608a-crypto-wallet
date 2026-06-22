  #include "DisplayManager.h"
  
  #define ANSI_CLEAR "\033[2J\033[H"
  #define TFT_RST 4
  
  DisplayManager::DisplayManager(Adafruit_ST7735* display) : tft(display) {}
  
  void DisplayManager::initialize() {
      tft->initR(INITR_GREENTAB);
      tft->setRotation(2);
      tft->fillScreen(ST77XX_BLACK);
      tft->setTextWrap(false);
  }
  
  String DisplayManager::truncateText(const String& text, int maxChars) {
      if (text.length() <= maxChars) return text;
      return text.substring(0, maxChars - 3) + "...";
  }
  
  void DisplayManager::drawFullMenu(int selectedItem, const char** menuItems, int itemCount, bool walletUnlocked, const String& inputMode) {
      tft->fillScreen(ST77XX_BLACK);
      
      drawStatusBar(walletUnlocked, inputMode);
      tft->setCursor(0, 16);
      tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft->println("---------------------");
      drawMenuItems(menuItems, itemCount, selectedItem, walletUnlocked);
      int bottomLineY = 24 + (itemCount * MENU_ITEM_HEIGHT);
      tft->setCursor(0, bottomLineY);
      tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft->println("---------------------");   
  
      if (mirrorToSerial) {
          Serial.print(ANSI_CLEAR);
          Serial.println("Wallet: " + String(walletUnlocked ? "UNLOCKED" : "LOCKED"));
          Serial.println("Input: " + inputMode);
          Serial.println("---------------------");
          for (int i = 0; i < itemCount; i++) {
              if (i == selectedItem) Serial.print("> ");
              else Serial.print("  ");
              Serial.println(menuItems[i]);
          }
          Serial.println("---------------------");
      }
  }
  
  void DisplayManager::drawStatusBar(bool walletUnlocked, const String& inputMode) {
      clearRegion(0, 0, tft->width(), STATUS_BAR_HEIGHT);
      
      // Line 1: Wallet status
      tft->setCursor(0, 0);
      tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft->setTextSize(1);
      tft->print("Wallet: ");
      if (walletUnlocked) {
          tft->setTextColor(ST77XX_BLUE, ST77XX_BLACK);
          tft->print("UNLOCKED");
      } else {
          tft->setTextColor(ST77XX_GREEN, ST77XX_BLACK);
          tft->print("LOCKED");
      }
      
      // Line 2: Input mode (truncated)
      tft->setCursor(0, 8);
      tft->setTextColor(ST77XX_CYAN, ST77XX_BLACK);
      tft->print("Input: ");
      tft->print(truncateText(inputMode, 14)); // Limit to 14 chars for this line 
      
  }
  
  void DisplayManager::updateMenu(int selectedItem, int previousItem, const char** menuItems, int itemCount, bool walletUnlocked, const String& inputMode) {
      if (selectedItem != previousItem) {
          updateMenuItem(previousItem, false, menuItems[previousItem], walletUnlocked);
          updateMenuItem(selectedItem, true, menuItems[selectedItem], walletUnlocked);
      }
  
      if (mirrorToSerial) {
          Serial.print(ANSI_CLEAR);
          Serial.println("Wallet: " + String(walletUnlocked ? "UNLOCKED" : "LOCKED"));
          Serial.println("Input: " + inputMode);
          Serial.println("---------------------");
          for (int i = 0; i < itemCount; i++) {
              if (i == selectedItem) Serial.print("> ");
              else Serial.print("  ");
              Serial.println(menuItems[i]);
          }
          Serial.println("---------------------");
      }
  }
  
  void DisplayManager::updateMenuItem(int index, bool selected, const char* text, bool walletUnlocked) {
      int yPos = 24 + (index * MENU_ITEM_HEIGHT); // Below separator
      
      clearRegion(0, yPos, tft->width(), MENU_ITEM_HEIGHT);
      
      tft->setCursor(5, yPos);
      tft->setTextSize(1);
      
      if (selected) {
          tft->setTextColor(ST77XX_BLACK, ST77XX_WHITE);
          tft->print("> ");
      } else {
          tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
          tft->print("  ");
      }
      tft->print(text);
      
  
  }
  
  void DisplayManager::drawMenuItems(const char** menuItems, int itemCount, int selectedItem, bool walletUnlocked) {
      for (int i = 0; i < itemCount; i++) {
          updateMenuItem(i, i == selectedItem, menuItems[i], walletUnlocked);
      }
  }
  
  void DisplayManager::showPinEntry(int currentInput, int cursorPos, const bool* digitVisible) {
      tft->fillScreen(ST77XX_BLACK);
    
      tft->setCursor(0, 20);
      tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft->setTextSize(1);
      tft->println("ENTER PIN (4 digits)");
      tft->println("-------------------");
      drawPinDigit(0, -1, false, true);
      drawPinDigit(1, -1, false, true);
      drawPinDigit(2, -1, false, true);
      drawPinDigit(3, -1, false, true);
      drawPinCursor(0);
      tft->setTextSize(1);
      tft->setCursor(0, 90);
      tft->println("-------------------");
      tft->println("L/R:Move  U/D:Change");
      tft->println("BTN:Select/Confirm");
  
      // ✅ Añade esto
      if (mirrorToSerial) {
          Serial.print(ANSI_CLEAR);
          Serial.println("\n=== ENTER PIN ===");
          Serial.print("PIN: ");
          for (int i = 0; i < 4; i++) {
              if (digitVisible[i]) Serial.print(currentInput);
              else Serial.print("*");
          }
          Serial.println();
          Serial.println("-------------------");
          Serial.println("Use arrows or numbers to input");
      }
  }
  
  void DisplayManager::drawPinCursor(int pos) {
      clearRegion(10,75,120,10);
      tft->setCursor(10 + (25 * pos), 75);
      tft->setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
      tft->setTextSize(2);
      tft->println("^");
  }

  void DisplayManager::showWiFiTextEntry(const String& label, const int* slots, int slotCount, int cursorPos) {
      tft->fillScreen(ST77XX_BLACK);
      tft->setCursor(0, 8);
      tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft->setTextSize(1);
      tft->println(label);
      tft->println("---------------------");

      for (int i = 0; i < slotCount; i++) {
          drawWiFiSlot(i, slots[i], i == cursorPos);
      }

      tft->setTextSize(1);
      tft->setCursor(0, 100);
      tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft->println("Use UP/DOWN to change char");
      tft->println("LEFT/RIGHT to move, ENTER to accept");
      if (mirrorToSerial) {
          Serial.print(ANSI_CLEAR);
          Serial.println("=== " + label + " ===");
          Serial.print("Input: ");
          for (int i = 0; i < slotCount; i++) {
              if (slots[i] == -1) Serial.print("_");
              else Serial.print((char)slots[i]);
          }
          Serial.println();
          Serial.println("Use arrows to edit, ENTER to accept");
      }
  }

  void DisplayManager::showWiFiTestScreen(const String& step1, const String& step2, const String& step3, const String& footer) {
      tft->fillScreen(ST77XX_BLACK);
      tft->setCursor(0, 0);
      tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft->setTextSize(1);
      tft->println("WI-FI INTERNET TEST");
      tft->println("---------------------");
      tft->println(step1);
      tft->println(step2);
      tft->println(step3);
      tft->println();
      tft->println(footer);

      if (mirrorToSerial) {
          Serial.print(ANSI_CLEAR);
          Serial.println("WI-FI INTERNET TEST");
          Serial.println("---------------------");
          Serial.println(step1);
          Serial.println(step2);
          Serial.println(step3);
          Serial.println();
          Serial.println(footer);
      }
  }

  void DisplayManager::updateWiFiTextEntry(const int* slots, int slotCount, int cursorPos, int previousCursorPos, const int* previousSlots, int previousSlotCount) {
      if (previousCursorPos < 0) {
          for (int i = 0; i < slotCount; i++) {
              drawWiFiSlot(i, slots[i], i == cursorPos);
          }
          return;
      }

      int overlap = (slotCount < previousSlotCount) ? slotCount : previousSlotCount;

      // redraw changed or cursor-affected slots in overlap
      for (int i = 0; i < overlap; i++) {
          bool needsRedraw = false;
          if (slots[i] != previousSlots[i]) needsRedraw = true;
          if (i == previousCursorPos || i == cursorPos) needsRedraw = true;
          if (needsRedraw) {
              drawWiFiSlot(i, slots[i], i == cursorPos);
          }
      }

      // draw newly added slots
      for (int i = previousSlotCount; i < slotCount; i++) {
          drawWiFiSlot(i, slots[i], i == cursorPos);
      }

      // clear removed slots
      for (int i = slotCount; i < previousSlotCount; i++) {
          clearWiFiSlot(i);
      }
  }

  void DisplayManager::getWiFiSlotPosition(int index, int& x, int& y) {
      x = 5;
      y = 40;
      const int w = 18;
      const int spacing = 2;
      for (int i = 0; i < index; i++) {
          x += w + spacing;
          if (x + w > tft->width()) {
              x = 5;
              y += 30;
          }
      }
  }

  void DisplayManager::drawWiFiSlot(int index, int slotValue, bool selected) {
      int x, y;
      getWiFiSlotPosition(index, x, y);
      bool empty = (slotValue == -1);
      const int w = 18;
      const int h = 24;
      int bg;
      int fg;

      if (empty) {
          bg = ST77XX_BLUE;
          fg = ST77XX_WHITE;
      } else if (selected) {
          bg = ST77XX_YELLOW;
          fg = ST77XX_BLACK;
      } else {
          bg = ST77XX_BLACK;
          fg = ST77XX_WHITE;
      }

      tft->fillRect(x - 2, y - 2, w, h, bg);
      tft->drawRect(x - 2, y - 2, w, h, ST77XX_WHITE);
      tft->setCursor(x, y);
      tft->setTextSize(2);
      tft->setTextColor(fg, bg);
      if (empty) {
          tft->print(" ");
      } else {
          tft->print((char)slotValue);
      }
  }

  void DisplayManager::clearWiFiSlot(int index) {
      int x, y;
      getWiFiSlotPosition(index, x, y);
      const int w = 18;
      const int h = 24;
      tft->fillRect(x - 2, y - 2, w + 4, h + 4, ST77XX_BLACK);
  }
  
  void DisplayManager::clearRegion(int x, int y, int width, int height) {
      tft->fillRect(x, y, width, height, ST77XX_BLACK);
  }
  
  void DisplayManager::drawPinDigit(int position, int digit, bool visible, bool cursor) {
      int xPos = 10 + (position * 25);
      
      int yPos = 50;
      
      // Draw cursor
      if (cursor) {
          tft->fillRect(xPos - 2, yPos - 2, 20, 24, ST77XX_YELLOW);
      }
      
      tft->setCursor(xPos, yPos);
      tft->setTextSize(2);
      
      if (cursor) {
          tft->setTextColor(ST77XX_BLACK, ST77XX_YELLOW);
      } else {
          tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      }
      
      if (digit != -1) {
          tft->print(digit);
      } else {
          tft->print("*");
      }
  }
  
  void DisplayManager::showMessage(const String& message, bool requireButton) {
      tft->fillScreen(ST77XX_BLACK);
      tft->setCursor(0, 50);
      tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft->setTextSize(1);
      tft->setTextWrap(true);
      tft->println(message);
      
      if (requireButton) {
          tft->setCursor(0, 150);
          tft->println("Press to continue...");
      }
  
      // ✅ Añade esto
      if (mirrorToSerial) {
          Serial.print(ANSI_CLEAR);
          Serial.println("\n=== MESSAGE ===");
          Serial.println(message);
          if (requireButton) Serial.println("Press to continue...");
      }
  }
  
  void DisplayManager::showAddress(const String& address) {
      tft->fillScreen(ST77XX_BLACK);
      tft->setCursor(0, 30);
      tft->setTextColor(ST77XX_WHITE);
      tft->println("=== ADDRESS ===");
      tft->println(address);
  
      if (mirrorToSerial) {
          Serial.print(ANSI_CLEAR);
          Serial.println("\n=== ADDRESS ===");
          Serial.println(address);
      }
  }
  
  void DisplayManager::showPrivateKey(const String& key) {
      tft->fillScreen(ST77XX_BLACK);
      tft->setCursor(0, 20);
      tft->setTextColor(ST77XX_RED);
      tft->println("PRIVATE KEY:");
      tft->println(key);
  
      if (mirrorToSerial) {
          Serial.print(ANSI_CLEAR);
          Serial.println("\n=== PRIVATE KEY ===");
          Serial.println(key);
      }
  }
  
void DisplayManager::showInfo(bool walletUnlocked, bool keyGenerated, bool pinSet, bool wifiSaved, const String& inputMode) {
      tft->fillScreen(ST77XX_BLACK);
      tft->setCursor(0, 20);
      tft->setTextColor(ST77XX_WHITE);
      tft->println("=== INFO ===");
      tft->println("Wallet: " + String(walletUnlocked ? "UNLOCKED" : "LOCKED"));
      tft->println("Key Generated: " + String(keyGenerated ? "YES" : "NO"));
      tft->println("PIN Saved: " + String(pinSet ? "YES" : "NO"));
      tft->println("Wi-Fi Saved: " + String(wifiSaved ? "YES" : "NO"));
      tft->println("Input Mode: " + inputMode);
      tft->println("\nPress ENTER to exit");

      if (mirrorToSerial) {
          Serial.print(ANSI_CLEAR);
          Serial.println("\n=== INFO ===");
          Serial.println("Wallet: " + String(walletUnlocked ? "UNLOCKED" : "LOCKED"));
          Serial.println("Key Generated: " + String(keyGenerated ? "YES" : "NO"));
          Serial.println("PIN Saved: " + String(pinSet ? "YES" : "NO"));
          Serial.println("Wi-Fi Saved: " + String(wifiSaved ? "YES" : "NO"));
          Serial.println("Input Mode: " + inputMode);
          Serial.println("\nPress ENTER to exit");
      }
  }
  
  void DisplayManager::setMirrorToSerial(bool enabled) {
      mirrorToSerial = enabled;
  }
  
  void DisplayManager::redrawToSerial(int selectedItem, const char** menuItems, int itemCount, bool walletUnlocked, const String& inputMode) {
      if (!mirrorToSerial) return;
      Serial.print(ANSI_CLEAR);
      Serial.println("Wallet: " + String(walletUnlocked ? "UNLOCKED" : "LOCKED"));
      Serial.println("Input: " + inputMode);
      Serial.println("---------------------");
      for (int i = 0; i < itemCount; i++) {
          if (i == selectedItem) Serial.print("> ");
          else Serial.print("  ");
          Serial.println(menuItems[i]);
      }
      Serial.println("---------------------");
      Serial.flush();  // importante para enviar inmediatamente
  }
  
void DisplayManager::refreshScreen(const StateManager::AppData& state, const char** menuItems, int menuItemCount, bool keyGenerated, bool pinSet, String activeInputMode, bool walletChanged, bool wifiSaved) {

      // 1️⃣ CAMBIO DE ESTADO → redraw completo
      if (state.currentState != state.previousState) {
  
          switch (state.currentState) {
  
              case StateManager::STATE_MENU:
                  drawFullMenu(
                      state.selectedMenuItem,
                      menuItems,
                      menuItemCount,
                      state.walletUnlocked,
                      state.inputMode
                  );
                  break;
  
              case StateManager::STATE_PIN_ENTRY:
                  showPinEntry(
                      state.pinInput[state.pinCursorPosition],
                      state.pinCursorPosition,
                      state.pinDigitVisible
                  );
                  break;
  
              case StateManager::STATE_SHOW_MESSAGE:
                  showMessage(
                      state.message,
                      state.messageRequireButton
                  );
                  break;
  
              case StateManager::STATE_SHOW_ADDRESS:
                  showAddress(state.message);
                  break;
  
              case StateManager::STATE_SHOW_PRIVATE_KEY:
                  showPrivateKey(state.message);
                  break;
  
              case StateManager::STATE_SHOW_INFO:
                  showInfo(
                      state.walletUnlocked,
                      keyGenerated,
                      pinSet,
                      wifiSaved,
                      state.inputMode
                  );
                  break;
          }
  
          return; // ⛔ nada más se procesa en este ciclo
      }
  
      // 2️⃣ MISMO ESTADO → actualizaciones parciales
      switch (state.currentState) {
  
          case StateManager::STATE_MENU:
              if (walletChanged) {
                  drawStatusBar(state.walletUnlocked, state.inputMode);
              }
              updateMenu(
                  state.selectedMenuItem,
                  state.previousMenuItem,
                  menuItems,
                  menuItemCount,
                  state.walletUnlocked,
                  state.inputMode
              );
              break;
  
          case StateManager::STATE_PIN_ENTRY:
              drawPinDigit(
                      state.pinCursorPosition,
                      state.pinInput[state.pinCursorPosition],
                      state.pinDigitVisible[state.pinCursorPosition],
                      true
                  );
              break;
  
          default:
              // Otros estados no necesitan updates parciales
              break;
      }
  }
